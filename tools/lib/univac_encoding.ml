open! Core
open! Import
open Instruction

(* Encode base RISC-V instructions into 2x18-bit words
   Word1: [reg1(5) | reg2(5) | opcode(8)]
   Word2: value (sign-extended 18-bit) *)

let make_word1 ~reg1 ~reg2 ~opcode =
  ((reg1 land 0x1F) lsl 13) lor ((reg2 land 0x1F) lsl 8) lor (opcode land 0xFF)
;;

let twos_comp_to_ones_comp_pc_delta offset =
  (* Align to 2-byte boundary (RISC-V requirement) *)
  let offset = offset land lnot 1 in
  let pc_delta = offset / 2 in
  (* Convert to ones complement *)
  if pc_delta < 0
  then (
    let ones_comp = pc_delta - 1 in
    ones_comp land 0x3FFFF)
  else pc_delta
;;

let encode_rtype { dest; src1; src2 } opcode =
  (* Pre-encode src2 as src2*2 for R-type instructions (opcodes 0-9) *)
  make_word1 ~reg1:dest ~reg2:src1 ~opcode, src2 * 2
;;

let encode_itype { dest; src; immediate } opcode =
  make_word1 ~reg1:dest ~reg2:src ~opcode, immediate
;;

let encode_shift { dest; src; shamt } opcode =
  make_word1 ~reg1:dest ~reg2:src ~opcode, shamt
;;

let encode_srli { dest; src; shamt } opcode =
  let rsha_base = 0o504300 in
  let precomputed_rsha = shamt lor rsha_base in
  make_word1 ~reg1:dest ~reg2:src ~opcode, precomputed_rsha
;;

let encode_branch { src1; src2; target } opcode =
  make_word1 ~reg1:src1 ~reg2:src2 ~opcode, twos_comp_to_ones_comp_pc_delta target
;;

let encode_jump reg1 reg2 opcode value =
  if value < -(1 lsl 17) || value >= 1 lsl 17
  then raise_s [%message "Jump offset does not fit in 18 bits" (value : int)];
  make_word1 ~reg1 ~reg2 ~opcode, twos_comp_to_ones_comp_pc_delta value
;;

let encode_upper { dest; immediate } opcode =
  (* 20-bit immediate: upper 2 bits in reg2, lower 18 bits in word2 *)
  let upper_2_bits = (immediate lsr 18) land 0x3 in
  let lower_18_bits = immediate land 0x3FFFF in
  make_word1 ~reg1:dest ~reg2:upper_2_bits ~opcode, lower_18_bits
;;

let encode_load ({ dest; address } : load) opcode =
  make_word1 ~reg1:dest ~reg2:address.register ~opcode, address.offset
;;

let encode_store ({ src; address } : store) opcode =
  make_word1 ~reg1:src ~reg2:address.register ~opcode, address.offset
;;

let encode = function
  | Add r -> encode_rtype r 0
  | Sub r -> encode_rtype r 1
  | Slt r -> encode_rtype r 2
  | Sltu r -> encode_rtype r 3
  | And r -> encode_rtype r 4
  | Or r -> encode_rtype r 5
  | Xor r -> encode_rtype r 6
  | Sll r -> encode_rtype r 7
  | Srl r -> encode_rtype r 8
  | Sra r -> encode_rtype r 9
  | Addi { dest; src; immediate } ->
    (* Pre-decrement the immediate so the built-in +1 in ADDA absorbs the
       compensating SUBA CONST_1 in the OP_ADDI handler. *)
    make_word1 ~reg1:dest ~reg2:src ~opcode:10, immediate - 1
  | Slti i -> encode_itype i 11
  | Sltiu i -> encode_itype i 12
  | Andi i -> encode_itype i 13
  | Ori i -> encode_itype i 14
  | Xori i -> encode_itype i 15
  | Slli s -> encode_shift s 16
  | Srli s -> encode_srli s 17
  | Srai { dest; src; shamt } ->
    let rsha_base = 0o504300 in
    let shift_plus_4 = (shamt + 4) land 0x3F in
    (* Mask to 6 bits for RSHA's shift range *)
    let precomputed_rsha = shift_plus_4 lor rsha_base in
    make_word1 ~reg1:dest ~reg2:src ~opcode:18, precomputed_rsha
  | Lb l -> encode_load l 19
  | Lh l -> encode_load l 20
  | Lw l -> encode_load l 21
  | Lbu l -> encode_load l 22
  | Lhu l -> encode_load l 23
  | Sb s -> encode_store s 24
  | Sh s -> encode_store s 25
  | Sw s -> encode_store s 26
  | Beq b -> encode_branch b 27
  | Bne b -> encode_branch b 28
  | Blt b -> encode_branch b 29
  | Bge b -> encode_branch b 30
  | Bltu b -> encode_branch b 31
  | Bgeu b -> encode_branch b 32
  | Jal { dest; offset } -> encode_jump dest 0 33 offset
  | Jalr { dest; base; offset } -> make_word1 ~reg1:dest ~reg2:base ~opcode:34, offset
  | Lui u -> encode_upper u 35
  | Auipc u -> encode_upper u 36
  | Ecall -> make_word1 ~reg1:0 ~reg2:0 ~opcode:37, 0
  | Mul r -> encode_rtype r 38
  | Mulh r -> encode_rtype r 39
  | Mulhsu r -> encode_rtype r 40
  | Mulhu r -> encode_rtype r 41
  | Div r -> encode_rtype r 42
  | Divu r -> encode_rtype r 43
  | Rem r -> encode_rtype r 44
  | Remu r -> encode_rtype r 45
  | _ -> raise_s [%message "Instruction not supported in reduced encoding"]
;;

let sign_extend_18 v = if v land (1 lsl 17) <> 0 then v lor (-1 lsl 18) else v

let ones_comp_pc_delta_to_twos_comp_offset pc_delta_18bit =
  let pc_delta =
    if pc_delta_18bit land (1 lsl 17) <> 0
    then (pc_delta_18bit + 1) lor (-1 lsl 18) (* Add 1 and sign extend *)
    else pc_delta_18bit
  in
  let offset = pc_delta * 2 in
  offset land lnot 1
;;

let decode word1 word2 =
  let reg1 = (word1 lsr 13) land 0x1F
  and reg2 = (word1 lsr 8) land 0x1F in
  let opcode = word1 land 0xFF
  and value = word2
  and svalue = sign_extend_18 word2 in
  (* For R-type instructions (opcodes 0-9), word2 is pre-doubled, so divide by 2 *)
  let rtype = { dest = reg1; src1 = reg2; src2 = value / 2 } in
  let itype = { dest = reg1; src = reg2; immediate = svalue } in
  let shift_type = { dest = reg1; src = reg2; shamt = value } in
  let load_type : Instruction.load =
    { dest = reg1; address = { register = reg2; offset = svalue } }
  in
  let store_type : Instruction.store =
    { src = reg1; address = { register = reg2; offset = svalue } }
  in
  let branch_type =
    { src1 = reg1; src2 = reg2; target = ones_comp_pc_delta_to_twos_comp_offset word2 }
  in
  let utype = { dest = reg1; immediate = (reg2 lsl 18) lor value } in
  match opcode with
  | 0 -> add rtype
  | 1 -> sub rtype
  | 2 -> slt rtype
  | 3 -> sltu rtype
  | 4 -> and_ rtype
  | 5 -> or_ rtype
  | 6 -> xor rtype
  | 7 -> sll rtype
  | 8 -> srl rtype
  | 9 -> sra rtype
  | 10 -> addi { dest = reg1; src = reg2; immediate = svalue + 1 }
  | 11 -> slti itype
  | 12 -> sltiu itype
  | 13 -> andi itype
  | 14 -> ori itype
  | 15 -> xori itype
  | 16 -> slli shift_type
  | 17 -> srli { dest = reg1; src = reg2; shamt = value land 0x1F }
  | 18 -> srai { dest = reg1; src = reg2; shamt = (value land 0x3F) - 4 }
  | 19 -> lb load_type
  | 20 -> lh load_type
  | 21 -> lw load_type
  | 22 -> lbu load_type
  | 23 -> lhu load_type
  | 24 -> sb store_type
  | 25 -> sh store_type
  | 26 -> sw store_type
  | 27 -> beq branch_type
  | 28 -> bne branch_type
  | 29 -> blt branch_type
  | 30 -> bge branch_type
  | 31 -> bltu branch_type
  | 32 -> bgeu branch_type
  | 33 -> jal { dest = reg1; offset = ones_comp_pc_delta_to_twos_comp_offset word2 }
  | 34 -> jalr { dest = reg1; base = reg2; offset = svalue }
  | 35 -> lui utype
  | 36 -> auipc utype
  | 37 -> ecall
  | 38 -> mul rtype
  | 39 -> mulh rtype
  | 40 -> mulhsu rtype
  | 41 -> mulhu rtype
  | 42 -> div rtype
  | 43 -> divu rtype
  | 44 -> rem rtype
  | 45 -> remu rtype
  | _ -> raise_s [%message "Unknown opcode" (opcode : int)]
;;

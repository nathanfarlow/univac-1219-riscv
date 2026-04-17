open! Core
open! Import
open Instruction

let extract instruction ~from_bit ~to_bit =
  let width = to_bit - from_bit + 1 in
  let mask = (1 lsl width) - 1 in
  (instruction lsr from_bit) land mask
;;

let get_opcode instruction = extract instruction ~from_bit:0 ~to_bit:6
let get_rd instruction = extract instruction ~from_bit:7 ~to_bit:11
let get_funct3 instruction = extract instruction ~from_bit:12 ~to_bit:14
let get_rs1 instruction = extract instruction ~from_bit:15 ~to_bit:19
let get_rs2 instruction = extract instruction ~from_bit:20 ~to_bit:24
let get_funct7 instruction = extract instruction ~from_bit:25 ~to_bit:31

let get_i_imm instruction =
  extract instruction ~from_bit:20 ~to_bit:31 |> sign_extend ~bits:12
;;

let decode_r_type instruction =
  { dest = get_rd instruction; src1 = get_rs1 instruction; src2 = get_rs2 instruction }
;;

let decode_i_type instruction =
  { dest = get_rd instruction
  ; src = get_rs1 instruction
  ; immediate = get_i_imm instruction
  }
;;

let decode_shift_type instruction =
  { dest = get_rd instruction
  ; src = get_rs1 instruction
  ; shamt = extract instruction ~from_bit:20 ~to_bit:24 |> mask ~bits:5
  }
;;

let decode_u_type instruction =
  { dest = get_rd instruction
  ; immediate = extract instruction ~from_bit:12 ~to_bit:31 |> mask ~bits:20
  }
;;

let decode_csr_reg instruction =
  { dest = get_rd instruction
  ; csr = extract instruction ~from_bit:20 ~to_bit:31 |> mask ~bits:12
  ; src = get_rs1 instruction
  }
;;

let decode_csr_imm instruction =
  { dest = get_rd instruction
  ; csr = extract instruction ~from_bit:20 ~to_bit:31 |> mask ~bits:12
  ; imm = get_rs1 instruction |> mask ~bits:5
  }
;;

let decode_amo instruction =
  { dest = get_rd instruction; address = get_rs1 instruction; src = get_rs2 instruction }
;;

let decode_memory_address instruction offset = { offset; register = get_rs1 instruction }

let decode_instruction instruction =
  let opcode = get_opcode instruction in
  let funct3 = get_funct3 instruction in
  let funct7 = get_funct7 instruction in
  match opcode with
  | 0x37 -> Lui (decode_u_type instruction)
  | 0x17 -> Auipc (decode_u_type instruction)
  | 0x6F ->
    let rd = get_rd instruction in
    let imm_20 = extract instruction ~from_bit:31 ~to_bit:31 in
    let imm_19_12 = extract instruction ~from_bit:12 ~to_bit:19 in
    let imm_11 = extract instruction ~from_bit:20 ~to_bit:20 in
    let imm_10_1 = extract instruction ~from_bit:21 ~to_bit:30 in
    let imm =
      (imm_20 lsl 20) lor (imm_19_12 lsl 12) lor (imm_11 lsl 11) lor (imm_10_1 lsl 1)
    in
    let offset = sign_extend ~bits:21 imm in
    Jal { dest = rd; offset }
  | 0x67 ->
    let rd = get_rd instruction in
    let rs1 = get_rs1 instruction in
    let offset = get_i_imm instruction in
    Jalr { dest = rd; base = rs1; offset }
  | 0x63 ->
    let rs1 = get_rs1 instruction in
    let rs2 = get_rs2 instruction in
    let imm_12 = extract instruction ~from_bit:31 ~to_bit:31 in
    let imm_11 = extract instruction ~from_bit:7 ~to_bit:7 in
    let imm_10_5 = extract instruction ~from_bit:25 ~to_bit:30 in
    let imm_4_1 = extract instruction ~from_bit:8 ~to_bit:11 in
    let imm =
      (imm_12 lsl 12) lor (imm_11 lsl 11) lor (imm_10_5 lsl 5) lor (imm_4_1 lsl 1)
    in
    let target = sign_extend ~bits:13 imm in
    let branch = { src1 = rs1; src2 = rs2; target } in
    (match funct3 with
     | 0 -> Beq branch
     | 1 -> Bne branch
     | 4 -> Blt branch
     | 5 -> Bge branch
     | 6 -> Bltu branch
     | 7 -> Bgeu branch
     | _ -> raise_s [%message "Invalid branch funct3" (funct3 : int)])
  | 0x03 ->
    let address = decode_memory_address instruction (get_i_imm instruction) in
    let load : Instruction.load = { dest = get_rd instruction; address } in
    (match funct3 with
     | 0 -> Lb load
     | 1 -> Lh load
     | 2 -> Lw load
     | 4 -> Lbu load
     | 5 -> Lhu load
     | _ -> raise_s [%message "Invalid load funct3" (funct3 : int)])
  | 0x23 ->
    let imm_11_5 = extract instruction ~from_bit:25 ~to_bit:31 in
    let imm_4_0 = extract instruction ~from_bit:7 ~to_bit:11 in
    let imm = (imm_11_5 lsl 5) lor imm_4_0 in
    let offset = sign_extend ~bits:12 imm in
    let address = decode_memory_address instruction offset in
    let store : Instruction.store = { src = get_rs2 instruction; address } in
    (match funct3 with
     | 0 -> Sb store
     | 1 -> Sh store
     | 2 -> Sw store
     | _ -> raise_s [%message "Invalid store funct3" (funct3 : int)])
  | 0x13 ->
    (match funct3 with
     | 0 -> Addi (decode_i_type instruction)
     | 1 -> Slli (decode_shift_type instruction)
     | 2 -> Slti (decode_i_type instruction)
     | 3 -> Sltiu (decode_i_type instruction)
     | 4 -> Xori (decode_i_type instruction)
     | 5 ->
       (match funct7 with
        | 0x00 -> Srli (decode_shift_type instruction)
        | 0x20 -> Srai (decode_shift_type instruction)
        | _ -> raise_s [%message "Invalid shift funct7" (funct7 : int)])
     | 6 -> Ori (decode_i_type instruction)
     | 7 -> Andi (decode_i_type instruction)
     | _ -> raise_s [%message "Invalid op-imm funct3" (funct3 : int)])
  | 0x33 ->
    let rtype = decode_r_type instruction in
    (match funct7, funct3 with
     | 0x00, 0 -> Add rtype
     | 0x20, 0 -> Sub rtype
     | 0x00, 1 -> Sll rtype
     | 0x00, 2 -> Slt rtype
     | 0x00, 3 -> Sltu rtype
     | 0x00, 4 -> Xor rtype
     | 0x00, 5 -> Srl rtype
     | 0x20, 5 -> Sra rtype
     | 0x00, 6 -> Or rtype
     | 0x00, 7 -> And rtype
     | 0x01, 0 -> Mul rtype
     | 0x01, 1 -> Mulh rtype
     | 0x01, 2 -> Mulhsu rtype
     | 0x01, 3 -> Mulhu rtype
     | 0x01, 4 -> Div rtype
     | 0x01, 5 -> Divu rtype
     | 0x01, 6 -> Rem rtype
     | 0x01, 7 -> Remu rtype
     | _ -> raise_s [%message "Invalid op funct7/funct3" (funct7 : int) (funct3 : int)])
  | 0x0F -> Fence
  | 0x73 ->
    let csr_addr = extract instruction ~from_bit:20 ~to_bit:31 in
    (match funct3 with
     | 0 ->
       (match csr_addr with
        | 0 -> Ecall
        | 1 -> Ebreak
        | 0x302 -> Mret
        | 0x105 -> Wfi
        | _ -> raise_s [%message "Invalid system instruction" (csr_addr : int)])
     | 1 -> Csrrw (decode_csr_reg instruction)
     | 2 -> Csrrs (decode_csr_reg instruction)
     | 3 -> Csrrc (decode_csr_reg instruction)
     | 5 -> Csrrwi (decode_csr_imm instruction)
     | 6 -> Csrrsi (decode_csr_imm instruction)
     | 7 -> Csrrci (decode_csr_imm instruction)
     | _ -> raise_s [%message "Invalid system funct3" (funct3 : int)])
  | 0x2F ->
    let irmid = extract instruction ~from_bit:27 ~to_bit:31 in
    (match irmid with
     | 0x02 ->
       let rd = get_rd instruction in
       let rs1 = get_rs1 instruction in
       lr_w ~dest:rd ~address:rs1
     | 0x03 ->
       let rd = get_rd instruction in
       let rs1 = get_rs1 instruction in
       let rs2 = get_rs2 instruction in
       sc_w ~dest:rd ~address:rs1 ~src:rs2
     | 0x01 -> Amoswap_w (decode_amo instruction)
     | 0x00 -> Amoadd_w (decode_amo instruction)
     | 0x04 -> Amoxor_w (decode_amo instruction)
     | 0x0C -> Amoand_w (decode_amo instruction)
     | 0x08 -> Amoor_w (decode_amo instruction)
     | 0x10 -> Amomin_w (decode_amo instruction)
     | 0x14 -> Amomax_w (decode_amo instruction)
     | 0x18 -> Amominu_w (decode_amo instruction)
     | 0x1C -> Amomaxu_w (decode_amo instruction)
     | _ -> raise_s [%message "Invalid atomic operation" (irmid : int)])
  | _ -> raise_s [%message "Invalid opcode" (opcode : int)]
;;

let parse_all ~binary : t list =
  let iobuf = Iobuf.of_string binary in
  let instructions = ref [] in
  while Iobuf.length iobuf >= 4 do
    let instruction_word = Iobuf.Consume.uint32_le iobuf in
    let instruction = decode_instruction instruction_word in
    instructions := instruction :: !instructions
  done;
  List.rev !instructions
;;

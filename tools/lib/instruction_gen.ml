open! Core
open! Import
open Instruction
open Quickcheck.Generator.Let_syntax

(* RISC-V %hi/%lo address loading: lui rd, %hi(addr); addi rd, rd, %lo(addr) *)
let addr_setup_instructions ~dest addr =
  let lower = addr land 0xFFF in
  let hi = (addr lsr 12) + if lower > 2047 then 1 else 0 in
  let lo = if lower > 2047 then lower - 4096 else lower in
  lui { dest; immediate = hi }
  :: (if lo = 0 then [] else [ addi { dest; src = dest; immediate = lo } ])
;;

let memory_operation ~memory_base ~memory_size make_instr =
  assert (memory_size >= 8);
  let%bind addr_reg = Int.gen_incl 1 31
  and reg = quickcheck_generator_register
  and base_offset = Int.gen_incl 0 (memory_size - 4) in
  let%map offset =
    Int.gen_incl
      (Int.max (-2048) (-base_offset))
      (Int.min 2047 (memory_size - 4 - base_offset))
  in
  addr_setup_instructions ~dest:addr_reg (memory_base + base_offset)
  @ [ make_instr reg addr_reg offset ]
;;

let load ~memory_base ~memory_size ctor =
  memory_operation ~memory_base ~memory_size (fun dest addr_reg offset ->
    ctor { dest; address = { register = addr_reg; offset } })
;;

let store ~memory_base ~memory_size ctor =
  memory_operation ~memory_base ~memory_size (fun src addr_reg offset ->
    ctor { src; address = { register = addr_reg; offset } })
;;

let rtype_gen =
  let%map dest = quickcheck_generator_register
  and src1 = quickcheck_generator_register
  and src2 = quickcheck_generator_register in
  { dest; src1; src2 }
;;

let add = rtype_gen >>| add
let sub = rtype_gen >>| sub
let slt = rtype_gen >>| slt
let sltu = rtype_gen >>| sltu
let and_ = rtype_gen >>| and_
let or_ = rtype_gen >>| or_
let xor = rtype_gen >>| xor
let sll = rtype_gen >>| sll
let srl = rtype_gen >>| srl
let sra = rtype_gen >>| sra
let mul = rtype_gen >>| mul
let mulh = rtype_gen >>| mulh
let mulhsu = rtype_gen >>| mulhsu
let mulhu = rtype_gen >>| mulhu
let div = rtype_gen >>| div
let divu = rtype_gen >>| divu
let rem = rtype_gen >>| rem
let remu = rtype_gen >>| remu
let addi = quickcheck_generator_itype >>| addi
let slti = quickcheck_generator_itype >>| slti
let sltiu = quickcheck_generator_itype >>| sltiu
let andi = quickcheck_generator_itype >>| andi
let ori = quickcheck_generator_itype >>| ori
let xori = quickcheck_generator_itype >>| xori
let slli = quickcheck_generator_shift_type >>| slli
let srli = quickcheck_generator_shift_type >>| srli
let srai = quickcheck_generator_shift_type >>| srai
let lui = quickcheck_generator_utype >>| lui
let auipc = quickcheck_generator_utype >>| auipc

let single_instruction_gen =
  Quickcheck.Generator.union
    [ add
    ; sub
    ; slt
    ; sltu
    ; and_
    ; or_
    ; xor
    ; sll
    ; srl
    ; sra
    ; mul
    ; mulh
    ; mulhsu
    ; mulhu
    ; addi
    ; slti
    ; sltiu
    ; andi
    ; ori
    ; xori
    ; slli
    ; srli
    ; srai
    ; lui
    ; auipc
    ]
;;

let forward_branch ctor instr_gen =
  let%map src1 = quickcheck_generator_register
  and src2 = quickcheck_generator_register
  and maybe_skipped = instr_gen in
  [ ctor { src1; src2; target = 4 }; maybe_skipped ]
;;

let backward_branch ctor instr_gen =
  let%map src1 = quickcheck_generator_register
  and src2 = quickcheck_generator_register
  and instr2 = instr_gen in
  [ jal { dest = 0; offset = 12 }
  ; instr2
  ; jal { dest = 0; offset = 8 }
  ; ctor { src1; src2; target = -8 }
  ]
;;

let jalr instr_gen =
  let%map base_reg = Int.gen_incl 1 31
  and dest_reg = Int.gen_incl 1 31
  and skipped = instr_gen
  and tricky = Int.gen_incl 0 1 in
  [ jal { dest = base_reg; offset = 4 }
  ; jalr { dest = dest_reg; base = base_reg; offset = 8 + tricky }
  ; skipped
  ]
;;

let jal instr_gen =
  let%map dest1 = quickcheck_generator_register
  and dest2 = quickcheck_generator_register
  and dest3 = quickcheck_generator_register
  and skipped = instr_gen in
  [ jal { dest = dest1; offset = 8 }
  ; jal { dest = dest2; offset = 12 }
  ; jal { dest = dest3; offset = -4 }
  ; skipped
  ]
;;

let suffix = [ Instruction.addi { dest = 17; src = 0; immediate = 2 }; Ecall ]

let full_program ~max_length ~memory_base ~memory_size ~include_divrem =
  let open Quickcheck.Generator in
  let instruction_gen =
    if include_divrem
    then union [ single_instruction_gen; div; divu; rem; remu ]
    else single_instruction_gen
  in
  let loads =
    [ lb; lh; lw; lbu; lhu ] |> List.map ~f:(load ~memory_base ~memory_size) |> union
  in
  let stores = [ sb; sh; sw ] |> List.map ~f:(store ~memory_base ~memory_size) |> union in
  let forward_branches =
    [ beq; bne; blt; bge; bltu; bgeu ]
    |> List.map ~f:(fun ctor -> forward_branch ctor instruction_gen)
    |> union
  in
  let backward_branches =
    [ beq; bne; blt; bge; bltu; bgeu ]
    |> List.map ~f:(fun ctor -> backward_branch ctor instruction_gen)
    |> union
  in
  let rec loop remaining acc =
    if remaining <= 0
    then return (List.rev acc)
    else
      union
        [ instruction_gen >>| List.return
        ; loads
        ; stores
        ; forward_branches
        ; backward_branches
        ; jal instruction_gen
        ; jalr instruction_gen
        ]
      >>= fun group -> loop (remaining - List.length group) (group :: acc)
  in
  loop max_length []
;;

open! Core
open! Import
open Base_quickcheck.Generator.Let_syntax

type register = int [@@deriving sexp_of, quickcheck, equal]

let quickcheck_generator_register = Base_quickcheck.Generator.int_inclusive 0 31
let quickcheck_observer_register = Base_quickcheck.Observer.int
let quickcheck_shrinker_register = Base_quickcheck.Shrinker.int

let signed_immediate_gen bits =
  let min = -(1 lsl (bits - 1)) in
  let max = (1 lsl (bits - 1)) - 1 in
  Int.gen_incl min max
;;

let unsigned_immediate_gen bits = Int.gen_incl 0 ((1 lsl bits) - 1)

type memory_operand =
  { offset : int
  ; register : register
  }
[@@deriving sexp_of, quickcheck, equal]

let quickcheck_generator_memory_operand =
  let%map offset = signed_immediate_gen 12
  and register = quickcheck_generator_register in
  { offset; register }
;;

type rtype =
  { dest : register
  ; src1 : register
  ; src2 : register
  }
[@@deriving sexp_of, quickcheck, equal]

type itype =
  { dest : register
  ; src : register
  ; immediate : int
  }
[@@deriving sexp_of, quickcheck, equal]

let quickcheck_generator_itype =
  let%map dest = quickcheck_generator_register
  and src = quickcheck_generator_register
  and immediate = signed_immediate_gen 12 in
  { dest; src; immediate }
;;

type shift_type =
  { dest : register
  ; src : register
  ; shamt : int
  }
[@@deriving sexp_of, quickcheck, equal]

let quickcheck_generator_shift_type =
  let%map dest = quickcheck_generator_register
  and src = quickcheck_generator_register
  and shamt = unsigned_immediate_gen 5 in
  { dest; src; shamt }
;;

type load =
  { dest : register
  ; address : memory_operand
  }
[@@deriving sexp_of, quickcheck, equal]

type store =
  { src : register
  ; address : memory_operand
  }
[@@deriving sexp_of, quickcheck, equal]

type branch =
  { src1 : register
  ; src2 : register
  ; target : int
  }
[@@deriving sexp_of, quickcheck, equal]

let quickcheck_generator_branch =
  let%map src1 = quickcheck_generator_register
  and src2 = quickcheck_generator_register
  and target =
    let%map raw = signed_immediate_gen 13 in
    raw land lnot 1
  in
  { src1; src2; target }
;;

type utype =
  { dest : register
  ; immediate : int
  }
[@@deriving sexp_of, quickcheck, equal]

let quickcheck_generator_utype =
  let%map dest = quickcheck_generator_register
  and immediate = unsigned_immediate_gen 20 in
  { dest; immediate }
;;

type csr_reg =
  { dest : register
  ; csr : int
  ; src : register
  }
[@@deriving sexp_of, quickcheck, equal]

let quickcheck_generator_csr_reg =
  let%map dest = quickcheck_generator_register
  and csr = unsigned_immediate_gen 12
  and src = quickcheck_generator_register in
  { dest; csr; src }
;;

type csr_imm =
  { dest : register
  ; csr : int
  ; imm : int
  }
[@@deriving sexp_of, quickcheck, equal]

let quickcheck_generator_csr_imm =
  let%map dest = quickcheck_generator_register
  and csr = unsigned_immediate_gen 12
  and imm = unsigned_immediate_gen 5 in
  { dest; csr; imm }
;;

type amo =
  { dest : register
  ; address : register
  ; src : register
  }
[@@deriving sexp_of, quickcheck, equal]

type jal =
  { dest : register
  ; offset : int
  }
[@@deriving sexp_of, quickcheck, equal]

let quickcheck_generator_jal =
  let%map dest = quickcheck_generator_register
  and offset =
    let%map raw = signed_immediate_gen 21 in
    raw land lnot 1
  in
  { dest; offset }
;;

type jalr =
  { dest : register
  ; base : register
  ; offset : int
  }
[@@deriving sexp_of, quickcheck, equal]

let quickcheck_generator_jalr =
  let%map dest = quickcheck_generator_register
  and base = quickcheck_generator_register
  and offset = signed_immediate_gen 12 in
  { dest; base; offset }
;;

type t =
  | Add of rtype
  | Sub of rtype
  | Addi of itype
  | Lb of load
  | Sb of store
  | Jal of jal
  | Jalr of jalr
  | Beq of branch
  | Slt of rtype
  | Lui of utype
  | Ecall
  | Ebreak
  | Lh of load
  | Lw of load
  | Sh of store
  | Sw of store
  | Bne of branch
  | Blt of branch
  | Bge of branch
  | Bltu of branch
  | Bgeu of branch
  | Slti of itype
  | Sltu of rtype
  | Sltiu of itype
  | Auipc of utype
  | And of rtype
  | Or of rtype
  | Xor of rtype
  | Slli of shift_type
  | Srli of shift_type
  | Srai of shift_type
  | Sll of rtype
  | Srl of rtype
  | Sra of rtype
  | Andi of itype
  | Ori of itype
  | Xori of itype
  | Lbu of load
  | Lhu of load
  | Mul of rtype
  | Mulh of rtype
  | Mulhsu of rtype
  | Mulhu of rtype
  | Div of rtype
  | Divu of rtype
  | Rem of rtype
  | Remu of rtype
  | Lr_w of
      { dest : register
      ; address : register
      }
  | Sc_w of
      { dest : register
      ; address : register
      ; src : register
      }
  | Amoswap_w of amo
  | Amoadd_w of amo
  | Amoxor_w of amo
  | Amoand_w of amo
  | Amoor_w of amo
  | Amomin_w of amo
  | Amomax_w of amo
  | Amominu_w of amo
  | Amomaxu_w of amo
  | Csrrw of csr_reg
  | Csrrs of csr_reg
  | Csrrc of csr_reg
  | Csrrwi of csr_imm
  | Csrrsi of csr_imm
  | Csrrci of csr_imm
  | Mret
  | Wfi
  | Fence
[@@deriving sexp_of, quickcheck, variants, equal]

(* Assembly formatting helpers *)
let rtype op { dest; src1; src2 } = sprintf "%s x%d, x%d, x%d" op dest src1 src2
let itype op { dest; src; immediate } = sprintf "%s x%d, x%d, %d" op dest src immediate
let shift op { dest; src; shamt } = sprintf "%s x%d, x%d, %d" op dest src shamt
let utype op { dest; immediate } = sprintf "%s x%d, %d" op dest immediate
let branch op { src1; src2; target } = sprintf "%s x%d, x%d, .+%d" op src1 src2 target

let load op ({ dest; address = { register; offset } } : load) =
  sprintf "%s x%d, %d(x%d)" op dest offset register
;;

let store op ({ src; address = { register; offset } } : store) =
  sprintf "%s x%d, %d(x%d)" op src offset register
;;

let amo op { dest; address; src } = sprintf "%s x%d, x%d, (x%d)" op dest src address
let csr_reg op { dest; csr; src } = sprintf "%s x%d, %d, x%d" op dest csr src
let csr_imm op { dest; csr; imm } = sprintf "%s x%d, %d, %d" op dest csr imm

let to_asm = function
  | Add r -> rtype "add" r
  | Sub r -> rtype "sub" r
  | Slt r -> rtype "slt" r
  | Sltu r -> rtype "sltu" r
  | And r -> rtype "and" r
  | Or r -> rtype "or" r
  | Xor r -> rtype "xor" r
  | Sll r -> rtype "sll" r
  | Srl r -> rtype "srl" r
  | Sra r -> rtype "sra" r
  | Mul r -> rtype "mul" r
  | Mulh r -> rtype "mulh" r
  | Mulhsu r -> rtype "mulhsu" r
  | Mulhu r -> rtype "mulhu" r
  | Div r -> rtype "div" r
  | Divu r -> rtype "divu" r
  | Rem r -> rtype "rem" r
  | Remu r -> rtype "remu" r
  | Addi i -> itype "addi" i
  | Slti i -> itype "slti" i
  | Sltiu i -> itype "sltiu" i
  | Andi i -> itype "andi" i
  | Ori i -> itype "ori" i
  | Xori i -> itype "xori" i
  | Slli s -> shift "slli" s
  | Srli s -> shift "srli" s
  | Srai s -> shift "srai" s
  | Lb l -> load "lb" l
  | Lh l -> load "lh" l
  | Lw l -> load "lw" l
  | Lbu l -> load "lbu" l
  | Lhu l -> load "lhu" l
  | Sb s -> store "sb" s
  | Sh s -> store "sh" s
  | Sw s -> store "sw" s
  | Beq b -> branch "beq" b
  | Bne b -> branch "bne" b
  | Blt b -> branch "blt" b
  | Bge b -> branch "bge" b
  | Bltu b -> branch "bltu" b
  | Bgeu b -> branch "bgeu" b
  | Jal { dest; offset } -> sprintf "jal x%d, .+%d" dest offset
  | Jalr { dest; base; offset } -> sprintf "jalr x%d, x%d, %d" dest base offset
  | Lui u -> utype "lui" u
  | Auipc u -> utype "auipc" u
  | Ecall -> "ecall"
  | Ebreak -> "ebreak"
  | Mret -> "mret"
  | Wfi -> "wfi"
  | Fence -> "fence"
  | Lr_w { dest; address } -> sprintf "lr.w x%d, (x%d)" dest address
  | Sc_w { dest; address; src } -> sprintf "sc.w x%d, x%d, (x%d)" dest src address
  | Amoswap_w a -> amo "amoswap.w" a
  | Amoadd_w a -> amo "amoadd.w" a
  | Amoxor_w a -> amo "amoxor.w" a
  | Amoand_w a -> amo "amoand.w" a
  | Amoor_w a -> amo "amoor.w" a
  | Amomin_w a -> amo "amomin.w" a
  | Amomax_w a -> amo "amomax.w" a
  | Amominu_w a -> amo "amominu.w" a
  | Amomaxu_w a -> amo "amomaxu.w" a
  | Csrrw c -> csr_reg "csrrw" c
  | Csrrs c -> csr_reg "csrrs" c
  | Csrrc c -> csr_reg "csrrc" c
  | Csrrwi c -> csr_imm "csrrwi" c
  | Csrrsi c -> csr_imm "csrrsi" c
  | Csrrci c -> csr_imm "csrrci" c
;;

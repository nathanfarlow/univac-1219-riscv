open! Core
open! Import

type register = int [@@deriving sexp_of, quickcheck, equal]

type memory_operand =
  { offset : int (** 12-bit signed *)
  ; register : register
  }
[@@deriving sexp_of, quickcheck, equal]

type rtype =
  { dest : register
  ; src1 : register
  ; src2 : register
  }
[@@deriving sexp_of, quickcheck, equal]

type itype =
  { dest : register
  ; src : register
  ; immediate : int (** 12-bit signed *)
  }
[@@deriving sexp_of, quickcheck, equal]

type shift_type =
  { dest : register
  ; src : register
  ; shamt : int (** 5-bit unsigned *)
  }
[@@deriving sexp_of, quickcheck, equal]

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
  ; target : int (** 13-bit signed *)
  }
[@@deriving sexp_of, quickcheck, equal]

type utype =
  { dest : register
  ; immediate : int (** 20-bit unsigned *)
  }
[@@deriving sexp_of, quickcheck, equal]

type csr_reg =
  { dest : register
  ; csr : int (** 12-bit unsigned *)
  ; src : register
  }
[@@deriving sexp_of, quickcheck, equal]

type csr_imm =
  { dest : register
  ; csr : int (** 12-bit unsigned *)
  ; imm : int (** 5-bit unsigned *)
  }
[@@deriving sexp_of, quickcheck, equal]

type amo =
  { dest : register
  ; address : register
  ; src : register
  }
[@@deriving sexp_of, quickcheck, equal]

type jal =
  { dest : register
  ; offset : int (** 21-bit signed *)
  }
[@@deriving sexp_of, quickcheck, equal]

type jalr =
  { dest : register
  ; base : register
  ; offset : int (** 12-bit signed *)
  }
[@@deriving sexp_of, quickcheck, equal]

type t =
  | Add of rtype (** Add *)
  | Sub of rtype (** Subtract *)
  | Addi of itype (** Add Immediate *)
  | Lb of load (** Load Byte *)
  | Sb of store (** Store Byte *)
  | Jal of jal (** Jump and Link *)
  | Jalr of jalr (** Jump and Link Register *)
  | Beq of branch (** Branch if Equal *)
  | Slt of rtype (** Set Less Than *)
  | Lui of utype (** Load Upper Immediate *)
  | Ecall (** Environment Call *)
  | Ebreak (** Environment Break *)
  | Lh of load (** Load Halfword *)
  | Lw of load (** Load Word *)
  | Sh of store (** Store Halfword *)
  | Sw of store (** Store Word *)
  | Bne of branch (** Branch if Not Equal *)
  | Blt of branch (** Branch if Less Than *)
  | Bge of branch (** Branch if Greater or Equal *)
  | Bltu of branch (** Branch if Less Than Unsigned *)
  | Bgeu of branch (** Branch if Greater or Equal Unsigned *)
  | Slti of itype (** Set Less Than Immediate *)
  | Sltu of rtype (** Set Less Than Unsigned *)
  | Sltiu of itype (** Set Less Than Immediate Unsigned *)
  | Auipc of utype (** Add Upper Immediate to PC *)
  | And of rtype (** Bitwise AND *)
  | Or of rtype (** Bitwise OR *)
  | Xor of rtype (** Bitwise XOR *)
  | Slli of shift_type (** Shift Left Logical Immediate *)
  | Srli of shift_type (** Shift Right Logical Immediate *)
  | Srai of shift_type (** Shift Right Arithmetic Immediate *)
  | Sll of rtype (** Shift Left Logical *)
  | Srl of rtype (** Shift Right Logical *)
  | Sra of rtype (** Shift Right Arithmetic *)
  | Andi of itype (** Bitwise AND Immediate *)
  | Ori of itype (** Bitwise OR Immediate *)
  | Xori of itype (** Bitwise XOR Immediate *)
  | Lbu of load (** Load Byte Unsigned *)
  | Lhu of load (** Load Halfword Unsigned *)
  (* RV32M - Multiply/Divide Extension *)
  | Mul of rtype (** Multiply *)
  | Mulh of rtype (** Multiply High Signed *)
  | Mulhsu of rtype (** Multiply High Signed-Unsigned *)
  | Mulhu of rtype (** Multiply High Unsigned *)
  | Div of rtype (** Divide Signed *)
  | Divu of rtype (** Divide Unsigned *)
  | Rem of rtype (** Remainder Signed *)
  | Remu of rtype (** Remainder Unsigned *)
  (* RV32A - Atomic Extension *)
  | Lr_w of
      { dest : register
      ; address : register
      } (** Load Reserved Word *)
  | Sc_w of
      { dest : register
      ; address : register
      ; src : register
      } (** Store Conditional Word *)
  | Amoswap_w of amo (** Atomic Memory Operation: Swap Word *)
  | Amoadd_w of amo (** Atomic Memory Operation: Add Word *)
  | Amoxor_w of amo (** Atomic Memory Operation: XOR Word *)
  | Amoand_w of amo (** Atomic Memory Operation: AND Word *)
  | Amoor_w of amo (** Atomic Memory Operation: OR Word *)
  | Amomin_w of amo (** Atomic Memory Operation: Min Word *)
  | Amomax_w of amo (** Atomic Memory Operation: Max Word *)
  | Amominu_w of amo (** Atomic Memory Operation: Min Unsigned Word *)
  | Amomaxu_w of amo (** Atomic Memory Operation: Max Unsigned Word *)
  (* CSR - Control and Status Register Instructions *)
  | Csrrw of csr_reg (** CSR Read/Write *)
  | Csrrs of csr_reg (** CSR Read and Set Bits *)
  | Csrrc of csr_reg (** CSR Read and Clear Bits *)
  | Csrrwi of csr_imm (** CSR Read/Write Immediate *)
  | Csrrsi of csr_imm (** CSR Read and Set Bits Immediate *)
  | Csrrci of csr_imm (** CSR Read and Clear Bits Immediate *)
  (* System Instructions *)
  | Mret (** Machine Return *)
  | Wfi (** Wait for Interrupt *)
  (* Memory Ordering *)
  | Fence (** Memory Fence *)
[@@deriving sexp_of, quickcheck, variants, equal]

(** Convert an instruction to its assembly string representation *)
val to_asm : t -> string

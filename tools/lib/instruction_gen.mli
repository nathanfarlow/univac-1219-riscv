open! Core
open! Import

(** {1 Single Instruction Generators} *)

val add : Instruction.t Quickcheck.Generator.t
val sub : Instruction.t Quickcheck.Generator.t
val slt : Instruction.t Quickcheck.Generator.t
val sltu : Instruction.t Quickcheck.Generator.t
val and_ : Instruction.t Quickcheck.Generator.t
val or_ : Instruction.t Quickcheck.Generator.t
val xor : Instruction.t Quickcheck.Generator.t
val sll : Instruction.t Quickcheck.Generator.t
val srl : Instruction.t Quickcheck.Generator.t
val sra : Instruction.t Quickcheck.Generator.t
val addi : Instruction.t Quickcheck.Generator.t
val slti : Instruction.t Quickcheck.Generator.t
val sltiu : Instruction.t Quickcheck.Generator.t
val andi : Instruction.t Quickcheck.Generator.t
val ori : Instruction.t Quickcheck.Generator.t
val xori : Instruction.t Quickcheck.Generator.t
val slli : Instruction.t Quickcheck.Generator.t
val srli : Instruction.t Quickcheck.Generator.t
val srai : Instruction.t Quickcheck.Generator.t
val lui : Instruction.t Quickcheck.Generator.t
val auipc : Instruction.t Quickcheck.Generator.t
val mul : Instruction.t Quickcheck.Generator.t
val mulh : Instruction.t Quickcheck.Generator.t
val mulhsu : Instruction.t Quickcheck.Generator.t
val mulhu : Instruction.t Quickcheck.Generator.t
val div : Instruction.t Quickcheck.Generator.t
val divu : Instruction.t Quickcheck.Generator.t
val rem : Instruction.t Quickcheck.Generator.t
val remu : Instruction.t Quickcheck.Generator.t

(** {1 Multi-Instruction Generators} *)

(** Load with address setup. Dependencies: LUI, ADDI *)
val load
  :  memory_base:int
  -> memory_size:int
  -> (Instruction.load -> Instruction.t)
  -> Instruction.t list Quickcheck.Generator.t

(** Store with address setup. Dependencies: LUI, ADDI *)
val store
  :  memory_base:int
  -> memory_size:int
  -> (Instruction.store -> Instruction.t)
  -> Instruction.t list Quickcheck.Generator.t

(** Forward branch (target = +4, skips one instruction) *)
val forward_branch
  :  (Instruction.branch -> Instruction.t)
  -> Instruction.t Quickcheck.Generator.t
  -> Instruction.t list Quickcheck.Generator.t

(** Backward branch (small loop). Dependencies: JAL *)
val backward_branch
  :  (Instruction.branch -> Instruction.t)
  -> Instruction.t Quickcheck.Generator.t
  -> Instruction.t list Quickcheck.Generator.t

(** JAL sequence (tests jumps and link register) *)
val jal
  :  Instruction.t Quickcheck.Generator.t
  -> Instruction.t list Quickcheck.Generator.t

(** JALR sequence (tests indirect jumps). Dependencies: JAL *)
val jalr
  :  Instruction.t Quickcheck.Generator.t
  -> Instruction.t list Quickcheck.Generator.t

(** Exit syscall: [ADDI x17, x0, 2; ECALL] *)
val suffix : Instruction.t list

(** Full program with all instruction types *)
val full_program
  :  max_length:int
  -> memory_base:int
  -> memory_size:int
  -> include_divrem:bool
  -> Instruction.t list list Quickcheck.Generator.t

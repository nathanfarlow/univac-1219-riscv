open! Core
open! Import

(** Encode an instruction into two 18 bit words. *)
val encode : Instruction.t -> int * int

(** Used for round trip testing of [encode] *)
val decode : int -> int -> Instruction.t

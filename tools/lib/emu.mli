open! Core
open! Import
open Instruction

module State : sig
  type t

  val read_register : t -> register -> int
  val write_register : t -> register -> int -> unit
  val pc : t -> int
end

val run : memory_base:int -> Instruction.t list -> State.t
val run_binary : memory_base:int -> string -> State.t

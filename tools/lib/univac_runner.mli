open! Core
open! Async
open! Import

val encode_program_with_data : Instruction.t list -> string -> string

val embed_program_to_file
  :  output_file:string
  -> ?template_file:string
  -> string
  -> unit Deferred.t

val execute : Instruction.t list -> Process.Output.t Deferred.Or_error.t
val run : Instruction.t list -> (int array * int) Deferred.Or_error.t
val parse_timing : Process.Output.t -> int

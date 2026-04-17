open! Core
open! Async

(** Execute code blocks (delimited by ```) and replace with their output. *)
val process_template : string -> string Deferred.t

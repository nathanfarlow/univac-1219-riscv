open! Core
open! Import
open Instruction

val decode_instruction : int -> t
val parse_all : binary:string -> t list

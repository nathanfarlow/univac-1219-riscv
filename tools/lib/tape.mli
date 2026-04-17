open! Core
open! Async

(** Convert a .76 file to a .bin file. This is the bin file you'd write to the
    floppy disk at the museum. *)
val convert_76_to_bin : input_file:Filename.t -> output_file:Filename.t -> unit Deferred.t

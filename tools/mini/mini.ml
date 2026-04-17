open! Core
open Ctypes
module C = Mini_c.Functions
module Types = Mini_c.Types

type emulator_result = { regs : int array }

let execute_binary_file ~max_instructions (file_path : Filename.t) =
  let result_ptr = C.execute_binary_file file_path max_instructions in
  let result_struct = !@result_ptr in
  let regs_array = getf result_struct Types.EmulatorResult.regs in
  let regs =
    Array.init 32 ~f:(fun i -> Unsigned.UInt32.to_int (CArray.get regs_array i))
  in
  C.free_emulator_result result_ptr;
  Some { regs }
;;

open! Core

type emulator_result = { regs : int array }

val execute_binary_file : max_instructions:int -> Filename.t -> emulator_result option

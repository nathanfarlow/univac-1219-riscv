open! Core
open Ctypes
module Types = Types_generated

module Functions (F : FOREIGN) = struct
  open F

  let execute_binary_file =
    foreign
      "execute_binary_file"
      (string @-> int @-> returning (ptr Types.EmulatorResult.emulator_result))
  ;;

  let free_emulator_result =
    foreign
      "free_emulator_result"
      (ptr Types.EmulatorResult.emulator_result @-> returning void)
  ;;
end

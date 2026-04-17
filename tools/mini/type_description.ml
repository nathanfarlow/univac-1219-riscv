open! Core
open Ctypes

module Types (F : TYPE) = struct
  open F

  module EmulatorResult = struct
    type emulator_result

    let emulator_result : emulator_result structure typ = structure "EmulatorResult"
    let regs = field emulator_result "regs" (array 32 uint32_t)
    let () = seal emulator_result
  end
end

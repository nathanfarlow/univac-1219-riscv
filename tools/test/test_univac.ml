open! Core
open! Async
open! Tools
open! Lithium

let mask_auipc =
  List.map ~f:(function
    | Instruction.Auipc { dest; immediate } ->
      Instruction.Auipc { dest; immediate = immediate land ((1 lsl 18) - 1) }
    | instr -> instr)
;;

module Univac = struct
  let execute instructions = Univac_runner.execute (mask_auipc instructions)

  let run instructions =
    Univac_runner.run (mask_auipc instructions) >>| Or_error.map ~f:fst
  ;;

  let run_with_timing instructions = Univac_runner.run (mask_auipc instructions)
end

let%expect_test "fuzz RISC-V instructions on UNIVAC" =
  let%bind total_timing =
    Test_helpers.fuzz_compare_runners_parallel_with_timing
      ~runner1:(fun instrs -> Test_helpers.run_oracle ~memory_base:0 instrs)
      ~runner2_with_timing:Univac.run_with_timing
      ~trials:2000
      ~suffix:Instruction_gen.suffix
      ~instruction_gen:
        (Instruction_gen.full_program
           ~max_length:7000
           ~memory_base:0xB000
           ~memory_size:32
           ~include_divrem:false
         |> Quickcheck.Generator.map ~f:(List.map ~f:mask_auipc))
      ~max_concurrent:16
  in
  print_s
    [%message
      "Total microseconds across all univac fuzz tests spent executing UNIVAC \
       instructions"
        (total_timing : int)];
  [%expect
    {|
    ("Total microseconds across all univac fuzz tests spent executing UNIVAC instructions"
     (total_timing 7161312200))
    |}];
  return ()
;;

let%expect_test "putc syscall" =
  let%bind output =
    Test_helpers.ecall_char_printing_instructions ()
    >>= Univac.execute
    >>| Or_error.ok_exn
  in
  print_endline output.Process.Output.stdout;
  [%expect {| HI |}];
  return ()
;;

let%expect_test "memclear syscall" =
  let memory_base = 0xB000 in
  let%bind instrs = Test_helpers.memclear_syscall_instructions ~memory_base in
  instrs @ Instruction_gen.suffix
  |> Test_helpers.compare_runners_async
       ~runner1:(fun instrs ->
         Test_helpers.run_oracle ~memory_base instrs |> Deferred.Or_error.return)
       ~runner2:Univac.run
  |> Deferred.Or_error.ok_exn
;;

let%expect_test "time syscall" =
  let%bind instrs =
    [ "addi x17, x0, 4"; "ecall" ]
    |> String.concat ~sep:"\n"
    |> Test_helpers.parse_assembly
  in
  let%bind regs = instrs @ Instruction_gen.suffix |> Univac.run >>| Or_error.ok_exn in
  print_s [%message (regs.(10) : int)];
  [%expect {| ("regs.(10)" 0) |}];
  return ()
;;

open! Core
open! Async
open! Tools
open! Lithium

let run_mini instructions =
  instructions
  |> List.map ~f:Instruction.to_asm
  |> String.concat ~sep:"\n"
  |> Test_helpers.with_assemble ~f:(fun ~file_path ~contents:_ ->
    match Mini.execute_binary_file ~max_instructions:200000 file_path with
    | Some r -> Deferred.Or_error.return (r.Mini.regs, 0)
    | None -> Or_error.error_s [%message "Mini emulator failed"] |> Deferred.return)
;;

let%expect_test "fuzz instruction programs" =
  let%bind _timing =
    Test_helpers.fuzz_compare_runners_parallel_with_timing
      ~runner1:(fun instrs -> Test_helpers.run_oracle ~memory_base:0x80000000 instrs)
      ~runner2_with_timing:run_mini
      ~trials:100
      ~suffix:Instruction_gen.suffix
      ~instruction_gen:
        (Instruction_gen.full_program
           ~include_divrem:true
           ~max_length:5000
           ~memory_base:(0x80000000 + (60 * 1024 * 1024))
           ~memory_size:10)
      ~max_concurrent:16
  in
  [%expect {| |}];
  return ()
;;

let%expect_test "putc syscall" =
  let%bind _state =
    Test_helpers.ecall_char_printing_instructions () >>| Emu.run ~memory_base:0x80000000
  in
  [%expect {| Hi |}];
  return ()
;;

let%expect_test "memclear syscall" =
  let memory_base = 0x80000000 in
  let%bind instrs = Test_helpers.memclear_syscall_instructions ~memory_base in
  let state = instrs @ Instruction_gen.suffix |> Test_helpers.run_oracle ~memory_base in
  (* Verify that registers x1-x4 are all zero after memclear *)
  print_s
    [%message (state.(1) : int) (state.(2) : int) (state.(3) : int) (state.(4) : int)];
  [%expect {| (("state.(1)" 0) ("state.(2)" 0) ("state.(3)" 0) ("state.(4)" 0)) |}];
  return ()
;;

let%expect_test "time syscall" =
  let memory_base = 0x80000000 in
  let%bind instrs =
    [ "addi x17, x0, 4"; "ecall" ]
    |> String.concat ~sep:"\n"
    |> Test_helpers.parse_assembly
  in
  let state = instrs @ Instruction_gen.suffix |> Test_helpers.run_oracle ~memory_base in
  print_s [%message (state.(10) : int)];
  [%expect {| ("state.(10)" 0) |}];
  return ()
;;

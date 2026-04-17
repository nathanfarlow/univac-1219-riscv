open! Core
open! Async
open! Tools
open! Lithium

let with_assemble assembly ~f =
  Expect_test_helpers_async.with_temp_dir (fun temp_dir ->
    let ( ~/ ) f = temp_dir ^/ f in
    let asm_file = ~/"test.s" in
    let obj_file = ~/"test.o" in
    let bin_file = ~/"test.bin" in
    let%bind () = Writer.save asm_file ~contents:assembly in
    let%bind () =
      Process.run_expect_no_output_exn
        ~prog:"riscv64-unknown-elf-as"
        ~args:[ "-march=rv32ima_zicsr"; "-mabi=ilp32"; "-o"; obj_file; asm_file ]
        ()
    in
    let%bind () =
      Process.run_expect_no_output_exn
        ~prog:"riscv64-unknown-elf-objcopy"
        ~args:[ "-O"; "binary"; "-j"; ".text"; obj_file; bin_file ]
        ()
    in
    let%bind binary_contents = Reader.file_contents bin_file in
    f ~file_path:bin_file ~contents:binary_contents)
;;

let parse_assembly assembly =
  with_assemble assembly ~f:(fun ~file_path:_ ~contents ->
    Parser.parse_all ~binary:contents |> return)
;;

let run_oracle ~memory_base instructions =
  let state = Emu.run ~memory_base instructions in
  Array.init 32 ~f:(Emu.State.read_register state)
;;

let sign_extend_32 n = (n lsl 32) asr 32

let compare_registers ~expected ~actual =
  List.range 1 32
  |> List.filter_map ~f:(fun i ->
    let exp = sign_extend_32 expected.(i) in
    let act = sign_extend_32 actual.(i) in
    match exp = act with
    | true -> None
    | false -> Some (i, exp, act))
;;

let format_error ~diffs ~instructions =
  let diff_str =
    diffs
    |> List.map ~f:(fun (reg, expected, actual) ->
      sprintf
        "  x%d: expected=%d (0x%08x), actual=%d (0x%08x)"
        reg
        expected
        expected
        actual
        actual)
    |> String.concat ~sep:"\n"
  in
  let instr_str =
    instructions
    |> List.map ~f:(fun i -> "  " ^ (i |> Instruction.sexp_of_t |> Sexp.to_string_hum))
    |> String.concat ~sep:"\n"
  in
  sprintf "Register differences:\n%s\n\nInstructions:\n%s" diff_str instr_str
;;

let compare_runners_async ~runner1 ~runner2 instructions =
  let open Deferred.Or_error.Let_syntax in
  let%bind actual1 =
    Deferred.Or_error.tag_s
      (runner1 instructions)
      ~tag:[%message "Expected runner failed" (instructions : Instruction.t list)]
  in
  let%bind actual2 =
    Deferred.Or_error.tag_s
      (runner2 instructions)
      ~tag:[%message "Actual runner failed" (instructions : Instruction.t list)]
  in
  match compare_registers ~expected:actual1 ~actual:actual2 with
  | [] -> return ()
  | diffs -> Deferred.Or_error.error_string (format_error ~diffs ~instructions)
;;

(** Returns parsed instructions for testing ecall character printing.
    Prints "Hi" when executed. *)
let ecall_char_printing_instructions () =
  [ "addi x10, x0, 72"
  ; "addi x17, x0, 1"
  ; "ecall"
  ; "addi x10, x0, 105"
  ; "addi x17, x0, 1"
  ; "ecall"
  ]
  |> String.concat ~sep:"\n"
  |> parse_assembly
;;

let memclear_syscall_instructions ~memory_base =
  let base_upper = memory_base lsr 12 in
  [ (* Initialize memory with non-zero values using LUI+ADDI for 32-bit values *)
    sprintf "lui x10, 0x%x" base_upper
  ; (* Write non-zero pattern to first word *)
    "lui x11, 0x12345"
  ; "addi x11, x11, 0x678"
  ; "sw x11, 0(x10)"
  ; (* Write non-zero pattern to second word *)
    "lui x11, 0xDEADB"
  ; "addi x11, x11, -0x411"
  ; "sw x11, 4(x10)"
  ; (* Write non-zero pattern to third word *)
    "lui x11, 0xCAFEB"
  ; "addi x11, x11, -0x542"
  ; "sw x11, 8(x10)"
  ; (* Write non-zero pattern to fourth word *)
    "lui x11, 0xABCDE"
  ; "addi x11, x11, -0xFF"
  ; "sw x11, 12(x10)"
  ; (* Set up memclear syscall *)
    sprintf "lui x10, 0x%x" base_upper
  ; sprintf "lui x11, 0x%x" base_upper
  ; "addi x11, x11, 16"
  ; "addi x17, x0, 3"
  ; "ecall"
  ; (* Verify memory is cleared - load and check *)
    sprintf "lui x10, 0x%x" base_upper
  ; "lw x1, 0(x10)"
  ; "lw x2, 4(x10)"
  ; "lw x3, 8(x10)"
  ; "lw x4, 12(x10)"
  ]
  |> String.concat ~sep:"\n"
  |> parse_assembly
;;

let domain_parallel_map ~num_domains items ~f =
  let chunk_size = (List.length items + num_domains - 1) / num_domains in
  items
  |> List.chunks_of ~length:chunk_size
  |> List.map ~f:(fun chunk -> Domain.spawn (fun () -> List.map chunk ~f))
  |> List.concat_map ~f:Domain.join
;;

let fuzz_compare_runners_parallel_with_timing
      ~runner1
      ~runner2_with_timing
      ~trials
      ~instruction_gen
      ~suffix
      ~max_concurrent
  =
  let open Deferred.Let_syntax in
  let test_cases =
    List.init trials ~f:(fun i ->
      let groups =
        Quickcheck.random_value
          ~seed:(`Deterministic [%string "fuzz_test_%{i#Int}"])
          instruction_gen
      in
      groups, List.concat groups @ suffix)
  in
  let%bind results =
    test_cases
    |> domain_parallel_map ~num_domains:32 ~f:(fun (_, instrs) -> runner1 instrs)
    |> List.zip_exn test_cases
    |> Deferred.List.map
         ~how:(`Max_concurrent_jobs max_concurrent)
         ~f:(fun ((groups, instrs), expected) ->
           runner2_with_timing instrs
           >>| function
           | Ok (actual, timing) ->
             (match compare_registers ~expected ~actual with
              | [] -> Ok (timing, None)
              | diffs -> Error (format_error ~diffs ~instructions:instrs, groups))
           | Error e -> Error (Error.to_string_hum e, groups))
  in
  match List.find results ~f:Result.is_error with
  | Some (Error (_, failing_groups)) ->
    let with_suffix instrs = instrs @ suffix in
    let test_fn instrs =
      compare_runners_async
        ~runner1:(fun i -> Deferred.Or_error.return (runner1 (with_suffix i)))
        ~runner2:(fun i -> runner2_with_timing (with_suffix i) >>| Or_error.map ~f:fst)
        instrs
    in
    let%bind reduced =
      In_thread.run (fun () ->
        Lithium.reduce failing_groups ~is_interesting:(function
          | [] -> false
          | groups ->
            Thread_safe.block_on_async_exn (fun () ->
              List.concat groups |> test_fn >>| Or_error.is_error)))
    in
    List.concat reduced |> test_fn |> Deferred.Or_error.ok_exn >>| fun () -> 0
  | _ ->
    results
    |> List.filter_map ~f:(function
      | Ok (timing, _) -> Some timing
      | Error _ -> None)
    |> List.sum (module Int) ~f:Fn.id
    |> return
;;

open! Core
open! Async
open! Tools

let%test_unit "fuzz assemble/parse round trip" =
  Quickcheck.test
    ~sexp_of:[%sexp_of: Instruction.t list]
    ~trials:10
    (Quickcheck.Generator.list_with_length 5000 Instruction.quickcheck_generator)
    ~f:(fun instructions ->
      let assembly =
        instructions |> List.map ~f:Instruction.to_asm |> String.concat ~sep:"\n"
      in
      let parsed =
        Thread_safe.block_on_async_exn (fun () ->
          Test_helpers.with_assemble assembly ~f:(fun ~file_path:_ ~contents ->
            Parser.parse_all ~binary:contents |> return))
      in
      match List.equal Instruction.equal instructions parsed with
      | true -> ()
      | false ->
        raise_s
          [%message
            "Round trip failed"
              (instructions : Instruction.t list)
              (parsed : Instruction.t list)])
;;

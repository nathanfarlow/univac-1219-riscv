open! Core
open! Async

let delim = "<<<DELIM>>>"
let pattern = Re.Perl.compile_pat "```(.*?)```" ~opts:[ `Dotall ]

let extract_blocks content =
  Re.all pattern content |> List.map ~f:(fun g -> Re.Group.get g 1)
;;

let wrap_block code = [%string "let () = print_endline \"%{delim}\" ;;\n%{code}\n;;"]

let execute_all_blocks blocks =
  let temp = Filename_unix.temp_file "template_" ".ml" in
  Monitor.protect
    ~finally:(fun () ->
      Core_unix.unlink temp;
      return ())
    (fun () ->
       let%bind () =
         List.map blocks ~f:wrap_block
         |> String.concat ~sep:"\n"
         |> ( ^ ) "#use \"topfind\";;\n#require \"core\";;\nopen Core;;\n"
         |> fun script -> Writer.save temp ~contents:script
       in
       Process.run_exn ~prog:"opam" ~args:[ "exec"; "--"; "ocaml"; temp ] ())
;;

let parse_outputs output =
  Str.split_delim (Str.regexp_string delim) output
  |> List.tl_exn
  |> List.map ~f:String.strip
;;

let process_template content =
  match extract_blocks content with
  | [] -> return content
  | blocks ->
    let%map output = execute_all_blocks blocks in
    let results = parse_outputs output |> Queue.of_list in
    Re.replace pattern content ~f:(fun _ -> Queue.dequeue_exn results)
;;

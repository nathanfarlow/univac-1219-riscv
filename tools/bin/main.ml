open Core
open Async
open Tools

let parse_binary_split ~binary_file ~num_instructions =
  let%map contents = Reader.file_contents binary_file in
  let instruction_bytes = num_instructions * 4 in
  if String.length contents < instruction_bytes
  then
    raise_s
      [%message
        "File too small"
          (String.length contents : int)
          (instruction_bytes : int)
          (num_instructions : int)];
  ( Parser.parse_all ~binary:(String.prefix contents instruction_bytes)
  , String.drop_prefix contents instruction_bytes )
;;

let run_emulator =
  Command.async
    ~summary:"Run RISC-V emulator on a binary file"
    (let%map_open.Command binary_file = anon ("BINARY_FILE" %: Filename_unix.arg_type) in
     fun () ->
       Reader.file_contents binary_file >>| Emu.run_binary ~memory_base:0 >>| ignore)
;;

let template =
  Command.async
    ~summary:"Process template file, execute code blocks and replace with output"
    (let%map_open.Command input_file = anon ("INPUT_FILE" %: Filename_unix.arg_type) in
     fun () ->
       let%bind content = Reader.file_contents input_file in
       let%map processed = Templating.process_template content in
       print_string processed)
;;

let univac_embed =
  Command.async
    ~summary:"Embed RISC-V binary into UNIVAC template and write .pp file"
    (let%map_open.Command binary_file = anon ("BINARY_FILE" %: Filename_unix.arg_type)
     and num_instructions =
       flag "-n" (required int) ~doc:"NUM Number of instructions to encode"
     and output_file = flag "-o" (required string) ~doc:"FILE Output .pp file"
     and template_file =
       flag "-t" (optional Filename_unix.arg_type) ~doc:"FILE Template file"
     in
     fun () ->
       parse_binary_split ~binary_file ~num_instructions
       >>| Tuple2.uncurry Univac_runner.encode_program_with_data
       >>= Univac_runner.embed_program_to_file ~output_file ?template_file)
;;

let tape =
  Command.async
    ~summary:"Convert .76 tape file to binary (exclude checksum)"
    (let%map_open.Command input_file = anon ("INPUT.76" %: Filename_unix.arg_type)
     and output_file = flag "-o" (required string) ~doc:"FILE Output binary file" in
     fun () -> Tape.convert_76_to_bin ~input_file ~output_file)
;;

let ascii_art =
  Command.async
    ~summary:"Generate UNIVAC source that displays ASCII art via OUT"
    (let%map_open.Command input_file = anon ("TEXT_FILE" %: Filename_unix.arg_type)
     and raw =
       flag "-raw" no_arg ~doc:" Pass bytes through verbatim (no line-ending conversion)"
     in
     fun () ->
       let%map text = Reader.file_contents input_file in
       let chars =
         if raw
         then text
         else
           String.rstrip text
           |> String.split_lines
           |> List.map ~f:String.rstrip
           |> String.concat ~sep:"\r\n"
       in
       let n = String.length chars in
       print_string
         {|ORG &O1750
SADD &O1750

SIL 0
ENTSR 0

EXF 0
DATA EF
DATA EF

OUT 0
DATA ART_END
DATA ART_START

>DONE JP DONE

>EF DATA &O13
|};
       String.iteri chars ~f:(fun i ch ->
         let label =
           match i with
           | 0 -> ">ART_START "
           | i when i = n - 1 -> ">ART_END "
           | _ -> ""
         in
         printf "%sDATA &O%o\n" label (Char.to_int ch));
       print_string "\nEND\n")
;;

let () =
  Command_unix.run
    (Command.group
       ~summary:"RISC-V tools"
       [ "run", run_emulator
       ; "univac-embed", univac_embed
       ; "template", template
       ; "tape", tape
       ; "ascii-art", ascii_art
       ])
;;

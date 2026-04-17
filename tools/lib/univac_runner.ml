open! Core
open! Async
open! Import

let reg_offset = 1130

let univac_emu_dir () =
  Option.value (Sys.getenv "DUNE_SOURCEROOT") ~default:"../.." ^ "/../univac-1219-emu"
;;

let univac_dir () =
  Option.value (Sys.getenv "DUNE_SOURCEROOT") ~default:"../.." ^ "/../asm/riscv-emu"
;;

let load_split_files () =
  let dir = univac_dir () in
  let%bind files =
    Sys.readdir dir
    >>| Array.to_list
    >>| List.filter ~f:(String.is_suffix ~suffix:".univac")
    >>| List.filter ~f:(fun file -> not (String.is_prefix file ~prefix:".#"))
    >>| List.sort ~compare:String.compare
  in
  Deferred.List.map files ~how:`Parallel ~f:(fun file ->
    Reader.file_contents (dir ^/ file))
  >>| String.concat ~sep:"\n"
;;

let template_contents_cache =
  lazy
    (let%bind raw_template = load_split_files () in
     Templating.process_template raw_template)
;;

let encode_program instructions =
  List.map instructions ~f:Univac_encoding.encode
  |> List.concat_mapi ~f:(fun i (w1, w2) ->
    let w2_masked = if w2 < 0 then w2 land 0x3FFFF else w2 in
    if i = 0
    then [ sprintf "DATA &O%o" w1; sprintf ">RISCV_PROGRAM_PLUS_ONE DATA &O%o" w2_masked ]
    else [ sprintf "DATA &O%o" w1; sprintf "DATA &O%o" w2_masked ])
  |> String.concat ~sep:"\n"
;;

let encode_data data =
  String.to_list data
  |> List.chunks_of ~length:2
  |> List.map ~f:(fun bytes ->
    List.foldi bytes ~init:0 ~f:(fun i acc ch -> acc lor (Char.to_int ch lsl (i * 8)))
    |> sprintf "DATA &O%o")
  |> String.concat ~sep:"\n"
;;

let encode_program_with_data instructions data_bytes =
  [ encode_program instructions; encode_data data_bytes ]
  |> List.filter ~f:(fun s -> not (String.is_empty s))
  |> String.concat ~sep:"\n"
;;

let embed_program_to_file ~output_file ?template_file program_encoding =
  let%bind template_contents =
    match template_file with
    | Some file -> Reader.file_contents file
    | None -> Lazy.force template_contents_cache
  in
  let data =
    String.substr_replace_all
      template_contents
      ~pattern:"%%PROGRAM%%"
      ~with_:program_encoding
  in
  Writer.save output_file ~contents:data
;;

let parse_timing output =
  output.Process.Output.stderr
  |> String.split_lines
  |> List.find_map ~f:(String.chop_prefix ~prefix:"Total time: ")
  |> Option.bind ~f:(String.chop_suffix ~suffix:" microseconds")
  |> Option.bind ~f:Int.of_string_opt
  |> Option.value ~default:0
;;

let parse_registers dump_file =
  let%map contents = Reader.file_contents dump_file in
  let lines = String.split_lines contents in
  Array.init 32 ~f:(fun i ->
    let lower = List.nth_exn lines (reg_offset + (i * 2)) |> Int.of_string in
    let upper = List.nth_exn lines (reg_offset + (i * 2) + 1) |> Int.of_string in
    let value = (upper lsl 18) lor lower in
    sign_extend ~bits:32 value)
;;

let with_temp_dir f =
  let temp_dir = Filename_unix.temp_dir "univac_test" "" in
  Monitor.protect
    (fun () -> f temp_dir)
    ~finally:(fun () ->
      return
        (Sys_unix.readdir temp_dir
         |> Array.iter ~f:(fun file -> Sys_unix.remove (temp_dir ^/ file));
         Core_unix.rmdir temp_dir))
;;

let write_univac_program ~temp_dir program_encoding =
  let output_file = temp_dir ^/ "emu.univac.pp" in
  let%map () = embed_program_to_file ~output_file program_encoding in
  output_file
;;

let execute_with_temp_dir ~temp_dir program_encoding =
  let%bind output_file = write_univac_program ~temp_dir program_encoding in
  let dump_file = temp_dir ^/ "memory.dump" in
  let args =
    [ "5s"
    ; "cargo"
    ; "run"
    ; "--quiet"
    ; "--release"
    ; "--bin"
    ; "emulator"
    ; "--"
    ; "--source-file"
    ; output_file
    ; "--dump-memory"
    ; dump_file
    ; "--stop"
    ; "0"
    ]
  in
  let%bind.Deferred.Or_error process =
    Process.create ~working_dir:(univac_emu_dir ()) ~prog:"timeout" ~args ()
  in
  let%bind output = Process.collect_output_and_wait process in
  Deferred.Or_error.return (dump_file, output)
;;

let execute instructions =
  with_temp_dir (fun temp_dir ->
    Deferred.Or_error.map
      (execute_with_temp_dir ~temp_dir (encode_program instructions))
      ~f:snd)
;;

let run instructions =
  with_temp_dir (fun temp_dir ->
    let%bind.Deferred.Or_error dump_file, output =
      execute_with_temp_dir ~temp_dir (encode_program instructions)
    in
    let%bind file_exists = Sys.file_exists dump_file in
    match file_exists with
    | `Yes ->
      let%bind registers = parse_registers dump_file in
      Deferred.Or_error.return (registers, parse_timing output)
    | `No | `Unknown ->
      Deferred.Or_error.error_s
        [%message
          "Memory dump file was not created - emulator likely failed"
            (dump_file : string)
            ~stderr:(output.Process.Output.stderr : string)
            ~stdout:(output.Process.Output.stdout : string)])
;;

open! Core
open! Async

let convert_76_to_bin ~input_file ~output_file =
  Reader.file_lines input_file
  >>| List.filter_map ~f:(fun s -> String.strip s |> Int.of_string_opt)
  >>| (fun l -> List.take l (List.length l - 3)) (* Drop checksum *)
  >>| List.map ~f:Char.of_int_exn
  >>| String.of_list
  >>= fun contents -> Writer.save output_file ~contents
;;

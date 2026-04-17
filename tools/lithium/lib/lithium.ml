open! Core

let filter_chunks_of_size items ~chunk_size ~is_interesting =
  let rec filter_chunks prefix = function
    | [] -> prefix
    | chunk :: remaining_chunks ->
      let without_chunk = prefix @ List.concat remaining_chunks in
      (match is_interesting without_chunk with
       | true -> filter_chunks prefix remaining_chunks
       | false -> filter_chunks (prefix @ chunk) remaining_chunks)
  in
  List.chunks_of items ~length:chunk_size |> filter_chunks []
;;

let rec reduce_rounds items ~chunk_size ~is_interesting =
  match chunk_size with
  | 0 -> items
  | _ ->
    let new_items = filter_chunks_of_size items ~chunk_size ~is_interesting in
    if List.length new_items < List.length items
    then reduce_rounds new_items ~chunk_size ~is_interesting
    else reduce_rounds items ~chunk_size:(chunk_size / 2) ~is_interesting
;;

let reduce items = reduce_rounds items ~chunk_size:(List.length items)

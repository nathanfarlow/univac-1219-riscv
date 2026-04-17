open! Core

let sign_extend ~bits value =
  let sign_bit = 1 lsl (bits - 1) in
  if value land sign_bit <> 0 then value - (1 lsl bits) else value
;;

let mask ~bits value =
  let mask = (1 lsl bits) - 1 in
  value land mask
;;

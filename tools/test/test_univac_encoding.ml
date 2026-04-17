open! Core
open! Tools

let clamp n = Int.clamp_exn n ~min:(-1 lsl 17) ~max:((1 lsl 17) - 1)

let%test_unit "fuzz encode/decode round trip" =
  Quickcheck.test
    ~sexp_of:[%sexp_of: Instruction.t]
    ~trials:10
    (Instruction.quickcheck_generator
     |> Quickcheck.Generator.map ~f:(function
       | Jal { dest; offset } ->
         (* Clamp then align to 2-byte boundary *)
         Instruction.jal { dest; offset = clamp offset land lnot 1 }
       | Jalr { dest; base; offset } ->
         (* Clamp then align to 2-byte boundary *)
         Instruction.jalr { dest; base; offset = clamp offset land lnot 1 }
       | Beq { src1; src2; target } ->
         Instruction.beq { src1; src2; target = clamp target land lnot 1 }
       | Bne { src1; src2; target } ->
         Instruction.bne { src1; src2; target = clamp target land lnot 1 }
       | Blt { src1; src2; target } ->
         Instruction.blt { src1; src2; target = clamp target land lnot 1 }
       | Bge { src1; src2; target } ->
         Instruction.bge { src1; src2; target = clamp target land lnot 1 }
       | Bltu { src1; src2; target } ->
         Instruction.bltu { src1; src2; target = clamp target land lnot 1 }
       | Bgeu { src1; src2; target } ->
         Instruction.bgeu { src1; src2; target = clamp target land lnot 1 }
       | instr -> instr))
    ~f:(fun instr ->
      match
        Univac_encoding.encode instr |> fun (w1, w2) -> Univac_encoding.decode w1 w2
      with
      | decoded when Instruction.equal instr decoded -> ()
      | decoded ->
        raise_s
          [%message
            "Round trip failed: %s -> %s"
              (instr : Instruction.t)
              (decoded : Instruction.t)]
      | exception exn
        when String.is_substring (Exn.to_string exn) ~substring:"not supported" -> ())
;;

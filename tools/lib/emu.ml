open! Core
open! Import
open Instruction

module State = struct
  type t =
    { mutable pc : int
    ; registers : int array
    ; memory : (read_write, Iobuf.seek) Iobuf.t
    ; memory_base : int
    ; memory_size : int
    }

  let read_register state reg = sign_extend ~bits:32 state.registers.(reg)

  let write_register state reg value =
    if reg <> 0 then state.registers.(reg) <- mask ~bits:32 value
  ;;

  let pc state = state.pc

  let create ~memory_size ~memory_base ~image =
    let memory = Iobuf.create ~len:memory_size in
    (* Fill with some non zero value to make sure we don't rely on
       zero-initialized memory *)
    Iobuf.Fill.char memory '\xff';
    let state =
      { pc = memory_base
      ; registers = Array.create ~len:32 0
      ; memory
      ; memory_base
      ; memory_size
      }
    in
    let src_iobuf = Iobuf.of_string image in
    let copy_len = min (Iobuf.length src_iobuf) (Iobuf.length state.memory) in
    Iobuf.Blit.blito ~src:src_iobuf ~src_len:copy_len ~dst:state.memory ~dst_pos:0 ();
    state
  ;;

  let translate_address state addr =
    let relative_addr = mask ~bits:32 addr - state.memory_base in
    if relative_addr >= 0 && relative_addr < state.memory_size
    then relative_addr
    else
      raise_s
        [%message
          "Memory access out of bounds: 0x%08x (base=0x%08x, size=%d)"
            (addr : int)
            (state.memory_base : int)
            (state.memory_size : int)]
  ;;

  let mem_op f state addr = f state.memory ~pos:(translate_address state addr)

  let load_u8, load_u16, load_u32 =
    Iobuf.Peek.(uint8, uint16_le, uint32_le) |> Tuple3.map ~f:mem_op
  ;;

  let store_u8, store_u16, store_u32 =
    Iobuf.Poke.(uint8_trunc, uint16_le_trunc, uint32_le_trunc) |> Tuple3.map ~f:mem_op
  ;;
end

let rtype state { dest; src1; src2 } f =
  let open State in
  let v1 = read_register state src1 in
  let v2 = read_register state src2 in
  write_register state dest (f v1 v2)
;;

let itype state { dest; src; immediate } f =
  let open State in
  let v = read_register state src in
  write_register state dest (f v immediate)
;;

let branch state { src1; src2; target } cond_f =
  let open State in
  let v1 = read_register state src1 in
  let v2 = read_register state src2 in
  if cond_f v1 v2 then state.pc <- state.pc + target - 4
;;

let branch_unsigned state { src1; src2; target } cond_f =
  let open State in
  let v1 = mask ~bits:32 (read_register state src1) in
  let v2 = mask ~bits:32 (read_register state src2) in
  if cond_f v1 v2 then state.pc <- state.pc + target - 4
;;

let mul v1 v2 = Int64.(of_int v1 * of_int v2)

let mulh v1 v2 =
  let result = mul v1 v2 in
  Int64.to_int_exn (Int64.shift_right result 32)
;;

let div_64 v1 v2 = Int64.(of_int v1 / of_int v2 |> to_int_exn)
let rem_64 v1 v2 = Int64.(of_int v1 |> fun v1 -> rem v1 (of_int v2) |> to_int_exn)

let unsigned_op state op ~zero args =
  rtype state args (fun v1 v2 ->
    let v1 = mask ~bits:32 v1 in
    let v2 = mask ~bits:32 v2 in
    if v2 = 0
    then zero v1
    else (
      let result = op v1 v2 in
      mask ~bits:32 result))
;;

let signed_op state op ~zero ~overflow args =
  rtype state args (fun v1 v2 ->
    if v2 = 0
    then zero v1
    else if v1 = Int.min_value && v2 = -1
    then overflow v1
    else op v1 v2)
;;

let shift_op state shift_fn args =
  rtype state args (fun v1 v2 ->
    let shamt = mask ~bits:5 v2 in
    shift_fn v1 shamt)
;;

let shift_imm state shift_fn { dest; src; shamt } =
  State.write_register state dest (shift_fn (State.read_register state src) shamt)
;;

let read_char_stdin () =
  if Core_unix.isatty Core_unix.stdin
  then (
    let open Caml_unix in
    let old_attr = tcgetattr stdin in
    let new_attr = { old_attr with c_icanon = false; c_echo = true } in
    tcsetattr stdin TCSANOW new_attr;
    let ch = Stdlib.input_char Stdlib.stdin in
    tcsetattr stdin TCSANOW old_attr;
    ch)
  else
    (* No echo when stdin is piped - enables binary protocols *)
    Stdlib.input_char Stdlib.stdin
;;

let compare_set cmp v1 v2 = if cmp v1 v2 then 1 else 0
let compare_set_unsigned cmp v1 v2 = compare_set cmp (mask ~bits:32 v1) (mask ~bits:32 v2)

let calc_address state address =
  State.read_register state address.register + address.offset
;;

let load state load_fn dest address =
  let addr = calc_address state address in
  State.write_register state dest (load_fn addr)
;;

let store state store_fn src address =
  let addr = calc_address state address in
  store_fn state addr (State.read_register state src)
;;

let execute_instruction state instr =
  let open State in
  match instr with
  | Add args -> rtype state args ( + )
  | Sub args -> rtype state args ( - )
  | Addi args -> itype state args ( + )
  | Slt args -> rtype state args (compare_set ( < ))
  | Sltu args -> rtype state args (compare_set_unsigned ( < ))
  | Slti args -> itype state args (compare_set ( < ))
  | Sltiu args -> itype state args (compare_set_unsigned ( < ))
  | And args -> rtype state args ( land )
  | Or args -> rtype state args ( lor )
  | Xor args -> rtype state args ( lxor )
  | Andi args -> itype state args ( land )
  | Ori args -> itype state args ( lor )
  | Xori args -> itype state args ( lxor )
  | Sll args -> shift_op state ( lsl ) args
  | Srl args -> shift_op state (fun v1 shamt -> mask ~bits:32 v1 lsr shamt) args
  | Sra args -> shift_op state ( asr ) args
  | Slli args -> shift_imm state ( lsl ) args
  | Srli args -> shift_imm state (fun v shamt -> mask ~bits:32 v lsr shamt) args
  | Srai args -> shift_imm state ( asr ) args
  | Lui { dest; immediate } -> write_register state dest (immediate lsl 12)
  | Auipc { dest; immediate } -> write_register state dest (state.pc + (immediate lsl 12))
  | Jal { dest; offset } ->
    write_register state dest (state.pc + 4);
    state.pc <- state.pc + offset - 4
  | Jalr { dest; base; offset } ->
    (* Read base register as unsigned for address calculation *)
    let base_val = mask ~bits:32 state.registers.(base) in
    let target = (base_val + offset) land lnot 1 in
    write_register state dest (state.pc + 4);
    state.pc <- target - 4
  | Beq args -> branch state args ( = )
  | Bne args -> branch state args ( <> )
  | Blt args -> branch state args ( < )
  | Bge args -> branch state args ( >= )
  | Bltu args -> branch_unsigned state args ( < )
  | Bgeu args -> branch_unsigned state args ( >= )
  | Lb { dest; address } ->
    load state (fun addr -> load_u8 state addr |> sign_extend ~bits:8) dest address
  | Lh { dest; address } ->
    load state (fun addr -> load_u16 state addr |> sign_extend ~bits:16) dest address
  | Lw { dest; address } -> load state (load_u32 state) dest address
  | Lbu { dest; address } -> load state (load_u8 state) dest address
  | Lhu { dest; address } -> load state (load_u16 state) dest address
  | Sb { src; address } -> store state store_u8 src address
  | Sh { src; address } -> store state store_u16 src address
  | Sw { src; address } -> store state store_u32 src address
  | Mul args ->
    rtype state args (fun v1 v2 ->
      let result = mul v1 v2 in
      let result_32 = Int64.(result land 0xFFFFFFFFL) in
      mask ~bits:32 (Int64.to_int_exn result_32))
  | Mulh args -> rtype state args (fun v1 v2 -> mulh v1 v2)
  | Mulhsu args -> rtype state args (fun v1 v2 -> mulh v1 (mask ~bits:32 v2))
  | Mulhu args ->
    rtype state args (fun v1 v2 -> mulh (mask ~bits:32 v1) (mask ~bits:32 v2))
  | Div args -> signed_op state div_64 ~zero:(fun _ -> -1) ~overflow:(fun v1 -> v1) args
  | Divu args -> unsigned_op state div_64 ~zero:(fun _ -> Int.max_value) args
  | Rem args -> signed_op state rem_64 ~zero:(fun v1 -> v1) ~overflow:(fun _ -> 0) args
  | Remu args -> unsigned_op state rem_64 ~zero:(fun v1 -> mask ~bits:32 v1) args
  | Ecall ->
    let syscall_num = read_register state 17 in
    (match syscall_num with
     | 0 ->
       (try read_char_stdin () |> Char.to_int |> write_register state 10 with
        | End_of_file -> write_register state 10 (-1))
     | 1 -> read_register state 10 |> mask ~bits:8 |> Char.of_int_exn |> printf "%c%!"
     | 2 -> raise Exit
     | 3 ->
       let start_addr = mask ~bits:32 (read_register state 10) in
       let end_addr = mask ~bits:32 (read_register state 11) in
       let addr = ref start_addr in
       while !addr < end_addr do
         store_u32 state !addr 0;
         addr := !addr + 4
       done
     | 4 -> write_register state 10 0
     | _ -> failwith ("Unknown syscall: " ^ Int.to_string syscall_num))
  | _ ->
    Printf.eprintf "Unimplemented instruction at PC=0x%08x\n%!" state.pc;
    failwith "Instruction not implemented"
;;

let execute_program ~memory_base ~image ~fetch_instruction =
  let state = State.create ~memory_size:(64 * 1024 * 1024) ~memory_base ~image in
  let instr_count = ref 0 in
  let rec loop () =
    match fetch_instruction state |> Option.map ~f:(execute_instruction state) with
    | Some () ->
      instr_count := !instr_count + 1;
      state.pc <- state.pc + 4;
      loop ()
    | None | (exception Exit) -> ()
  in
  loop ();
  state, !instr_count
;;

let run ~memory_base instructions =
  let instr_array = Array.of_list instructions in
  let instr_count = Array.length instr_array in
  execute_program ~memory_base ~image:"" ~fetch_instruction:(fun state ->
    let pc_index = (state.pc - state.memory_base) / 4 in
    if pc_index >= 0 && pc_index < instr_count then Some instr_array.(pc_index) else None)
  |> fst
;;

let run_binary ~memory_base binary_data =
  let state, instr_count =
    execute_program ~memory_base ~image:binary_data ~fetch_instruction:(fun state ->
      let instruction_word = State.load_u32 state state.pc in
      Some (Parser.decode_instruction instruction_word))
  in
  eprintf "Instructions executed: %d\n%!" instr_count;
  state
;;

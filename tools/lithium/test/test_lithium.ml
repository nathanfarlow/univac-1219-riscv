open! Core
open! Lithium

let gen_list ~max_len element_gen =
  let open Quickcheck.Generator.Let_syntax in
  let%bind len = Int.gen_incl 0 max_len in
  List.gen_with_length len element_gen
;;

let verify_one_minimal reduced ~test =
  List.for_all reduced ~f:(fun item_to_remove ->
    let without_item =
      List.filter reduced ~f:(fun x -> not (phys_equal x item_to_remove))
    in
    not (test without_item))
;;

let verify_lithium_invariants original reduced ~test =
  (* Reduced must pass the test if original passed *)
  let original_passes = test original in
  if original_passes
  then (
    let reduced_passes = test reduced in
    [%test_result: bool]
      reduced_passes
      ~expect:true
      ~message:"Reduced input must still satisfy test condition";
    (* Verify one-minimal property *)
    [%test_result: bool]
      (verify_one_minimal reduced ~test)
      ~expect:true
      ~message:"Result must be one-minimal");
  (* Reduced should always be a subset of original *)
  List.iter reduced ~f:(fun item ->
    [%test_result: bool]
      (List.mem original item ~equal:phys_equal)
      ~expect:true
      ~message:"Reduced elements must be present in original")
;;

let%test_unit "lithium with sum condition" =
  Quickcheck.test
    ~sexp_of:[%sexp_of: int list]
    ~trials:10
    (gen_list ~max_len:100 (Int.gen_incl (-10) 100))
    ~f:(fun original ->
      let test items =
        let sum = List.sum (module Int) items ~f:Fn.id in
        sum >= 20
      in
      let reduced = reduce original ~is_interesting:test in
      verify_lithium_invariants original reduced ~test)
;;

let%test_unit "lithium with multiple required elements" =
  Quickcheck.test
    ~sexp_of:[%sexp_of: int list]
    ~trials:10
    (gen_list ~max_len:100 (Int.gen_incl 1 20))
    ~f:(fun original ->
      let test items =
        List.mem items 5 ~equal:Int.equal && List.mem items 10 ~equal:Int.equal
      in
      let reduced = reduce original ~is_interesting:test in
      verify_lithium_invariants original reduced ~test)
;;

let%test_unit "lithium with single element condition" =
  Quickcheck.test
    ~sexp_of:[%sexp_of: int list]
    ~trials:10
    (gen_list ~max_len:100 (Int.gen_incl 1 20))
    ~f:(fun original ->
      let target = 7 in
      let test items = List.mem items target ~equal:Int.equal in
      let reduced = reduce original ~is_interesting:test in
      verify_lithium_invariants original reduced ~test)
;;

let%test_unit "lithium with string condition" =
  let string_gen = String.gen_with_length 10 Char.gen_alpha in
  let error_string_gen =
    let open Quickcheck.Generator.Let_syntax in
    let%map prefix = String.gen_with_length 5 Char.gen_alpha in
    prefix ^ "error"
  in
  let mixed_gen = Quickcheck.Generator.union [ string_gen; error_string_gen ] in
  Quickcheck.test
    ~sexp_of:[%sexp_of: string list]
    ~trials:10
    (gen_list ~max_len:50 mixed_gen)
    ~f:(fun original ->
      let test items = List.exists items ~f:(String.is_substring ~substring:"error") in
      let reduced = reduce original ~is_interesting:test in
      verify_lithium_invariants original reduced ~test)
;;

let%test_unit "lithium with length condition" =
  Quickcheck.test
    ~sexp_of:[%sexp_of: int list]
    ~trials:10
    (gen_list ~max_len:50 (Int.gen_incl (-10) 10))
    ~f:(fun original ->
      let test items = List.length items >= 3 in
      let reduced = reduce original ~is_interesting:test in
      verify_lithium_invariants original reduced ~test)
;;

let%test_unit "lithium with modulo condition" =
  Quickcheck.test
    ~sexp_of:[%sexp_of: int list]
    ~trials:10
    (gen_list ~max_len:50 (Int.gen_incl 1 50))
    ~f:(fun original ->
      let test items = List.exists items ~f:(fun x -> x % 7 = 0) in
      let reduced = reduce original ~is_interesting:test in
      verify_lithium_invariants original reduced ~test)
;;

let%test_unit "lithium with complex condition" =
  Quickcheck.test
    ~sexp_of:[%sexp_of: int list]
    ~trials:10
    (gen_list ~max_len:50 (Int.gen_incl 1 25))
    ~f:(fun original ->
      let test items =
        let sum = List.sum (module Int) items ~f:Fn.id in
        let has_even = List.exists items ~f:(fun x -> x % 2 = 0) in
        sum >= 30 && has_even && List.length items >= 2
      in
      let reduced = reduce original ~is_interesting:test in
      verify_lithium_invariants original reduced ~test)
;;

let%test_unit "lithium is deterministic" =
  Quickcheck.test
    ~sexp_of:[%sexp_of: int list]
    ~trials:10
    (gen_list ~max_len:50 (Int.gen_incl (-10) 10))
    ~f:(fun original ->
      let test items =
        let sum = List.sum (module Int) items ~f:Fn.id in
        sum >= 15
      in
      (* Run lithium twice and check results are identical *)
      let result1 = reduce original ~is_interesting:test in
      let result2 = reduce original ~is_interesting:test in
      [%test_result: int list]
        result2
        ~expect:result1
        ~message:"Lithium should be deterministic")
;;

let%test_unit "lithium has logarithmic complexity (needle-in-haystack)" =
  Quickcheck.test
    ~sexp_of:[%sexp_of: int list]
    ~trials:10
    (gen_list ~max_len:100 (Int.gen_incl 1 100))
    ~f:(fun original ->
      let n = List.length original in
      if n >= 16
      then (
        let needle = -999999 in
        let haystack = needle :: original in
        let calls = ref 0 in
        let test items =
          incr calls;
          List.mem items needle ~equal:Int.equal
        in
        let result = reduce haystack ~is_interesting:test in
        let max_calls =
          (2 * (n + 1) * Int.of_float (Float.log (Float.of_int (n + 1)) /. Float.log 2.0))
          + 1
        in
        [%test_result: bool]
          (!calls <= max_calls)
          ~expect:true
          ~message:(sprintf "%d calls > %d max for size %d" !calls max_calls (n + 1));
        [%test_result: int list] result ~expect:[ needle ]))
;;

let%test_unit "lithium handles edge cases" =
  Quickcheck.test
    ~sexp_of:[%sexp_of: int list]
    ~trials:10
    (gen_list ~max_len:5 (Int.gen_incl (-5) 5)) (* Small lists for edge cases *)
    ~f:(fun original ->
      (* Test that always passes - should reduce to empty list *)
      let trivial_test _items = true in
      let reduced = reduce original ~is_interesting:trivial_test in
      [%test_result: int list]
        reduced
        ~expect:[]
        ~message:"Trivial test should reduce to empty list")
;;

let%expect_test "verify optimized algorithm performance" =
  (* Test that our position-based algorithm works correctly *)
  let test items = List.sum (module Int) items ~f:Fn.id >= 20 in
  let original = [ 1; 2; 3; 4; 5; 6; 7; 8; 9; 10; 11; 12; 13; 14; 15 ] in
  printf
    "Testing optimized algorithm with: %s\n"
    (Sexp.to_string_hum ([%sexp_of: int list] original));
  printf
    "Original sum: %d, passes test: %b\n"
    (List.sum (module Int) original ~f:Fn.id)
    (test original);
  let calls = ref 0 in
  let counting_test items =
    incr calls;
    test items
  in
  let reduced = reduce original ~is_interesting:counting_test in
  printf "Reduced: %s\n" (Sexp.to_string_hum ([%sexp_of: int list] reduced));
  printf
    "Reduced sum: %d, passes test: %b\n"
    (List.sum (module Int) reduced ~f:Fn.id)
    (test reduced);
  printf "Total test calls: %d\n" !calls;
  let is_one_minimal =
    List.for_all reduced ~f:(fun item ->
      let without_item = List.filter reduced ~f:(fun x -> not (Int.equal x item)) in
      not (test without_item))
  in
  printf "Is one-minimal: %b\n" is_one_minimal;
  [%expect
    {|
    Testing optimized algorithm with: (1 2 3 4 5 6 7 8 9 10 11 12 13 14 15)
    Original sum: 120, passes test: true
    Reduced: (12 13)
    Reduced sum: 25, passes test: true
    Total test calls: 14
    Is one-minimal: true
    |}]
;;

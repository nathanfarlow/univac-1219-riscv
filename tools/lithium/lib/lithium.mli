open! Core

(** [reduce items ~is_interesting] applies the Lithium hierarchical reduction
    algorithm to reduce the input list to a 1-minimal form while preserving the
    interesting property.

    The algorithm uses a divide-and-conquer approach with exponentially
    decreasing chunk sizes, making it efficient for large inputs. Time
    complexity is typically O(log n) in the best case (when large chunks can be
    removed) and O(n²) in the worst case.

    @param items The initial list of elements to reduce (e.g., lines of code,
      test steps)
    @param is_interesting A predicate function that returns [true] if the input
      still reproduces the desired behavior (e.g., triggers a bug)
    @return A 1-minimal list where removing any single element would make
      [is_interesting] false

    Example: Reducing a crashing test case from 896 lines to 3 essential lines. *)
val reduce : 'a list -> is_interesting:('a list -> bool) -> 'a list

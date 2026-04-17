;;; univac-1219-mode.el --- Major mode for UNIVAC-1219 assembly language -*- lexical-binding: t; -*-

;; Copyright (C) 2025

;; Author:
;; Keywords: languages, assembly, univac
;; Version: 1.0.0

;;; Commentary:

;; A major mode for editing UNIVAC-1219 assembly language files.
;; Provides syntax highlighting for instructions, directives, labels,
;; comments, and operands based on the assembler implementation.

;;; Code:

(defvar univac-1219-mode-syntax-table
  (let ((table (make-syntax-table)))
    ;; Comments start with semicolon
    (modify-syntax-entry ?\; "<" table)
    (modify-syntax-entry ?\n ">" table)
    ;; Numbers and identifiers
    (modify-syntax-entry ?_ "w" table)
    (modify-syntax-entry ?& "w" table)  ; For octal prefixes
    (modify-syntax-entry ?+ "." table)  ; For label offsets
    (modify-syntax-entry ?- "." table)  ; For label offsets and negative numbers
    table)
  "Syntax table for `univac-1219-mode'.")

;; Instruction keywords - extracted from instruction.rs
(defconst univac-1219-instructions
  '("CMAL" "CMALB" "SLSU" "SLSUB" "CMSK" "CMSKB" "ENTAU" "ENTAUB"
    "ENTAL" "ENTALB" "ADDAL" "ADDALB" "SUBAL" "SUBALB" "ADDA" "ADDAB"
    "SUBA" "SUBAB" "MULAL" "MULALB" "DIVA" "DIVAB" "IRJP" "IRJPB"
    "ENTB" "ENTBB" "JP" "JPB" "ENTBK" "ENTBKB" "CL" "CLB" "STRB"
    "STRBB" "STRAL" "STRALB" "STRAU" "STRAUB" "SIN" "SOUT" "SEXF"
    "IN" "OUT" "EXF" "RTC" "INSTP" "OUTSTP" "EXFSTP" "SRSM" "SKPIIN"
    "SKPOIN" "SKPEIN" "WTFI" "OUTOV" "EXFOV" "RIL" "RXL" "SIL" "SXL"
    "RSHAU" "RSHAL" "RSHA" "SF" "LSHAU" "LSHAL" "LSHA" "SKP" "SKPNBO"
    "SKPOV" "SKPNOV" "SKPODD" "SKPEVN" "STOP" "SKPNR" "RND" "CPAL"
    "CPAU" "CPA" "ENTICR" "ENTSR" "SLSET" "SLCL" "SLCP" "IJPEI" "IJP"
    "BSK" "ISK" "JPAUZ" "JPALZ" "JPAUNZ" "JPALNZ" "JPAUP" "JPALP"
    "JPAUNG" "JPALNG" "ENTALK" "ADDALK" "STRICR" "BJP" "STRADR" "STRSR"
    "RJP"))

;; Assembler directives - from assembler.rs parse function
(defconst univac-1219-directives
  '("ORG" "SADD" "DATA" "AS" "AW" "EVEN" "END"))

;; Special operands
(defconst univac-1219-operands
  '("LOK"))

;; Instruction descriptions using modern C-style notation

;; y is 12 bit memory location (semantics depend on memory mode). k is 6 bit
;; immediate value. u is 12 bit immediate value.
;; Timing is in microseconds (µs) based on UNIVAC-1219 hardware specifications.
(defconst univac-1219-instruction-help
  '(("CMAL" . "<y> - [4µs] Compare AL with *y, set flags")
    ("CMALB" . "<y> - [4µs] Compare AL with *(y+B), set flags")
    ("SLSU" . "<y> - [4µs] Selective substitute: AL = (~AU & AL) | (AU & *y)")
    ("SLSUB" . "<y> - [4µs] Selective substitute with B addressing")
    ("CMSK" . "<y> - [4µs] Compare (AL & AU) with (*y & AU), set flags")
    ("CMSKB" . "<y> - [4µs] Compare (AL & AU) with (*(y+B) & AU), set flags")
    ("ENTAU" . "<y> - [4µs] Load AU from memory: AU = *y")
    ("ENTAUB" . "<y> - [4µs] Load AU from memory: AU = *(y+B)")
    ("ENTAL" . "<y> - [4µs] Load AL from memory: AL = *y")
    ("ENTALB" . "<y> - [4µs] Load AL from memory: AL = *(y+B)")
    ("ADDAL" . "<y> - [4µs] Add to AL: AL += *y")
    ("ADDALB" . "<y> - [4µs] Add to AL: AL += *(y+B)")
    ("SUBAL" . "<y> - [4µs] Subtract from AL: AL -= *y")
    ("SUBALB" . "<y> - [4µs] Subtract from AL: AL -= *(y+B)")
    ("ADDA" . "<y> - [6µs] Add double word: A += (mem[y+1] << 18) | mem[y]")
    ("ADDAB" . "<y> - [6µs] Add double word: A += (mem[y+1+B] << 18) | mem[y+B]")
    ("SUBA" . "<y> - [6µs] Subtract double word: A -= (mem[y+1] << 18) | mem[y]")
    ("SUBAB" . "<y> - [6µs] Subtract double word: A -= (mem[y+1+B] << 18) | mem[y+B]")
    ("MULAL" . "<y> - [14µs] A = AL * *y")
    ("MULALB" . "<y> - [14µs] A = AL * *(y+B)")
    ("DIVA" . "<y> - [14µs] Divide: AL = A / *y; AU = A % *y")
    ("DIVAB" . "<y> - [14µs] Divide: AL = A / *(y+B); AU = A % *(y+B)")
    ("IRJP" . "<y> - [6µs] Indirect return jump: mem[*y] = PC+1; PC = *y+1")
    ("IRJPB" . "<y> - [6µs] Indirect return jump: mem[*(y+B)] = PC+1; PC = *(y+B)+1")
    ("ENTB" . "<y> - [4µs] Load B register: B = *y")
    ("ENTBB" . "<y> - [4µs] Load B register: B = *(y+B)")
    ("JP" . "<y> - [2µs] Unconditional jump: PC = y")
    ("JPB" . "<y> - [2µs] Unconditional jump: PC = y+B")
    ("ENTBK" . "<u> - [2µs] Load B with constant: B = u (sign extended)")
    ("ENTBKB" . "<u> - [2µs] Add constant to B: B += u (sign extended)")
    ("CL" . "<y> - [4µs] Clear memory: *y = 0")
    ("CLB" . "<y> - [4µs] Clear memory: *(y+B) = 0")
    ("STRB" . "<y> - [4µs] Store B: *y = B")
    ("STRBB" . "<y> - [4µs] Store B: *(y+B) = B")
    ("STRAL" . "<y> - [4µs] Store AL: *y = AL")
    ("STRALB" . "<y> - [4µs] Store AL: *(y+B) = AL")
    ("STRAU" . "<y> - [4µs] Store AU: *y = AU")
    ("STRAUB" . "<y> - [4µs] Store AU: *(y+B) = AU")
    ("SIN" . "[2µs] Set input active flag for channel k")
    ("SOUT" . "[2µs] Set output active flag for channel k")
    ("SEXF" . "[2µs] Set external function active flag for channel k")
    ("IN" . "<k> - [6µs] Input transfer: setup DMA channel k")
    ("OUT" . "<k> - [6µs] Output transfer: setup DMA channel k")
    ("EXF" . "<k> - [6µs] External function: setup channel k")
    ("RTC" . "[2µs] Enable real time clock monitor")
    ("INSTP" . "<k> - [2µs] Stop input: clear input flag for channel k")
    ("OUTSTP" . "<k> - [2µs] Stop output: clear output flag for channel k")
    ("EXFSTP" . "<k> - [2µs] Stop external function: clear flag for channel k")
    ("SRSM" . "<k> - [2µs] Set resume flag for channel group k")
    ("SKPIIN" . "<k> - [2µs] Skip if input inactive on channel k")
    ("SKPOIN" . "<k> - [2µs] Skip if output inactive on channel k")
    ("SKPEIN" . "<k> - [2µs] Skip if external function inactive on channel k")
    ("WTFI" . "[2µs] Wait for interrupt: halt until interrupt")
    ("OUTOV" . "<k> - [2µs] Force output word on channel k")
    ("EXFOV" . "<k> - [2µs] Force function word on channel k")
    ("RIL" . "[2µs] Remove interrupt lockout (enable all interrupts)")
    ("RXL" . "[2µs] Remove external interrupt lockout (enable external interrupts)")
    ("SIL" . "[2µs] Disable all interrupts")
    ("SXL" . "[2µs] Disable external interrupts")
    ("RSHAU" . "<k> - [4+2*(k/4)µs] Right arithmetic shift AU by k bits")
    ("RSHAL" . "<k> - [4+2*(k/4)µs] Right arithmetic shift AL by k bits")
    ("RSHA" . "<k> - [4+2*(k/4)µs] Right arithmetic shift A by k bits")
    ("SF" . "<k> - [4+2*(k/4)µs] Scale factor: normalize A register")
    ("LSHAU" . "<k> - [4+2*(k/4)µs] Left circular shift AU by k bits")
    ("LSHAL" . "<k> - [4+2*(k/4)µs] Left circular shift AL by k bits")
    ("LSHA" . "<k> - [4+2*(k/4)µs] Left circular shift A by k bits")
    ("SKP" . "<k> - [2µs] Skip if console key k is set")
    ("SKPNBO" . "[2µs] Skip if no borrow flag")
    ("SKPOV" . "[2µs] Skip if overflow flag set")
    ("SKPNOV" . "[2µs] Skip if no overflow flag")
    ("SKPODD" . "[2µs] Skip if (AL & AU) has odd bit count")
    ("SKPEVN" . "[2µs] Skip if (AL & AU) has even bit count")
    ("STOP" . "<k> - [2µs] Halt if console stop key k is set (k=40 unconditional)")
    ("SKPNR" . "<k> - [2µs] Skip if no resume flag for channel group k")
    ("RND" . "[2µs] Round: AL = AU + AL[17] if AU>=0, else AU - ~AL[17]")
    ("CPAL" . "[2µs] Complement AL: AL = ~AL")
    ("CPAU" . "[2µs] Complement AU: AU = ~AU")
    ("CPA" . "[2µs] Complement A: A = ~A")
    ("ENTICR" . "<k> - [2µs] Set index control register: ICR = k")
    ("ENTSR" . "<k> - [2µs] Set special register: SR = k")
    ("SLSET" . "<y> - [4µs] Bitwise OR: AL |= *y")
    ("SLCL" . "<y> - [4µs] Bitwise AND: AL &= *y")
    ("SLCP" . "<y> - [4µs] Bitwise XOR: AL ^= *y")
    ("IJPEI" . "<y> - [4µs] Indirect jump & enable interrupts: PC = *y")
    ("IJP" . "<y> - [4µs] Indirect jump: PC = *y")
    ("BSK" . "<y> - [4µs] B loop: if (B == *y) skip; else B++")
    ("ISK" . "<y> - [6µs] Index loop: if (*y == 0) skip; else (*y)--")
    ("JPAUZ" . "<y> - [2µs] Jump if AU == 0: if (!AU) PC = y")
    ("JPALZ" . "<y> - [2µs] Jump if AL == 0: if (!AL) PC = y")
    ("JPAUNZ" . "<y> - [2µs] Jump if AU != 0: if (AU) PC = y")
    ("JPALNZ" . "<y> - [2µs] Jump if AL != 0: if (AL) PC = y")
    ("JPAUP" . "<y> - [2µs] Jump if AU >= 0: if (AU >= 0) PC = y")
    ("JPALP" . "<y> - [2µs] Jump if AL >= 0: if (AL >= 0) PC = y")
    ("JPAUNG" . "<y> - [2µs] Jump if AU < 0: if (AU < 0) PC = y")
    ("JPALNG" . "<y> - [2µs] Jump if AL < 0: if (AL < 0) PC = y")
    ("ENTALK" . "<u> - [2µs] Load AL with constant: AL = u (sign extended)")
    ("ADDALK" . "<u> - [2µs] Add constant to AL: AL += u (sign extended)")
    ("STRICR" . "<y> - [4µs] Store ICR bits: *y = (*y & 0xFFF0) | ICR")
    ("BJP" . "<y> - [2µs] B loop jump: if (B--) PC = y")
    ("STRADR" . "<y> - [4µs] Store address bits: *y = (*y & 0xF000) | (AL & 0xFFF)")
    ("STRSR" . "<y> - [4µs] Store SR bits: *y = (*y & 0xFFC0) | SR; SR = 0")
    ("RJP" . "<y> - [4µs] Call subroutine: mem[y] = PC+1; PC = y+1")))

;; Custom face for triple-backtick documentation blocks
(defface univac-1219-doc-block-face
  '((((class color) (background dark))
     (:foreground "#b8c5e0" :background "#1a2840"))
    (((class color) (background light))
     (:foreground "#4a5a7f" :background "#e8ecf5"))
    (t
     (:inherit font-lock-comment-face)))
  "Face for triple-backtick documentation blocks in UNIVAC-1219 assembly.
Distinct blue color to stand out from regular comments."
  :group 'font-lock-highlighting-faces)

;; Font lock keywords
(defvar univac-1219-font-lock-keywords
  `(
    ;; Triple-backtick documentation blocks (must be first for proper override)
    ;; Uses [^\000]*? to match any character including newlines, non-greedily
    ;; OVERRIDE flag (t) ensures this replaces all other highlighting inside block
    ;; font-lock-multiline property prevents chunking issues with large blocks
    ("\\(```\\)\\([^\000]*?\\)\\(```\\)"
     (0 (list 'face 'univac-1219-doc-block-face
              'font-lock-multiline t)
        t))

    ;; Comments (semicolon to end of line)
    (";.*$" . font-lock-comment-face)

    ;; Labels (lines starting with >LABEL)
    ("^>\\([A-Z_][A-Z0-9_]*\\)" 1 font-lock-function-name-face)

    ;; Assembler directives
    (,(concat "\\b\\(" (mapconcat 'identity univac-1219-directives "\\|") "\\)\\b")
     . font-lock-preprocessor-face)

    ;; Instructions
    (,(concat "\\b\\(" (mapconcat 'identity univac-1219-instructions "\\|") "\\)\\b")
     . font-lock-keyword-face)

    ;; Special operands
    (,(concat "\\b\\(" (mapconcat 'identity univac-1219-operands "\\|") "\\)\\b")
     . font-lock-builtin-face)

    ;; Octal numbers (&O... or &...)
    ("\\b&O?[0-7]+\\b" . font-lock-constant-face)

    ;; Decimal numbers (including negative)
    ("\\b-?[0-9]+\\b" . font-lock-constant-face)

    ;; Symbol references (identifiers not already matched)
    ("\\b[A-Z_][A-Z0-9_]*\\b" . font-lock-variable-name-face)

    ;; Label offsets (+N or -N after symbols)
    ("[A-Z_][A-Z0-9_]*\\([+-][0-9]+\\)" 1 font-lock-constant-face)

    ;; ASCII strings (AS and AW content)
    ("\\b\\(AS\\|AW\\)[ \t]+\\(.*\\)$" 2 font-lock-string-face))
  "Font lock keywords for UNIVAC-1219 assembly mode.")

;; Helper function to extract argument from description
(defun univac-1219-extract-args (description)
  "Extract argument specification from instruction description."
  (if (string-match "^\\(<[^>]+>\\)" description)
      (match-string 1 description)
    ""))

;; Helper function to get short description
(defun univac-1219-get-short-desc (description)
  "Get short description after the dash."
  (if (string-match " - \\(.+\\)" description)
      (match-string 1 description)
    description))

;; Smart completion candidates with fuzzy matching and ranking
(defun univac-1219-smart-candidates (prefix)
  "Return smart-ranked candidates for PREFIX."
  (let ((prefix-upper (upcase prefix))
        (candidates '()))

    ;; Collect all candidates with metadata
    (dolist (instruction univac-1219-instructions)
      (when (or (string-prefix-p prefix-upper instruction)
                (string-match-p (regexp-quote prefix-upper) instruction))
        (let* ((help (assoc instruction univac-1219-instruction-help))
               (desc (if help (cdr help) ""))
               (args (univac-1219-extract-args desc))
               (short-desc (univac-1219-get-short-desc desc)))
          (push (list instruction 'instruction args short-desc
                      (if (string-prefix-p prefix-upper instruction) 1 2)) ; priority
                candidates))))

    (dolist (directive univac-1219-directives)
      (when (string-prefix-p prefix-upper directive)
        (push (list directive 'directive "" "Assembler directive" 1)
              candidates)))

    (dolist (operand univac-1219-operands)
      (when (string-prefix-p prefix-upper operand)
        (push (list operand 'operand "" "Special operand" 1)
              candidates)))

    ;; Sort by priority, then alphabetically
    (sort candidates (lambda (a b)
                       (let ((prio-a (nth 4 a))
                             (prio-b (nth 4 b)))
                         (if (= prio-a prio-b)
                             (string< (nth 0 a) (nth 0 b))
                           (< prio-a prio-b)))))

    ;; Return just the names
    (mapcar #'car candidates)))

;; Company backend for auto-completion
(defvar univac-1219-completion-cache nil
  "Cache for completion metadata.")

(defun univac-1219-company-backend (command &optional arg &rest ignored)
  "Company backend for UNIVAC-1219 assembly completion."
  (interactive (list 'interactive))
  (cl-case command
    (interactive (company-begin-backend 'univac-1219-company-backend))
    (prefix (and (eq major-mode 'univac-1219-mode)
                 (company-grab-symbol)))
    (candidates (univac-1219-smart-candidates arg))
    (annotation
     (let* ((item (upcase arg))
            (help (assoc item univac-1219-instruction-help)))
       (cond
        ((member item univac-1219-instructions)
         (if help
             (let ((args (univac-1219-extract-args (cdr help))))
               (format " %s [instruction]" args))
           " [instruction]"))
        ((member item univac-1219-directives) " [directive]")
        ((member item univac-1219-operands) " [operand]")
        (t nil))))
    (meta
     (let* ((item (upcase arg))
            (help (assoc item univac-1219-instruction-help)))
       (cond
        ((member item univac-1219-instructions)
         (if help
             (univac-1219-get-short-desc (cdr help))
           "UNIVAC-1219 instruction"))
        ((member item univac-1219-directives) "Assembler directive")
        ((member item univac-1219-operands) "Special operand")
        (t nil))))
    (doc-buffer
     (let ((help (assoc (upcase arg) univac-1219-instruction-help)))
       (when help
         (company-doc-buffer
          (format "%s %s\n\n%s"
                  (upcase arg)
                  (univac-1219-extract-args (cdr help))
                  (univac-1219-get-short-desc (cdr help)))))))
    (post-completion
     ;; Auto-add space after instruction for easier typing
     (when (member (upcase arg) univac-1219-instructions)
       (insert " ")))))

;; ElDoc function for instruction help
(defun univac-1219-eldoc-function ()
  "Return help string for instruction at point."
  (save-excursion
    (let ((bounds (bounds-of-thing-at-point 'word)))
      (when bounds
        (let* ((word (upcase (buffer-substring-no-properties
                              (car bounds) (cdr bounds))))
               (help (assoc word univac-1219-instruction-help)))
          (when help
            (format "%s: %s" word (cdr help))))))))

(defun univac-1219-show-help ()
  "Display help for the instruction at point in the minibuffer."
  (interactive)
  (let* ((word (thing-at-point 'word t))
         (help (when word
                 (assoc (upcase word) univac-1219-instruction-help))))
    (if help
        (message "%s: %s" (upcase word) (cdr help))
      (message "No help available for '%s'" (or word "symbol")))))

;; Indentation function
(defun univac-1219-indent-line ()
  "Indent current line for UNIVAC-1219 assembly."
  (interactive)
  (beginning-of-line)
  (let ((indent-column 0))
    (cond
     ;; Labels (lines starting with >) should be at column 0
     ((looking-at "^>") (setq indent-column 0))
     ;; Directives should be at column 0
     ((looking-at (concat "^\\(" (mapconcat 'identity univac-1219-directives "\\|") "\\)\\b"))
      (setq indent-column 0))
     ;; Instructions should be indented
     ((looking-at (concat "^\\(" (mapconcat 'identity univac-1219-instructions "\\|") "\\)\\b"))
      (setq indent-column 0))
     ;; Everything else gets standard indent
     (t (setq indent-column 0)))

    ;; Apply the indentation
    (if (= (current-indentation) indent-column)
        nil
      (let ((current-point (point)))
        (indent-line-to indent-column)
        (if (< current-point (point))
            (goto-char current-point))))))

;; Go to definition support
(defun univac-1219-goto-definition ()
  "Jump to the definition of the label at point.
Labels are defined with >LABELNAME and referenced as LABELNAME."
  (interactive)
  (let* ((symbol (thing-at-point 'symbol t))
         (label-pattern (when symbol (concat "^>" (upcase symbol) "\\b"))))
    (if (not symbol)
        (message "No symbol at point")
      (let ((start-pos (point)))
        (xref-push-marker-stack)
        (goto-char (point-min))
        (if (re-search-forward label-pattern nil t)
            (progn
              (beginning-of-line)
              (message "Found definition of %s" (upcase symbol)))
          (goto-char start-pos)
          (message "No definition found for %s" (upcase symbol)))))))

(defvar univac-1219-mode-map
  (let ((map (make-sparse-keymap)))
    (define-key map (kbd "M-.") 'univac-1219-goto-definition)
    map)
  "Keymap for UNIVAC-1219 mode.")

;;;###autoload
(define-derived-mode univac-1219-mode prog-mode "UNIVAC-1219"
  "Major mode for editing UNIVAC-1219 assembly language files."
  :syntax-table univac-1219-mode-syntax-table

  ;; Font locking
  (setq font-lock-defaults '(univac-1219-font-lock-keywords nil t))

  ;; Case-insensitive keywords
  (setq font-lock-keywords-case-fold-search t)

  ;; Indentation
  (setq-local indent-line-function 'univac-1219-indent-line)

  ;; Comments
  (setq-local comment-start ";")
  (setq-local comment-start-skip ";+\\s-*")

  ;; Tab settings
  (setq-local tab-width 8)
  (setq-local indent-tabs-mode t)

  ;; ElDoc support for instruction help
  (setq-local eldoc-documentation-function 'univac-1219-eldoc-function)
  (eldoc-mode 1)

  ;; Company mode for auto-completion
  (when (fboundp 'company-mode)
    (setq-local company-backends '(univac-1219-company-backend))
    (company-mode 1)))

;;;###autoload
(add-to-list 'auto-mode-alist '("\\.univac\\'" . univac-1219-mode))

;; Spacemacs integration
(when (fboundp 'spacemacs/set-leader-keys-for-major-mode)
  (spacemacs/set-leader-keys-for-major-mode 'univac-1219-mode
    "gg" 'univac-1219-goto-definition
    "gb" 'xref-pop-marker-stack
    "hh" 'univac-1219-show-help))

(provide 'univac-1219-mode)

;;; univac-1219-mode.el ends here

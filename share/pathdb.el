;; Copyright (C) 1992, 93, 94, 95, 96, 1998, 2000 Free Software Foundation, Inc.
;; Copyright (C) 2004, 2005 PathScale, Inc.

;; Parts of this file are derived from the gdb-gud interface which is
;; distributed in the file gud.el as part of GNU Emacs. The entire file
;; is released under the GNU General Public License.

;; These routines are free software; you can redistribute it and/or modify
;; it under the terms of the GNU General Public License as published by
;; the Free Software Foundation; either version 2, or (at your option)
;; any later version.

;; These routines are distributed in the hope that it will be useful,
;; but WITHOUT ANY WARRANTY; without even the implied warranty of
;; MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
;; GNU General Public License for more details.


;; pathdb pure terminal mode
;; -----------------------------------------------------------
(defun pathdb-term ()
  (interactive)

  ;; query the name of the program to debug
  (setq cmd (read-from-minibuffer "Debug program: "))
  (setq bname (concat "pathdb-" cmd))
  (setq bufvar (concat "*" (concat bname "*")))

  ;; invoke pathdb with program name
  (set-buffer (make-term bname "pathdb"
     nil "-fullname" cmd))

  ;; set term mode and switch buffers
  (term-mode) (term-char-mode)
  (switch-to-buffer bufvar)

  ;; divert all \C-x sequences to emacs
  (define-key term-raw-map "\C-x"
     (lookup-key (current-global-map) "\C-x"))

  ;; divert arrow keys off escaped characters
  (if (not window-system)
    (progn ; is emacs-nox
    (defvar pathdb-arrows-map (make-sparse-keymap))
    (define-key pathdb-arrows-map "A" 'previous-line)
    (define-key pathdb-arrows-map "B" 'next-line)
    (define-key pathdb-arrows-map "C" "\C-f")
    (define-key pathdb-arrows-map "D" "\C-b")
    (define-key term-raw-map "\eO" pathdb-arrows-map))
    (progn ; is emacs-x11
    (define-key term-raw-map [backspace] [delete])
    (define-key term-raw-map [up] 'previous-line)
    (define-key term-raw-map [down] 'next-line)
    (define-key term-raw-map [right] "\C-f")
    (define-key term-raw-map [left] "\C-b")
    (define-key term-raw-map [prior] 'backward-page)
    (define-key term-raw-map [next] 'forward-page)))
)

;; pathdb GUD interface
;; -----------------------------------------------------------
(require 'gud)

(defun pathdb (command-line)
  "Run pathdb on program FILE in buffer *gud-FILE*.
The directory containing FILE becomes the initial working directory
and source-file directory for your debugger."
  (interactive (list (gud-query-cmdline 'pathdb)))

  (gud-common-init command-line 'gud-gdb-massage-args
                   'gud-gdb-marker-filter 'gud-gdb-find-file)
  (set (make-local-variable 'gud-minor-mode) 'gdb)

  (gud-def gud-break  "break %f:%l"  "\C-b" "Set breakpoint at current line.")
  (gud-def gud-tbreak "tbreak %f:%l" "\C-t" "Set temporary breakpoint at current line.")
  (gud-def gud-remove "clear %f:%l"  "\C-d" "Remove breakpoint at current line")
  (gud-def gud-step   "step %p"      "\C-s" "Step one source line with display.")
  (gud-def gud-stepi  "stepi %p"     "\C-i" "Step one instruction with display.")
  (gud-def gud-next   "next %p"      "\C-n" "Step one line (skip functions).")
  (gud-def gud-cont   "cont"         "\C-r" "Continue with display.")
  (gud-def gud-finish "finish"       "\C-f" "Finish executing current function.")
  (gud-def gud-up     "up %p"        "<" "Up N stack frames (numeric arg).")
  (gud-def gud-down   "down %p"      ">" "Down N stack frames (numeric arg).")
  (gud-def gud-print  "print %e"     "\C-p" "Evaluate C expression at point.")

  (local-set-key "\C-i" 'gud-gdb-complete-command)
  (local-set-key [menu-bar debug tbreak] '("Temporary Breakpoint" . gud-tbreak))
  (local-set-key [menu-bar debug finish] '("Finish Function" . gud-finish))
  (local-set-key [menu-bar debug up] '("Up Stack" . gud-up))
  (local-set-key [menu-bar debug down] '("Down Stack" . gud-down))
  (setq comint-prompt-regexp "^pathdb>[ ]*")
  (setq paragraph-start comint-prompt-regexp)
  (run-hooks 'gdb-mode-hook)
  )


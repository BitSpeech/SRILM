;;;
;;; File:   decipher.el
;;; Author: ***PSI***
;;; Date:   Thu Dec 17 20:00:47 1992
;;;
;;; Description:
;;;    This is a set of EMACS commands for use with DECIPHER.
;;;
;;;    Commands defined:
;;;
;;;      Setup ".c" File Header:     M-C-m (ESC RET)
;;;         Inserts a ".c" file header at the point.
;;;
;;;      Setup ".h" File Header:     C-u M-C-m
;;;         Inserts a ".h" file header at the point.
;;;
;;;      Setup Makefile Header:      C-x c
;;;         Inserts a Makefile header at the point.
;;;
;;;      Setup CSH Scrip Header:     C-u C-x c
;;;         Inserts a CSH script header at the point.
;;;
;;;      Insert User Autograph:      C-x a
;;;         Inserts a string of the following format at the point:
;;;               decipher  17-Dec-1992
;;;
;;;      Insert DECIPHER Copyright:  M-C-d
;;;         Inserts the standard DECIPHER copyright notice at the point.
;;;
;;; Copyright (c) 1992, SRI International.  All Rights Reserved.
;;;
;;; RCS ID: $Id: decipher.el,v 1.2 1994/03/14 21:48:44 psi Exp $
;;;
;;; $Log: decipher.el,v $
;;; Revision 1.2  1994/03/14  21:48:44  psi
;;; Updated to correctly insert the RCS keywords without premature expansion.
;;;
; Revision 1.1  1993/01/25  02:37:04  decipher
; Initial revision
;
;;;


;;; Use "gnumake" instead of "/bin/make"....
(setq compile-command "gnumake -k")

;;; Define the file copyright notice string.
(defvar *Copyright-String* "Copyright (c) %s, SRI International.  All Rights Reserved."
  "String to be inserted into file copyright notices.")

;;; Define my setup C file header command....
(defun setup-c-file-header (arg)
  "Setup a C file with my favorite header information."
  (interactive "p")
  (let* ((date (current-time-string))
	 (year (substring date (- (length date) 4) (length date))))
    (goto-char (point-min))
    (insert "/*\n")
    (insert "    File:   \n")
    (insert (format "    Author: %s\n" (getenv "USER")))
    (insert (format "    Date:   %s\n" date))
    (insert "   \n")
    (insert "    Description:\n\n")
    (if (> arg 1)
	(progn
	  ;; Break up the RCS keyword over two lines to avoid premature expansion.
	  (insert (format (format "    %s\n\n    RCS ID: $Id" *Copyright-String*) year))
	  (insert "$\n")))
    (insert "*/\n\n")
    (if (= arg 1)
	(progn
	  (insert (format (format "#ifndef lint\nstatic char Copyright[] = \"%s\";\n" *Copyright-String*) year))
	  ;; Break up the RCS keyword over two lines to avoid premature expansion.
	  (insert "static char RcsId[] = \"$Id")
	  (insert "$\";\n#endif\n\n")))
    ;; Break up the RCS keyword over two lines to avoid premature expansion.
    (insert "/*\n *  $Log")
    (insert "$\n */\n\n")
    ;; Set the C language mode....
    (c-mode)))

;;; Define M-C-m (ESC RET) to set up a C file....
(define-key esc-map "\C-m" 'setup-c-file-header)

;;; Define my setup Makefile/shell script header command....
(defun setup-makefile-or-shell-script-header (arg)
  "Setup a Makefile or shell script (if arg > 1) with my favorite header information."
  (interactive "p")
  (let* ((date (current-time-string))
	 (year (substring date (- (length date) 4) (length date))))
    (goto-char (point-min))
    (insert (format "#%s\n" (if (> arg 1) "!/bin/csh -f\n#" "")))
    (insert "#    File:   \n")
    (insert (format "#    Author: %s\n" (getenv "USER")))
    (insert (format "#    Date:   %s\n" date))
    (insert "#\n")
    (insert "#    Description:\n")
    (insert "#\n")
    (insert (format (format "#    %s\n" *Copyright-String*) year))
    (insert "#\n");
    ;; Break up the RCS keyword over two lines to avoid premature expansion.
    (insert "#    RCS ID: $Id")
    (insert "$\n")
    (insert "#\n")
    ;; Break up the RCS keyword over two lines to avoid premature expansion.
    (insert "#    $Log")
    (insert "$\n")
    (insert "#\n\n")
    ;; Set text editing mode....
    (text-mode)))

;;; Define C-x c to insert my Makefile/shell script header....
;;; (C-x c was originally bound to byte-compile-file.)
(define-key ctl-x-map "c" 'setup-makefile-or-shell-script-header)

;;; Define an autograph insertion function....
(defun insert-autograph ()
  "Add an autograph comment."
  (interactive)
  (let* ((date (current-time-string))
	 (day (substring date 8 10))
	 (month (substring date 4 7))
	 (year (substring date (- (length date) 4) (length date))))
    (insert (format "%s  %s-%s-%s " (getenv "USER") day month year))))

;;; Define C-x a to insert my autograph....
;;; (C-x a was originally bound to append-to-buffer, which I rarely use.)
(define-key ctl-x-map "a" 'insert-autograph)

;;; Define a command to insert the standard DECIPHER copyright.
(defun insert-decipher-copyright ()
  "Insert the standard DECIPHER copyright at the current point."
  (interactive)
  (insert-file (concat (getenv "DECIPHER") "/common/DECIPHER-copyright")))

;;; Define M-C-d to insert the standard DECIPHER copyright (normally it does "down-list").
(define-key esc-map "\C-d" 'insert-decipher-copyright)





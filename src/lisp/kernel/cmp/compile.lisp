(in-package #:cmp)

;;;; Top-level interface: CL:COMPILE

(defparameter *lambda-args-num* 0)

(defmacro with-module (( &key module
                           (optimize nil)
                           (optimize-level '*optimization-level*)
                           dry-run) &rest body)
  `(let* ((*the-module* ,module))
     (or *the-module* (error "with-module *the-module* is NIL"))
     (multiple-value-prog1
         (with-irbuilder ((llvm-sys:make-irbuilder (thread-local-llvm-context)))
           ,@body)
       (cmp-log "About to optimize-module%N")
       ;;(cmp-log-dump-module ,module)
       (when (and ,optimize ,optimize-level (null ,dry-run)) (funcall ,optimize ,module ,optimize-level )))))

;;; See NOTE on compile-in-env below.
(defun compile-with-hook (compile-hook definition env)
  "Dispatch to clasp compiler or cleavir-clasp compiler if available.
We could do more fancy things here - like if cleavir-clasp fails, use the clasp compiler as backup."
  (with-compilation-results ()
    (if compile-hook
        (funcall compile-hook definition env)
        (error "no compile hook available"))))

;;; NOTE: cclasp may pass a definition that is a CST or AST.
;;; As such, this function should probably not examine the definition at all.
(defun compile-in-env (definition env
                       &optional (compile-hook *cleavir-compile-hook*))
  "Compile in the given environment"
  (with-compilation-unit ()
    (compile-with-hook compile-hook definition env)))

(defun builtin-wrapper-form (name)
  (when (and (fboundp name)
             (functionp (fdefinition name))
             (null (compiled-function-p (fdefinition name)))
             (typep (sys:function/entry-point (fdefinition name)) 'sys:global-bytecode-simple-fun))
    (let* ((function (fdefinition name))
           (entry-point (sys:function/entry-point function))
           (module (sys:global-bytecode-simple-fun/code entry-point))
           (compile-info (sys:bytecode-module/compile-info module))
           (code (car compile-info)))
      code)))

(export 'builtin-wrapper-form :cmp)

;;; Hook called to compile bytecode functions to native.
(defvar *btb-compile-hook*)

(defun %compile (definition environment)
  (cond
    ((and (typep definition 'core:global-bytecode-simple-fun)
          (boundp '*btb-compile-hook*))
     (compile-in-env definition environment *btb-compile-hook*))
    ((and (typep definition 'core:closure)
          (typep (core:function/entry-point definition)
                 'core:global-bytecode-simple-fun)
          (boundp '*btb-compile-hook*))
     (multiple-value-bind (csfun warn fail)
         (compile-in-env (core:function/entry-point definition)
                         environment *btb-compile-hook*)
       (let ((cells nil))
         (dotimes (i (core:closure-length definition))
           (push (core:closure-ref definition i) cells))
         (values (apply #'core:make-closure csfun (nreverse cells))
                 warn fail))))
    ((compiled-function-p definition) (values definition nil nil))
    ((functionp definition)
     (error "COMPILE doesn't know how to handle ~a definition"))
    ((and (consp definition) (eq (car definition) 'lambda))
     (cmp-log "compile form: {}%N" definition)
     (compile-in-env definition nil *cleavir-compile-hook*))
    (t (error "COMPILE doesn't know how to handle ~a" definition))))

(defun compile (name &optional definition)
  (multiple-value-bind (function warnp failp)
      ;; Get the actual compiled function and warnp+failp.
      (%compile (if (null definition)
                    (if (fboundp name)
                        (fdefinition name)
                        (error "No definition for ~a" name))
                    definition)
                nil)
    ;; Bind the name if applicable.
    (cond ((and (symbolp name) (macro-function name))
           (setf (macro-function name) function)
           (values name warnp failp))
          (name
           (setf (fdefinition name) function)
           (values name warnp failp))
          (t (values function warnp failp)))))

(defun compiler-stats ()
  (core:fmt t "Accumulated finalization time {}%N" llvm-sys:*accumulated-llvm-finalization-time*)
  (core:fmt t "Most recent finalization time {}%N" llvm-sys:*most-recent-llvm-finalization-time*)
  (core:fmt t "Number of compilations {}%N" llvm-sys:*number-of-llvm-finalizations*))

(export 'compiler-stats)

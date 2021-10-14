(in-package #:clasp-cleavir)

;;;; Variables and accessors used in the translation process.
;;;; In a separate file because translate-bir and landing-pad-bir both need
;;;; these and circular dependencies are bad.

(defvar *tags*)
(defvar *datum-values*)
(defvar *constant-values*)
(defvar *dynenv-storage*)
(defvar *unwind-ids*)
(defvar *function-info*)
(defvar *enclose-initializers*)

;;; In CSTs and stuff the origin is (spi . spi). Use the head.
(defun origin-spi (origin)
  (if (consp origin) (car origin) origin))

(defun ensure-origin (origin &optional (num 999905))
  (or origin
      (core:make-source-pos-info :filepos num :lineno num :column num)))

(defun delay-initializer (initializer-thunk)
  (push initializer-thunk *enclose-initializers*))

(defun force-initializers ()
  (loop (unless *enclose-initializers* (return))
        (funcall (pop *enclose-initializers*))))

(defun iblock-tag (iblock)
  (or (gethash iblock *tags*)
      (error "BUG: No tag for iblock: ~a" iblock)))

(defun datum-name-as-string (datum)
  ;; We need to write out setf names as well as symbols, in a simple way.
  ;; "simple" means no pretty printer, for a start.
  ;; Using SYMBOL-NAME like this is about 25x faster than using write-to-string,
  ;; and this function is called rather a lot so it's nice to make it faster.
  (let ((name (bir:name datum)))
    (if (symbolp name)
        (symbol-name name)
        (write-to-string name
                         :escape nil
                         :readably nil
                         :pretty nil))))

;;; This function is used for names for debug information, so we want them to be
;;; a little bit more complete.
(defun full-datum-name-as-string (datum)
  (let ((*package* (find-package "KEYWORD")))
    (write-to-string datum :escape t :readably nil :pretty nil)))

(defun vrtype->llvm (vrtype)
  (ecase vrtype
    ((:object) cmp:%t*%)
    ((:single-float) cmp:%float%)
    ((:double-float) cmp:%double%)))

(defun bind-variable (var)
  (if (bir:immutablep var)
      ;; Since immutable vars are just LLVM Values, they will be initialized
      ;; by their single VARIABLE-OUT call.
      nil
      (setf (gethash var *datum-values*)
            (ecase (bir:extent var)
              ((:local :dynamic)
               ;; just an alloca
               (let* ((name (datum-name-as-string var))
                      #+(or)
                      (fname (full-datum-name-as-string var))
                      ;; FIXME: Seems broken for an rtype of (); what should we
                      ;; do in that situation?
                      (vrtype (first (cc-bmir:rtype var)))
                      (alloca (cmp:alloca (vrtype->llvm vrtype) 1 name))
                      #+(or)
                      (spi (origin-spi (bir:origin var))))
                 ;; set up debug info
                 ;; Disable for now - FIXME and get it working
                 #+(or)(cmp:dbg-variable-alloca alloca fname spi)
                 ;; return
                 alloca))
              ((:indefinite)
               ;; make a cell
               (%intrinsic-invoke-if-landing-pad-or-call
                "cc_makeCell" nil (datum-name-as-string var)))))))

;; Return either the value or cell of a closed over variable depending
;; on whether it is immutable so we can close over the memory location
;; and implement the the cell indirection properly when the variable
;; is mutable and closed over.
(defun variable-as-argument (variable)
  (let ((value/cell (or (gethash variable *datum-values*)
                        (error "BUG: Variable or cell missing: ~a" variable))))
    (if (or (typep variable 'bir:catch)
            (bir:immutablep variable))
        value/cell
        (ecase (bir:extent variable)
          (:indefinite value/cell)
          (:dynamic (cmp:irc-bit-cast value/cell cmp:%t*%))
          (:local
           (error "Should not be passing the local variable ~a as an environment argument." variable))))))

(defun in (datum)
  (check-type datum (or bir:phi bir:ssa))
  (multiple-value-bind (dat presentp) (gethash datum *datum-values*)
    (if presentp
        dat
        (error "BUG: No variable for datum: ~a defined by ~a"
               datum (bir:definitions datum)))))

(defun variable-in (variable)
  (check-type variable bir:variable)
  (if (bir:immutablep variable)
      (or (gethash variable *datum-values*)
          (error "BUG: Variable missing: ~a" variable))
      (ecase (bir:extent variable)
        (:local
         (let ((alloca (or (gethash variable *datum-values*)
                           (error "BUG: Variable missing: ~a" variable))))
           (cmp:irc-load alloca)))
        (:dynamic
         (let ((alloca (or (gethash variable *datum-values*)
                           (error "BUG: DX cell missing: ~a" variable))))
           (cmp:irc-load (cmp:irc-bit-cast alloca cmp:%t**%))))
        (:indefinite
         (let ((cell (or (gethash variable *datum-values*)
                         (error "BUG: Cell missing: ~a" variable)))
               (offset (- cmp:+cons-car-offset+ cmp:+cons-tag+)))
           (cmp:irc-load-atomic (cmp::gen-memref-address cell offset)))))))

(defun out (value datum)
  (check-type datum bir:ssa)
  (assert (not (nth-value 1 (gethash datum *datum-values*)))
          ()
          "Double OUT for ~a: Old value ~a, new value ~a"
          datum (gethash datum *datum-values*) value)
  (setf (gethash datum *datum-values*) value))

(defun phi-out (value datum llvm-block)
  (check-type datum bir:phi)
  (let ((rt (cc-bmir:rtype datum)))
    (cond ((or (eq rt :multiple-values)
               (equal rt '(:object))) ; datum is a T_mv or T_O* respectively
           (llvm-sys:add-incoming (in datum) value llvm-block))
          ((null rt)) ; no values, do nothing
          ((and (listp rt)
                (every (lambda (x) (eq x :object)) rt))
           ;; Datum is a list of llvm data, and (in datum) is a list of phis.
           (loop for phi in (in datum)
                 for val in value
                 do (llvm-sys:add-incoming phi val llvm-block)))
          (t (error "BUG: Bad rtype ~a" rt)))))

(defun variable-out (value variable)
  (check-type variable bir:variable)
  (if (bir:immutablep variable)
      (prog1 (setf (gethash variable *datum-values*) value)
        ;; FIXME - this doesn't work yet
        #+(or)(cmp:dbg-variable-value
               value (full-datum-name-as-string variable)
               (origin-spi (bir:origin variable))))
      (ecase (bir:extent variable)
        (:local
         (let ((alloca (or (gethash variable *datum-values*)
                           (error "BUG: Variable missing: ~a" variable))))
           (cmp:irc-store value alloca)))
        (:dynamic
         (let ((alloca (or (gethash variable *datum-values*)
                           (error "BUG: DX cell missing: ~a" variable))))
           (cmp:irc-store value (cmp:irc-bit-cast alloca cmp:%t**%))))
        (:indefinite
         (let ((cell (or (gethash variable *datum-values*)
                         (error "BUG: Cell missing: ~a" variable)))
               (offset (- cmp:+cons-car-offset+ cmp:+cons-tag+)))
           (cmp:irc-store-atomic
            value
            (cmp::gen-memref-address cell offset)))))))

(defun dynenv-storage (dynenv)
  (check-type dynenv bir:dynamic-environment)
  (or (gethash dynenv *dynenv-storage*)
      (error "BUG: Missing dynenv storage for ~a" dynenv)))

(defun (setf dynenv-storage) (new dynenv)
  (setf (gethash dynenv *dynenv-storage*) new))

(defun get-destination-id (iblock)
  (or (gethash iblock *unwind-ids*)
      (error "Missing unwind ID for ~a" iblock)))

;; Does this iblock have nonlocal entrances?
(defun has-entrances-p (iblock)
  (not (cleavir-set:empty-set-p (bir:entrances iblock))))

(defun find-llvm-function-info (function)
  (or (gethash function *function-info*)
      (error "Missing llvm function info for BIR function ~a." function)))

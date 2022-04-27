(defun add-aclasp-sources (&optional (target :aclasp) wrappers)
  (k:sources target
             #~"kernel/tag/start.lisp"
             #~"kernel/lsp/prologue.lisp")
  (when wrappers
    (k:sources target
               #~"kernel/lsp/direct-calls.lisp"
               (k:make-source "generated/cl-wrappers.lisp" :variant)))
  (k:sources target
             #~"kernel/tag/min-start.lisp"
             #~"kernel/init.lisp"
             #~"kernel/tag/after-init.lisp"
             #~"kernel/cmp/runtime-info.lisp"
             #~"kernel/lsp/sharpmacros.lisp"
             #~"kernel/cmp/jit-setup.lisp"
             #~"kernel/clsymbols.lisp"
             #~"kernel/lsp/packages.lisp"
             #~"kernel/lsp/foundation.lisp"
             #~"kernel/lsp/export.lisp"
             #~"kernel/lsp/defmacro.lisp"
             #~"kernel/lsp/helpfile.lisp"
             #~"kernel/lsp/evalmacros.lisp"
             #~"kernel/lsp/claspmacros.lisp"
             #~"kernel/lsp/source-transformations.lisp"
             #~"kernel/lsp/arraylib.lisp"
             #~"kernel/lsp/setf.lisp"
             #~"kernel/lsp/listlib.lisp"
             #~"kernel/lsp/mislib.lisp"
             #~"kernel/lsp/defstruct.lisp"
             #~"kernel/lsp/predlib.lisp"
             #~"kernel/lsp/cdr-5.lisp"
             #~"kernel/lsp/cmuutil.lisp"
             #~"kernel/lsp/seqmacros.lisp"
             #~"kernel/lsp/seq.lisp"
             #~"kernel/lsp/seqlib.lisp"
             #~"kernel/lsp/iolib.lisp"
             #~"kernel/lsp/trace.lisp"
             #~"kernel/lsp/debug.lisp"
             #~"kernel/cmp/cmpexports.lisp"
             #~"kernel/cmp/cmpsetup.lisp"
             #~"kernel/cmp/cmpglobals.lisp"
             #~"kernel/cmp/cmputil.lisp"
             #~"kernel/cmp/cmpintrinsics.lisp"
             #~"kernel/cmp/primitives.lisp"
             #~"kernel/cmp/cmpir.lisp"
             #~"kernel/cmp/cmpeh.lisp"
             #~"kernel/cmp/debuginfo.lisp"
             #~"kernel/cmp/codegen-vars.lisp"
             #~"kernel/cmp/arguments.lisp"
             #~"kernel/cmp/cmplambda.lisp"
             #~"kernel/cmp/cmprunall.lisp"
             #~"kernel/cmp/cmpliteral.lisp"
             #~"kernel/cmp/typeq.lisp"
             #~"kernel/cmp/codegen-special-form.lisp"
             #~"kernel/cmp/codegen.lisp"
             #~"kernel/cmp/compile.lisp"
             #~"kernel/cmp/codegen-toplevel.lisp"
             #~"kernel/cmp/compile-file.lisp"
             #~"kernel/cmp/external-clang.lisp"
             #~"kernel/cmp/cmpname.lisp"
             #~"kernel/cmp/cmpbundle.lisp"
             #~"kernel/cmp/cmprepl.lisp"
             #~"kernel/tag/min-pre-epilogue.lisp"
             #~"kernel/lsp/epilogue-aclasp.lisp"
             #~"kernel/tag/min-end.lisp"))
    
(defun add-bclasp-sources (&optional (target :bclasp))
  (add-aclasp-sources target t)
  (k:sources target
             #~"kernel/tag/bclasp-start.lisp"
             #~"kernel/cmp/cmpwalk.lisp"
             #~"kernel/lsp/assert.lisp"
             #~"kernel/lsp/numlib.lisp"
             #~"kernel/lsp/describe.lisp"
             #~"kernel/lsp/module.lisp"
             #~"kernel/lsp/loop2.lisp"
             #~"kernel/cmp/disassemble.lisp"
             #~"kernel/cmp/opt/opt.lisp" ; need loop
             #~"kernel/cmp/opt/opt-character.lisp"
             #~"kernel/cmp/opt/opt-number.lisp"
             #~"kernel/cmp/opt/opt-type.lisp"
             #~"kernel/cmp/opt/opt-control.lisp"
             #~"kernel/cmp/opt/opt-sequence.lisp"
             #~"kernel/cmp/opt/opt-cons.lisp"
             #~"kernel/cmp/opt/opt-array.lisp"
             #~"kernel/cmp/opt/opt-object.lisp"
             #~"kernel/cmp/opt/opt-condition.lisp"
             #~"kernel/cmp/opt/opt-print.lisp"
             #~"kernel/lsp/shiftf-rotatef.lisp"
             #~"kernel/lsp/assorted.lisp"
             #~"kernel/lsp/packlib.lisp"
             #~"kernel/lsp/defpackage.lisp"
             #~"kernel/lsp/format.lisp"
             #~"kernel/lsp/mp.lisp"
             #~"kernel/lsp/atomics.lisp"
             #~"kernel/clos/package.lisp"
             #~"kernel/clos/static-gfs/package.lisp"
             #~"kernel/clos/static-gfs/flag.lisp"
             #~"kernel/clos/static-gfs/constructor.lisp"
             #~"kernel/clos/static-gfs/reinitializer.lisp"
             #~"kernel/clos/static-gfs/changer.lisp"
             #~"kernel/clos/hierarchy.lisp"
             #~"kernel/clos/cpl.lisp"
             #~"kernel/clos/std-slot-value.lisp"
             #~"kernel/clos/slot.lisp"
             #~"kernel/clos/boot.lisp"
             #~"kernel/clos/kernel.lisp"
             #~"kernel/clos/outcome.lisp"
             #~"kernel/clos/discriminate.lisp"
             #~"kernel/clos/dtree.lisp"
             #~"kernel/clos/effective-accessor.lisp"
             #~"kernel/clos/closfastgf.lisp"
             #~"kernel/clos/satiation.lisp"
             #~"kernel/clos/method.lisp"
             #~"kernel/clos/combin.lisp"
             #~"kernel/clos/std-accessors.lisp"
             #~"kernel/clos/defclass.lisp"
             #~"kernel/clos/slotvalue.lisp"
             #~"kernel/clos/standard.lisp"
             #~"kernel/clos/builtin.lisp"
             #~"kernel/clos/change.lisp"
             #~"kernel/clos/stdmethod.lisp"
             #~"kernel/clos/generic.lisp"
             #~"kernel/clos/fixup.lisp"
             #~"kernel/clos/static-gfs/cell.lisp"
             #~"kernel/clos/static-gfs/effective-method.lisp"
             #~"kernel/clos/static-gfs/svuc.lisp"
             #~"kernel/clos/static-gfs/shared-initialize.lisp"
             #~"kernel/clos/static-gfs/initialize-instance.lisp"
             #~"kernel/clos/static-gfs/allocate-instance.lisp"
             #~"kernel/clos/static-gfs/make-instance.lisp"
             #~"kernel/clos/static-gfs/compute-constructor.lisp"
             #~"kernel/clos/static-gfs/dependents.lisp"
             #~"kernel/clos/static-gfs/compiler-macros.lisp"
             #~"kernel/clos/static-gfs/reinitialize-instance.lisp"
             #~"kernel/clos/static-gfs/update-instance-for-different-class.lisp"
             #~"kernel/clos/static-gfs/change-class.lisp"
             #~"kernel/lsp/source-location.lisp"
             #~"kernel/lsp/defvirtual.lisp"
             #~"kernel/clos/conditions.lisp"
             #~"kernel/clos/print.lisp"
             #~"kernel/clos/streams.lisp"
             #~"kernel/clos/sequences.lisp"
             #~"kernel/lsp/pprint.lisp"
             #~"kernel/cmp/compiler-conditions.lisp"
             #~"kernel/lsp/packlib2.lisp"
             #~"kernel/clos/inspect.lisp"
             #~"kernel/lsp/fli.lisp"
             #~"kernel/lsp/posix.lisp"
             #~"modules/sockets/sockets.lisp"
             #~"kernel/lsp/top.lisp"
             #~"kernel/tag/pre-epilogue-bclasp.lisp"
             #~"kernel/lsp/epilogue-bclasp.lisp"
             #~"kernel/tag/bclasp.lisp"))

(defun add-cclasp-sources (&optional (target :cclasp))
  (add-bclasp-sources target)
  (k:sources target
             #~"kernel/contrib/Acclimation/packages.lisp" ;; Cleavir sources
             #~"kernel/contrib/Acclimation/locale.lisp"
             #~"kernel/contrib/Acclimation/date.lisp"
             #~"kernel/contrib/Acclimation/language.lisp"
             #~"kernel/contrib/Acclimation/language-english.lisp"
             #~"kernel/contrib/Acclimation/language-french.lisp"
             #~"kernel/contrib/Acclimation/language-swedish.lisp"
             #~"kernel/contrib/Acclimation/language-vietnamese.lisp"
             #~"kernel/contrib/Acclimation/language-japanese.lisp"
             #~"kernel/contrib/Acclimation/condition.lisp"
             #~"kernel/contrib/Acclimation/documentation.lisp"
             #~"kernel/contrib/Acclimation/init.lisp"
             #~"kernel/contrib/Concrete-Syntax-Tree/packages.lisp"
             #~"kernel/contrib/Concrete-Syntax-Tree/generic-functions.lisp"
             #~"kernel/contrib/Concrete-Syntax-Tree/cst.lisp"
             #~"kernel/contrib/Concrete-Syntax-Tree/cons-cst.lisp"
             #~"kernel/contrib/Concrete-Syntax-Tree/listify.lisp"
             #~"kernel/contrib/Concrete-Syntax-Tree/cstify.lisp"
             #~"kernel/contrib/Concrete-Syntax-Tree/cst-from-expression.lisp"
             #~"kernel/contrib/Concrete-Syntax-Tree/quasiquotation.lisp"
             #~"kernel/contrib/Concrete-Syntax-Tree/reconstruct.lisp"
             #~"kernel/contrib/Concrete-Syntax-Tree/declarations.lisp"
             #~"kernel/contrib/Concrete-Syntax-Tree/body.lisp"
             #~"kernel/contrib/Concrete-Syntax-Tree/list-structure.lisp"
             #~"kernel/contrib/Concrete-Syntax-Tree/bindings.lisp"
             #~"kernel/contrib/Concrete-Syntax-Tree/conditions.lisp"
             #~"kernel/contrib/Concrete-Syntax-Tree/condition-reporters-english.lisp"
             #~"kernel/contrib/Concrete-Syntax-Tree/Lambda-list/client.lisp"
             #~"kernel/contrib/Concrete-Syntax-Tree/Lambda-list/ensure-proper.lisp"
             #~"kernel/contrib/Concrete-Syntax-Tree/Lambda-list/grammar-symbols.lisp"
             #~"kernel/contrib/Concrete-Syntax-Tree/Lambda-list/lambda-list-keywords.lisp"
             #~"kernel/contrib/Concrete-Syntax-Tree/Lambda-list/grammar.lisp"
             #~"kernel/contrib/Concrete-Syntax-Tree/Lambda-list/standard-grammars.lisp"
             #~"kernel/contrib/Concrete-Syntax-Tree/Lambda-list/earley-item.lisp"
             #~"kernel/contrib/Concrete-Syntax-Tree/Lambda-list/earley-state.lisp"
             #~"kernel/contrib/Concrete-Syntax-Tree/Lambda-list/parser.lisp"
             #~"kernel/contrib/Concrete-Syntax-Tree/Lambda-list/scanner-action.lisp"
             #~"kernel/contrib/Concrete-Syntax-Tree/Lambda-list/earley.lisp"
             #~"kernel/contrib/Concrete-Syntax-Tree/Lambda-list/parse-top-levels.lisp"
             #~"kernel/contrib/Concrete-Syntax-Tree/Lambda-list/unparse.lisp"
             #~"kernel/contrib/alexandria/alexandria-1/package.lisp"
             #~"kernel/contrib/alexandria/alexandria-1/binding.lisp"
             #~"kernel/contrib/alexandria/alexandria-1/conditions.lisp"
             #~"kernel/contrib/alexandria/alexandria-1/definitions.lisp"
             #~"kernel/contrib/alexandria/alexandria-1/strings.lisp"
             #~"kernel/contrib/alexandria/alexandria-1/symbols.lisp"
             #~"kernel/contrib/alexandria/alexandria-1/macros.lisp"
             #~"kernel/contrib/alexandria/alexandria-1/control-flow.lisp"
             #~"kernel/contrib/alexandria/alexandria-1/functions.lisp"
             #~"kernel/contrib/alexandria/alexandria-1/hash-tables.lisp"
             #~"kernel/contrib/alexandria/alexandria-1/features.lisp"
             #~"kernel/contrib/alexandria/alexandria-1/lists.lisp"
             #~"kernel/contrib/alexandria/alexandria-1/types.lisp"
             #~"kernel/contrib/alexandria/alexandria-1/arrays.lisp"
             #~"kernel/contrib/alexandria/alexandria-1/io.lisp"
             #~"kernel/contrib/alexandria/alexandria-1/sequences.lisp"
             #~"kernel/contrib/alexandria/alexandria-1/numbers.lisp"
             #~"kernel/contrib/alexandria/alexandria-2/package.lisp"
             #~"kernel/contrib/alexandria/alexandria-2/arrays.lisp"
             #~"kernel/contrib/alexandria/alexandria-2/control-flow.lisp"
             #~"kernel/contrib/alexandria/alexandria-2/sequences.lisp"
             #~"kernel/contrib/alexandria/alexandria-2/lists.lisp"
             #~"kernel/contrib/closer-mop/closer-mop-packages.lisp"
             #~"kernel/contrib/closer-mop/closer-mop-shared.lisp"
             #~"kernel/contrib/closer-mop/closer-clasp.lisp"
             #~"kernel/contrib/Eclector/code/base/package.lisp"
             #~"kernel/contrib/Eclector/code/base/conditions.lisp"
             #~"kernel/contrib/Eclector/code/base/read-char.lisp"
             #~"kernel/contrib/Eclector/code/base/messages-english.lisp"
             #~"kernel/contrib/Eclector/code/readtable/package.lisp"
             #~"kernel/contrib/Eclector/code/readtable/variables.lisp"
             #~"kernel/contrib/Eclector/code/readtable/conditions.lisp"
             #~"kernel/contrib/Eclector/code/readtable/generic-functions.lisp"
             #~"kernel/contrib/Eclector/code/readtable/messages-english.lisp"
             #~"kernel/contrib/Eclector/code/readtable/simple/package.lisp"
             #~"kernel/contrib/Eclector/code/readtable/simple/readtable.lisp"
             #~"kernel/contrib/Eclector/code/readtable/simple/methods.lisp"
             #~"kernel/contrib/Eclector/code/readtable/simple/messages-english.lisp"
             #~"kernel/contrib/Eclector/code/reader/package.lisp"
             #~"kernel/contrib/Eclector/code/reader/generic-functions.lisp"
             #~"kernel/contrib/Eclector/code/reader/more-variables.lisp"
             #~"kernel/contrib/Eclector/code/reader/additional-conditions.lisp"
             #~"kernel/contrib/Eclector/code/reader/utilities.lisp"
             #~"kernel/contrib/Eclector/code/reader/tokens.lisp"
             #~"kernel/contrib/Eclector/code/reader/read-common.lisp"
             #~"kernel/contrib/Eclector/code/reader/read.lisp"
             #~"kernel/contrib/Eclector/code/reader/macro-functions.lisp"
             #~"kernel/contrib/Eclector/code/reader/init.lisp"
             #~"kernel/contrib/Eclector/code/reader/quasiquote-macro.lisp"
             #~"kernel/contrib/Eclector/code/reader/fixup.lisp"
             #~"kernel/contrib/Eclector/code/reader/messages-english.lisp"
             #~"kernel/contrib/Eclector/code/parse-result/package.lisp"
             #~"kernel/contrib/Eclector/code/parse-result/client.lisp"
             #~"kernel/contrib/Eclector/code/parse-result/generic-functions.lisp"
             #~"kernel/contrib/Eclector/code/parse-result/read.lisp"
             #~"kernel/contrib/Eclector/code/concrete-syntax-tree/package.lisp"
             #~"kernel/contrib/Eclector/code/concrete-syntax-tree/read.lisp"
             #~"kernel/contrib/Cleavir/Input-output/packages.lisp"
             #~"kernel/contrib/Cleavir/Input-output/io.lisp"
             #~"kernel/contrib/Cleavir/Attributes/packages.lisp"
             #~"kernel/contrib/Cleavir/Attributes/flags.lisp"
             #~"kernel/contrib/Cleavir/Attributes/attributes.lisp"
             #~"kernel/contrib/Cleavir/Primop/packages.lisp"
             #~"kernel/contrib/Cleavir/Primop/info.lisp"
             #~"kernel/contrib/Cleavir/Primop/definitions.lisp"
             #~"kernel/contrib/Cleavir/Set/packages.lisp"
             #~"kernel/contrib/Cleavir/Set/set.lisp"
             #~"kernel/contrib/Cleavir/Conditions/packages.lisp"
             #~"kernel/contrib/Cleavir/Conditions/program-condition.lisp"
             #~"kernel/contrib/Cleavir/Conditions/origin.lisp"
             #~"kernel/contrib/Cleavir/Conditions/note.lisp"
             #~"kernel/contrib/Cleavir/Ctype/packages.lisp"
             #~"kernel/contrib/Cleavir/Ctype/generic-functions.lisp"
             #~"kernel/contrib/Cleavir/Ctype/other-functions.lisp"
             #~"kernel/contrib/Cleavir/Ctype/default.lisp"
             #~"kernel/contrib/Cleavir/BIR/packages.lisp"
             #~"kernel/contrib/Cleavir/BIR/structure.lisp"
             #~"kernel/contrib/Cleavir/BIR/instructions.lisp"
             #~"kernel/contrib/Cleavir/BIR/map.lisp"
             #~"kernel/contrib/Cleavir/BIR/conditions.lisp"
             #~"kernel/contrib/Cleavir/BIR/graph-modifications.lisp"
             #~"kernel/contrib/Cleavir/BIR/verify.lisp"
             #~"kernel/contrib/Cleavir/BIR/disassemble.lisp"
             #~"kernel/contrib/Cleavir/BIR/condition-reporters-english.lisp"
             #~"kernel/contrib/Cleavir/Meter/packages.lisp"
             #~"kernel/contrib/Cleavir/Meter/meter.lisp"
             #~"kernel/contrib/Cleavir/Abstract-syntax-tree/packages.lisp"
             #~"kernel/contrib/Cleavir/Abstract-syntax-tree/general-purpose-asts.lisp"
             #~"kernel/contrib/Cleavir/Abstract-syntax-tree/fixnum-related-asts.lisp"
             #~"kernel/contrib/Cleavir/Abstract-syntax-tree/simple-float-related-asts.lisp"
             #~"kernel/contrib/Cleavir/Abstract-syntax-tree/cons-related-asts.lisp"
             #~"kernel/contrib/Cleavir/Abstract-syntax-tree/standard-object-related-asts.lisp"
             #~"kernel/contrib/Cleavir/Abstract-syntax-tree/array-related-asts.lisp"
             #~"kernel/contrib/Cleavir/Abstract-syntax-tree/scope-related-asts.lisp"
             #~"kernel/contrib/Cleavir/Abstract-syntax-tree/graphviz-drawing.lisp"
             #~"kernel/contrib/Cleavir/Abstract-syntax-tree/map-ast.lisp"
             #~"kernel/contrib/Cleavir/AST-to-BIR/packages.lisp"
             #~"kernel/contrib/Cleavir/AST-to-BIR/infrastructure.lisp"
             #~"kernel/contrib/Cleavir/AST-to-BIR/compile-general-purpose-asts.lisp"
             #~"kernel/contrib/Cleavir/AST-to-BIR/compile-multiple-value-related-asts.lisp"
             #~"kernel/contrib/Cleavir/AST-to-BIR/compile-primops.lisp"
             #~"kernel/contrib/Cleavir/AST-to-BIR/compile-fixnum-related-asts.lisp"
             #~"kernel/contrib/Cleavir/BIR-transformations/packages.lisp"
             #~"kernel/contrib/Cleavir/BIR-transformations/eliminate-catches.lisp"
             #~"kernel/contrib/Cleavir/BIR-transformations/process-captured-variables.lisp"
             #~"kernel/contrib/Cleavir/BIR-transformations/delete-temporary-variables.lisp"
             #~"kernel/contrib/Cleavir/BIR-transformations/interpolate-function.lisp"
             #~"kernel/contrib/Cleavir/BIR-transformations/copy-function.lisp"
             #~"kernel/contrib/Cleavir/BIR-transformations/inline.lisp"
             #~"kernel/contrib/Cleavir/BIR-transformations/simple-unwind.lisp"
             #~"kernel/contrib/Cleavir/BIR-transformations/meta-evaluate.lisp"
             #~"kernel/contrib/Cleavir/BIR-transformations/generate-type-checks.lisp"
             #~"kernel/contrib/Concrete-Syntax-Tree/Destructuring/variables.lisp"
             #~"kernel/contrib/Concrete-Syntax-Tree/Destructuring/generic-functions.lisp"
             #~"kernel/contrib/Concrete-Syntax-Tree/Destructuring/conditions.lisp"
             #~"kernel/contrib/Concrete-Syntax-Tree/Destructuring/whole-parameters.lisp"
             #~"kernel/contrib/Concrete-Syntax-Tree/Destructuring/condition-generation.lisp"
             #~"kernel/contrib/Concrete-Syntax-Tree/Destructuring/required-parameters.lisp"
             #~"kernel/contrib/Concrete-Syntax-Tree/Destructuring/optional-parameters.lisp"
             #~"kernel/contrib/Concrete-Syntax-Tree/Destructuring/rest-parameters.lisp"
             #~"kernel/contrib/Concrete-Syntax-Tree/Destructuring/key-parameters.lisp"
             #~"kernel/contrib/Concrete-Syntax-Tree/Destructuring/aux-parameters.lisp"
             #~"kernel/contrib/Concrete-Syntax-Tree/Destructuring/lambda-list.lisp"
             #~"kernel/contrib/Concrete-Syntax-Tree/Destructuring/parse-macro.lisp"
             #~"kernel/contrib/Concrete-Syntax-Tree/Destructuring/db-defmacro.lisp"
             #~"kernel/contrib/Concrete-Syntax-Tree/Destructuring/condition-reporters-english.lisp"
             #~"kernel/contrib/Cleavir/AST-transformations/packages.lisp"
             #~"kernel/contrib/Cleavir/AST-transformations/clone.lisp"
             #~"kernel/contrib/Cleavir/AST-transformations/replace.lisp"
             #~"kernel/contrib/Cleavir/AST-transformations/hoist-load-time-value.lisp"
             #~"kernel/contrib/Cleavir/Environment/packages.lisp"
             #~"kernel/contrib/Cleavir/Environment/query.lisp"
             #~"kernel/contrib/Cleavir/Environment/augmentation-functions.lisp"
             #~"kernel/contrib/Cleavir/Environment/default-augmentation-classes.lisp"
             #~"kernel/contrib/Cleavir/Environment/compile-time.lisp"
             #~"kernel/contrib/Cleavir/Environment/optimize-qualities.lisp"
             #~"kernel/contrib/Cleavir/Environment/declarations.lisp"
             #~"kernel/contrib/Cleavir/Environment/type-information.lisp"
             #~"kernel/contrib/Cleavir/Environment/default-info-methods.lisp"
             #~"kernel/contrib/Cleavir/Environment/eval.lisp"
             #~"kernel/contrib/Cleavir/Compilation-policy/packages.lisp"
             #~"kernel/contrib/Cleavir/Compilation-policy/conditions.lisp"
             #~"kernel/contrib/Cleavir/Compilation-policy/condition-reporters-english.lisp"
             #~"kernel/contrib/Cleavir/Compilation-policy/policy.lisp"
             #~"kernel/contrib/Cleavir/Compilation-policy/define-policy.lisp"
             #~"kernel/contrib/Cleavir/Compilation-policy/optimize.lisp"
             #~"kernel/contrib/Cleavir/Compilation-policy/compute.lisp"
             #~"kernel/contrib/Cleavir/CST-to-AST/packages.lisp"
             #~"kernel/contrib/Cleavir/CST-to-AST/conditions.lisp"
             #~"kernel/contrib/Cleavir/CST-to-AST/condition-reporters-english.lisp"
             #~"kernel/contrib/Cleavir/CST-to-AST/environment-augmentation.lisp"
             #~"kernel/contrib/Cleavir/CST-to-AST/environment-query.lisp"
             #~"kernel/contrib/Cleavir/CST-to-AST/variables.lisp"
             #~"kernel/contrib/Cleavir/CST-to-AST/generic-functions.lisp"
             #~"kernel/contrib/Cleavir/CST-to-AST/convert-function-reference.lisp"
             #~"kernel/contrib/Cleavir/CST-to-AST/convert-special-binding.lisp"
             #~"kernel/contrib/Cleavir/CST-to-AST/utilities.lisp"
             #~"kernel/contrib/Cleavir/CST-to-AST/set-or-bind-variable.lisp"
             #~"kernel/contrib/Cleavir/CST-to-AST/process-progn.lisp"
             #~"kernel/contrib/Cleavir/CST-to-AST/convert-sequence.lisp"
             #~"kernel/contrib/Cleavir/CST-to-AST/convert-variable.lisp"
             #~"kernel/contrib/Cleavir/CST-to-AST/convert.lisp"
             #~"kernel/contrib/Cleavir/CST-to-AST/process-init-parameter.lisp"
             #~"kernel/contrib/Cleavir/CST-to-AST/itemize-declaration-specifiers.lisp"
             #~"kernel/contrib/Cleavir/CST-to-AST/itemize-lambda-list.lisp"
             #~"kernel/contrib/Cleavir/CST-to-AST/lambda-list-from-parameter-groups.lisp"
             #~"kernel/contrib/Cleavir/CST-to-AST/convert-setq.lisp"
             #~"kernel/contrib/Cleavir/CST-to-AST/convert-let-and-letstar.lisp"
             #~"kernel/contrib/Cleavir/CST-to-AST/convert-code.lisp"
             #~"kernel/contrib/Cleavir/CST-to-AST/convert-lambda-call.lisp"
             #~"kernel/contrib/Cleavir/CST-to-AST/convert-constant.lisp"
             #~"kernel/contrib/Cleavir/CST-to-AST/convert-special.lisp"
             #~"kernel/contrib/Cleavir/CST-to-AST/convert-primop.lisp"
             #~"kernel/contrib/Cleavir/CST-to-AST/convert-cst.lisp"
             #~"kernel/contrib/Cleavir/CST-to-AST/cst-to-ast.lisp"
             #~"kernel/cleavir/packages.lisp"
             #~"kernel/cleavir/system.lisp"
             #~"kernel/cleavir/policy.lisp"
             #~"kernel/cleavir/reader.lisp"
             #~"kernel/cleavir/ast.lisp"
             #~"kernel/cleavir/convert-form.lisp"
             #~"kernel/cleavir/convert-special.lisp"
             #~"kernel/cleavir/ast-interpreter.lisp"
             #~"kernel/cleavir/toplevel.lisp"
             #~"kernel/cleavir/setup.lisp"
             #~"kernel/cleavir/fold.lisp"
             #~"kernel/cleavir/ir.lisp"
             #~"kernel/cleavir/compile-file-client.lisp"
             #~"kernel/cleavir/translation-environment.lisp"
             #~"kernel/cleavir/bir.lisp"
             #~"kernel/cleavir/bmir.lisp"
             #~"kernel/cleavir/vaslist.lisp"
             #~"kernel/cleavir/bir-to-bmir.lisp"
             #~"kernel/cleavir/representation-selection.lisp"
             #~"kernel/cleavir/landing-pad.lisp"
             #~"kernel/cleavir/primop.lisp"
             #~"kernel/cleavir/transform.lisp"
             #~"kernel/cleavir/translate.lisp"
             #~"kernel/cleavir/fixup-eclector-readtables.lisp"
             #~"kernel/cleavir/activate-clasp-readtables-for-eclector.lisp"
             #~"kernel/cleavir/define-unicode-tables.lisp"
             #~"kernel/cleavir/inline-prep.lisp"
             #~"kernel/cleavir/auto-compile.lisp"
             #~"kernel/cleavir/inline.lisp"
             #~"kernel/lsp/queue.lisp" ;; cclasp sources
             #~"kernel/cmp/compile-file-parallel.lisp"
             #~"kernel/lsp/generated-encodings.lisp"
             #~"kernel/lsp/encodings.lisp"
             #~"kernel/lsp/cltl2.lisp"
             #~"kernel/tag/pre-epilogue-cclasp.lisp"
             #~"kernel/lsp/epilogue-cclasp.lisp"
             #~"kernel/tag/cclasp.lisp"))

(add-aclasp-sources)

(add-bclasp-sources)

(add-cclasp-sources)

(k:sources :modules
           #~"modules/asdf/build/asdf.lisp"
           #~"modules/serve-event/serve-event.lisp")

(k:sources :install-code
           #~"modules/")
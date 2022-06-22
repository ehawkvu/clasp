(in-package #:koga)

(defun system-source-file (system)
  (let ((path (asdf:system-source-file system)))
    (when path
      (let ((sub-path (uiop:subpathp path (truename (root :code)))))
        (when sub-path
          (ignore-errors
            (make-pathname :host "SYS" 
                           :directory (list* :absolute (cdr (pathname-directory sub-path)))
                           :name (pathname-name sub-path)
                           :type (pathname-type sub-path)
                           :version nil)))))))

(defparameter +asdf-system-initargs+
  '((asdf:component-version :version)
    (asdf::component-description :description)
    (asdf::component-long-description :long-description)
    (asdf:system-author :author)
    (asdf:system-maintainer :maintainer)
    (asdf:system-license :license)
    (asdf:system-homepage :homepage)
    (asdf:system-bug-tracker :bug-tracker)
    (asdf:system-mailto :mailto)
    (asdf:system-long-name :long-name)
    (system-source-file :source-file)
    (asdf:system-source-control :source-control)))

(defmethod add-target-source (configuration target (source symbol))
  (multiple-value-bind (modules systems files)
      (asdf-groveler:grovel (list source)
                            :file-type 'asdf:cl-source-file
                            :features (features configuration))
    (when modules
      (error "Found module dependencies of ~{~#[~;~a~;~a and ~a~:;~@{~a~#[~;, and ~:;, ~]~}~]~} for system ~a."
             modules source))
    (loop with root = (truename (root :code))
          for file in files
          for relative-path = (uiop:subpathp file root)
          if relative-path
            do (add-target-source configuration target (make-source relative-path :code))
          else
            do (error "Found source path of ~a which is not relative to code root in system ~a."
                      file source))
    (loop for name in systems
          for system = (asdf:find-system name)
          do (pushnew (list* name
                             (loop for (func key) in +asdf-system-initargs+
                                   for value = (funcall func system)
                                   when value
                                     collect key and
                                     collect value))
                      (gethash target (target-systems configuration))
                      :key #'car))))

(defmethod add-target-source (configuration target (source (eql :extension-systems))
                              &aux (systems (extension-systems configuration)))
  (when systems
    (loop for system in (append '(:cffi-toolchain :cffi-grovel :cffi)
                                systems)
          do (add-target-source configuration target system))))

;; Sources that are added to iclasp also need to be installed and scanned for tags.
(defmethod add-target-source :after (configuration (target (eql :iclasp)) (source source))
  (when (eq :code (source-root source))
    (add-target-source configuration :install-code source)
    (add-target-source configuration :etags source))
  (add-target-source configuration :sclasp source))

;; Sources that are added to aclasp also need to be installed and scanned for tags.
(defmethod add-target-source :after (configuration (target (eql :aclasp)) (source source))
  (when (eq :code (source-root source))
    (add-target-source configuration :install-code source)
    (add-target-source configuration :etags source)))

;; Sources that are added to bclasp also need to be installed and scanned for tags.
(defmethod add-target-source :after (configuration (target (eql :bclasp)) (source source))
  (when (eq :code (source-root source))
    (add-target-source configuration :install-code source)
    (add-target-source configuration :etags source)))

;; Sources that are added to cclasp also need to be installed and scanned for tags.
(defmethod add-target-source :after (configuration (target (eql :cclasp)) (source source))
  (when (eq :code (source-root source))
    (add-target-source configuration :install-code source)
    (add-target-source configuration :etags source)))

;; For directories that are to be installed skip hidden files and waf files.
(defun add-target-directory (configuration target source)
  (loop with root = (merge-pathnames (resolve-source-root source)
                                     (uiop:getcwd))
        for path in (directory (merge-pathnames #P"**/*.*"
                                                (merge-pathnames (resolve-source source)
                                                                 (uiop:getcwd))))
        for rel-path = (uiop:subpathp (truename path) root)
        unless (or (uiop:absolute-pathname-p rel-path)
                   (uiop:directory-pathname-p rel-path)
                   (equal "wscript" (file-namestring rel-path))
                   (hidden-component-p (pathname-name rel-path))
                   (some #'hidden-component-p (pathname-directory rel-path)))
          do (add-target-source configuration target (make-source rel-path (source-root source)))))

(defmethod add-target-source (configuration (target (eql :install-code)) (source directory-source))
  (add-target-directory configuration target source))

(defmethod add-target-source (configuration (target (eql :install-extension-code)) (source directory-source))
  (add-target-directory configuration target source))

(defmethod add-target-source (configuration (target (eql :etags)) (source directory-source))
  (add-target-directory configuration target source))


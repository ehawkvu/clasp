#include <clasp/core/foundation.h>
#include <clasp/core/object.h>
#include <clasp/core/lisp.h>
#include <clasp/core/ql.h>
#include <clasp/core/array.h>
#include <clasp/core/pointer.h>
#include <clasp/core/multipleValues.h>
#include <clasp/core/debugger2.h>
#include <stdlib.h> // calloc, realloc, free
#include <execinfo.h> // backtrace

namespace core {

CL_DEFUN T_mv core__call_with_operating_system_backtrace(Function_sp function) {
  // Get an operating system backtrace, i.e. with the backtrace and
  // backtrace_symbols functions (which are not POSIX, but are present in both
  // GNU and Apple systems).
  // backtrace requires a number of frames to get, and will fill only that many
  // entries. To get the full backtrace, we repeatedly try larger frame numbers
  // until backtrace finally doesn't fill everything in.
#define START_BACKTRACE_SIZE 512
#define MAX_BACKTRACE_SIZE_LOG2 20
  size_t num = START_BACKTRACE_SIZE;
  void** buffer = (void**)calloc(sizeof(void*), num);
  for (size_t attempt = 0; attempt < MAX_BACKTRACE_SIZE_LOG2; ++attempt) {
    size_t returned = backtrace(buffer,num);
    if (returned < num) {
      char **strings = backtrace_symbols(buffer, returned);
      ql::list pointers;
      ql::list names;
      ql::list basepointers;
      void* bp = __builtin_frame_address(0);
      for (size_t j = 0; j < returned; ++j) {
        pointers << Pointer_O::create(buffer[j]);
        names << SimpleBaseString_O::make(strings[j]);
        basepointers << Pointer_O::create(bp);
        if (bp) bp = *(void**)bp;
      }
      free(buffer);
      free(strings);
      return eval::funcall(function, pointers.cons(), names.cons(),
                           basepointers.cons());
    }
    // realloc_array would be nice, but macs don't have it
    num *= 2;
    buffer = (void**)realloc(buffer, sizeof(void*)*num);
  }
  printf("%s:%d Couldn't get backtrace\n", __FILE__, __LINE__ );
  abort();
}

CL_DEFUN T_sp core__debugger_local_fname(DebuggerLocal_sp dl) {
  return dl->fname;
}
CL_DEFUN T_sp core__debugger_local_name(DebuggerLocal_sp dl) {
  return dl->name;
}
CL_DEFUN T_sp core__debugger_local_declfile(DebuggerLocal_sp dl) {
  return dl->declfile;
}
CL_DEFUN T_sp core__debugger_local_declline(DebuggerLocal_sp dl) {
  return dl->declline;
}

}; // namespace core

#include <stdlib.h>
//
// return the PC of the callsite to the dynamic memory management function
//
//   fname      pointer to character array to hold function name
//   fnlen      length of character array
//   ofs        pointer to offset to hold PC offset into function
//
// returns
//    0         on success
//   <0         on error
//

#include <libunwind.h>


int get_callinfo(char *fname, size_t fnlen, unsigned long long *ofs)
{
  unw_cursor_t cursor;
  unw_word_t ip, off;
  int r;
  unw_context_t context;
  unw_getcontext(&context);
 // unw_init_local(&cursor, &context);
 // r = unw_get_proc_name(&cursor, fname, fnlen, (unw_word_t *) ofs);
  return -1;
}

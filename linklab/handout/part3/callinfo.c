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

#define UNW_LOCAL_ONLY
#include <libunwind.h>
#include<string.h>

#include <errno.h>
#include <stdio.h>
int get_callinfo(char *fname, size_t fnlen, unsigned long long *ofs)
{
  unw_cursor_t cursor;
  unw_word_t ip, off, sp;
  int ret = -1;
  unw_context_t context;
  unw_getcontext(&context);
  unw_init_local(&cursor, &context);

 char procname[256];
 while (unw_step(&cursor) > 0) {

        long long unsigned int pri = *ofs;
        ret = unw_get_proc_name(&cursor, procname, 256, (unw_word_t *) ofs);

        if (ret && ret != -UNW_ENOMEM) {
            procname[0] = '?';
            procname[1] = 0;
        }
         unw_get_reg(&cursor, UNW_REG_IP, &ip);
        unw_get_reg(&cursor, UNW_REG_SP, &sp);
        //fprintf(stderr, "ip = 0x%lx (%s), sp = 0x%lx\n", (long)ip, procname, (long)sp);
        
        if(strcmp(procname, "main") == 0)
        {
            strcpy(fname, procname);
            *ofs -= 5;
           // *ofs = *ofs - (long long unsigned int)pri;
            return 0;
        }
  }
  return -1;
}

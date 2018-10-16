#ifndef __LOG__
#define __LOG__

#define ERROR_X  1
#define INFO_X   1
#define DEBUG_X  0

#define log_dbg(fmt, args...) {    \
     if(DEBUG_X){              \
         printf(fmt, ##args); \
     }                        \
   }

#define log_info(fmt, args...){ \
     if(INFO_X){ \
          printf(fmt, ##args);\
      }\
    }

#define log_err(fmt, args...){\
     if(ERROR_X){\
         printf(fmt, ##args);\
     }\
   }

#endif

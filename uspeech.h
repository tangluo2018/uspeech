/*
*  uspeech.h
*
*  uros speech recognize head file
*/

#include "audio_queue.h"

#define TEXT_BUFFER_SIZE 4096
#define MAX_USPEECH_AUDIO_SIZE (1024 * 1024)

#define LOCAL_SPEECH
#define FACTORY_TEST

/*uspeech return code*/
enum {
   USPEECH_SUCCESS                  = 0,  
   USPEECH_FAILE                    = -1,

   USPEECH_ERR_MALLOC_FAILE         = 10,
   USPEECH_ERR_PARAM_NULL           = 11,
   USPEECH_ERR_DATA_NULL            = 12,
   USPEECH_ERR_OPEN_FAILE           = 13
};

enum {
   ENGINE_TYPE_IFLYTEK     = 0,
   ENGINE_TYPE_MIX         = 1
};

enum {
	USPEECH_STATUS_BEGIN   = 0,
	USPEECH_STATUS_GOING   = 1,
	USPEECH_STATUS_END     = 2
};

/*speech recognize state struct*/
typedef struct speech_state_t {
   int             engine_type;
   void *          engine_data;
   int             status;
   int             connected;
   audio_queue     inqueue;
   unsigned int    text_size;
   char            text[TEXT_BUFFER_SIZE];
   audio_queue     outqueue;
   void            (*interrupt)(void *speech);
}speech_state;

int speech_init(int engine_type, void* engine_data);

int speech_audio_put(const char* audio_data, const unsigned int audio_len);

int speech_get_angle(void *);

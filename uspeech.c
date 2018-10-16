/*
* uspeech.c
* uros speech regcognize kit
*
* 2017-05-12 
* minhua huang
*
*/
#include <stdio.h>
#include <string.h>
#include "uspeech.h"
#include "iflytek.h"
#include "msp_errors.h"
#include "msp_cmn.h"

#define TRUE (1)
#define FALSE (0)

static speech_state* speech;
#define	BUFFER_SIZE	4096
static char *g_result = NULL;
static unsigned int g_buffersize = BUFFER_SIZE;

static int cae_thread_proc(void * arg)
{
    int err;
    ifly_t *ifly_data = (ifly_t *)arg;
    audio_buffer buf;
    
    while(1){
      err = audio_queue_get(&speech->inqueue, &buf);
      if(err != USPEECH_SUCCESS){
        return err;
      }
      if_cae_write(ifly_data->cae_handle, buf.data, buf.size);
      audio_data_free(&buf);
    }
    return 0;
}

static void show_result(char *string, char is_over)
{
	printf("\rResult: %s ", string);
	if(is_over)
		putchar('\n');
}

void on_result(const char* result, char is_last)
{
	if(result) {
	  unsigned int left = g_buffersize - 1 - strlen(g_result);
	  unsigned int size = strlen(result);
	  if(left < size) {
		g_result = (char*)realloc(g_result, g_buffersize + BUFFER_SIZE);
		if(g_result)
		  g_buffersize += BUFFER_SIZE;
		else {
		  printf("text result buffer  realloc failed\n");
		  return;
		}
	  }
	  strncat(g_result, result, size);
	  show_result(g_result, is_last);
	}
}

void on_speech_begin()
{
	if (g_result)
	{
		free(g_result);
	}
	g_result = (char*)malloc(BUFFER_SIZE);
	g_buffersize = BUFFER_SIZE;
	memset(g_result, 0, g_buffersize);
}

void on_speech_end(void *data, int reason)
{
	int err = 0;
	const char text_buf[4096];
	int text_size = 0;
	speech_state *speech = (speech_state *)data;
	speech->status = USPEECH_STATUS_END;
	ifly_t *ifly_data = (ifly_t *)speech->engine_data;

	if(speech->engine_type == ENGINE_TYPE_IFLYTEK && g_result != NULL){
		memset(text_buf, 0, 4096);
		err = if_json_parse(g_result, text_buf);
		if(err){
			printf("iflytek json text parse failed, err %d\n", err);
			return;
		}else{
		    text_size = strlen(text_buf);
		    speech->text_size = text_size;
		    strcpy(speech->text, text_buf);
		    show_result(speech->text, 1);
		}
        if(!ifly_data->tts_session_id){
        	err = if_tts_init(ifly_data, speech->connected);
        	if(err){
        		return;
        	}
        }
        /*if last speech still going on, interrupt it*/
        if(speech->interrupt)
        	speech->interrupt(speech);
        err = if_tts_write(speech, speech->text, speech->text_size);
        if(err){
        	return;
        }
	}
}

void speech_interrupt(void *user_data)
{
    speech_state *speech = (speech_state *)user_data;
    audio_queue_flush(&speech->outqueue);
}

static int iflytek_init(ifly_t * ifly_data)
{
    int err;
    pthread_t cae_tid;
    params_t * params = ifly_data->params;
    
    if(params == NULL || params->login_param == NULL){
       return USPEECH_ERR_PARAM_NULL;
    }
    err = MSPLogin(NULL, NULL, params->login_param);
    if(err != MSP_SUCCESS){
      return err;
    }

    ifly_notifier_t notifier_t = {
    		on_result,
    		on_speech_begin,
    		on_speech_end
    };
    ifly_data->notifier = notifier_t;

    err = audio_queue_init(&speech->inqueue);
    if(err){
    	return err;
    }
    err = audio_queue_init(&speech->outqueue);
    if(err){
    	return err;
    }
    /*init cae*/
    err = if_cae_init(speech);
    if(err != MSP_SUCCESS){
      return err;
    }

    /*build local grammar*/
#ifdef LOCAL_SPEECH
    err = if_build_grammar(params);
    if(err != MSP_SUCCESS){
    	return err;
    }
#endif

    err = pthread_create(&cae_tid, NULL, cae_thread_proc, (void*)ifly_data);
    if(err){
      return err;
    }

    return err;
}

void set_network_state(int state)
{
	speech->connected = state;
}

static int get_network_state()
{
	return TRUE;
}

int speech_init(int type, void* engine_data)
{
    int err = 0;
    ifly_t *ifly_data;

    speech = (speech_state *)malloc(sizeof(speech_state));
    if(speech == NULL){
      return USPEECH_ERR_MALLOC_FAILE;
    }

    speech->engine_type = type;
    speech->engine_data = engine_data;
    speech->interrupt = speech_interrupt;
    //speech->connected = get_network_state();
    switch(speech->engine_type){
      case ENGINE_TYPE_IFLYTEK:
    	   ifly_data = (ifly_t *)engine_data;
    	   ifly_data->qisr_session_id = NULL;
    	   ifly_data->tts_session_id = NULL;
    	   speech->connected = ifly_data->connected;
           err = iflytek_init(ifly_data);
           break;
      case ENGINE_TYPE_MIX:
           break;

      default:
           break;
    }

    return err;
}

int speech_audio_put(const char* audio_data, const unsigned int audio_len)
{
   int err;
   unsigned int size;

   if(audio_data == NULL || audio_len == 0){
      return USPEECH_ERR_DATA_NULL;
   }

   if(audio_queue_size(&speech->inqueue) >= MAX_USPEECH_AUDIO_SIZE ){
	   return 0;
   }

   if(speech->engine_type == ENGINE_TYPE_IFLYTEK){
       err = audio_queue_put(&speech->inqueue, audio_data, audio_len);
       if(err){
         return err;
       }
   }
   //else if(){}
   return 0;
}

int speech_audio_get(char** audio_data, unsigned int* size)
{
	int err;
	audio_buffer buf;

	err = audio_queue_get(&speech->outqueue, &buf);
	if(err){
		return err;
	}
	*audio_data = buf.data;
	*size = buf.size;
	return 0;
}

void speech_audio_free(void * audio_data)
{
	if(audio_data != NULL){
		free(audio_data);
		audio_data = NULL;
	}
}


/*
 *  isrd.c
 *  iflytek speech recongnize daemon
 *
 *  Create on: 2017.02.25
 *     Author: minhua huang
 */
#include <log/log.h>
#include <tinyalsa/asoundlib.h>
#include <pthread.h>
#include "config.h"
#include "uspeech.h"
#include "iflytek.h"

#ifdef FILE_OUTPUT_TEST
static FILE *audio_file = NULL;
#endif

static void set_volume(void *data, int size, int ratio)
{
     short sample;
     short *snd_data;
     int i;

     snd_data = (short *)data;
     for(i=0; i<size/2; i++){
       snd_data[i] = snd_data[i] >> ratio;
     }
}

void* audio_playback(void *arg)
{
   int ret;
   struct pcm_config config;
   struct pcm* pcm_snd;
   char * audio_data;
   unsigned int size;

   config.channels = 2;
   config.rate = 8000;//PLAYBACK_SAMPLERATE;
   config.format = PCM_FORMAT_S16_LE;
   config.start_threshold = 0;
   config.stop_threshold = 0;
   config.silence_threshold = 0;
   config.period_size = 1536;
   config.period_count = 8;

   pcm_snd = pcm_open(ALSA_PLAYBACK_CARD, ALSA_PLAYBACK_DEVICE, PCM_OUT, &config);
   if(!pcm_snd || !pcm_is_ready(pcm_snd)){
      ALOGE("Unable to open playback PCM device (%s)\n", pcm_get_error(pcm_snd));
      return NULL;
   }

   for(;;){
	   ret = speech_audio_get(&audio_data, &size);
       if(ret){
          break;
       }

       printf("speech_audio_get, size %d\n", size);
       if(audio_data != NULL){
           #ifdef FILE_OUTPUT_TEST
           fwrite(audio_data, size, 1, audio_file);
           #endif
           set_volume(audio_data, size, 5);
    	   if(pcm_write(pcm_snd, audio_data, size)){
    	       ALOGE("Error playing sample\n");
    	       break;
    	   }
    	   speech_audio_free(audio_data);
       }
   }
   return NULL;
}

void* record_thread(void *arg)
{
   struct pcm_config config;
   int ret;
   int buf_size = 1024;
   void *audio_data;
   struct pcm* pcm_snd;

   config.channels = 2;
   config.rate = RECORD_SAMPLERATE;
   config.format = PCM_FORMAT_S32_LE;
   config.start_threshold = 0;
   config.stop_threshold = 0;
   config.silence_threshold = 0;
   config.period_size = 1024;
   config.period_count = 4;

   pcm_snd = pcm_open(ALSA_RECORD_CARD, ALSA_RECORD_DEVICE, PCM_IN, &config);
   if(!pcm_snd || !pcm_is_ready(pcm_snd)){
     ALOGE("Unable to open record PCM device (%s)\n", pcm_get_error(pcm_snd));
     return -1;
   }
   
   buf_size = pcm_frames_to_bytes(pcm_snd, pcm_get_buffer_size(pcm_snd));
   ALOGI("Speech buffer size %d\n", buf_size);
   audio_data = malloc(buf_size);
   if(!audio_data){
	   ALOGE("Unable to malloc audio_data\n");
	   return -1;
   }

   while(1){
	 ret = pcm_read(pcm_snd, audio_data, buf_size);
	 if(ret){
	    ALOGE("Pcm read failed, ret %d\n", ret);
	    break;
	 }
	 ret = speech_audio_put(audio_data, buf_size);
	 if(ret){
	    ALOGE("speech_audio_put faile, err %d", ret);
	    break;
	 }
   }
   pcm_close(pcm_snd);
   return ret;
}

void do_exit(speech_state *speech)
{
   /*close recording pcm*/
	if(speech != NULL){
		free(speech);
	}
}

int main(int argc, char* argv[])
{
   int ret;
   speech_state *speech;
   const char *resPath;    
   int engine_type;
   ifly_t ifly_data;
   pthread_t record_tid;
   pthread_t playback_tid;
   int connected;

   #ifdef FILE_OUTPUT_TEST
   audio_file = fopen("/sdcard/playback.pcm", "wb+");
   if(!audio_file){
      printf("Could not open destination file /sdcard/playback.pcm\n");
      return -1;
   }
   #endif

   if(argc >= 2){
	   connected = atoi(argv[1]);
   }else{
	   connected = 0;
   }

   /*init speech regcognize engine*/
   engine_type = ENGINE_TYPE_IFLYTEK;
   ifly_data.params = (params_t*)malloc(sizeof(params_t));
   if(!ifly_data.params){
	   return USPEECH_ERR_MALLOC_FAILE;
   }

   ifly_data.params->login_param = MSP_LOGIN_PARAMS;
   ifly_data.params->iat_session_param = SRT_SESSION_PARAMS;
   ifly_data.params->jet_path = JET_RESOURCE_PATH;
   ifly_data.params->tts_session_param = TTS_SESSION_PARAMS;
   ifly_data.params->asr_build_param = ASR_BUILD_PARAMS;
   ifly_data.params->asr_grammar_file = ASR_GRAMMAR_FILE;
   ifly_data.params->asr_session_param = ASR_SESSION_PARAMS;
   ifly_data.params->tts_local_param = TTS_LOCAL_PARAMS;
   ifly_data.connected = connected;
   ret = speech_init(engine_type, (void *)&ifly_data);
   if(ret){
	   ALOGI("speech_init failed, err %d\n", ret);
	   return ret;
   }

   /*Open sound card to record speech*/
   ret = pthread_create(&record_tid, NULL, record_thread, NULL);
   if(ret){
      ALOGE("Create record thread failed\n");
      ret = -1;
      goto isrd_exit;
   }

   ret = pthread_create(&playback_tid, NULL, audio_playback, NULL);
   if(ret){
	   ALOGE("Create playback thread failed\n");
	   ret = -1;
	   goto isrd_exit;
   }

   //audio_playback(NULL);

   pthread_join(record_tid, NULL);
   pthread_join(playback_tid, NULL);

isrd_exit:
   do_exit(speech);
   return ret;
}

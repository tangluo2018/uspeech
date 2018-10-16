/*
*  iflytek.c
*
*  iflytek speech regcognice engine api
*
*
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cae.h"
#include "uspeech.h"
#include "iflytek.h"
#include "msp_errors.h"
#include "msp_types.h"
#include "qisr.h"
#include "cJSON.h"
#include "log.h"

//#define FILE_OUTPUT_TEST

#ifdef FILE_OUTPUT_TEST
static FILE *audio_file = NULL;
#endif

#define MAX_GRAMMARID_LEN  (32)
#define MAX_PARAMS_LEN     (1024)

typedef struct grammar_data_s {
	int     is_built;
	int     is_updated;
	int     errcode;
	char    grammar_id[MAX_GRAMMARID_LEN];
}grammar_data_t;

static cae_ivw ivw;

static int build_callback(int ecode, const char *info, void *udata)
{
	grammar_data_t *grammar_data = (grammar_data_t *)udata;
	if(grammar_data != NULL){
		grammar_data->is_updated = 1;
		grammar_data->errcode = ecode;
	}
	if(ecode == MSP_SUCCESS && info != NULL){
		log_info("Build grammar successfully, grammar id: %s\n", info);
		if(grammar_data != NULL){
			snprintf(grammar_data->grammar_id, MAX_GRAMMARID_LEN - 1, info);
		}
	}else{
	    log_err("Build grammar failed, errcode %d\n", ecode);
	}

	return 0;
}

int if_build_grammar(params_t *params)
{
	int err;
	FILE* grm_file;
	char* grm_content;
	unsigned int grm_cnt_len;
	char* grm_build_params;
	grammar_data_t grm_data;
	char* filename = params->asr_grammar_file;
	char* build_params = params->asr_build_param;

	grm_file = fopen(filename, "rb");
	if(!grm_file){
		log_err("Unable to open grammar file %s\n", filename);
		return USPEECH_ERR_OPEN_FAILE;
	}
	fseek(grm_file, 0, SEEK_END);
	grm_cnt_len = ftell(grm_file);
	fseek(grm_file, 0, SEEK_SET);

	grm_content = (char*)malloc(grm_cnt_len +1);
	if(!grm_content){
		log_err("Malloc grammar content failed\n");
		fclose(grm_file);
		return USPEECH_ERR_MALLOC_FAILE;
	}
	fread(grm_content, 1, grm_cnt_len, grm_file);
	grm_content[grm_cnt_len] = '\0';
	fclose(grm_file);

    grm_build_params = build_params;
	err = QISRBuildGrammar("bnf", grm_content, grm_cnt_len, grm_build_params, build_callback, (void*)&grm_data);
    if(err != MSP_SUCCESS){
    	log_err("QISBuildGrammar failed\n");
    }
    free(grm_content);
    grm_content = NULL;
    params->local_grammar_id = grm_data.grammar_id;
    return err;
}

int if_tts_init(ifly_t *ifly_data, int connected)
{
	int err;
    params_t *params = ifly_data->params;
    if(connected){
    	ifly_data->tts_session_id = QTTSSessionBegin(params->tts_session_param, &err);
    }else{
    	ifly_data->tts_session_id = QTTSSessionBegin(params->tts_local_param, &err);
    }

	if(MSP_SUCCESS != err){
	    printf("QISRSessionBegin failed, error %d\n", err);
	    return err;
	}
	return USPEECH_SUCCESS;
}

int if_tts_write(speech_state *speech, const char* text, unsigned int txt_len) {
	int err = MSP_SUCCESS;
	unsigned int audio_len;
	ifly_t *ifly_data = (ifly_t *)speech->engine_data;

	if (!text) {
		return USPEECH_ERR_DATA_NULL;
	}
	if(!ifly_data->tts_session_id){
		return MSP_ERROR_INVALID_HANDLE;
	}

	//log_info("if_tts_write,text size %d, text %s\n", txt_len, text);

	err = QTTSTextPut(ifly_data->tts_session_id, text, txt_len, NULL);
	if (MSP_SUCCESS != err) {
		printf("QTTSTextPut failed, error %d\n", err);
		goto tts_exit;
	}
	while (1) {
		/*Get speech*/
		const void *data = QTTSAudioGet(ifly_data->tts_session_id, &audio_len, &ifly_data->synth_stat,
				&err);
		if (MSP_SUCCESS != err) {
			printf("QTTSAudioGet failed, error %d\n", err);
			break;
		}
		if (NULL != data && audio_len > 0) {
			/*We got audio data and queue them*/
			//printf("before write audio, audio_len %d\n", audio_len);
            #ifdef FILE_OUTPUT_TEST
            fwrite(data, audio_len, 1, audio_file);
            #endif
			audio_queue_put(&speech->outqueue, data, audio_len);
		}
		if (MSP_TTS_FLAG_DATA_END == ifly_data->synth_stat)
			break;

		usleep(200 * 1000);
	}

tts_exit:
	QTTSSessionEnd(ifly_data->tts_session_id, "normal end");
	ifly_data->tts_session_id = NULL;
	return err;
}

static int if_iat_end(speech_state *speech)
{
	int err;
	const char *rslt;

	ifly_t *ifly_data = (ifly_t *)speech->engine_data;

	err = QISRAudioWrite(ifly_data->qisr_session_id, NULL, 0, MSP_AUDIO_SAMPLE_LAST, &ifly_data->ep_stat, &ifly_data->rec_stat);
	if(err){
		QISRSessionEnd(ifly_data->qisr_session_id, "write err");
		return err;
	}

	while(ifly_data->rec_stat != MSP_REC_STATUS_COMPLETE ){
	    rslt = QISRGetResult(ifly_data->qisr_session_id, &ifly_data->rec_stat, 0, &err);
		if(rslt && ifly_data->notifier.on_result)
			ifly_data->notifier.on_result(rslt, ifly_data->rec_stat == MSP_REC_STATUS_COMPLETE ? 1 : 0);

			usleep(200*1000); /* for cpu occupy, should sleep here */
	}
	if(ifly_data->qisr_session_id){
		QISRSessionEnd(ifly_data->qisr_session_id, "VAD Normal");
		ifly_data->qisr_session_id = NULL;
	}
	if(ifly_data->notifier.on_speech_end){
		ifly_data->notifier.on_speech_end((void *)speech, 0);
	}
	printf("==Once speech recognized end\n");
	return err;
}

static int if_iat_write(speech_state *speech, char *audio_data, unsigned int audio_len)
{
   const char *results = NULL;
   int err = 0;
   ifly_t * ifly_data = (ifly_t *)speech->engine_data;
   
   if(!audio_data || !audio_len){
     return USPEECH_ERR_DATA_NULL; 
   }

   err = QISRAudioWrite(ifly_data->qisr_session_id, audio_data, audio_len, ifly_data->audio_stat, &ifly_data->ep_stat, &ifly_data->rec_stat);
   if (err) {
      return err;
   }
   ifly_data->audio_stat = MSP_AUDIO_SAMPLE_CONTINUE;

   if (MSP_REC_STATUS_SUCCESS == ifly_data->rec_stat) {
	   results = QISRGetResult(ifly_data->qisr_session_id, &ifly_data->rec_stat, 0, &err);
		if (MSP_SUCCESS != err)	{
			return err;
		}
		if (NULL != results && ifly_data->notifier.on_result){
			ifly_data->notifier.on_result(results, ifly_data->rec_stat == MSP_REC_STATUS_COMPLETE ? 1 : 0);
		}
	}

	if (MSP_EP_AFTER_SPEECH == ifly_data->ep_stat){
		if_iat_end(speech);
	}

	return 0;
}

int if_iat_init(speech_state *speech)
{
	int errcode;
	ifly_t * ifly_data = (ifly_t *)speech->engine_data;
	params_t *params = ifly_data->params;
	ifly_data->qisr_session_id = QISRSessionBegin(NULL, params->iat_session_param, &errcode);
	if(errcode != MSP_SUCCESS){
		return errcode;
	}
	ifly_data->ep_stat = MSP_EP_LOOKING_FOR_SPEECH;
	ifly_data->rec_stat = MSP_REC_STATUS_SUCCESS;
	ifly_data->audio_stat = MSP_AUDIO_SAMPLE_FIRST;
	if(ifly_data->notifier.on_speech_begin){
		ifly_data->notifier.on_speech_begin();
	}
	speech->status = USPEECH_STATUS_BEGIN;
	return 0;
}

int if_speech_init(speech_state *speech, int connected)
{
	int errcode;
	char asr_params[MAX_PARAMS_LEN] = {NULL};
	ifly_t * ifly_data = (ifly_t *)speech->engine_data;
	params_t *params = ifly_data->params;
	if(connected){
	    ifly_data->qisr_session_id = QISRSessionBegin(NULL, params->iat_session_param, &errcode);
	}else{
		snprintf(asr_params, MAX_PARAMS_LEN -1, "%s, local_grammar = %s", params->asr_session_param, params->local_grammar_id);
		ifly_data->qisr_session_id = QISRSessionBegin(NULL, asr_params, &errcode);
	}
	if(errcode != MSP_SUCCESS){
		return errcode;
	}
	ifly_data->ep_stat = MSP_EP_LOOKING_FOR_SPEECH;
	ifly_data->rec_stat = MSP_REC_STATUS_SUCCESS;
	ifly_data->audio_stat = MSP_AUDIO_SAMPLE_FIRST;
	if(ifly_data->notifier.on_speech_begin){
		ifly_data->notifier.on_speech_begin();
	}
	speech->status = USPEECH_STATUS_BEGIN;
	return 0;

}

/* wakeup callback */
void ivw_callback(short angle, short channel, float power, short CMScore, short beam, void *user_data)
{
   printf("CAEIvwCb, angle: %d\n", angle);

#ifdef FACTORY_TEST
   ivw.angle = angle;
   ivw.channel = channel;
   ivw.power = power;
   ivw.CMScore = CMScore;
   ivw.beam = beam;
   if(ivw_user_callback){
       ivw_user_callback(&ivw);
   }
#endif

   speech_state *speech;
   speech = (speech_state*)user_data;

   //cae_set_real_beam(speech->gCAEHandle, beam);

   /*interrupt speech*/
   if(speech->interrupt){
	   speech->interrupt(speech);
   }
}

/*audio data processed callbak*/
void audio_callback(const void *audio_data, unsigned int audio_len, int param1, const void *param2, void *user_data)
{
   int err;
   speech_state *speech;
   ifly_t *ifly_data;

   speech = (speech_state*)user_data;
   ifly_data = (ifly_t *)speech->engine_data;

	if (!ifly_data->qisr_session_id) {
		err = if_speech_init(speech, speech->connected);
		if (err) {
			printf("iflytek speech init failed, err %d\n", err);
			return;
		}
	}

#ifdef FACTORY_TEST
	/*NOT IAT WRITE*/
	return;
#endif

	//printf("CAEAudioCb queue 16k audio data, len %d\n", audio_len);
	/*send audio data to iflytek to regcognize*/
	err = if_iat_write((speech_state *) user_data, audio_data, audio_len);
	if (err) {
		//log_err("iflytek audio iat writed failed, err %d\n", err);
		speech->status = USPEECH_STATUS_END;
		if(ifly_data->qisr_session_id){
			QISRSessionEnd(ifly_data->qisr_session_id, "Inormal");
			ifly_data->qisr_session_id = NULL;
		}
	}
}

/*iflytec CAE*/
int if_cae_init(speech_state *speech)
{
   int err;
   char *resPath;
   ifly_t *ifly_data = (ifly_t *)speech->engine_data;
   resPath = ifly_data->params->jet_path;
   err = CAENew(&ifly_data->cae_handle, resPath, ivw_callback, audio_callback, NULL, speech);
   if(err != MSP_SUCCESS){
      return err;
   }

   CAESetRealBeam(ifly_data->cae_handle, 0);

   #ifdef FILE_OUTPUT_TEST
   audio_file = fopen("/sdcard/tts.pcm", "wb+");
   if(!audio_file){
      printf("Could not open destination file /sdcard/tts.pcm\n");
      return -1;
   }
   #endif

   return 0;
}

int if_cae_write(CAE_HANDLE handle, const char* audio_data, const unsigned int audio_len)
{
   int err;
   if(handle != NULL){
      err = CAEAudioWrite(handle, audio_data, audio_len);
      if(err != MSP_SUCCESS){
        return err;
      }
   }else{
      return USPEECH_FAILE;
   }

   return 0;
}

int if_json_parse(const char* src, char* dst)
{
	int err;
    ifly_text_t ifly_text;
    cJSON* root, *answer, data;
    int rc;
    cJSON* ws_data, *ws;
    int i = 0, j = 0;
    int ws_count;
    cJSON* cw_data, *cw, *w;
    int cw_count;
    char *string;
    unsigned int len;

	if(src == NULL || strlen(src) == 0){
		err = USPEECH_ERR_DATA_NULL;
		return err;
	}
	memset(ifly_text.text_answer.text, 0 , MAX_TEXT_BUFFER_SIZE);
    root = cJSON_Parse(src);
    if(cJSON_GetObjectItem(root, "rc") != NULL){
        rc = cJSON_GetObjectItem(root, "rc")->valueint;
        if(rc != 0){
            	return USPEECH_ERR_DATA_NULL;
        }

        cJSON* operation = cJSON_GetObjectItem(root, "operation");
        ifly_text.operation = operation->valuestring;
        cJSON* service = cJSON_GetObjectItem(root, "service");
        ifly_text.service = service->valuestring;

        if(strcmp(ifly_text.service, "openQA") == 0 ||
        		strcmp(ifly_text.service, "calc") == 0 ||
        		strcmp(ifly_text.service, "datetime") == 0 ||
        		strcmp(ifly_text.service, "baike") == 0 ||
        		strcmp(ifly_text.service, "faq") == 0 ||
        		strcmp(ifly_text.service, "chat") == 0){
            answer = cJSON_GetObjectItem(root, "answer");
            //ifly_text.text_answer.text = cJSON_GetObjectItem(answer, "text")->valuestring;
            ifly_text.text_answer.type = cJSON_GetObjectItem(answer, "type")->valuestring;
            strcpy(dst, cJSON_GetObjectItem(answer, "text")->valuestring);
        }
    }

    if((ws_data = cJSON_GetObjectItem(root, "ws")) != NULL){
    	ws_count = cJSON_GetArraySize(ws_data);
    	for(i=0; i< ws_count; i++){
    		ws = cJSON_GetArrayItem(ws_data, i);
    		cw_data = cJSON_GetObjectItem(ws, "cw");
    		cw_count = cJSON_GetArraySize(cw_data);
    		for(j=0; j<cw_count; j++){
    			cw = cJSON_GetArrayItem(cw_data, j);
    			w = cJSON_GetObjectItem(cw, "w");
    			string = w->valuestring;
    			ifly_text.text_local.local_ws[i].local_cw[j].w = string;
    			strncat(ifly_text.text_answer.text, string, strlen(string));
    		}
    	}
    	len = strlen(ifly_text.text_answer.text);
    	printf("string len: %d\n", len);
        strcpy(dst, ifly_text.text_answer.text);
        dst[len + 1] = '\0';
    }

    cJSON_Delete(root);

    return 0;
}

int iflytek_get_ivw(user_callback user_angle_callback)
{
	//printf("iflytek_get_ivw: user_angle_callback %x\n", user_angle_callback);
	ivw_user_callback = user_angle_callback;
	return 0;
}

/*
*  iflytek.h
*/

#define MAX_LOCAL_CW_COUNT (32)
#define MAX_LOCAL_WS_COUNT (32)
#define MAX_TEXT_BUFFER_SIZE  (4096)

typedef struct params_s {
   char* login_param;
   char* iat_session_param;
   char* asr_session_param;
   char* asr_build_param;
   char* asr_grammar_file;
   char* local_grammar_id;
   char* tts_session_param;
   char* tts_local_param;
   char* jet_path;
}params_t;

typedef struct iflytek_notifier_s{
	void (*on_result)(const char *result, char is_last);
	void (*on_speech_begin)();                           /*void (*on_speech_begin)(char** txt, unsigned int txt_bufsize)*/
	void (*on_speech_end)(void* speech, int reason);	/* 0 if VAD.  others, error : see E_SR_xxx and msp_errors.h  */
}ifly_notifier_t;

typedef struct local_cw_s{
	int    sc;
	int    id;
	char   w;
	int    gm;
}local_cw_t;

typedef struct local_ws_s{
    int           bg;
    char*         slot;
    local_cw_t    local_cw[MAX_LOCAL_CW_COUNT];
}local_ws_t;

typedef struct text_local_s{
	int            sn;
	int            ls;
	int            bg;
	int            ed;
    local_ws_t     local_ws[MAX_LOCAL_WS_COUNT];
	int sc;
}text_local_t;

typedef struct text_data_s{

}text_data_t;

typedef struct text_answer_s{
	char*    type;
	char     text[MAX_TEXT_BUFFER_SIZE];
}text_answer_t;

/*iflytek semantic struct of json text data*/
typedef struct iflytek_text_s{
	int               rc;
	char*             service;
	char*             operation;
    text_data_t       text_data;
    text_answer_t     text_answer;

    text_local_t       text_local;
}ifly_text_t;

/*iflytek data struct*/
typedef struct iflytek_s {
   params_t*          params;
   void *             cae_handle;
   char*              qisr_session_id;
   int                audio_stat;
   int                ep_stat;
   int                rec_stat;
   ifly_notifier_t    notifier;
   char*              tts_session_id;
   int                synth_stat;
   int                connected;
}ifly_t;

typedef struct cae_ivw_s {
	short angle;
	short channel;
	float power;
	short CMScore;
	short beam;
}cae_ivw;

int if_cae_init(speech_state *speech);

int if_cae_write(void* handle, const char* audio_data, const unsigned int audio_len);

int if_tts_init(ifly_t *ifly_data, int connected);

int if_tts_write(speech_state *speech, const char* text, unsigned int txt_len);

int if_json_parse(const char* src, char* dst);

typedef int (*user_callback)(cae_ivw *ivw);
user_callback ivw_user_callback;

int iflytek_get_ivw(user_callback user_angle_callback);

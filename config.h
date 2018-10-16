#ifndef CONFIG_H
#define CONFIG_H

#include <stdio.h>

#define LOG_TAG "IFLYTEK"

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

//#define FILE_OUTPUT_TEST

#define CAE_LIBRARY_PATH "/system/lib/libcae.so"

#define JET_RESOURCE_PATH "fo|/mnt/sdcard/ivw_resource_youyou.jet|0|913856"

/*sound record card*/
#define ALSA_RECORD_CARD 0

/*sound record device*/
#define ALSA_RECORD_DEVICE 0

#define ALSA_PLAYBACK_CARD 1

#define ALSA_PLAYBACK_DEVICE 0

#define SAMPLERATE_8K  8000

#define SAMPLERATE_16K 16000

#define SAMPLERATE_44K 44100

#define SAMPLERATE_64K 64000

#define SAMPLERATE_96K 96000

#define RECORD_SAMPLERATE SAMPLERATE_64K

#define PLAYBACK_SAMPLERATE SAMPLERATE_16K

#define RECORD_CHANNELS 2

#define PLAYBACK_CHANNELS 2

/*Mobile speech platform login params*/
#define MSP_LOGIN_PARAMS "appid = 58b3dc60"

/*Recongnized text results buffer size in bytes*/
#define SRT_RESULT_BUFFER_SIZE 4096

/*Speech recongnize session params*/
//#define SRT_SESSION_PARAMS "sub=iat,sch=1,nlp_version=2.0,aue=speech-wb;7,result_type=plain,result_encoding=utf8, \
  language=zh_cn,accent=mandarin,sample_rate=16000,domain=iat,vad_bos=1000,vad_eos=1000"
#define SRT_SESSION_PARAMS "sub = iat, domain = iat, language = zh_cn, accent = mandarin, sample_rate = 16000, sch = 1, result_type = plain, result_encoding = utf8, nlp_version=2.0"
//#define SRT_SESSION_PARAMS "sub=iat,auf=audio/L16;rate=16000,aue=speex-wb,ent=sms16k,sch=1,rst=json,rse=utf8,nlp_version=2.0"

/*Text to speech session params*/
//#define TTS_SESSION_PARAMS "voice_name=xiaoyan,text_encoding=utf8,sample_rate=16000,speed=50,volume=50,pitch=50,rdn=2"
#define TTS_SESSION_PARAMS "voice_name = xiaoyan, text_encoding = utf8, sample_rate = 16000, speed = 50, volume = 50, pitch = 50, rdn = 2"

#define TTS_LOCAL_PARAMS "engine_type = local, text_encoding = utf8, voice_name = xiaoyan, tts_res_path = fo|/sdcard/iflytek/xiaoyan.jet;fo|/sdcard/iflytek/tts_common.jet, sample_rate = 16000, speed = 50, volume = 50, pitch = 50, rdn = 2"

#define ASR_BUILD_PARAMS "engine_type = local, asr_res_path = fo|/sdcard/iflytek/common.jet, sample_rate = 16000, grm_build_path = /sdcard/iflytek"

#define ASR_GRAMMAR_FILE "/sdcard/iflytek/call.bnf"

#define ASR_SESSION_PARAMS "engine_type = local, asr_res_path = fo|/sdcard/iflytek/common.jet, sample_rate = 16000, grm_build_path = /sdcard/iflytek, result_type = json, result_encoding = UTF-8"

#endif

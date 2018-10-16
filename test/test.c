//#include <log/log.h>
#include <stdio.h>
#include "qtts.h"
#include "msp_cmn.h"
#include "msp_errors.h"
#include "config.h"
#include "common.h"

//#define LOG_TAG "Iflytek"

int main()
{
  int ret;
  FILE *fp;
  char buf[2048];
  int readsize = 0;

  printf("This is a test process!!!\n");
  const char* text = "您好，这是一个语音合成示例";
  const char* login_params = MSP_LOGIN_PARAMS;

  ret = MSPLogin(NULL, NULL, login_params);
  if(ret != MSP_SUCCESS){
     printf("MSPLogin failed, error code: %d\n", ret);
     return -1;
  }
  printf("start to text to speech ...\n");

  ret = TTS_CreateSession();
  
  //ret = TTS_Process(NULL, text, strlen(text));

  fp = fopen("/sdcard/tts_test.pcm", "r");
  if(fp == NULL){
    printf("open tts_test.pcm failed\n");
  }


  ret = SRT_CreateSession();

  while((readsize = fread(buf, 1, 2048, fp)) > 0){
    ret = SRT_Process(buf, readsize);
  }
  return 0;
}

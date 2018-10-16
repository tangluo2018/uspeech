#include <stdio.h>
#include <string.h>
#include <dlfcn.h>


typedef void * CAE_LIBHANDLE;

typedef void * CAE_HANDLE; 

typedef void (*cae_ivw_fn)(short angle, short channel, float power, short CMScore, short beam, char *param1, void *param2, void *userData);

typedef void (*cae_audio_fn)(const void *audioData, unsigned int audioLen, int param1, const void *param2, void *userData);

int CAENew(CAE_HANDLE *cae, const char* resPath, cae_ivw_fn ivwCb, cae_audio_fn audioCb, const char *param, void *userData);
typedef int (* Proc_CAENew)(CAE_HANDLE *cae, const char* resPath, cae_ivw_fn ivwCb, cae_audio_fn audioCb, const char *param, void *userData);

int CAEAudioWrite(CAE_HANDLE cae, const void *audioData, unsigned int audioLen);
typedef int (* Proc_CAEAudioWrite)(CAE_HANDLE cae, const void *audioData, unsigned int audioLen);

int CAEResetEng(CAE_HANDLE cae);
typedef int (* Proc_CAEResetEng)(CAE_HANDLE cae);

int CAESetRealBeam(CAE_HANDLE cae, int beam);
typedef int (* Proc_CAESetRealBeam)(CAE_HANDLE cae, int beam);

int CAESetWParam(CAE_HANDLE cae, const char* param, const char* value);
typedef int (* Proc_CAESetWParam)(CAE_HANDLE cae, const char* param, const char* value);

int CAEGetWParam(CAE_HANDLE cae, const char* param, char* value, unsigned int *valueLen);
typedef int (* Proc_CAEGetWParam)(CAE_HANDLE cae, const char* param, char* value, unsigned int *valueLen);

char* CAEGetVersion();
typedef char (* Proc_CAEGetVersion)();

int CAEDestroy(CAE_HANDLE cae);
typedef int (* Proc_CAEDestroy)(CAE_HANDLE cae);

int CAEGetChannel();
typedef int (* Proc_CAEGetChannel)();

int CAESetShowLog(int show_log);
typedef int (* Proc_CAESetShowLog)(int show_log);

static CAE_HANDLE g_cae = NULL;
static int in_buff_size = 0;
static volatile int in_pcm_count = 0;
static int audio_record_status = 0;
static Proc_CAENew api_cae_new;
static Proc_CAEAudioWrite api_cae_audio_write;
static Proc_CAEResetEng api_cae_reset_eng;
static Proc_CAESetRealBeam api_cae_set_real_beam;
static Proc_CAESetWParam api_cae_set_wparam;
static Proc_CAEGetWParam api_cae_get_wparam;
static Proc_CAEGetVersion api_cae_get_version;
static Proc_CAEGetChannel api_cae_get_channel;
static Proc_CAEDestroy api_cae_destroy;
static Proc_CAESetShowLog api_cae_set_show_log;



CAE_LIBHANDLE cae_LoadLibrary(const char* lib_name)
{
    return dlopen(lib_name, RTLD_LAZY);
}

int cae_FreeLibrary(CAE_LIBHANDLE lib_handle)
{
    dlclose(lib_handle);
    return 0;
}

void* cae_GetProcAddress(CAE_LIBHANDLE lib_handle, const char* fun_name)
{
    return dlsym(lib_handle, fun_name);
}

static void CAEIvwCb(short angle, short channel, float power, short CMScore, short beam, void *userData)
{
    if(g_cae != NULL){
        api_cae_set_real_beam(g_cae, beam);
    }
}

static void CAEAudioCb(const void *audioData, unsigned int audioLen, int param1, const void *param2, void *userData)
{
    //CAEUserData *usDta = (CAEUserData *)userData;
    //queue_write(usDta->queue, audioData, audioLen);
    //share_write_data(audioData, audioLen);
}

static int initFuncs()
{
    const char* libname = "/system/lib/libcae.so";
    void* hInstance = cae_LoadLibrary(libname);
	
    if(hInstance == NULL)
    {
	printf("Can not open library %s!\n", libname);
	return -1;
    }
    api_cae_new = (Proc_CAENew)cae_GetProcAddress(hInstance, "CAENew");
    api_cae_audio_write = (Proc_CAEAudioWrite)cae_GetProcAddress(hInstance, "CAEAudioWrite");
    api_cae_reset_eng = (Proc_CAEResetEng)cae_GetProcAddress(hInstance, "CAEResetEng");
    api_cae_set_real_beam = (Proc_CAESetRealBeam)cae_GetProcAddress(hInstance, "CAESetRealBeam");
    api_cae_set_wparam = (Proc_CAESetWParam)cae_GetProcAddress(hInstance, "CAESetWParam");
    api_cae_get_wparam = (Proc_CAEGetWParam)cae_GetProcAddress(hInstance, "CAEGetWParam");
    api_cae_get_version = (Proc_CAEGetVersion)cae_GetProcAddress(hInstance, "CAEGetVersion");
    api_cae_get_channel= (Proc_CAEGetChannel)cae_GetProcAddress(hInstance, "CAEGetChannel");
    api_cae_destroy = (Proc_CAEDestroy)cae_GetProcAddress(hInstance, "CAEDestroy");
    api_cae_set_show_log = (Proc_CAESetShowLog)cae_GetProcAddress(hInstance, "CAESetShowLog");
	
    return 1;
}


int main()
{
    int ret;
    const char *resPath = "fo|/system/etc/ivw_resource.jet|0|913856";
    
    if(initFuncs() != 1){
       return -1;
    }
    if(g_cae != NULL){
       api_cae_destroy(g_cae);
       g_cae = NULL;
    }
    printf("Init CAE");
    ret = api_cae_new(&g_cae, resPath, CAEIvwCb, CAEAudioCb, NULL, NULL);
    if(0 != ret){
        printf("CAENew failed");
	return -1;
    }
    api_cae_set_real_beam(g_cae, 0);
    api_cae_set_show_log(1);


  return 0;
}



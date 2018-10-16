LOCAL_PATH:=$(call my-dir)

include $(CLEAR_VARS)
LOCAL_CFLAGS := -g
LOCAL_C_INCLUDES += system/core/include \
                    external/tinyalsa/include \
                    external/iflytek/include
LOCAL_SRC_FILES := if_sch.c 

LOCAL_STATIC_LIBRARIES :=
LOCAL_SHARED_LIBRARIES := liblog libtinyalsa libuspeech #libdl
LOCAL_MODULE := if_sch
include $(BUILD_EXECUTABLE)

include $(CLEAR_VARS)
LOCAL_C_INCLUDES += external/uspeech/include
LOCAL_LDFLAGS += -L$(LOCAL_PATH)/libs/
LOCAL_SRC_FILES := uspeech.c \
                   iflytek.c \
		   audio_queue.c \
		   cJSON.c

LOCAL_LDLIBS += -lcae -lmsc		   
LOCAL_MODULE := libuspeech
include $(BUILD_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_SRC_FILES := audio_queue.c
LOCAL_MODULE := libaudioqueue
include $(BUILD_SHARED_LIBRARY)


include $(CLEAR_VARS)
LOCAL_PREBUILT_LIBS := libs/libcae.so \
                       libs/libmsc.so
include $(BUILD_MULTI_PREBUILT)

#include $(CLEAR_VARS)
#LOCAL_MODULE := ivw_resource_youyou.jet
#LOCAL_MODULE_CLASS := ETC
#LOCAL_MODULE_PATH := 
#LOCAL_SRC_FILES := $(LOCAL_MODULE)
#include $(BUILD_PREBUILT)


#include $(CLEAR_VARS)
#LOCAL_C_INCLUDES += $(LOCAL_PATH)/include \
                    system/core/include
#LOCAL_SRC_FILES := test.c \
                   if_tts.c \
		   if_srt.c
#LOCAL_SHARED_LIBRARIES := liblog libmsc
#LOCAL_MODULE := if_test
#include $(BUILD_EXECUTABLE)

LOCAL_PATH:=$(call my-dir)

include $(CLEAR_VARS)
LOCAL_C_INCLUDES += system/core/include \
                    external/tinyalsa/include
LOCAL_SRC_FILES := pcm_play.c 

LOCAL_STATIC_LIBRARIES :=
LOCAL_SHARED_LIBRARIES := libtinyalsa
LOCAL_MODULE := pcm_play
include $(BUILD_EXECUTABLE)

include $(CLEAR_VARS)
LOCAL_C_INCLUDES += system/core/include
LOCAL_SRC_FILES := loadcae.c 

LOCAL_SHARED_LIBRARIES := libdl
LOCAL_MODULE := loadcae
include $(BUILD_EXECUTABLE)


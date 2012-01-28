LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_WHOLE_STATIC_LIBRARIES := \
	libomx_m4venc_component_lib

ifeq ($(TARGET_BOARD_PLATFORM),s5p6442)
LOCAL_WHOLE_STATIC_LIBRARIES += \
	libmfcencapi
else
LOCAL_WHOLE_STATIC_LIBRARIES += \
 	libpvm4vencoder
endif

LOCAL_MODULE := libomx_m4venc_sharedlibrary

-include $(PV_TOP)/Android_platform_extras.mk

-include $(PV_TOP)/Android_system_extras.mk

LOCAL_SHARED_LIBRARIES +=   libomx_sharedlibrary libopencore_common 

include $(BUILD_SHARED_LIBRARY)
include   $(PV_TOP)/codecs_v2/omx/omx_m4venc/Android.mk
ifeq ($(TARGET_BOARD_PLATFORM),s5p6442)
include   $(PV_TOP)/codecs_v2/video/s3c_mfc/enc/Android.mk
else
include   $(PV_TOP)/codecs_v2/video/m4v_h263/enc/Android.mk
endif


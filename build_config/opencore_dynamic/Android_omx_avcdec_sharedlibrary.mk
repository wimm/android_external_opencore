LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_WHOLE_STATIC_LIBRARIES := \
	libomx_avc_component_lib

ifeq ($(TARGET_BOARD_PLATFORM),s5p6442)
LOCAL_WHOLE_STATIC_LIBRARIES += \
 	libmfcdecapi
else
LOCAL_WHOLE_STATIC_LIBRARIES += \
 	libpvavcdecoder
endif

LOCAL_MODULE := libomx_avcdec_sharedlibrary

-include $(PV_TOP)/Android_platform_extras.mk

-include $(PV_TOP)/Android_system_extras.mk

LOCAL_SHARED_LIBRARIES +=   libomx_sharedlibrary libopencore_common

include $(BUILD_SHARED_LIBRARY)
include   $(PV_TOP)/codecs_v2/omx/omx_h264/Android.mk
ifeq ($(TARGET_BOARD_PLATFORM),s5p6442)
else
include   $(PV_TOP)/codecs_v2/video/avc_h264/dec/Android.mk
endif


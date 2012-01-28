LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_WHOLE_STATIC_LIBRARIES := \
	libomx_m4v_component_lib

ifeq ($(TARGET_BOARD_PLATFORM),s5p6442)
LOCAL_WHOLE_STATIC_LIBRARIES += \
	libmfcdecapi
else
LOCAL_WHOLE_STATIC_LIBRARIES += \
 	libpvmp4decoder
endif

LOCAL_MODULE := libomx_m4vdec_sharedlibrary

-include $(PV_TOP)/Android_platform_extras.mk

-include $(PV_TOP)/Android_system_extras.mk

LOCAL_SHARED_LIBRARIES +=   libomx_sharedlibrary libopencore_common 

include $(BUILD_SHARED_LIBRARY)
include   $(PV_TOP)/codecs_v2/omx/omx_m4v/Android.mk
ifeq ($(TARGET_BOARD_PLATFORM),s5p6442)
include   $(PV_TOP)/codecs_v2/video/s3c_mfc/dec/Android.mk
else
include   $(PV_TOP)/codecs_v2/video/m4v_h263/dec/Android.mk
endif


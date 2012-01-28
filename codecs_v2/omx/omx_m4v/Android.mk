LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
	src/omx_mpeg4_component.cpp \
 	src/mpeg4_dec.cpp


LOCAL_MODULE := libomx_m4v_component_lib

LOCAL_CFLAGS :=  $(PV_CFLAGS)

LOCAL_ARM_MODE := arm

LOCAL_STATIC_LIBRARIES := 

LOCAL_SHARED_LIBRARIES := 

LOCAL_C_INCLUDES := \
	$(PV_TOP)/codecs_v2/omx/omx_m4v/src \
 	$(PV_TOP)/codecs_v2/omx/omx_m4v/include \
 	$(PV_TOP)/extern_libs_v2/khronos/openmax/include \
 	$(PV_TOP)/codecs_v2/omx/omx_baseclass/include \
 	$(PV_INCLUDES)

ifeq ($(TARGET_BOARD_PLATFORM),s5p6442)
LOCAL_C_INCLUDES += \
	$(PV_TOP)/codecs_v2/video/s3c_mfc/dec/src \
	$(PV_TOP)/codecs_v2/video/s3c_mfc/dec/include
else
LOCAL_C_INCLUDES += \
 	$(PV_TOP)/codecs_v2/video/m4v_h263/dec/src \
 	$(PV_TOP)/codecs_v2/video/m4v_h263/dec/include
endif

LOCAL_COPY_HEADERS_TO := $(PV_COPY_HEADERS_TO)

LOCAL_COPY_HEADERS := \
	include/omx_mpeg4_component.h \
 	include/mpeg4_dec.h

ifeq ($(TARGET_BOARD_PLATFORM),s5p6442)
LOCAL_CFLAGS  += -DSLSI_S5P6442
else
LOCAL_COPY_HEADERS += \
 	include/mpeg4video_frame.h
endif

include $(BUILD_STATIC_LIBRARY)

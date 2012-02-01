LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
    autodetect.cpp \
    metadatadriver.cpp \
    playerdriver.cpp \
    thread_init.cpp \
    mediascanner.cpp \
    android_surface_output.cpp \
    android_audio_output.cpp \
    android_audio_stream.cpp \
    android_audio_mio.cpp \
    android_audio_output_threadsafe_callbacks.cpp

LOCAL_C_INCLUDES := $(PV_INCLUDES) \
    $(PV_TOP)/engines/common/include \
    $(PV_TOP)/fileformats/mp4/parser/include \
    $(PV_TOP)/pvmi/media_io/pvmiofileoutput/include \
    $(PV_TOP)/nodes/pvmediaoutputnode/include \
    $(PV_TOP)/nodes/pvmediainputnode/include \
    $(PV_TOP)/nodes/pvmp4ffcomposernode/include \
    $(PV_TOP)/engines/player/include \
    $(PV_TOP)/nodes/common/include \
    $(PV_TOP)/fileformats/pvx/parser/include \
	$(PV_TOP)/nodes/pvprotocolenginenode/download_protocols/common/src \
    libs/drm/mobile1/include \
    include/graphics \
    external/skia/include/corecg \
    external/tremor/Tremor \
    external/icu4c/common \
    $(call include-path-for, graphics corecg)

LOCAL_MODULE := libandroidpv

LOCAL_SHARED_LIBRARIES := libui libutils libbinder
LOCAL_STATIC_LIBRARIES := libosclbase libosclerror libosclmemory libosclutil

LOCAL_LDLIBS +=

ifeq ($(TARGET_BOARD_PLATFORM),s5p6442)
LOCAL_CFLAGS  += -DSLSI_S5P6442
LOCAL_C_INCLUDES += device/sec_proprietary/libgralloc
endif
ifeq ($(MFC_ZERO_COPY),true)
LOCAL_CFLAGS  += -DZERO_COPY
endif

include $(BUILD_STATIC_LIBRARY)


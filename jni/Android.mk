LOCAL_PATH   := $(call my-dir)
CORE_DIR     := $(LOCAL_PATH)/..

include $(LOCAL_PATH)/../Makefile.common

COREFLAGS := -DINLINE=inline -DHAVE_STDINT_H -DHAVE_INTTYPES_H -D__LIBRETRO__ -DLIBRETRO -DHAVE_STB_VORBIS -fPIC

ifeq ($(basegame),xatrix)
COREFLAGS += -DXATRIX
else ifeq ($(basegame),rogue)
COREFLAGS += -DROGUE
else ifeq ($(basegame),zaero)
COREFLAGS += -DZAERO
endif

include $(CLEAR_VARS)
LOCAL_MODULE    := retro
LOCAL_SRC_FILES := $(SOURCES_CXX) $(SOURCES_C)
LOCAL_CXXFLAGS  := $(COREFLAGS) $(INCFLAGS)
LOCAL_CFLAGS    := $(COREFLAGS) $(INCFLAGS)
LOCAL_LDFLAGS   := -Wl,-version-script=$(CORE_DIR)/link.T
LOCAL_C_INCLUDES = $(CORE_DIR)

include $(BUILD_SHARED_LIBRARY)

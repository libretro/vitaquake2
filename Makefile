STATIC_LINKING := 0
AR             := ar
HAVE_OPENGL    := 0
HAVE_CDAUDIO   := 1

ifneq ($(V),1)
   Q := @
endif

ifneq ($(SANITIZER),)
   CFLAGS   := -fsanitize=$(SANITIZER) $(CFLAGS) 
   CXXFLAGS := -fsanitize=$(SANITIZER) $(CXXFLAGS)
   LDFLAGS  := -fsanitize=$(SANITIZER) $(LDFLAGS)
endif

ifeq ($(platform),)
platform = unix
ifeq ($(shell uname -a),)
   platform = win
else ifneq ($(findstring MINGW,$(shell uname -a)),)
   platform = win
else ifneq ($(findstring Darwin,$(shell uname -a)),)
   platform = osx
else ifneq ($(findstring win,$(shell uname -a)),)
   platform = win
endif
endif

# system platform
system_platform = unix
ifeq ($(shell uname -a),)
	EXE_EXT = .exe
	system_platform = win
else ifneq ($(findstring Darwin,$(shell uname -a)),)
	system_platform = osx
	arch = intel
        ifeq ($(shell uname -p),powerpc)
	        arch = ppc
        endif
        ifeq ($(shell uname -p),arm)
		arch = arm
        endif
else ifneq ($(findstring MINGW,$(shell uname -a)),)
	system_platform = win
endif

CORE_DIR    += .
ifeq ($(basegame),xatrix)
TARGET_NAME := vitaquake2-xatrix
else ifeq ($(basegame),rogue)
TARGET_NAME := vitaquake2-rogue
else ifeq ($(basegame),zaero)
TARGET_NAME := vitaquake2-zaero
else
TARGET_NAME := vitaquake2
endif
LIBM		    = -lm


ifeq ($(STATIC_LINKING), 1)
EXT := a
endif

ifneq (,$(findstring unix,$(platform)))
	EXT ?= so
   TARGET := $(TARGET_NAME)_libretro.$(EXT)
   fpic := -fPIC
   HAVE_OPENGL = 1
	GL_LIB := -lGL
   SHARED := -shared -Wl,--version-script=$(CORE_DIR)/link.T -Wl,--no-undefined
else ifeq ($(platform), linux-portable)
   TARGET := $(TARGET_NAME)_libretro.$(EXT)
   fpic := -fPIC -nostdlib
	HAVE_OPENGL = 1
	GL_LIB := -lGL
   SHARED := -shared -Wl,--version-script=$(CORE_DIR)/link.T
	LIBM :=

else ifneq (,$(findstring rockchip,$(platform)))
   EXT ?= so
   TARGET := $(TARGET_NAME)_libretro.$(EXT)
   CFLAGS += -D_POSIX_C_SOURCE=199309L -DMESA_EGL_NO_X11_HEADERS -DEGL_NO_X11
   fpic := -fPIC
   SHARED := -shared -Wl,--version-script=$(CORE_DIR)/link.T -Wl,--no-undefined
   GLES := 1
   ifneq (,$(findstring RK3399,$(platform)))
       GLES31 := 1
   endif

else ifneq (,$(findstring osx,$(platform)))
   TARGET := $(TARGET_NAME)_libretro.dylib
   fpic := -fPIC
   #HAVE_OPENGL = 1
   #GL_LIB := -framework OpenGL
   SHARED := -dynamiclib

ifeq ($(UNIVERSAL),1)
ifeq ($(archs),ppc)
   ARCHFLAGS = -arch ppc -arch ppc64
else ifeq ($(archs),arm64)
   ARCHFLAGS = -arch x86_64 -arch arm64
else
   ARCHFLAGS = -arch i386 -arch x86_64
endif
   CXXFLAGS += $(ARCHFLAGS)
   LFLAGS += $(ARCHFLAGS)
endif

   ifeq ($(CROSS_COMPILE),1)
		TARGET_RULE   = -target $(LIBRETRO_APPLE_PLATFORM) -isysroot $(LIBRETRO_APPLE_ISYSROOT)
		CFLAGS   += $(TARGET_RULE)
		CPPFLAGS += $(TARGET_RULE)
		CXXFLAGS += $(TARGET_RULE)
		LDFLAGS  += $(TARGET_RULE)
   endif

	CFLAGS  += $(ARCHFLAGS)
	CXXFLAGS  += $(ARCHFLAGS)
	LDFLAGS += $(ARCHFLAGS)

else ifneq (,$(findstring ios,$(platform)))
   TARGET := $(TARGET_NAME)_libretro_ios.dylib
   fpic := -fPIC
   SHARED := -dynamiclib
   MINVERSION :=

ifeq ($(IOSSDK),)
   IOSSDK := $(shell xcodebuild -version -sdk iphoneos Path)
endif
   DEFINES := -DIOS
ifeq ($(platform),ios-arm64)
   CC = cc -arch arm64 -isysroot $(IOSSDK)
else
   CC = cc -arch armv7 -isysroot $(IOSSDK)
endif
ifeq ($(platform),$(filter $(platform),ios9 ios-arm64))
   MINVERSION = -miphoneos-version-min=8.0
else
   MINVERSION = -miphoneos-version-min=5.0
endif
   CFLAGS   += $(MINVERSION)
   CXXFLAGS += $(MINVERSION)
   LDFLAGS  += $(MINVERSION)

else ifeq ($(platform), tvos-arm64)
   EXT?=dylib
   TARGET := $(TARGET_NAME)_libretro_tvos.$(EXT)
   fpic := -fPIC
   SHARED := -dynamiclib
   DEFINES := -DIOS
ifeq ($(IOSSDK),)
   IOSSDK := $(shell xcodebuild -version -sdk appletvos Path)
endif
   CC = cc -arch arm64 -isysroot $(IOSSDK)
   MINVERSION = -mappletvos-version-min=11.0
   CFLAGS   += $(MINVERSION)
   CXXFLAGS += $(MINVERSION)
   LDFLAGS  += $(MINVERSION)

else ifneq (,$(findstring qnx,$(platform)))
	TARGET := $(TARGET_NAME)_libretro_qnx.so
   fpic := -fPIC
   SHARED := -shared -Wl,--version-script=$(CORE_DIR)/link.T -Wl,--no-undefined
	CC = qcc -Vgcc_ntoarmv7le
	CXX = QCC -Vgcc_ntoarmv7le
else ifeq ($(platform), emscripten)
   TARGET := $(TARGET_NAME)_libretro_emscripten.bc
   CFLAGS += -D_XOPEN_SOURCE=700
   AR := emar
   STATIC_LINKING = 1
   ifneq ($(MEMORY),)
      LDFLAGS += -s TOTAL_MEMORY=$(MEMORY)
   endif
else ifeq ($(platform), vita)
   TARGET := $(TARGET_NAME)_libretro_vita.a
   CC = arm-vita-eabi-gcc
   AR = arm-vita-eabi-ar
   CFLAGS += -DVITA
   CXXFLAGS += -Wl,-q -Wall -O3
   STATIC_LINKING = 1
   HAVE_CDAUDIO = 0
else ifeq ($(platform), libnx)
    include $(DEVKITPRO)/libnx/switch_rules
    EXT=a
    TARGET := $(TARGET_NAME)_libretro_$(platform).$(EXT)
    DEFINES := -DSWITCH=1 -fcommon -U__linux__ -U__linux -DRARCH_INTERNAL
    CFLAGS	:=	 $(DEFINES) -g -O3 \
                 -fPIE -I$(LIBNX)/include/ -ffunction-sections -fdata-sections -ftls-model=local-exec -Wl,--allow-multiple-definition -specs=$(LIBNX)/switch.specs
    CFLAGS += $(INCDIRS) -I$(PORTLIBS)/include/
    CFLAGS	+=	-D__SWITCH__ -DHAVE_LIBNX -march=armv8-a -mtune=cortex-a57 -mtp=soft
    CXXFLAGS := $(ASFLAGS) $(CFLAGS) -fno-rtti -std=gnu++11
    CFLAGS += -std=gnu11
    STATIC_LINKING = 1
    HAVE_OPENGL =1
# Lightweight PS3 Homebrew SDK
else ifeq ($(platform), psl1ght)
   EXT=a
   TARGET := $(TARGET_NAME)_libretro_$(platform).$(EXT)
   CC = $(PS3DEV)/ppu/bin/ppu-gcc$(EXE_EXT)
   CXX = $(PS3DEV)/ppu/bin/ppu-g++$(EXE_EXT)
   CC_AS = $(PS3DEV)/ppu/bin/ppu-gcc$(EXE_EXT)
   AR = $(PS3DEV)/ppu/bin/ppu-ar$(EXE_EXT)
   CFLAGS += -D__PS3__ -D__PSL1GHT__ -mcpu=cell
   CXXFLAGS += -D__PS3__ -D__PSL1GHT__ -mcpu=cell
   STATIC_LINKING = 1
   HAVE_OPENGL = 0
# WiiU
else ifeq ($(platform), wiiu)
   EXT=a
   TARGET := $(TARGET_NAME)_libretro_$(platform).$(EXT)
   CC = $(DEVKITPPC)/bin/powerpc-eabi-gcc$(EXE_EXT)
   CXX = $(DEVKITPPC)/bin/powerpc-eabi-g++$(EXE_EXT)
   AR = $(DEVKITPPC)/bin/powerpc-eabi-ar$(EXE_EXT)
   CFLAGS += -DGEKKO -DHW_RVL -DWIIU -mcpu=750 -meabi -mhard-float
   CXXFLAGS += -DGEKKO -DHW_RVL -DWIIU -mcpu=750 -meabi -mhard-float
   CXXFLAGS += -ffunction-sections -fdata-sections -D__wiiu__ -D__wut__
   STATIC_LINKING = 1
   HAVE_OPENGL = 0
# CTR (3DS)
else ifeq ($(platform), ctr)
    TARGET := $(TARGET_NAME)_libretro_$(platform).a
    CC = $(DEVKITARM)/bin/arm-none-eabi-gcc$(EXE_EXT)
    CXX = $(DEVKITARM)/bin/arm-none-eabi-g++$(EXE_EXT)
    AR = $(DEVKITARM)/bin/arm-none-eabi-ar$(EXE_EXT)
    DEFINES += -D_3DS -DARM11 -march=armv6k -mtune=mpcore -mfloat-abi=hard -I$(CTRULIB)/include
    CFLAGS += $(DEFINES) -Dstricmp=strcasecmp -Dstrnicmp=strncasecmp -fcommon
    CXXFLAGS += $(CFLAGS) -std=gnu++11
    STATIC_LINKING = 1
    HAVE_OPENGL = 0
# GCW0
else ifeq ($(platform), gcw0)
   TARGET := $(TARGET_NAME)_libretro.so
   CC = /opt/gcw0-toolchain/usr/bin/mipsel-linux-gcc
   AR = /opt/gcw0-toolchain/usr/bin/mipsel-linux-ar
   fpic := -fPIC
   SHARED := -shared -Wl,--version-script=$(CORE_DIR)/link.T -Wl,--no-undefined
   CFLAGS += -DDINGUX -D_POSIX_C_SOURCE=199309L -fomit-frame-pointer -march=mips32 -mtune=mips32r2 -mhard-float
   HAVE_OPENGL = 0
else
   CC ?= gcc
   TARGET := $(TARGET_NAME)_libretro.dll
   HAVE_OPENGL = 1
   SHARED := -shared -static-libgcc -static-libstdc++ -s -Wl,--version-script=$(CORE_DIR)/link.T -Wl,--no-undefined
   GL_LIB = -lopengl32
endif

ifeq ($(GLES), 1)
   HAVE_OPENGL = 0
   CFLAGS += -DHAVE_OPENGLES
   GL_LIB += -lGLESv2
   ifeq ($(GLES31), 1)
      CFLAGS += -DHAVE_OPENGLES3 -DHAVE_OPENGLES_3_1
   else ifeq ($(GLES3), 1)
      CFLAGS += -DHAVE_OPENGLES3
   else
      CFLAGS += -DHAVE_OPENGLES2
   endif
endif

LDFLAGS += $(LIBM)

ifeq ($(DEBUG), 1)
   CFLAGS += -O0 -g -DDEBUG
   CXXFLAGS += -O0 -g -DDEBUG
else
   CFLAGS += -O3 -DNDEBUG
   CXXFLAGS += -O3 -DNDEBUG
endif

include Makefile.common

OBJECTS := $(SOURCES_C:.c=.o)

CFLAGS   += -Wall -D__LIBRETRO__ $(fpic) -DREF_HARD_LINKED -DRELEASE -DGAME_HARD_LINKED -DOSTYPE=\"$(OSTYPE)\" -DARCH=\"$(ARCH)\" -fsigned-char
CXXFLAGS += -Wall -D__LIBRETRO__ $(fpic) -fpermissive

ifeq ($(HAVE_OPENGL),1)
CFLAGS   += -DHAVE_OPENGL
endif

ifeq ($(HAVE_CDAUDIO),1)
CFLAGS   += -DHAVE_CDAUDIO -DHAVE_STB_VORBIS
endif

ifeq ($(basegame),xatrix)
CFLAGS   += -DXATRIX
else ifeq ($(basegame),rogue)
CFLAGS   += -DROGUE
else ifeq ($(basegame),zaero)
CFLAGS   += -DZAERO
endif

ifneq (,$(findstring unix,$(platform)))
CFLAGS += -std=gnu99
else
CFLAGS += -std=c99
endif
CFLAGS     += $(INCFLAGS)
CXXFLAGS   += $(INCFLAGS)

all: $(TARGET)

$(TARGET): $(OBJECTS)
ifeq ($(STATIC_LINKING), 1)
	$(AR) rcs $@ $(OBJECTS)
else
	$(CC) $(fpic) $(SHARED) $(INCLUDES) -o $@ $(OBJECTS) $(LDFLAGS) $(GL_LIB)
endif

%.o: %.c
	$(CC) $(CFLAGS) $(fpic) -c -o $@ $<

clean:
	rm -f $(OBJECTS) $(TARGET)

.PHONY: clean

print-%:
	@echo '$*=$($*)'

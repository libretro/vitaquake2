STATIC_LINKING := 0
AR             := ar
HAVE_OPENGL    := 0

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

ifeq ($(ARCHFLAGS),)
ifeq ($(archs),ppc)
   ARCHFLAGS = -arch ppc -arch ppc64
else
   ARCHFLAGS = -arch i386 -arch x86_64
endif
endif

ifeq ($(platform), osx)
ifndef ($(NOUNIVERSAL))
   CXXFLAGS += $(ARCHFLAGS)
   LFLAGS += $(ARCHFLAGS)
endif
endif

ifeq ($(STATIC_LINKING), 1)
EXT := a
endif

ifeq ($(platform), unix)
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
else ifneq (,$(findstring osx,$(platform)))
   TARGET := $(TARGET_NAME)_libretro.dylib
   fpic := -fPIC
   #HAVE_OPENGL = 1
   #GL_LIB := -framework OpenGL
   SHARED := -dynamiclib
else ifneq (,$(findstring ios,$(platform)))
   TARGET := $(TARGET_NAME)_libretro_ios.dylib
	fpic := -fPIC
	SHARED := -dynamiclib

ifeq ($(IOSSDK),)
   IOSSDK := $(shell xcodebuild -version -sdk iphoneos Path)
endif

	DEFINES := -DIOS
	CC = cc -arch armv7 -isysroot $(IOSSDK)
ifeq ($(platform),ios9)
CC     += -miphoneos-version-min=8.0
CXXFLAGS += -miphoneos-version-min=8.0
else
CC     += -miphoneos-version-min=5.0
CXXFLAGS += -miphoneos-version-min=5.0
endif
else ifneq (,$(findstring qnx,$(platform)))
	TARGET := $(TARGET_NAME)_libretro_qnx.so
   fpic := -fPIC
   SHARED := -shared -Wl,--version-script=$(CORE_DIR)/link.T -Wl,--no-undefined
else ifeq ($(platform), emscripten)
   TARGET := $(TARGET_NAME)_libretro_emscripten.bc
   fpic := -fPIC
   CFLAGS += -D_XOPEN_SOURCE=700
   SHARED := -shared -Wl,--version-script=$(CORE_DIR)/link.T -Wl,--no-undefined
else ifeq ($(platform), vita)
   TARGET := $(TARGET_NAME)_vita.a
   CC = arm-vita-eabi-gcc
   AR = arm-vita-eabi-ar
   CFLAGS += -DVITA
   CXXFLAGS += -Wl,-q -Wall -O3
	STATIC_LINKING = 1
else ifeq ($(platform), libnx)
    include $(DEVKITPRO)/libnx/switch_rules
    EXT=a
    TARGET := $(TARGET_NAME)_libretro_$(platform).$(EXT)
    DEFINES := -DSWITCH=1 -U__linux__ -U__linux -DRARCH_INTERNAL
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
   CFLAGS += -D__CELLOS_LV2__ -D__PSL1GHT__ -mcpu=cell
   CXXFLAGS += -D__CELLOS_LV2__ -D__PSL1GHT__ -mcpu=cell
   STATIC_LINKING = 1
   HAVE_OPENGL = 0
# CTR (3DS)
else ifeq ($(platform), ctr)
    TARGET := $(TARGET_NAME)_libretro_$(platform).a
    CC = $(DEVKITARM)/bin/arm-none-eabi-gcc$(EXE_EXT)
    CXX = $(DEVKITARM)/bin/arm-none-eabi-g++$(EXE_EXT)
    AR = $(DEVKITARM)/bin/arm-none-eabi-ar$(EXE_EXT)
    DEFINES += -D_3DS -DARM11 -march=armv6k -mtune=mpcore -mfloat-abi=hard -I$(DEVKITPRO)/ctrulib/libctru/include
    CFLAGS += $(DEFINES) -Dstricmp=strcasecmp -Dstrnicmp=strncasecmp
    CXXFLAGS += $(CFLAGS) -std=gnu++11
    STATIC_LINKING = 1
    HAVE_OPENGL = 0
else
   CC = gcc
   TARGET := $(TARGET_NAME)_libretro.dll
   HAVE_OPENGL = 1
   SHARED := -shared -static-libgcc -static-libstdc++ -s -Wl,--version-script=$(CORE_DIR)/link.T -Wl,--no-undefined
   GL_LIB = -lopengl32
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

ifeq ($(basegame),xatrix)
CFLAGS   += -DXATRIX
else ifeq ($(basegame),rogue)
CFLAGS   += -DROGUE
else ifeq ($(basegame),zaero)
CFLAGS   += -DZAERO
endif

ifeq ($(platform), unix)
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
	@$(if $(Q), $(shell echo echo LD $@),)
	$(Q)$(CC) $(fpic) $(SHARED) $(INCLUDES) -o $@ $(OBJECTS) $(LDFLAGS) $(GL_LIB)
endif

%.o: %.c
	@$(if $(Q), $(shell echo echo CC $<),)
	$(Q)$(CC) $(CFLAGS) $(fpic) -c -o $@ $<

clean:
	rm -f $(OBJECTS) $(TARGET)

.PHONY: clean

print-%:
	@echo '$*=$($*)'

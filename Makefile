#---------------------------------------------------------------------------------
.SUFFIXES:
#---------------------------------------------------------------------------------

ifeq ($(strip $(DEVKITPRO)),)
$(error "Please set DEVKITPRO in your environment. export DEVKITPRO=<path to>/devkitpro")
endif

TOPDIR ?= $(CURDIR)
include $(DEVKITPRO)/libnx/switch_rules

#---------------------------------------------------------------------------------
# Configuration
#---------------------------------------------------------------------------------
TARGET		:=	WiiUMenu
BUILD		:=	build_switch
SOURCES		:=	src src/core src/ui src/app
DATA		:=
INCLUDES	:=	src src/third_party/stb lib/libnxtc/include
ROMFS		:=	romfs

APP_TITLE	:=	SwitchU
APP_AUTHOR	:=	PoloNX
APP_VERSION	:=	1.0.0

CONFIG_JSON	:=	SwitchU.json
QLAUNCH_TID	:=	0100000000001000

#---------------------------------------------------------------------------------
# Compiler flags
#---------------------------------------------------------------------------------
ARCH	:=	-march=armv8-a+crc+crypto -mtune=cortex-a57 -mtp=soft -fPIE

CFLAGS	:=	-g -Wall -O2 -ffunction-sections \
			$(ARCH) $(DEFINES)
CFLAGS	+=	$(INCLUDE) -D__SWITCH__

CXXFLAGS	:= $(CFLAGS) -std=c++20 -fno-rtti -fexceptions

ASFLAGS	:=	-g $(ARCH)
LDFLAGS	=	-specs=$(DEVKITPRO)/libnx/switch.specs -g $(ARCH) -Wl,-Map,$(notdir $*.map)

LIBS	:=	-ldeko3d -lnxtc -lnx -lz \
			`$(PREFIX)pkg-config --libs SDL2_mixer SDL2_ttf sdl2`

LIBDIRS	:= $(CURDIR)/lib/libnxtc $(PORTLIBS) $(LIBNX)

#---------------------------------------------------------------------------------
# Shader compilation (GLSL -> DKSH via uam)
#---------------------------------------------------------------------------------
UAM				:=	$(DEVKITPRO)/tools/bin/uam
SHADER_DIR		:=	$(CURDIR)/shaders
ROMFS_SHDR_DIR	:=	$(CURDIR)/romfs/shaders

VERT_SRC := $(wildcard $(SHADER_DIR)/*_vsh.glsl)
FRAG_SRC := $(wildcard $(SHADER_DIR)/*_fsh.glsl)
VERT_DKSH := $(patsubst $(SHADER_DIR)/%_vsh.glsl,$(ROMFS_SHDR_DIR)/%_vsh.dksh,$(VERT_SRC))
FRAG_DKSH := $(patsubst $(SHADER_DIR)/%_fsh.glsl,$(ROMFS_SHDR_DIR)/%_fsh.dksh,$(FRAG_SRC))
ALL_DKSH := $(VERT_DKSH) $(FRAG_DKSH)

$(ROMFS_SHDR_DIR)/%_vsh.dksh: $(SHADER_DIR)/%_vsh.glsl
	@echo "  SHADER(vert) $(notdir $<)"
	@mkdir -p $(dir $@)
	@$(UAM) -s vert -o $@ $<

$(ROMFS_SHDR_DIR)/%_fsh.dksh: $(SHADER_DIR)/%_fsh.glsl
	@echo "  SHADER(frag) $(notdir $<)"
	@mkdir -p $(dir $@)
	@$(UAM) -s frag -o $@ $<

#---------------------------------------------------------------------------------
# Build targets
#---------------------------------------------------------------------------------
ifneq ($(BUILD),$(notdir $(CURDIR)))

export OUTPUT	:=	$(CURDIR)/$(TARGET)
export TOPDIR	:=	$(CURDIR)
export VPATH	:=	$(foreach dir,$(SOURCES),$(CURDIR)/$(dir)) \
			$(foreach dir,$(DATA),$(CURDIR)/$(dir))
export DEPSDIR	:=	$(CURDIR)/$(BUILD)

CFILES		:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.c)))
CPPFILES	:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.cpp)))
SFILES		:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.s)))
BINFILES	:=	$(foreach dir,$(DATA),$(notdir $(wildcard $(dir)/*.*)))

ifeq ($(strip $(CPPFILES)),)
	export LD	:=	$(CC)
else
	export LD	:=	$(CXX)
endif

export OFILES_BIN	:=	$(addsuffix .o,$(BINFILES))
export OFILES_SRC	:=	$(CPPFILES:.cpp=.o) $(CFILES:.c=.o) $(SFILES:.s=.o)
export OFILES 	:=	$(OFILES_BIN) $(OFILES_SRC)
export HFILES_BIN	:=	$(addsuffix .h,$(subst .,_,$(BINFILES)))

export INCLUDE	:=	$(foreach dir,$(INCLUDES),-I$(CURDIR)/$(dir)) \
			$(foreach dir,$(LIBDIRS),-I$(dir)/include) \
			-I$(CURDIR)/$(BUILD)

export LIBPATHS	:=	$(foreach dir,$(LIBDIRS),-L$(dir)/lib)

export APP_JSON	:=	$(TOPDIR)/$(CONFIG_JSON)

# SD card output directory
SD_OUT		:=	$(CURDIR)/sd_out
SD_CONTENTS	:=	$(SD_OUT)/atmosphere/contents/$(QLAUNCH_TID)

.PHONY: all clean shaders libnxtc install

all: libnxtc shaders $(BUILD)

libnxtc:
	@$(MAKE) --no-print-directory -C lib/libnxtc release

shaders: $(ALL_DKSH)

$(BUILD): libnxtc shaders
	@[ -d $@ ] || mkdir -p $@
	@$(MAKE) --no-print-directory -C $(BUILD) -f $(CURDIR)/Makefile

install: all
	@echo "Packaging for Atmosphère (qlaunch replacement)..."
	@mkdir -p $(SD_CONTENTS)
	@cp $(TARGET).nsp $(SD_CONTENTS)/exefs.nsp
	@rm -rf $(SD_CONTENTS)/romfs
	@cp -r $(ROMFS) $(SD_CONTENTS)/romfs
	@echo "Done! Copy sd_out/ contents to your SD card."
	@echo "  $(SD_CONTENTS)/exefs.nsp"
	@echo "  $(SD_CONTENTS)/romfs/"

clean:
	@echo clean ...
	@rm -fr $(BUILD) $(TARGET).nsp $(TARGET).nso $(TARGET).npdm $(TARGET).elf
	@rm -rf exefs $(SD_OUT)
	@rm -f $(ROMFS_SHDR_DIR)/*.dksh
	@$(MAKE) --no-print-directory -C lib/libnxtc clean

else
.PHONY: all

DEPENDS	:=	$(OFILES:.o=.d)

all: $(OUTPUT).nsp

$(OUTPUT).elf: $(OFILES)
$(OFILES_SRC): $(HFILES_BIN)

%.bin.o %_bin.h: %.bin
	@echo $(notdir $<)
	@$(bin2o)

-include $(DEPENDS)

endif

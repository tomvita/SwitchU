#---------------------------------------------------------------------------------
# nxui — Reusable Switch UI library (static, for devkitA64 / libnx)
#---------------------------------------------------------------------------------
.SUFFIXES:

ifeq ($(strip $(DEVKITPRO)),)
$(error "Please set DEVKITPRO in your environment.")
endif

include $(DEVKITPRO)/libnx/switch_rules

#---------------------------------------------------------------------------------
TARGET		:=	nxui
BUILD		:=	build
SOURCES		:=	src/core src/widgets src/focus
INCLUDES	:=	include include/nxui/third_party/stb

#---------------------------------------------------------------------------------
ARCH	:=	-march=armv8-a+crc+crypto -mtune=cortex-a57 -mtp=soft -fPIE

CFLAGS	:=	-Wall -O3 -flto=auto -ffunction-sections -fdata-sections \
			-ffast-math -DNDEBUG $(ARCH)

CFLAGS	+=	$(foreach dir,$(INCLUDES),-I$(CURDIR)/$(dir)) \
			$(foreach dir,$(LIBDIRS),-I$(dir)/include) \
			-I$(CURDIR)/$(BUILD) -D__SWITCH__

CXXFLAGS	:= $(CFLAGS) -std=c++20 -fno-rtti -fexceptions

LIBDIRS	:=	$(PORTLIBS) $(LIBNX)

#---------------------------------------------------------------------------------
ifneq ($(BUILD),$(notdir $(CURDIR)))

export VPATH	:=	$(foreach dir,$(SOURCES),$(CURDIR)/$(dir))
export DEPSDIR	:=	$(CURDIR)/$(BUILD)

CPPFILES	:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(CURDIR)/$(dir)/*.cpp)))
CFILES		:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(CURDIR)/$(dir)/*.c)))

export OFILES	:=	$(CPPFILES:.cpp=.o) $(CFILES:.c=.o)
export INCLUDE	:=	$(foreach dir,$(INCLUDES),-I$(CURDIR)/$(dir)) \
			$(foreach dir,$(LIBDIRS),-I$(dir)/include)

.PHONY: all clean release debug

release: all
debug: all

all: lib/lib$(TARGET).a

lib/lib$(TARGET).a: $(OFILES)

$(OFILES):
	@[ -d $(BUILD) ] || mkdir -p $(BUILD)
	@$(MAKE) --no-print-directory -C $(BUILD) -f $(CURDIR)/Makefile DEPSBUILD=$(CURDIR)/$(BUILD) $@

clean:
	@echo "  CLEAN nxui"
	@rm -rf $(BUILD) lib

else

DEPENDS	:= $(OFILES:.o=.d)
-include $(DEPENDS)

endif

# Actual build rules (invoked from BUILD directory)
%.o: %.cpp
	@echo "  CXX   $(notdir $<)"
	@$(CXX) -MMD -MP -MF $(DEPSDIR)/$*.d $(CXXFLAGS) $(INCLUDE) -c $< -o $@

%.o: %.c
	@echo "  CC    $(notdir $<)"
	@$(CC) -MMD -MP -MF $(DEPSDIR)/$*.d $(CFLAGS) $(INCLUDE) -c $< -o $@

lib/lib$(TARGET).a: $(OFILES)
	@echo "  AR    lib$(@F)"
	@mkdir -p lib
	@rm -f $@
	@$(AR) rcs $@ $(foreach o,$(OFILES),$(BUILD)/$(o))

# ──────────────────────────────────────────────────────────────────────────────
# SwitchU — GNU Make build (daemon + menu NSPs)
#
# Produces:
#   build/switchu-daemon/switchu-daemon.nsp
#   build/switchu-menu/switchu-menu.nsp
#
# Usage:
#   make              — build both NSPs (release, deko3d backend)
#   make daemon       — build daemon NSP only
#   make menu         — build menu NSP only
#   make clean        — remove build directory
#   make BACKEND=sdl2 — build with SDL2 backend instead of deko3d
# ──────────────────────────────────────────────────────────────────────────────
.SUFFIXES:
.PHONY: all daemon menu shaders clean

# ── Environment ───────────────────────────────────────────────────────────────
ifeq ($(strip $(DEVKITPRO)),)
$(error "Please set DEVKITPRO in your environment. export DEVKITPRO=<path to>/devkitpro")
endif

DEVKITA64	:= $(DEVKITPRO)/devkitA64
LIBNX		:= $(DEVKITPRO)/libnx
PORTLIBS	:= $(DEVKITPRO)/portlibs/switch
DKPTOOLS	:= $(DEVKITPRO)/tools/bin

PREFIX		:= $(DEVKITA64)/bin/aarch64-none-elf-
CC			:= $(PREFIX)gcc
CXX			:= $(PREFIX)g++
AR			:= $(PREFIX)gcc-ar

# ── Backend selection (deko3d or sdl2) ────────────────────────────────────────
BACKEND		?= deko3d

# ── Directories ───────────────────────────────────────────────────────────────
ROOTDIR		:= $(CURDIR)
BUILDDIR	:= $(ROOTDIR)/build

# Per-target output dirs
DAEMON_DIR	:= $(BUILDDIR)/switchu-daemon
MENU_DIR	:= $(BUILDDIR)/switchu-menu
NXTC_DIR	:= $(BUILDDIR)/libnxtc
NXUI_DIR	:= $(BUILDDIR)/libnxui

# ── Architecture flags ────────────────────────────────────────────────────────
ARCH		:= -march=armv8-a+crc+crypto -mtune=cortex-a57 -mtp=soft -fPIE

COMMON_FLAGS	:= $(ARCH) -D__SWITCH__ -ffunction-sections -fdata-sections \
				   -I$(LIBNX)/include -I$(PORTLIBS)/include

CFLAGS		:= $(COMMON_FLAGS) -O3 -flto=auto -ffast-math
CXXFLAGS	:= $(CFLAGS) -std=c++20 -fno-rtti -fexceptions

ASFLAGS		:= -g $(ARCH)

LDFLAGS		:= -specs=$(LIBNX)/switch.specs $(ARCH) \
			   -Wl,--gc-sections -flto=auto \
			   -L$(LIBNX)/lib -L$(PORTLIBS)/lib

# ── Version ───────────────────────────────────────────────────────────────────
VERSION		:= 1.0.1

# ══════════════════════════════════════════════════════════════════════════════
# Source lists
# ══════════════════════════════════════════════════════════════════════════════

# ── libnxtc (C static library) ───────────────────────────────────────────────
NXTC_SRCS	:= $(wildcard lib/libnxtc/source/*.c)
NXTC_OBJS	:= $(patsubst lib/libnxtc/source/%.c,$(NXTC_DIR)/%.o,$(NXTC_SRCS))
NXTC_LIB	:= $(NXTC_DIR)/libnxtc.a

# ── libnxui (C++ static library) ─────────────────────────────────────────────
NXUI_CORE_SRCS	:= $(filter-out %_sdl2.cpp,$(wildcard lib/nxui/src/core/*.cpp))
ifeq ($(BACKEND),sdl2)
NXUI_CORE_SRCS	:= $(filter-out lib/nxui/src/core/GpuDevice.cpp lib/nxui/src/core/Renderer.cpp lib/nxui/src/core/Texture.cpp,\
                    $(wildcard lib/nxui/src/core/*.cpp))
NXUI_DEFINES	:= -DNXUI_BACKEND_SDL2
else
NXUI_DEFINES	:= -DNXUI_BACKEND_DEKO3D
endif
NXUI_SRCS		:= $(NXUI_CORE_SRCS) \
				   $(wildcard lib/nxui/src/widgets/*.cpp) \
				   $(wildcard lib/nxui/src/focus/*.cpp)
NXUI_OBJS		:= $(patsubst lib/nxui/src/%.cpp,$(NXUI_DIR)/%.o,$(NXUI_SRCS))
NXUI_LIB		:= $(NXUI_DIR)/libnxui.a

NXUI_INCLUDES	:= -Ilib/nxui/include -Ilib/nxui/include/nxui/third_party/stb

# ── switchu-daemon ────────────────────────────────────────────────────────────
DAEMON_SRCS		:= projects/daemon/src/main.cpp
DAEMON_OBJS		:= $(patsubst projects/daemon/src/%.cpp,$(DAEMON_DIR)/%.o,$(DAEMON_SRCS))
DAEMON_ELF		:= $(DAEMON_DIR)/switchu-daemon.elf
DAEMON_NSP		:= $(DAEMON_DIR)/switchu-daemon.nsp

DAEMON_INCLUDES	:= -Iprojects/common/include -Ilib/libnxtc/include
DAEMON_DEFINES	:=
DAEMON_LIBS		:= -L$(NXTC_DIR) -lnxtc -lz -lnx

# ── switchu-menu ──────────────────────────────────────────────────────────────
MENU_SRCS		:= projects/menu/src/main.cpp \
				   $(wildcard projects/menu/src/core/*.cpp) \
				   $(wildcard projects/menu/src/widgets/*.cpp) \
				   $(wildcard projects/menu/src/settings/*.cpp) \
				   $(wildcard projects/menu/src/settings/tabs/*.cpp) \
				   $(wildcard projects/menu/src/sidebar/*.cpp) \
				   $(wildcard projects/menu/src/launcher/*.cpp) \
				   $(wildcard projects/menu/src/bluetooth/*.cpp)
MENU_OBJS		:= $(patsubst projects/menu/src/%.cpp,$(MENU_DIR)/%.o,$(MENU_SRCS))
MENU_ELF		:= $(MENU_DIR)/switchu-menu.elf
MENU_NSP		:= $(MENU_DIR)/switchu-menu.nsp

MENU_INCLUDES	:= -Iprojects/common/include -Iprojects/menu/src \
				   -Ilib/libnxtc/include $(NXUI_INCLUDES)
MENU_DEFINES	:= -DSWITCHU_MENU -DSWITCHU_VERSION=\"$(VERSION)\" $(NXUI_DEFINES)

# Menu link libraries (order matters for static linking)
ifeq ($(BACKEND),sdl2)
MENU_LIBS		:= -L$(NXUI_DIR) -lnxui -L$(NXTC_DIR) -lnxtc \
				   -lSDL2_mixer -lSDL2_ttf -lSDL2 \
				   -lmpg123 -lvorbisidec -lmodplug -lopusfile -lopus -logg \
				   -lharfbuzz -lfreetype -lbz2 -lpng16 \
				   -lwebpdemux -lwebp \
				   -lEGL -lglapi -ldrm_nouveau \
				   -lz -lnx -lm
else
MENU_LIBS		:= -L$(NXUI_DIR) -lnxui -L$(NXTC_DIR) -lnxtc \
				   -lSDL2_mixer -lSDL2_ttf -lSDL2 \
				   -ldeko3d \
				   -lmpg123 -lvorbisidec -lmodplug -lopusfile -lopus -logg \
				   -lharfbuzz -lfreetype -lbz2 -lpng16 \
				   -lwebpdemux -lwebp \
				   -lEGL -lglapi -ldrm_nouveau \
				   -lz -lnx -lm
endif

# ── Shaders ───────────────────────────────────────────────────────────────────
SHADER_SRCS		:= $(wildcard shaders/*.glsl)
SHADER_OUTS		:= $(patsubst shaders/%.glsl,romfs/shaders/%.dksh,$(SHADER_SRCS))
UAM				:= $(DKPTOOLS)/uam

# ══════════════════════════════════════════════════════════════════════════════
# Top-level targets
# ══════════════════════════════════════════════════════════════════════════════

all: daemon menu

daemon: $(DAEMON_NSP)
	@echo "  =>  $(DAEMON_NSP)"

menu: shaders $(MENU_NSP)
	@echo "  =>  $(MENU_NSP)"

shaders: $(SHADER_OUTS)

clean:
	@echo "  CLEAN"
	@rm -rf $(BUILDDIR)

# ══════════════════════════════════════════════════════════════════════════════
# Shader compilation (GLSL → DKSH via uam)
# ══════════════════════════════════════════════════════════════════════════════

romfs/shaders/%_vsh.dksh: shaders/%_vsh.glsl
	@echo "  UAM   $(notdir $<)"
	@mkdir -p $(dir $@)
	@$(UAM) -s vert -o $@ $<

romfs/shaders/%_fsh.dksh: shaders/%_fsh.glsl
	@echo "  UAM   $(notdir $<)"
	@mkdir -p $(dir $@)
	@$(UAM) -s frag -o $@ $<

# ══════════════════════════════════════════════════════════════════════════════
# libnxtc
# ══════════════════════════════════════════════════════════════════════════════

$(NXTC_LIB): $(NXTC_OBJS)
	@echo "  AR    $(notdir $@)"
	@mkdir -p $(dir $@)
	@$(AR) rcs $@ $^

$(NXTC_DIR)/%.o: lib/libnxtc/source/%.c
	@echo "  CC    $(notdir $<)"
	@mkdir -p $(dir $@)
	@$(CC) -MMD -MP $(CFLAGS) -Ilib/libnxtc/include -c $< -o $@

# ══════════════════════════════════════════════════════════════════════════════
# libnxui
# ══════════════════════════════════════════════════════════════════════════════

$(NXUI_LIB): $(NXUI_OBJS)
	@echo "  AR    $(notdir $@)"
	@mkdir -p $(dir $@)
	@$(AR) rcs $@ $^

$(NXUI_DIR)/%.o: lib/nxui/src/%.cpp
	@echo "  CXX   $(notdir $<)"
	@mkdir -p $(dir $@)
	@$(CXX) -MMD -MP $(CXXFLAGS) $(NXUI_INCLUDES) $(NXUI_DEFINES) -c $< -o $@

# ══════════════════════════════════════════════════════════════════════════════
# switchu-daemon
# ══════════════════════════════════════════════════════════════════════════════

$(DAEMON_NSP): $(DAEMON_ELF) projects/daemon/daemon.json
	@echo "  NSO   switchu-daemon"
	@mkdir -p $(DAEMON_DIR)/exefs
	@$(DKPTOOLS)/elf2nso $(DAEMON_ELF) $(DAEMON_DIR)/exefs/main
	@echo "  NPDM  switchu-daemon"
	@$(DKPTOOLS)/npdmtool projects/daemon/daemon.json $(DAEMON_DIR)/exefs/main.npdm
	@echo "  PFS0  $(notdir $@)"
	@$(DKPTOOLS)/build_pfs0 $(DAEMON_DIR)/exefs $@

$(DAEMON_ELF): $(DAEMON_OBJS) $(NXTC_LIB)
	@echo "  LD    $(notdir $@)"
	@$(CXX) $(LDFLAGS) -o $@ $(DAEMON_OBJS) $(DAEMON_LIBS)

$(DAEMON_DIR)/%.o: projects/daemon/src/%.cpp
	@echo "  CXX   daemon/$(notdir $<)"
	@mkdir -p $(dir $@)
	@$(CXX) -MMD -MP $(CXXFLAGS) $(DAEMON_INCLUDES) $(DAEMON_DEFINES) -c $< -o $@

# ══════════════════════════════════════════════════════════════════════════════
# switchu-menu
# ══════════════════════════════════════════════════════════════════════════════

$(MENU_NSP): $(MENU_ELF) projects/menu/menu.json
	@echo "  NSO   switchu-menu"
	@mkdir -p $(MENU_DIR)/exefs
	@$(DKPTOOLS)/elf2nso $(MENU_ELF) $(MENU_DIR)/exefs/main
	@echo "  NPDM  switchu-menu"
	@$(DKPTOOLS)/npdmtool projects/menu/menu.json $(MENU_DIR)/exefs/main.npdm
	@echo "  PFS0  $(notdir $@)"
	@$(DKPTOOLS)/build_pfs0 $(MENU_DIR)/exefs $@

$(MENU_ELF): $(MENU_OBJS) $(NXUI_LIB) $(NXTC_LIB)
	@echo "  LD    $(notdir $@)"
	@$(CXX) $(LDFLAGS) -o $@ $(MENU_OBJS) $(MENU_LIBS)

$(MENU_DIR)/%.o: projects/menu/src/%.cpp
	@echo "  CXX   menu/$(notdir $<)"
	@mkdir -p $(dir $@)
	@$(CXX) -MMD -MP $(CXXFLAGS) $(MENU_INCLUDES) $(MENU_DEFINES) -c $< -o $@

# ══════════════════════════════════════════════════════════════════════════════
# Auto-dependencies
# ══════════════════════════════════════════════════════════════════════════════

-include $(NXTC_OBJS:.o=.d)
-include $(NXUI_OBJS:.o=.d)
-include $(DAEMON_OBJS:.o=.d)
-include $(MENU_OBJS:.o=.d)

# =============================================================================
# Vita Dreamcast Emulator — Build System
# =============================================================================
# Requires VitaSDK: https://vitasdk.org
#   export VITASDK=/usr/local/vitasdk
#   export PATH="$VITASDK/bin:$PATH"
#
# Build:  make -j$(nproc)
# Output: dc-vita.vpk  (install with VitaShell)
# =============================================================================

VITASDK ?= /usr/local/vitasdk
export VITASDK

PREFIX  := $(VITASDK)/bin/arm-vita-eabi-
CC      := $(PREFIX)gcc
STRIP   := $(PREFIX)strip
OBJCOPY := $(PREFIX)objcopy

TARGET  := dc-vita
ELF     := $(TARGET).elf
VELF    := $(TARGET).velf
VPK     := $(TARGET).vpk
SFO     := sce_sys/param.sfo

BUILD   := build
SRC     := src

# -O2 is the sweet spot for Cortex-A9; -Os would hurt the interpreter badly.
# We explicitly avoid -ffast-math: Dreamcast games rely on IEEE FP behavior.
CFLAGS  := -std=c11 -O2 -pipe -g0 \
           -Wall -Wextra -Wno-unused-parameter -Wno-sign-compare \
           -fno-strict-aliasing -fomit-frame-pointer \
           -ffunction-sections -fdata-sections \
           -march=armv7-a -mfpu=neon -mfloat-abi=hard \
           -I$(SRC) \
           -I$(VITASDK)/arm-vita-eabi/include

LDFLAGS := -Wl,-q,--gc-sections -Wl,-O1

# Only pull the stubs we actually reference; keep the import table small.
LIBS    := -lSceDisplay_stub \
           -lSceAudio_stub \
           -lSceCtrl_stub \
           -lScePower_stub \
           -lSceSysmodule_stub \
           -lSceKernelThreadMgr_stub \
           -lSceProcessmgr_stub \
           -lSceAppMgr_stub \
           -lSceCommonDialog_stub \
           -lm

SOURCES := \
  src/vita/vita_main.c \
  src/vita/vita_platform.c \
  src/vita/vita_audio.c \
  src/vita/vita_video.c \
  src/vita/vita_input.c \
  src/vita/vita_power.c \
  src/core/dc.c \
  src/core/memory.c \
  src/core/scheduler.c \
  src/sh4/sh4.c \
  src/sh4/sh4_interpreter.c \
  src/sh4/sh4_dynarec.c \
  src/pvr2/pvr2.c \
  src/aica/aica.c \
  src/gdrom/gdrom.c \
  src/maple/maple.c \
  src/util/log.c \
  src/util/profiler.c

OBJS := $(patsubst src/%.c,$(BUILD)/%.o,$(SOURCES))
DEPS := $(OBJS:.o=.d)

.PHONY: all clean size

all: $(VPK)

$(BUILD)/%.o: src/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -MMD -MP -c $< -o $@

$(ELF): $(OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) $^ $(LIBS) -o $@
	@$(STRIP) $@ -o $@.stripped 2>/dev/null || true
	@mv -f $@.stripped $@ 2>/dev/null || true

$(VELF): $(ELF)
	vita-elf-create $< $@

$(SFO):
	@mkdir -p sce_sys
	vita-mksfoex -s TITLE_ID=DCVITA0001 -d ATTRIBUTE2=12 "$(TARGET)" $@

$(VPK): $(VELF) $(SFO) assets/.keep
	vita-make-fself -s $(VELF) eboot.bin
	vita-pack-vpk -s $(SFO) -b eboot.bin -a assets/=assets $@

assets/.keep:
	@mkdir -p assets
	@touch $@

size: $(ELF)
	@$(PREFIX)size $(ELF) 2>/dev/null || size $(ELF)

clean:
	rm -rf $(BUILD) $(ELF) $(VELF) $(VPK) eboot.bin $(SFO)

-include $(DEPS)

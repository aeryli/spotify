# Target Name
TARGET      := 3ds_spotify
TOPDIR      := $(CURDIR)

# Set devkitPro environment paths with defaults
DEVKITPRO   ?= /opt/devkitpro
DEVKITARM   ?= $(DEVKITPRO)/devkitARM
CTRULIB     ?= $(DEVKITPRO)/libctru
PORTLIBS    ?= $(DEVKITPRO)/portlibs/3ds

# Ensure devkitARM toolchain binaries are targeted
PREFIX      := arm-none-eabi-
CC          := $(DEVKITARM)/bin/$(PREFIX)gcc
CXX         := $(DEVKITARM)/bin/$(PREFIX)g++
LD          := $(DEVKITARM)/bin/$(PREFIX)g++
AR          := $(DEVKITARM)/bin/$(PREFIX)ar

# Directories
BUILD       := build
SOURCES     := sources

# Application Details
APP_TITLE   := 3ds spotify!!
APP_AUTHOR  := User
APP_ICON    := $(CTRULIB)/default_icon.png

# Compiler Options
ARCH        := -march=armv6k -mtune=mpcore -mfloat-abi=hard -mtp=soft
CFLAGS      := -Wall -O2 -mword-relocations $(ARCH)
CXXFLAGS    := $(CFLAGS) -fno-rtti -fno-exceptions

# Included Libraries & Specs
SPECS       := -specs=3dsx.specs
LIBS        := -lvorbisidec -logg -lctru -lm
LIBDIRS     := $(CTRULIB) $(PORTLIBS)

ifneq ($(BUILD),$(notdir $(CURDIR)))

export OUTPUT   := $(CURDIR)/$(TARGET)
export VPATH    := $(foreach dir,$(SOURCES),$(CURDIR)/$(dir))
export DEPSDIR  := $(CURDIR)/$(BUILD)

CFILES      := $(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.c)))
CPPFILES    := $(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.cpp)))
export OBJS := $(addsuffix .o,$(basename $(CFILES))) $(addsuffix .o,$(basename $(CPPFILES)))

export CC CXX LD
export LDFLAGS  := $(SPECS) $(ARCH) -Wl,-Map,$(TARGET).map $(foreach dir,$(LIBDIRS),-L$(dir)/lib)
export INCLUDE  := $(foreach dir,$(LIBDIRS),-I$(dir)/include) -I$(CURDIR)/$(BUILD)

.PHONY: all clean

all: $(BUILD)
	@$(MAKE) --no-print-directory -C $(BUILD) -f $(CURDIR)/Makefile

$(BUILD):
	@mkdir -p $@

clean:
	@rm -rf $(BUILD) $(TARGET).3dsx $(TARGET).elf

else

DEPENDS := $(OBJS:.o=.d)

all: $(OUTPUT).3dsx

$(OUTPUT).3dsx: $(OUTPUT).elf
	@3dsxtool $< $@
	@echo "Built $@"

$(OUTPUT).elf: $(OBJS)
	@echo "Linking $@"
	$(LD) $(OBJS) $(LDFLAGS) $(LIBS) -o $@

-include $(DEPENDS)

%.o: %.cpp
	$(CXX) -MMD -MP -MF $(DEPSDIR)/$*.d $(CXXFLAGS) $(INCLUDE) -c $< -o $@

%.o: %.c
	$(CC) -MMD -MP -MF $(DEPSDIR)/$*.d $(CFLAGS) $(INCLUDE) -c $< -o $@

endif
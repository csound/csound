APP_TYPE = BOOT_QSPI
LDFLAGS += -u _printf_float

# Project Name
TARGET = daisyCsoundMidi

# Sources
CPP_SOURCES = daisyCsoundMidi.cpp

# Library Locations
LIBDAISY_DIR = ../../libDaisy/
DAISYSP_DIR = ../../DaisySP/

# Csound Library and Include Locations
CSOUND_INCLUDE_DIR = ../../include/csound
CSOUND_LIB_DIR     = ../../lib

# Full path to the Csound static library
CSOUND_STATIC_LIB = $(CSOUND_LIB_DIR)/libcsound.a

# Add to the existing flags
C_INCLUDES += -I$(CSOUND_INCLUDE_DIR)
LIBS += $(CSOUND_STATIC_LIB)
LIBDIR  += -L$(CSOUND_LIB_DIR)

# Use Bootloader v5.4
# BOOT_BIN = $(shell pwd)/../dsy_bootloader_v5_4.bin
BOOT_BIN = ../dsy_bootloader_v5_4.bin

# Use Custom Linker Script
# LDSCRIPT = $(shell pwd)/../STM32H750IB_qspi_custom.lds
LDSCRIPT = ../STM32H750IB_qspi_custom.lds

# Core location, and generic Makefile.
SYSTEM_FILES_DIR = $(LIBDAISY_DIR)/core
include $(SYSTEM_FILES_DIR)/Makefile

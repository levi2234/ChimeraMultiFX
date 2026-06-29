# =============================================================================
# ChimeraMultiFX — Daisy Seed Firmware Makefile
# =============================================================================
# Usage:
#   make           — compile
#   make program-dfu — flash via USB DFU (hold BOOT then tap RESET first)
#   make program   — flash via ST-Link (SWD)
#   make clean     — remove build artefacts
#
# Select the active effect by setting CPP_SOURCES below.
# =============================================================================

# Project Name
TARGET = ChimeraFX

# Sources — single entry point; effects are header-only classes
CPP_SOURCES = main.cpp

# Library Locations
LIBDAISY_DIR = deps/libDaisy
DAISYSP_DIR  = deps/DaisySP

# Core location, and generic makefile.
SYSTEM_FILES_DIR = $(LIBDAISY_DIR)/core
include $(SYSTEM_FILES_DIR)/Makefile

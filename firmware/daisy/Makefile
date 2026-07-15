# =============================================================================
# ChimeraMultiFX — Daisy Seed Firmware Makefile
# =============================================================================
# Usage:
#   make              — compile the QSPI application
#   make program-boot — install the Daisy bootloader from STM DFU (one time)
#   make program-dfu  — upload the application during the bootloader DFU window
#   make clean        — remove build artefacts
#
# Select the active effect by setting CPP_SOURCES below.
# =============================================================================

# Project Name
TARGET = ChimeraFX

# Store and execute the application from the Daisy Seed's external QSPI flash.
# The Daisy bootloader must be installed in internal flash before programming.
APP_TYPE = BOOT_QSPI

# Keep code compact and reduce QSPI instruction fetches.
OPT ?= -Os

# Sources — single entry point; effects are header-only classes
CPP_SOURCES = main.cpp

# Project header roots
C_INCLUDES += -I. -Iinclude

# Library Locations
LIBDAISY_DIR = deps/libDaisy
DAISYSP_DIR  = deps/DaisySP

# Core location, and generic makefile.
SYSTEM_FILES_DIR = $(LIBDAISY_DIR)/core
include $(SYSTEM_FILES_DIR)/Makefile

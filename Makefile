.DEFAULT_GOAL := all
ifeq (${MAKECMDGOALS},)
MAKECMDGOALS := all
endif

export ARM_ARCH = armv6-m
export ARCH_CROSS = cortex-m0plus

export CHIP_TYPE ?= rp2040
export BOARD_TYPE ?= pi-pico

export BUILD_TYPE ?= debug
export PROJECT_ROOT ?= ${CURDIR}
export OUTPUT_ROOT ?= ${PROJECT_ROOT}/build
export TOOLS_ROOT ?= ${PROJECT_ROOT}/tools
export PREFIX ?= ${PROJECT_ROOT}/images

export INSTALL_ROOT ?= ${PREFIX}$(if ${BOARD_TYPE},/${BOARD_TYPE},)$(if ${BUILD_TYPE},/${BUILD_TYPE},)
export BUILD_ROOT ?= ${OUTPUT_ROOT}$(if ${BOARD_TYPE},/${BOARD_TYPE},)$(if ${BUILD_TYPE},/${BUILD_TYPE},)

ifeq (${V},)
SILENT=--silent
endif

MAKEFLAGS += ${SILENT}

ifeq (${MAKECMDGOAL},setup)
include ${TOOLS_ROOT}/makefiles/setup.mk
else
include ${TOOLS_ROOT}/makefiles/tree.mk

SUBDIRS := $(filter-out, host-tools, ${SUBDIRS})

test: init board diag cmsis sys diag bootstrap lib hal rtos
target: test

#apps: init board diag cmsis sys diag bootstrap lib hal rtos
#target: test apps

endif

distclean:
	@echo "DISTCLEAN ${BUILD_ROOT} ${INSTALL_ROOT}"
	-${RM} -r ${BUILD_ROOT} ${INSTALL_ROOT}

realclean:
	-${RM} -r ${OUTPUT_ROOT} ${PREFIX} 

info:
	@echo "ARM_ARCH=${ARM_ARCH}"
	@echo "ARCH_CROSS=${ARCH_CROSS}"
	@echo "BOARD_TYPE=${BOARD_TYPE}"
	@echo "BUILD_TYPE=${BUILD_TYPE}"
	@echo "PROJECT_ROOT=${PROJECT_ROOT}"
	@echo "OUTPUT_ROOT=${OUTPUT_ROOT}"
	@echo "TOOLS_ROOT=${TOOLS_ROOT}"
	@echo "PREFIX=${PREFIX}"
	@echo "BUILD_ROOT=${BUILD_ROOT}"
	@echo "INSTALL_ROOT=${INSTALL_ROOT}"
.PHONY: info

help:
	@echo "all           Build everything"
	@echo "setup         Setup host build tools"
	@echo "clean         Run the clean action of all projects"
	@echo "distclean     Delete all build artifacts and qualified directories"
	@echo "realclean     Delete all build artifacts, directories and external repositories"
	@echo "info          Display build configuration"
.PHONY: help

${V}.SILENT:

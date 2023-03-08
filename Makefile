#export ARCH=arm
#export ARCH_CROSS=cortexa9t2hf
#export BUILD_TYPE ?= debug

export PROJECT_ROOT ?= ${CURDIR}
export OUTPUT_ROOT ?= ${PROJECT_ROOT}/build
export TOOLS_ROOT ?= ${PROJECT_ROOT}/tools
export PREFIX ?= ${PROJECT_ROOT}/rootfs

export INSTALL_ROOT := ${PREFIX}$(if ${ARCH},/${ARCH},)$(if ${BUILD_TYPE},/${BUILD_TYPE},)
export BUILD_ROOT := ${OUTPUT_ROOT}$(if ${ARCH},/${ARCH},)$(if ${BUILD_TYPE},/${BUILD_TYPE},)

MAKEFLAGS += --jobs=$(shell grep -c ^processor /proc/cpuinfo)

include ${TOOLS_ROOT}/makefiles/tree.mk

target: example

distclean:
	@echo "DISTCLEAN ${BUILD_ROOT} ${INSTALL_ROOT}"
	-${RM} -r ${BUILD_ROOT} ${INSTALL_ROOT}

realclean:
	-${RM} -r ${OUTPUT_ROOT} ${PREFIX} 

info:
	@echo "ARCH=${ARCH}"
	@echo "ARCH_CROSS=${ARCH_CROSS}"
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
	@echo "clean         Run the clean action of all projects"
	@echo "distclean     Delete all build artifacts and qualified directories"
	@echo "realclean     Delete all build artifacts, directories and external repositories"
	@echo "info          Display build configuration"
.PHONY: help

${V}.SILENT:

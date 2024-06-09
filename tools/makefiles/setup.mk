GITHUB_ACCOUNT ?= sgstreet

PICO_SDK_REPO := pico-sdk
PICO_SDK_BRANCH ?= master
PICO_SDK_PATH := ${PROJECT_ROOT}/tmp/${PICO_SDK_REPO}
PICO_SDK_URL := https://github.com/raspberrypi/${PICO_SDK_REPO}.git
PICO_SDK_TARGET := ${PICO_SDK_PATH}/CMakeLists.txt

SVDCONV_VERSION ?= 3.3.46
SVDCONV_ARCHIVE := svdconv-${SVDCONV_VERSION}-linux-amd64.tbz2
SVDCONV_PATH := ${PROJECT_ROOT}/tmp/
SVDCONV_URL := https://github.com/Open-CMSIS-Pack/devtools/releases/download/tools/svdconv/${SVDCONV_VERSION}/${SVDCONV_ARCHIVE}
SVDCONV_TARGET := ${PROJECT_ROOT}/local/bin/svdconv

COMPILER_VERSION ?= v0.1.0
COMPILER_ARCHIVE := gcc-arm-none-eabi-12-2-0.tar.xz
COMPILER_PATH := ${PROJECT_ROOT}/tmp
COMPILER_URL := https://github.com/red-rocket-computing/cross-dev/releases/download/${COMPILER_VERSION}/${COMPILER_ARCHIVE}
COMPILER_TARGET := ${PROJECT_ROOT}/local/bin/arm-none-eabi-gcc

SETUP_TARGETS := ${PICO_SDK_TARGET} ${SVDCONV_TARGET} ${COMPILER_TARGET}
SETUP_PATHS := ${PICO_SDK_PATH} ${SVDCONV_PATH} ${COMPILER_PATH}

include ${PROJECT_ROOT}/tools/makefiles/common.mk

${PICO_SDK_TARGET}:
	@echo "FETCHING ${PICO_SDK_URL}"
	git clone -b ${PICO_SDK_BRANCH} ${PICO_SDK_URL} ${PICO_SDK_PATH}
	git -C ${PICO_SDK_PATH} submodule update --init
	[ -d ${PICO_SDK_PATH}/build ] || mkdir -p ${PICO_SDK_PATH}/build
	@echo "BUILDING ${PICO_SKD_REPO}"
	cd ${PICO_SDK_PATH}/build && cmake ..
	cd ${PICO_SDK_PATH}/build && make ELF2UF2Build PioasmBuild
	[ -d ${PROJECT_ROOT}/local/bin ] || mkdir -p ${PROJECT_ROOT}/local/bin
	install ${PICO_SDK_PATH}/build/elf2uf2/elf2uf2 ${PROJECT_ROOT}/local/bin/elf2uf2
	install ${PICO_SDK_PATH}/build/pioasm/pioasm ${PROJECT_ROOT}/local/bin/pioasm

${SVDCONV_TARGET}:
	@echo "DOWNLOADING ${SVDCONV_URL}"
	[ -d ${SVDCONV_PATH} ] || mkdir -p ${SVDCONV_PATH}
	cd ${SVDCONV_PATH} && wget -nv ${SVDCONV_URL}
	@echo "INSTALLING ${@}"
	[ -d ${PROJECT_ROOT}/local/bin ] || mkdir -p ${PROJECT_ROOT}/local/bin
	tar -C ${PROJECT_ROOT}/local/bin -xf ${SVDCONV_PATH}/${SVDCONV_ARCHIVE}

${COMPILER_TARGET}:
	@echo "DOWNLOADING ${COMPILER_URL}"
	[ -d ${COMPILER_PATH} ] || mkdir -p ${COMPILER_PATH}
	cd ${COMPILER_PATH} && wget -nv ${COMPILER_URL}
	@echo "INSTALLING ${@}"
	[ -d ${PROJECT_ROOT}/local ] || mkdir -p ${PROJECT_ROOT}/local
	tar -C ${PROJECT_ROOT}/local --strip-components=1 -xf ${COMPILER_PATH}/${COMPILER_ARCHIVE}

setup-realclean:
	@echo "REMOVING ${SETUP_PATHS}"
	-$(RM) -rf ${SETUP_PATHS}
.PHONY: setup-realclean

setup-tools: ${SETUP_TARGETS}
	@:
.PHONY: setup-tools

host-tools:
	@echo "ENTERING $@"
	+${MAKE} --no-print-directory -C $@ -f $@.mk all
.PHONY: host-tools

setup: setup-tools
	@:
.PHONY: setup


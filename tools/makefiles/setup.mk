GITHUB_ACCOUNT ?= sgstreet

PICO_SDK_REPO := pico-sdk
PICO_SDK_BRANCH ?= master
PICO_SDK_PATH := ${PROJECT_ROOT}/tmp/${PICO_SDK_REPO}
PICO_SDK_URL := https://github.com/raspberrypi/${PICO_SDK_REPO}.git
PICO_SDK_TARGET := ${PICO_SDK_PATH}/CMakeLists.txt

#https://github.com/Open-CMSIS-Pack/devtools/releases/download/tools/svdconv/3.3.44/svdconv-3.3.44-linux64-amd64.tbz2

SVDCONV_VERSION ?= 3.3.45
SVDCONV_ARCHIVE := svdconv-${SVDCONV_VERSION}-linux64-amd64.tbz2
SVDCONV_PATH := ${PROJECT_ROOT}/tmp/
SVDCONV_URL := https://github.com/Open-CMSIS-Pack/devtools/releases/download/tools/svdconv/${SVDCONV_VERSION}/${SVDCONV_ARCHIVE}
SVDCONV_TARGET := ${PROJECT_ROOT}/local/bin/svdconv

SETUP_TARGETS := ${PICO_SDK_TARGET} ${SVDCONV_TARGET}
SETUP_PATHS := ${PICO_SDK_PATH} ${SVDCONV_PATH}

include ${PROJECT_ROOT}/tools/makefiles/common.mk

${PICO_SDK_TARGET}:
	@echo "FETCHING ${PICO_SDK_URL}"
	git clone -b ${PICO_SDK_BRANCH} ${PICO_SDK_URL} ${PICO_SDK_PATH}
	git -C ${PICO_SDK_PATH} submodule update --init
	[ -d ${PICO_SDK_PATH}/build ] || mkdir -p ${PICO_SDK_PATH}/build
	@echo "BUILDING ${PICO_SKD_REPO}"
	cd ${PICO_SDK_PATH}/build && cmake ..
	cd ${PICO_SDK_PATH}/build && make ELF2UF2Build PioasmBuild
	install ${PICO_SDK_PATH}/build/elf2uf2/elf2uf2 ${PROJECT_ROOT}/local/bin/elf2uf2
	install ${PICO_SDK_PATH}/build/pioasm/pioasm ${PROJECT_ROOT}/local/bin/pioasm

${SVDCONV_TARGET}:
	@echo "DOWNLOADING ${SVDCONV_URL}"
	[ -d ${SVDCONV_PATH} ] || mkdir -p ${SVDCONV_PATH}
	cd ${SVDCONV_PATH} && wget -nv ${SVDCONV_URL}
	@echo "INSTALLING ${@}"
	[ -d ${PROJECT_ROOT}/local/bin ] || mkdir -p ${PROJECT_ROOT}/local/bin
	tar -C  ${PROJECT_ROOT}/local/bin -xf ${SVDCONV_PATH}/${SVDCONV_ARCHIVE}

setup-realclean:
	@echo "REMOVING ${SETUP_PATHS}"
	-$(RM) -rf ${SETUP_PATHS}
.PHONY: setup-realclean

setup-tools: ${SETUP_TARGETS}
	@:
# 	$(Q) $(MAKE) --no-print-directory -C ${PROJECT_ROOT}/host-tools -f host-tools.mk all
# .PHONY: setup-tools

host-tools:
	@echo "ENTERING $@"
	+${MAKE} --no-print-directory -C $@ -f $@.mk all
.PHONY: host-tools

setup: setup-tools host-tools
	@:
.PHONY: setup


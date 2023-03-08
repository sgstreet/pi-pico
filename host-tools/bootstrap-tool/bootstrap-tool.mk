ARCH_CROSS := host
BOARD_TYPE := host
BUILD_ROOT := ${OUTPUT_ROOT}$(if ${BOARD_TYPE},/${BOARD_TYPE},)$(if ${BUILD_TYPE},/${BUILD_TYPE},)
INSTALL_ROOT := ${PROJECT_ROOT}/local/bin
BUILD_TYPE := debug

ifeq ($(findstring ${BUILD_ROOT},${CURDIR}),)

include ${PROJECT_ROOT}/tools/makefiles/target.mk

else

EXTRA_DEPS := ${SOURCE_DIR}/bootstrap-tool.mk
EXTRA_CLEAN := ${INSTALL_ROOT}/bootstrap-tool

include ${PROJECT_ROOT}/tools/makefiles/project.mk

CFLAGS := -pipe -O0 -g3 -fno-move-loop-invariants -feliminate-unused-debug-types -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -Wall -Wunused -Wuninitialized -Wmissing-declarations -Werror -std=gnu11 -funwind-tables

all: ${INSTALL_ROOT}/bootstrap-tool

${INSTALL_ROOT}/bootstrap-tool: ${CURDIR}/bootstrap-tool.elf
	@echo "INSTALLING ${@}"
	$(INSTALL) -m 660 -D ${<} ${@}

endif

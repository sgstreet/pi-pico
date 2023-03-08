ARCH_CROSS := host
BOARD_TYPE := host
BUILD_ROOT := ${OUTPUT_ROOT}$(if ${BOARD_TYPE},/${BOARD_TYPE},)$(if ${BUILD_TYPE},/${BUILD_TYPE},)
INSTALL_ROOT := ${PROJECT_ROOT}/local/bin
BUILD_TYPE := debug

ifeq ($(findstring ${BUILD_ROOT},${CURDIR}),)

include ${PROJECT_ROOT}/tools/makefiles/target.mk

else

EXTRA_DEPS := ${SOURCE_DIR}/rtos-toolkit-jlink-plugin.mk
EXTRA_CLEAN := ${INSTALL_ROOT}/rtos-toolkit-jlink-plugin.so ${CURDIR}/rtos-toolkit-jlink-plugin.so

include ${PROJECT_ROOT}/tools/makefiles/project.mk

CFLAGS += -fPIC
LDFLAGS += -fPIC
LDLIBS += -lpthread -lrt -ldl

all: ${INSTALL_ROOT}/rtos-toolkit-jlink-plugin.so
	@:

${INSTALL_ROOT}/rtos-toolkit-jlink-plugin.so: ${CURDIR}/rtos-toolkit-jlink-plugin.so
	@echo "INSTALL ${@}"
	$(INSTALL) -D ${CURDIR}/rtos-toolkit-jlink-plugin.so ${@}

endif

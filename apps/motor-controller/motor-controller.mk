ifeq ($(findstring ${BUILD_ROOT},${CURDIR}),)
include ${PROJECT_ROOT}/tools/makefiles/target.mk
else

EXTRA_DEPS := ${SOURCE_DIR}/motor-controller.mk ${PROJECT_ROOT}/ldscripts/sections.ld ${PROJECT_ROOT}/ldscripts/memory.ld ${PROJECT_ROOT}/ldscripts/regions-sram.ld ${PROJECT_ROOT}/ldscripts/regions-flash.ld
EXTRA_CLEAN := ${INSTALL_ROOT}/motor-controller.bin ${INSTALL_ROOT}/motor-controller.elf ${INSTALL_ROOT}/motor-controller.uf2

TARGET_OBJ_LIBS := bootstrap
TARGET_OBJ_LIBS += board board/${BOARD_TYPE}
TARGET_OBJ_LIBS += diag diag/board
TARGET_OBJ_LIBS += sys
TARGET_OBJ_LIBS += cmsis/device/rp2040
TARGET_OBJ_LIBS += init
TARGET_OBJ_LIBS += lib
TARGET_OBJ_LIBS += hardware hardware/${CHIP_TYPE}
TARGET_OBJ_LIBS += rtos/rtos-toolkit rtos/rtos-toolkit/cmsis-rtos2
TARGET_OBJ_LIBS += devices/usb-device devices/usb-device/tinyusb devices/usb-device/tinyusb/common devices/usb-device/tinyusb/device devices/usb-device/tinyusb/class/cdc
TARGET_OBJ_LIBS += devices devices/cdc-serial
TARGET_OBJ_LIBS += svc svc/shell svc/event-bus svc/logger
TARGET_OBJ_LIBS += apps/debug

include ${PROJECT_ROOT}/tools/makefiles/project.mk

all: ${INSTALL_ROOT}/motor-controller.bin ${INSTALL_ROOT}/motor-controller.elf ${INSTALL_ROOT}/motor-controller.uf2

${INSTALL_ROOT}/motor-controller.uf2: ${CURDIR}/motor-controller.uf2
	@echo "INSTALLING ${@}"
	$(INSTALL) -m 660 -D ${<} ${@}

${INSTALL_ROOT}/motor-controller.elf: ${CURDIR}/motor-controller.elf
	@echo "INSTALLING ${@}"
	$(INSTALL) -m 660 -D ${<} ${@}

${INSTALL_ROOT}/motor-controller.bin: ${CURDIR}/motor-controller.bin
	@echo "INSTALLING ${@}"
	$(INSTALL) -m 660 -D ${<} ${@}

endif

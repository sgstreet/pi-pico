CROSS_COMPILE ?= ${PROJECT_ROOT}/local/bin/arm-none-eabi-
CROSS_FLAGS ?= -mcpu=cortex-m0plus -mtune=cortex-m0plus -march=armv6s-m -mfloat-abi=soft -mthumb -mno-unaligned-access --specs=picolibc.specs

#-fdebug-prefix-map=old_path=new_path

CC := ${CROSS_COMPILE}gcc
CXX := ${CROSS_COMPILE}g++
LD := ${CROSS_COMPILE}g++
AR := ${CROSS_COMPILE}ar
AS := ${CROSS_COMPILE}as
OBJCOPY := ${CROSS_COMPILE}objcopy
OBJDUMP := ${CROSS_COMPILE}objdump
SIZE := ${CROSS_COMPILE}size
NM := ${CROSS_COMPILE}nm
INSTALL := install
INSTALL_STRIP := install --strip-program=${CROSS_COMPILE}strip -s

CPPFLAGS := -I${PROJECT_ROOT}/include -D_GNU_SOURCE
ARFLAGS := cr
ASFLAGS := ${CROSS_FLAGS} 

CFLAGS := ${CROSS_FLAGS} -pipe -fno-omit-frame-pointer -feliminate-unused-debug-types -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -Wall -Wunused -Wuninitialized -Wmissing-declarations -Werror -std=gnu11 -funwind-tables -mpoke-function-name
CXXFLAGS := ${CROSS_FLAGS} -pipe -fno-omit-frame-pointer -feliminate-unused-debug-types -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -Wall -Wunused -Wuninitialized -Wmissing-declarations -Werror -std=gnu++21 -mpoke-function-name

LDSCRIPTS ?= -L${PROJECT_ROOT}/ldscripts -T memory.ld -T regions-flash.ld -T sections.ld
LDFLAGS ?= ${CROSS_FLAGS} -Wl,--no-warn-rwx-segment ${LDSCRIPTS} --specs=nosys.specs -nostartfiles -Xlinker --gc-sections
LOADLIBES ?= 
LDLIBS ?= -lm

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

CPPFLAGS := 
ARFLAGS := cr
ASFLAGS := ${CROSS_FLAGS} 

CFLAGS := ${CROSS_FLAGS} -pipe -feliminate-unused-debug-types -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -Wall -Wunused -Wuninitialized -Wmissing-declarations -Werror -std=gnu11 -funwind-tables
CXXFLAGS := ${CROSS_FLAGS} -pipe -feliminate-unused-debug-types -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -Wall -Wunused -Wuninitialized -Wmissing-declarations -Werror -std=gnu++21

LDSCRIPTS :=
LDFLAGS := ${CROSS_FLAGS} -Wl,-O1 -Wl,--hash-style=gnu
LOADLIBES := 
LDLIBS :=


#
# Copyright (C) 2017 Red Rocket Computing, LLC
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
#
# project.mk
#
# Created on: Mar 16, 2017
#     Author: Stephen Street (stephen@redrocketcomputing.com)
#

include ${PROJECT_ROOT}/tools/makefiles/common.mk

ifneq ($(wildcard ${SOURCE_DIR}/subdir.mk),)
include ${SOURCE_DIR}/subdir.mk
else
SRC += $(wildcard ${SOURCE_DIR}/*.c)
SRC += $(wildcard ${SOURCE_DIR}/*.S)
SRC += $(wildcard ${SOURCE_DIR}/*.s)
endif

OBJ := $(patsubst %.c,%.o,$(filter %.c,${SRC})) $(patsubst %.cpp,%.o,$(filter %.cpp,${SRC})) $(patsubst %.s,%.o,$(filter %.s,${SRC})) $(patsubst %.S,%.o,$(filter %.S,${SRC}))
OBJ := $(subst ${PROJECT_ROOT},${BUILD_ROOT},${OBJ})
TARGET_OBJ := $(foreach dir, $(addprefix ${BUILD_ROOT}/, ${TARGET_OBJ_LIBS}), $(wildcard $(dir)/*.o)) $(addprefix ${BUILD_ROOT}/, ${TARGET_OBJS}) ${EXTRA_OBJS}

#$(info SUBDIRS=${SUBDIRS})
#$(info SOURCE_DIR=${SOURCE_DIR})
#$(info BUILD_ROOT=${BUILD_ROOT})
#$(info SRC=${SRC})
#$(info OBJ=${OBJ})
#$(info TARGET_OBJS=${TARGET_OBJS})
#$(info TARGET_OBJ_LIBS=${TARGET_OBJ_LIBS})
#$(info TARGET_OBJ=${TARGET_OBJ})
#$(info EXTRA_DEPS=${EXTRA_DEPS})
#$(info EXTRA_ELF_DEPS=${EXTRA_ELF_DEPS})
#$(info EXTRA_TARGETS=${EXTRA_TARGETS})
#$(info TARGET=${TARGET})
#$(info ALL-TARGET=$(addprefix ${CURDIR}/,${TARGET}))
#$(info CURDIR=${CURDIR})
#$(info MAKECMDGOALS=${MAKECMDGOALS})

.SECONDARY:

all: ${EXTRA_TARGETS} ${OBJ}

clean:
	@echo "CLEANING ${CURDIR}"
	${RM} ${CURDIR}/*.bin ${CURDIR}/*.elf ${CURDIR}/*.map ${CURDIR}/*.smap ${CURDIR}/*.dis ${OBJ} ${OBJ:%.o=%.d} ${OBJ:%.o=%.dis} ${OBJ:%.o=%.o.lst} ${EXTRA_CLEAN}

distclean:

install:

${EXTRA_DEPS} ${EXTRA_LIB_DEPS} ${EXTRA_ELF_DEPS} ${TARGET_OBJ}:

${CURDIR}/%.a: ${OBJ} ${TARGET_OBJ} ${EXTRA_LIB_DEPS} ${EXTRA_DEPS}
	@echo "ARCHIVING $@"
	$(AR) ${ARFLAGS} $@ ${OBJ} ${TARGET_OBJ}
 
${CURDIR}/%.so: ${OBJ} ${TARGET_OBJ} ${EXTRA_LIB_DEPS} ${EXTRA_DEPS}
	@echo "LINKING $@"
	$(LD) --shared -Wl,-soname,$@ ${LOADLIBES} ${LDFLAGS} -Wl,--cref -Wl,-Map,"$(basename ${@}).map" -o $@ ${OBJ} ${LDLIBS}
	$(NM) -n $@ | grep -v '\( [aNUw] \)\|\(__crc_\)\|\( \$[adt]\)' > ${@:%.so=%.smap}

${CURDIR}/%.elf: ${OBJ} ${TARGET_OBJ} ${EXTRA_ELF_DEPS} ${EXTRA_DEPS}
	@echo "LINKING $@"
	$(LD) ${LDFLAGS} -Wl,--cref -Wl,-Map,"$(basename ${@}).map" ${LOADLIBES} -o $@ ${OBJ} ${TARGET_OBJ} ${LDLIBS}
	$(OBJDUMP) -S $@ > ${@:%.elf=%.dis}
	$(NM) -n $@ | grep -v '\( [aNUw] \)\|\(__crc_\)\|\( \$[adt]\)' > ${@:%.elf=%.smap}
	@echo "SIZE $@"
	$(SIZE) -B $@

${CURDIR}/%.bin: ${CURDIR}/%.elf
	@echo "GENERATING $@"
#	$(OBJCOPY) --gap=0xff -O binary $< $@
	$(OBJCOPY) -O binary $< $@

${CURDIR}/%.o: ${SOURCE_DIR}/%.c ${EXTRA_OBJ_DEPS}
	@echo "COMPILING $<"
	$(CC) ${CPPFLAGS} ${CFLAGS} -Wa,-adhlns="$@.lst" -MMD -MP -c -o $@ $<

${CURDIR}/%.o: ${SOURCE_DIR}/%.cpp ${EXTRA_OBJ_DEPS}
	@echo "COMPILING $<"
	$(CXX) ${CPPFLAGS} ${CXXFLAGS} -Wa,-adhlns="$@.lst" -MMD -MP -c -o $@ $<

${CURDIR}/%.o: ${SOURCE_DIR}/%.s ${EXTRA_OBJ_DEPS}
	@echo "ASSEMBLING $<"
	$(AS) ${ASFLAGS} -adhlns="$@.lst" -o $@ $<

${CURDIR}/%.o: ${SOURCE_DIR}/%.S ${EXTRA_OBJ_DEPS}
	@echo "ASSEMBLING $<"
	$(CC) ${CPPFLAGS} -x assembler-with-cpp ${ASFLAGS} -Wa,-adhlns="$@.lst" -c -o $@ $<

ifneq (${MAKECMDGOALS},clean)
-include ${OBJ:.o=.d}
endif


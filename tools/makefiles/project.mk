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

SUBDIRS ?= $(subst ${SOURCE_DIR}/,,$(shell find ${SOURCE_DIR} -mindepth 2 -name "subdir.mk" -printf "%h "))
SRC :=

include ${PROJECT_ROOT}/tools/makefiles/common.mk

include $(patsubst %,${SOURCE_DIR}/%/subdir.mk,${SUBDIRS})
-include ${SOURCE_DIR}/subdir.mk

OBJ := $(patsubst %.c,%.o,$(filter %.c,${SRC})) $(patsubst %.cpp,%.o,$(filter %.cpp,${SRC})) $(patsubst %.s,%.o,$(filter %.s,${SRC})) $(patsubst %.S,%.o,$(filter %.S,${SRC}))
OBJ := $(subst ${PROJECT_ROOT},${BUILD_ROOT},${OBJ})
TARGET_OBJ := $(foreach dir, $(addprefix ${BUILD_ROOT}/, ${TARGET_OBJ_LIBS}), $(wildcard $(dir)/*.o)) $(addprefix ${BUILD_ROOT}/, ${TARGET_OBJS}) ${EXTRA_OBJS}

#$(info SUBDIRS=${SUBDIRS})
#$(info SOURCE_DIR=${SOURCE_DIR})
#$(info BUILD_ROOT=${BUILD_ROOT})
#$(info SRC=${SRC})
#$(info OBJ=${OBJ})
#$(info TARGET_OBJ=${TARGET_OBJ})
#$(info EXTRA_DEPS=${EXTRA_DEPS})
#$(info EXTRA_TARGETS=${EXTRA_TARGETS})
#$(info TARGET=${TARGET})
#$(info ALL-TARGET=$(addprefix ${CURDIR}/,${TARGET}))
#$(info CURDIR=${CURDIR})

.SECONDARY:

ifeq (${TARGET},)
all: ${SUBDIRS} ${EXTRA_TARGETS} ${OBJ}
else
$(addprefix ${CURDIR}/,${TARGET}): ${SUBDIRS} ${EXTRA_TARGETS} 
all: $(addprefix ${CURDIR}/,${TARGET}) 
endif

clean: ${SUBDIRS}
	@echo "CLEANING ${CURDIR}"
	${RM} ${TARGET} $(basename ${TARGET}).img $(basename ${TARGET}).bin $(basename ${TARGET}).elf $(basename ${TARGET}).map $(basename ${TARGET}).smap $(basename ${TARGET}).dis ${OBJ} ${OBJ:%.o=%.d} ${OBJ:%.o=%.dis} ${OBJ:%.o=%.o.lst} ${EXTRA_CLEAN}

${SUBDIRS}:
	mkdir -p ${CURDIR}/$@

${CURDIR}/%.a: ${OBJ} ${EXTRA_LIB_DEPS} ${EXTRA_DEPS}
	@echo "ARCHIVING $@"
	$(AR) ${ARFLAGS} $@ ${OBJ}

#$(addprefix ${CURDIR}/,${TARGET}): ${OBJ} ${TARGET_OBJ} ${EXTRA_DEPS}
#	@echo "LINKING EXE $@"
#	$(LD) ${LDFLAGS} ${LOADLIBES} -o $@ ${OBJ} ${TARGET_OBJ} ${LDLIBS}
#	$(OBJDUMP) -S $@ > $@.dis
#	$(NM) -n $@ | grep -v '\( [aNUw] \)\|\(__crc_\)\|\( \$[adt]\)' > $@.smap
#	@echo "SIZE $@"
#	$(SIZE) -B $@

${CURDIR}/%.so: ${OBJ} ${EXTRA_LIB_DEPS} ${EXTRA_DEPS}
	@echo "LINKING $@"
	$(LD) --shared -Wl,-soname,$@ ${LOADLIBES} ${LDFLAGS} -Wl,--cref -Wl,-Map,"$(basename ${@}).map" -o $@ ${OBJ} ${LDLIBS}
	$(NM) -n $@ | grep -v '\( [aNUw] \)\|\(__crc_\)\|\( \$[adt]\)' > ${@:%.so=%.smap}

${CURDIR}/%.elf: ${OBJ} ${TARGET_OBJ} ${EXTRA_ELF_DEPS} ${EXTRA_DEPS}
	@echo "LINKING $@"
	$(LD) ${LDFLAGS} -Wl,--cref -Wl,-Map,"$(basename ${@}).map" ${LOADLIBES} -o $@ ${OBJ} ${TARGET_OBJ} ${LDLIBS}
#	$(OBJDUMP) -S $@ > ${@:%.elf=%.dis}
	$(NM) -n $@ | grep -v '\( [aNUw] \)\|\(__crc_\)\|\( \$[adt]\)' > ${@:%.elf=%.smap}
	@echo "SIZE $@"
	$(SIZE) -B $@

${CURDIR}/%.bin: ${CURDIR}/%.elf
	@echo "GENERATING $@"
	$(OBJCOPY) --gap=0xff -O binary $< $@

${CURDIR}/%.o: ${SOURCE_DIR}/%.c
	@echo "COMPILING $<"
	$(CC) ${CPPFLAGS} ${CFLAGS} -Wa,-adhlns="$@.lst" -MMD -MP -c -o $@ $<
    
${CURDIR}/%.o: ${SOURCE_DIR}/%.cpp
	@echo "COMPILING $<"
	$(CXX) ${CPPFLAGS} ${CXXFLAGS} -Wa,-adhlns="$@.lst" -MMD -MP -c -o $@ $<

${CURDIR}/%.o: ${SOURCE_DIR}/%.s
	@echo "ASSEMBLING $<"
	$(AS) ${ASFLAGS} -adhlns="$@.lst" -o $@ $<

${CURDIR}/%.o: ${SOURCE_DIR}/%.S
	@echo "ASSEMBLING $<"
	$(CC) ${CPPFLAGS} -x assembler-with-cpp ${ASFLAGS} -Wa,-adhlns="$@.lst" -c -o $@ $<

ifneq (${MAKECMDGOALS},clean)
-include ${OBJ:.o=.d}
endif


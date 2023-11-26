export ELF2UF2 ?= ${PROJECT_ROOT}/local/bin/elf2uf2
export PIOASM ?= ${PROJECT_ROOT}/local/bin/pioasm
 
CPPFLAGS += -I${PROJECT_ROOT}/include/cmsis -D__BOARD="<board/${BOARD_TYPE}.h>" -D__HAL="<hal/${CHIP_TYPE}/hal-${CHIP_TYPE}.h>"

EXTRA_CLEAN := ${CURDIR}/*.pio.h ${CURDIR}/*.uf2

${CURDIR}/%.pio.h: ${SOURCE_DIR}/%.pio
	@echo "ASSEMBLING $<"
	$(PIOASM) ${PIOFLAGS} $< $@

${CURDIR}/%.uf2: ${CURDIR}/%.elf
	@echo "GENERATING $@"
	$(ELF2UF2) $< $@


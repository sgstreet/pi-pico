/* Based on GCC ARM embedded samples.
   Defines the following symbols for use by code:
    __exidx_start
    __exidx_end
    __etext
    __data_start__
    __preinit_array_start
    __preinit_array_end
    __init_array_start
    __init_array_end
    __fini_array_start
    __fini_array_end
    __data_end__
    __bss_start__
    __bss_end__
    __end__
    end
    __stack
*/

ENTRY(Reset_Handler)

SECTIONS
{
	/* Set __stack to top of the stack region reserving 1024 byte for the fault stack */
	PROVIDE(__fault_stack__ = ORIGIN(STACK) + LENGTH(STACK));
	PROVIDE(__fault_stack_0 = ORIGIN(CORE0) + LENGTH(CORE0));
	PROVIDE(__fault_stack_1 = ORIGIN(CORE1) + LENGTH(CORE1));

	PROVIDE(__stack = ORIGIN(STACK) + LENGTH(STACK) - 1024);
	PROVIDE(__stack_size = LENGTH(STACK) - 1024);

	PROVIDE(__stack_0 = ORIGIN(CORE0) + LENGTH(CORE0) - 1024);
	PROVIDE(__stack_0_size = LENGTH(CORE0) - 1024);

	PROVIDE(__stack_1 = ORIGIN(CORE1) + LENGTH(CORE1) - 1024);
	PROVIDE(__stack_1_size = LENGTH(CORE1) - 1024);

	__start__ = ORIGIN(TEXT);

	/*
	   Second stage bootloader is prepended to the image. It must be 256 bytes big
	   and checksummed. It is usually built by the boot_stage2 target
	   in the Raspberry Pi Pico SDK
	*/
	.boot2 :
	{
		__boot2_start__ = .;
		KEEP (*(.bootstrap))
		KEEP (*(.boot2))
		__boot2_end__ = .;
	} > BOOTSTRAP

	.vector :
	{
		. = ALIGN(4);

		__vector_start__ = .;
		KEEP(*(.vector))
		__vector_end__ = .;

		*(.after_vector.* .after_vector)

	} > ISR
	__vector = LOADADDR(.vector);
	__vector_size = __vector_end__ - __vector_start__;

	.text :
	{
		. = ALIGN(4);
		__text_start__ = .;

		*(.isr .isr.*)

		*(.init)
		*(EXCLUDE_FILE(*libgcc.a: *libc.a:*mem*.o *libm.a:) .text*)
		*(.fini)

		/* Pull all c'tors into .text */
		*crtbegin.o(.ctors)
		*crtbegin?.o(.ctors)
		*(EXCLUDE_FILE(*crtend?.o *crtend.o) .ctors)
		*(SORT(.ctors.*))
		*(.ctors)

		/* Followed by destructors */
		*crtbegin.o(.dtors)
		*crtbegin?.o(.dtors)
		*(EXCLUDE_FILE(*crtend?.o *crtend.o) .dtors)
		*(SORT(.dtors.*))
		*(.dtors)

		*(.eh_frame*)
		. = ALIGN(4);
		__text_end__ = .;

	} > TEXT
	__text = LOADADDR(.text);
	__text_size = __text_end__ - __text_start__;

	.inits :
	{
		. = ALIGN(4);
		__inits_start__ = .;

		 /* The preinit arrays */
		. = ALIGN(4);
		PROVIDE_HIDDEN(__preinit_array_start = .);

		KEEP(*(SORT(.preinit_array_sysinit.*)))
		KEEP(*(.preinit_array_sysinit))

		KEEP(*(SORT(.preinit_array_platform.*)))
		KEEP(*(.preinit_array_platform))

		KEEP(*(SORT(preinit_array.*)))
		KEEP(*(.preinit_array))

		PROVIDE_HIDDEN(__preinit_array_end = .);

		/* init data */
		. = ALIGN(4);
		PROVIDE_HIDDEN(__init_array_start = .);
		KEEP(*(SORT(.init_array.*)))
		KEEP(*(.init_array))
		PROVIDE_HIDDEN(__init_array_end = .);

		/* fini data */
		. = ALIGN(4);
		PROVIDE_HIDDEN(__fini_array_start = .);
		KEEP(*(SORT(.fini_array.*)))
		KEEP(*(.fini_array))
		PROVIDE_HIDDEN(__fini_array_end = .);

		. = ALIGN(4);
		__inits_end__ = .;

	} > RODATA
	__inits = LOADADDR(.inits);
	__inits_size = __inits_end__ - __inits_start__;

	.rodata :
	{
		. = ALIGN(4);
		__rodata_start__ = .;
		*(EXCLUDE_FILE(*libgcc.a: *libc.a:*mem*.o *libm.a:) .rodata*)
		. = ALIGN(4);
		*(SORT_BY_ALIGNMENT(SORT_BY_NAME(.flashdata*)))

		. = ALIGN(4);
		PROVIDE_HIDDEN(__systick_array_start = .);
		KEEP(*(SORT(.systick_array.*)))
		KEEP(*(.systick_array))
		PROVIDE_HIDDEN(__systick_array_end = .);

		. = ALIGN(4);
		PROVIDE_HIDDEN(__shell_cmd_start = .);
		KEEP(*(.shell_cmd));
		PROVIDE_HIDDEN(__shell_cmd_end = .);

		. = ALIGN(4);
		__rodata_end__ = .;
	} > RODATA
	__rodata = LOADADDR(.rodata);
	__rodata_size = __rodata_end__ - __rodata_start__;

	.ARM.extab :
	{
		. = ALIGN(4);
		__extab_start__ = .;
		__extab_start = .;
		*(.ARM.extab* .gnu.linkonce.armextab.*)
		__extab_end = .;
		__extab_end__ = .;
	} > RODATA
	__extab_size = __extab_end__ - __extab_start__;

	.ARM.exidx :
	{
		. = ALIGN(4);
		__exidx_start__ = .;
		__exidx_start = .;
		*(.ARM.exidx* .gnu.linkonce.armexidx.*)
		__exidx_end = .;
		__exidx_end__ = .;
	} > RODATA
	__exidx_size = __exidx_end__ - __exidx_start__;

	__unwind_info = LOADADDR(.ARM.extab);
	__unwind_info_size =  (__extab_size) + (__exidx_size);

	__etext = .;

	.vtor_0 (NOLOAD) :
	{
		FILL(0xff)
		. = ALIGN(256);
		. += __vector_size;
		. = ALIGN(4);
	} > CORE0
	__vtor_0 = LOADADDR(.vtor_0);
	__vtor_0_size = SIZEOF(.vtor_0);

	.vtor_1 (NOLOAD) :
	{
		FILL(0xff)
		. = ALIGN(256);
		. += __vector_size;
		. = ALIGN(4);
	} > CORE1
	__vtor_1 = LOADADDR(.vtor_1);
	__vtor_1_size = SIZEOF(.vtor_1);

	.core_local_0 (NOLOAD) :
	{
		FILL(0x00)

		. = ALIGN(__core_align);
		. += __core_data_size;

		. = ALIGN(__tls_align);
		__core_local_tls_0 = .;
		. += __tls_size;

		. = ALIGN(4);

	} > CORE0
	__core_local_0 = LOADADDR(.core_local_0);
	__core_local_0_size = SIZEOF(.core_local_0);
	__tls_block_offset = __core_local_tls_0 - __core_local_0;

	.core_local_1 (NOLOAD) :
	{
		FILL(0x00)

		. = ALIGN(__core_align);
		. += __core_data_size;

		. = ALIGN(__tls_align);
		__core_local_tls_1 = .;
		. += __tls_size;

		. = ALIGN(4);

	} > CORE1
	__core_local_1 = LOADADDR(.core_local_1);
	__core_local_1_size = SIZEOF(.core_local_1);

	.fast :
	{
		. = ALIGN(4);
		__fast_start__ = .;

		*(.fast .fast.*);
		*(.time_critical .time_critical*)
		*(.text .text*)

		. = ALIGN(4);
		__fast_end__ = .;
	} > FAST AT > RODATA
	__fast = LOADADDR(.fast);
	__fast_size = __fast_end__ - __fast_start__;

	.data :
	{
		. = ALIGN(4);
		__data_start__ = .;

		/* fast .rodata; i.e. stuff we exclude above because we want it in RAM */
		*(.rodata .rodata.*)

		. = ALIGN(4);
		*(.data .data.*)

		. = ALIGN(4);
		*(.after_data .after_data.*)

		. = ALIGN(4);
		__data_end__ = .;
	} > DATA AT> RODATA
	__data = LOADADDR(.data);
	__end__ = LOADADDR(.data) + SIZEOF(.data);
	__data_size = __data_end__ - __data_start__;

	.core_data :
	{
		FILL(0x00)
		. = ALIGN(4);
		__core_data_start__ = .;
		LONG(0xdededede);
		KEEP(*(.core_data .core_data.*))
		LONG(0xdededede);
		__core_data_end__ = .;
		. = ALIGN(4);
	} > RODATA
	__core_data = LOADADDR(.core_data);
	__core_data_size = SIZEOF(.core_data);
	__core_align = ALIGNOF(.core_data);

	.tdata :
	{
		. = ALIGN(4);
		__tdata_start__ = .;

		*(.tdata .tdata.* .gnu.linkonce.td.*)

		__tdata_end__ = .;
		. = ALIGN(4);

	} > RODATA
	__tdata = LOADADDR(.tdata);
	__tdata_size = SIZEOF(.tdata);
	__tdata_source = __tdata;
	__tdata_source_end = __tdata + __tdata_size;

	__end__ = LOADADDR(.tdata) + SIZEOF(.tdata);

	.tbss (NOLOAD) :
	{
		. = ALIGN(4);
		__tbss_start__ = .;

		*(.tbss .tbss.* .gnu.linkonce.tb.*)
		*(.tcommon)

		. = ALIGN(4);
		__tbss_end__ = .;
	} > BSS
	__tbss_size = SIZEOF(.tbss);
	__tls_size = __tdata_size + __tbss_size;
	__tbss_offset = ADDR(.tbss) - ADDR(.tdata);
	__tls_align = MAX(ALIGNOF(.tdata), ALIGNOF(.tbss));
	__arm32_tls_tcb_offset = MAX(8, __tls_align);
	__arm64_tls_tcb_offset = MAX(16, __tls_align);

	.bss (NOLOAD) :
	{
		. = ALIGN(4);
		__bss_start__ = .;

		*(.bss_begin .bss_begin.*)
		*(SORT_BY_ALIGNMENT(SORT_BY_NAME(.bss*)))
		*(.bss .bss.*)
		*(COMMON)
		*(.bss_end .bss_end.*)

		. = ALIGN(4);
		__bss_end__ = .;
	} > BSS
	__bss = LOADADDR(.bss);
	__bss_size = __bss_end__ - __bss_start__;

 	.copy.table :
	{
		. = ALIGN(4);
		__copy_table_start__ = .;

		LONG(__vector)
		LONG(__vtor_0)
		LONG(__vtor_0_size)

		LONG(__core_data)
		LONG(__core_local_0)
		LONG(__core_data_size)

		LONG(__tdata)
		LONG(__core_local_tls_0)
		LONG(__tdata_size)

		LONG(__vector)
		LONG(__vtor_1)
		LONG(__vtor_1_size)

		LONG(__core_data)
		LONG(__core_local_1)
		LONG(__core_data_size)

		LONG(__tdata)
		LONG(__core_local_tls_1)
		LONG(__tdata_size)

		LONG(__fast)
		LONG(__fast_start__)
		LONG(__fast_size)

		LONG(__data)
		LONG(__data_start__)
		LONG(__data_size)

		__copy_table_end__ = .;
	} > RODATA

	.zero.table :
	{
		. = ALIGN(4);
		__zero_table_start__ = .;

		LONG(__bss_start__)
		LONG(__bss_size)

		__zero_table_end__ = .;
	} > RODATA

	.heap :
	{
		. = ALIGN(8);
		__heap_start = .;
		__heap_end = ORIGIN(HEAP) + LENGTH(HEAP);
	} > HEAP
	__heap = LOADADDR(.heap);
	__heap_size = __heap_end - __heap_start;
}

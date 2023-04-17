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
	/* Set __stack to top of the stack region reserving 512 byte for the fault stack */
	PROVIDE(__fault_stack__ = ORIGIN(STACK) + LENGTH(STACK));
	PROVIDE(__stack = ORIGIN(STACK) + LENGTH(STACK) - 512);
	PROVIDE(__stack_size = 4096 - 512);

	__start__ = ORIGIN(TEXT);
	
	/* Second stage bootloader is prepended to the image. It must be 256 bytes big
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
		        
/*		*(.text .text.*)*/
		        
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
	__test = LOADADDR(.text);
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
/*		*(.rodata .rodata.*)*/
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
	
	.vtor (NOLOAD) :
	{
		FILL(0xff)
		. = ALIGN(256);
		__vtor_start__ = .;
		
		. += __vector_size;
		
		. = ALIGN(4);
		__vtor_end__ = .;
    } > FAST
	__vtor = LOADADDR(.vtor);
	__vtor_size = __vtor_end__ - __vtor_start__;
	
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

	.tdata :
	{
		. = ALIGN(4);
		__tdata_start__ = .;
		
		*(.tdata .tdata.* .gnu.linkonce.td.*)
		
		__tdata_end__ = .;
		. = ALIGN(4);
	} > DATA AT> RODATA
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
		LONG(__vtor_start__)
		LONG(__vtor_size)
		
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
		__heap_end = ORIGIN(HEAP) + LENGTH(HEAP) - __stack_size;;
	} > HEAP
	__heap = LOADADDR(.heap);
	__heap_size = __heap_end - __heap_start;
}


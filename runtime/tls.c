/*
 * tls.c
 *
 *  Created on: Apr 27, 2023
 *      Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <cmsis/cmsis.h>

extern void *__core_data_size;
extern void *__core_0;
extern void *__core_1;

extern void *__arm32_tls_tcb_offset;
extern char __tdata_source[];
extern char __tdata_size[];
extern char __tbss_size[];
extern char __tbss_offset[];

#undef __fast_section
#define __fast_section

void *_cls = 0;
core_local void *_tls = 0;

void *__aeabi_read_cls(void);
__naked __fast_section void *__aeabi_read_cls(void)
{
	asm volatile (
		".syntax unified\n"
		"ldr r0, =0xd0000000\n"
		"ldr r0, [r0, #0]\n"
		"cmp r0, #0\n"
		"beq 0f\n"
		"ldr r0, =__core_1\n"
		"mov pc, lr\n"
		"0: \n"
		"ldr r0, =__core_0\n"
		"mov pc, lr\n"
	);}

void *__aeabi_read_core_cls(unsigned long core);
__naked __fast_section void *__aeabi_read_core_cls(unsigned long core)
{
	asm volatile (
		".syntax unified\n"
		".syntax unified\n"
		"cmp r0, #0\n"
		"beq 0f\n"
		"ldr r0, =__core_1\n"
		"mov pc, lr\n"
		"0: \n"
		"ldr r0, =__core_0\n"
		"mov pc, lr\n"
	);
}

void *__wrap___aeabi_read_tp(void);
__naked __fast_section void *__wrap___aeabi_read_tp(void)
{
	asm volatile (
		".syntax unified\n"
		"push {r1, lr}\n"
		"ldr r0,=__core_data\n"
		"ldr r1,=_tls\n"
		"subs r1, r1, r0\n"
		"bl __aeabi_read_cls\n"
		"ldr r0, [r0, r1]\n"
		"pop {r1, pc}\n"
	);
}

void __wrap__set_tls(void *tls);
__fast_section __optimize void __wrap__set_tls(void *tls)
{
	cls_datum(_tls) = tls - ((size_t)&__arm32_tls_tcb_offset);
}

void __wrap__init_tls(void *__tls);
__no_optimize void __wrap__init_tls(void *__tls)
{
	char *src;
	char *dst;
	char *end;

	/* Copy tls initialized data */
	src = __tdata_source;
	dst = __tls;
	end = __tls + (size_t)__tdata_size;
	while (dst < end)
		*src++ = *dst++;

	/* Clear tls zero data */
	dst = __tls + (size_t)__tbss_offset;
	end = __tls + (size_t)__tbss_offset + (size_t)__tbss_size;
	while (dst < end)
		*dst++ = 0;
}

bool __cls_check(void);
bool __cls_check(void)
{
	uint32_t *begin_marker = __aeabi_read_core_cls(0);
	uint32_t *end_marker = __aeabi_read_core_cls(0) + (size_t)&__core_data_size - sizeof(uint32_t);
	if (*begin_marker != 0xdededede || *end_marker != 0xdededede)
		return false;

	begin_marker = __aeabi_read_core_cls(1);
	end_marker = __aeabi_read_core_cls(1) + (size_t)&__core_data_size - sizeof(uint32_t);
	return *begin_marker == 0xdededede && *end_marker == 0xdededede;
}

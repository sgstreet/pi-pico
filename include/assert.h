/*
 * assert.h
 *
 *  Created on: Feb 26, 2023
 *      Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#ifndef _ASSERT_H
#define _ASSERT_H

#ifdef __cplusplus
extern "C" {
#endif

#define static_assert(x,y)

extern void __assert_fail(const char *exp) __attribute__((noreturn));

#ifdef NDEBUG

/*
 * NDEBUG doesn't just suppress the faulting behavior of assert(),
 * but also all side effects of the assert() argument.  This behavior
 * is required by the C standard, and allows the argument to reference
 * variables that are not defined without NDEBUG.
 */
#define assert(x) ((void)(0))

#else

#define assert(x) ((x) ? (void)0 : __assert_fail(#x))

#endif

#ifdef __cplusplus
}
#endif

#endif				/* _ASSERT_H */

#pragma once

#include <services.h>

#undef UNIMPLEMENTED
#define UNIMPLEMENTED					\
    {							\
	HalVgaPrint("%s UNIMPLEMENTED\n", __func__);	\
	return STATUS_NOT_IMPLEMENTED;			\
    }

#if defined(CONFIG_DEBUG_BUILD)
VOID vDbgPrint(PCSTR Format, va_list args);
#else
#define DbgPrint(...)
#endif

#define assert_ret(expr)	if (!(expr)) { return STATUS_NTOS_BUG; }


/*
 * Additional alignment macros
 */
#define IS_ALIGNED_BY(addr, align)	((ULONG_PTR)(addr) == ALIGN_DOWN_BY(addr, align))
#define IS_ALIGNED(addr, type)		((ULONG_PTR)(addr) == ALIGN_DOWN(addr, type))

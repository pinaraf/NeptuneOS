#pragma once

#define RTLP_DBGTRACE_MODULE_NAME	"NTDLL"

#include <string.h>
#include <stdint.h>
#include <nt.h>
#include <sel4/sel4.h>
#include <services.h>
#include <ntdll_syssvc_gen.h>
#include <debug.h>
#include <assert.h>
#include <image.h>
#include <util.h>

#define CDECL		__cdecl

#undef DPRINT
#undef DPRINT1
#define DPRINT DbgTrace
#define DPRINT1 DbgTrace

#define ROUND_DOWN(x, align)	ALIGN_DOWN_BY(x, align)
#define PAGE_ROUND_DOWN(x)	ROUND_DOWN(x, PAGE_SIZE)
#define ROUND_UP(x, align)	ALIGN_UP_BY(x, align)
#define PAGE_ROUND_UP(x)	ROUND_UP(x, PAGE_SIZE)
#define PAGE_SHIFT		seL4_PageBits

/* We cannot include win32 headers so define ULongToPtr here */
static inline void *ULongToPtr(const unsigned long ul)
{
    return (void*)((ULONG_PTR)ul);
}
#define UlongToPtr ULongToPtr

static inline unsigned long PtrToUlong(const void *p)
{
    return ((unsigned long)(ULONG_PTR)p);
}

#define min(a, b) (((a) < (b)) ? (a) : (b))
#define max(a, b) (((a) > (b)) ? (a) : (b))

/* Use native clang/gcc implementation of structured exception handling */
#include <excpt.h>
#define _SEH2_TRY __try
#define _SEH2_FINALLY __finally
#define _SEH2_EXCEPT(...) __except(__VA_ARGS__)
#define _SEH2_END
#define _SEH2_GetExceptionInformation() (GetExceptionInformation())
#define _SEH2_GetExceptionCode() (GetExceptionCode())
#define _SEH2_AbnormalTermination() (AbnormalTermination())
#define _SEH2_YIELD(STMT_) STMT_
#define _SEH2_LEAVE __leave
#define _SEH2_VOLATILE

/* Defined in winbase.h. We cannot include Win32 headers here */
#define EXCEPTION_NONCONTINUABLE_EXCEPTION ((DWORD)0xC0000025)

#define SharedUserData ((KUSER_SHARED_DATA *CONST) KUSER_SHARED_DATA_CLIENT_ADDR)

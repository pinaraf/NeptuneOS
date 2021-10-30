#pragma once

#include <services.h>

#define ARRAY_LENGTH(x)		(sizeof(x) / sizeof((x)[0]))

/* NTSTATUS Bits:
 * 0--15   Status code
 * 16--28  Facility
 * 29      Customer code flag
 * 30--31  Severity
 */

#define CUSTOMER_FLAG_BIT	29

#define FACILITY_ASYNC		0x43
#define FACILITY_SEL4		0x44
#define FACILITY_NTOS		0x45

#define ASYNC_SUCCESS(Code)	((NTSTATUS)(Code | (FACILITY_ASYNC << 16) | (1UL << CUSTOMER_FLAG_BIT) | ERROR_SEVERITY_SUCCESS))
#define ASYNC_INFORMATION(Code)	((NTSTATUS)(Code | (FACILITY_ASYNC << 16) | (1UL << CUSTOMER_FLAG_BIT) | ERROR_SEVERITY_INFORMATIONAL))
#define ASYNC_ERROR(Code)	((NTSTATUS)(Code | (FACILITY_ASYNC << 16) | (1UL << CUSTOMER_FLAG_BIT) | ERROR_SEVERITY_ERROR))
#define SEL4_ERROR(Code)	((NTSTATUS)(Code | (FACILITY_SEL4 << 16) | (1UL << CUSTOMER_FLAG_BIT) | ERROR_SEVERITY_ERROR))
#define NTOS_SUCCESS(Code)	((NTSTATUS)(Code | (FACILITY_NTOS << 16) | (1UL << CUSTOMER_FLAG_BIT) | ERROR_SEVERITY_SUCCESS))
#define NTOS_INFORMATION(Code)	((NTSTATUS)(Code | (FACILITY_NTOS << 16) | (1UL << CUSTOMER_FLAG_BIT) | ERROR_SEVERITY_INFORMATIONAL))
#define NTOS_ERROR(Code)	((NTSTATUS)(Code | (FACILITY_NTOS << 16) | (1UL << CUSTOMER_FLAG_BIT) | ERROR_SEVERITY_ERROR))

#define IS_ASYNC_STATUS(Code)			((((((ULONG)(Code)) >> 16) << 3) >> 3) == FACILITY_ASYNC)

#define STATUS_ASYNC_PENDING			ASYNC_INFORMATION(1)
#define STATUS_ASYNC_STACK_OVERFLOW		ASYNC_ERROR(2)
#define STATUS_ASYNC_BUGBUG			ASYNC_ERROR(3)
#define STATUS_NTOS_BUG				NTOS_ERROR(1)
#define STATUS_NTOS_NO_REPLY			NTOS_INFORMATION(2)
#define STATUS_NTOS_DRIVER_ALREADY_LOADED	NTOS_SUCCESS(3)

/*
 * Doubly-linked list helper routines
 */
static inline VOID InvalidateListEntry(IN PLIST_ENTRY ListEntry)
{
    ListEntry->Flink = ListEntry->Blink = NULL;
}

static inline ULONG GetListLength(IN PLIST_ENTRY ListEntry)
{
    ULONG Length = 0;
    for (PLIST_ENTRY Ptr = ListEntry->Flink; Ptr != ListEntry; Ptr = Ptr->Flink) {
	Length++;
    }
    return Length;
}

#define LoopOverList(Entry, ListHead, Type, Field)			\
    for (Type *Entry = CONTAINING_RECORD((ListHead)->Flink, Type, Field), \
	     *__LoopOverList_flink = CONTAINING_RECORD((Entry)->Field.Flink, Type, Field); \
	 &(Entry)->Field != (ListHead); Entry = __LoopOverList_flink,	\
	     __LoopOverList_flink = CONTAINING_RECORD((__LoopOverList_flink)->Field.Flink, Type, Field))

#define ReverseLoopOverList(Entry, ListHead, Type, Field)		\
    for (Type *Entry = CONTAINING_RECORD((ListHead)->Blink, Type, Field), \
	     *__ReverseLoop_blink = CONTAINING_RECORD((Entry)->Field.Blink, Type, Field); \
	 &(Entry)->Field != (ListHead); Entry = __ReverseLoop_blink,	\
	     __ReverseLoop_blink = __CONTAINING_RECORD((__ReverseLoop_blink)->Field.Blink, Type, Field))


/*
 * Additional alignment macros
 */
#define IS_ALIGNED_BY(addr, align)	((ULONG_PTR)(addr) == ALIGN_DOWN_BY(addr, align))
#define IS_ALIGNED(addr, type)		((ULONG_PTR)(addr) == ALIGN_DOWN(addr, type))

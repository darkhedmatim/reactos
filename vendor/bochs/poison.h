#ifndef POISON_H_
#define POISON_H_

#define _POISON_R 0x00000001
#define _POISON_W 0x00000002
#define _UNPOISON_R 0x00000004
#define _UNPOISON_W 0x00000008

#if defined(__GNUC__)
#define _POISONAPI __inline__ __attribute__((always_inline))
#elif defined(_MSC_VER)
#define _POISONAPI __forceinline
#elif defined __cplusplus
#define _POISONAPI inline
#else
#define _POISONAPI
#endif

#if defined(_MSC_VER)
#include <intrin.h>

_POISONAPI void _POISON(const void * addr, int flags)
{
	_ReadWriteBarrier();
	__asm mov eax, flags
	__asm mov esi, addr
	__asm _emit 00fh
	__asm _emit 027h
}

_POISONAPI void _REP_POISON(const void * addr, int flags, size_t count)
{
	_ReadWriteBarrier();
	__asm mov eax, flags
	__asm mov esi, addr
	__asm mov ecx, count
	__asm _emit 0f3h
	__asm _emit 00fh
	__asm _emit 027h
}
#endif

#if defined(__GNUC__)
_POISONAPI void _POISON(const void * addr, int flags)
{
	__asm__ __volatile__("" : : : "memory");
	__asm__ __volatile__(".byte 0x0f; .byte 0x27" : "=S"(addr) : "S"(addr), "a"(flags) : "memory");
}

_POISONAPI void _REP_POISON(const void * addr, int flags, size_t count)
{
	__asm__ __volatile__("" : : : "memory");
	__asm__ __volatile__("rep; .byte 0x0f; .byte 0x27" : "=S"(addr), "=c"(count) : "S"(addr), "a"(flags), "c"(count) : "memory");
}
#endif

_POISONAPI void POISON_BYTE_R(const void * addr) { _POISON(addr, _POISON_R); }
_POISONAPI void POISON_BYTE_W(const void * addr) { _POISON(addr, _POISON_W); }
_POISONAPI void POISON_BYTE_RW(const void * addr) { _POISON(addr, _POISON_R | _POISON_W); }
_POISONAPI void POISON_MEMORY_R(const void * addr, size_t count) { _REP_POISON(addr, _POISON_R, count); }
_POISONAPI void POISON_MEMORY_W(const void * addr, size_t count) { _REP_POISON(addr, _POISON_W, count); }
_POISONAPI void POISON_MEMORY_RW(const void * addr, size_t count) { _REP_POISON(addr, _POISON_R | _POISON_W, count); }

_POISONAPI void UNPOISON_BYTE_R(const void * addr) { _POISON(addr, _UNPOISON_R); }
_POISONAPI void UNPOISON_BYTE_W(const void * addr) { _POISON(addr, _UNPOISON_W); }
_POISONAPI void UNPOISON_BYTE_RW(const void * addr) { _POISON(addr, _UNPOISON_R | _UNPOISON_W); }
_POISONAPI void UNPOISON_MEMORY_R(const void * addr, size_t count) { _REP_POISON(addr, _UNPOISON_R, count); }
_POISONAPI void UNPOISON_MEMORY_W(const void * addr, size_t count) { _REP_POISON(addr, _UNPOISON_W, count); }
_POISONAPI void UNPOISON_MEMORY_RW(const void * addr, size_t count) { _REP_POISON(addr, _UNPOISON_R | _UNPOISON_W, count); }

#undef _POISONAPI

#undef _POISON_R
#undef _POISON_W
#undef _UNPOISON_R
#undef _UNPOISON_W

#endif

/* EOF */

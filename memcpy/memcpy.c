
#include <stdint.h>
#include "memcpy.h"

#define ALIGNED_MASK_128 0x0000000f
#define ALIGNED_MASK_256 0x0000001f

#ifdef _M_IX86
#define PTR_ALIGNED_MASK 0x03
#else
#define PTR_ALIGNED_MASK 0x07
#endif

void* aligned_memcpy_mmx(void* dst, const void* src, size_t num)
{
    if (((uintptr_t)dst & PTR_ALIGNED_MASK) ||
        ((uintptr_t)src & PTR_ALIGNED_MASK))
        return 0;

    _asm {
        push edi;
        push esi;
        push ebx;
        push ecx;
        push edx;

        mov edi, dst;
        mov esi, src;
        mov ebx, num;
        mov edx, num;

        and edx, 0x0000003f;
        and ebx, 0xffffff40;

        ; Use movsb if less then 64 bytes
        test ebx, ebx;
        jz __tail_bytes;

        ; Setup loop counter
        mov ecx, ebx;
        shr ecx, 6;

__copy_64b:
        movq mm0, [esi   ];
        movq mm1, [esi+8 ];
        movq mm2, [esi+16];
        movq mm3, [esi+24];
        movq mm4, [esi+32];
        movq mm5, [esi+40];
        movq mm6, [esi+48];
        movq mm7, [esi+56];

        movntq [edi   ], mm0;
        movntq [edi+8 ], mm1;
        movntq [edi+16], mm2;
        movntq [edi+24], mm3;
        movntq [edi+32], mm4;
        movntq [edi+40], mm5;
        movntq [edi+48], mm6;
        movntq [edi+56], mm7;

        add esi, 64;
        add edi, 64;

        loop __copy_64b;

        sfence;
        emms;

__tail_bytes:
        test edx, edx;
        jz __done;
        mov ecx, edx;
        rep movsb;

__done:
        pop edx;
        pop ecx;
        pop ebx;
        pop esi;
        pop edi;
    }

    return dst;
}

void* aligned_memcpy_sse2(void* dst, const void* src, size_t num)
{
    if (((uintptr_t)dst & PTR_ALIGNED_MASK) ||
        ((uintptr_t)src & PTR_ALIGNED_MASK))
        return 0;

    __asm {
        push edi;
        push esi;
        push ebx;
        push ecx;
        push edx;

        mov edi, dst;
        mov esi, src;
        mov ebx, num;
        mov edx, num;

        and edx, 0x0000007f;
        and ebx, 0xffffff80;

        ; Use movsb if less then 128 bytes
        test ebx, ebx;
        jz __tail_bytes;

        ; Setup loop counter
        mov ecx, ebx;
        shr ecx, 7;

__copy_128b:
        movdqa xmm0, [esi    ];
        movdqa xmm1, [esi+16 ];
        movdqa xmm2, [esi+32 ];
        movdqa xmm3, [esi+48 ];
        movdqa xmm4, [esi+64 ];
        movdqa xmm5, [esi+80 ];
        movdqa xmm6, [esi+96 ];
        movdqa xmm7, [esi+112];

        movntdq [edi    ], xmm0;
        movntdq [edi+16 ], xmm1;
        movntdq [edi+32 ], xmm2;
        movntdq [edi+48 ], xmm3;
        movntdq [edi+64 ], xmm4;
        movntdq [edi+80 ], xmm5;
        movntdq [edi+96 ], xmm6;
        movntdq [edi+112], xmm7;

        add esi, 128;
        add edi, 128;

        loop __copy_128b;

__tail_bytes:
        test edx, edx;
        jz __done;
        mov ecx, edx;
        rep movsb;

__done:
        pop edx;
        pop ecx;
        pop ebx;
        pop esi;
        pop edi;
    }

    return dst;
}

void* unaligned_memcpy_sse(void* dst, const void* src, size_t num)
{
    __asm {
        push edi;
        push esi;
        push ebx;
        push ecx;
        push edx;

        ; Handle unaligned bytes if dst address is not aligned
        mov edx, dst;
        mov ecx, 0x00000000;
        and edx, ALIGNED_MASK_128;
        test edx, edx;
        jz __already_aligned;

        ; Copy unaligned bytes
        mov edi, dst;
        mov esi, src;
        mov ecx, edx;
        rep movsb;
        sub num, ecx;

__already_aligned:
        mov edi, dst;
        mov esi, src;
        add edi, ecx;
        add esi, ecx;
        mov ebx, num;
        mov edx, num;

        and edx, 0x0000007f;
        and ebx, 0xffffff80;

        ; Use movsb if less than 128 bytes
        test ebx, ebx;
        jz __tail_bytes;

        ; Setup loop counter
            mov ecx, ebx;
        shr ecx, 7;

__copy_128b:
        movaps xmm0, [esi    ];
        movaps xmm1, [esi+16 ];
        movaps xmm2, [esi+32 ];
        movaps xmm3, [esi+48 ];
        movaps xmm4, [esi+64 ];
        movaps xmm5, [esi+80 ];
        movaps xmm6, [esi+96 ];
        movaps xmm7, [esi+112];

        movntps [edi    ], xmm0;
        movntps [edi+16 ], xmm1;
        movntps [edi+32 ], xmm2;
        movntps [edi+48 ], xmm3;
        movntps [edi+64 ], xmm4;
        movntps [edi+80 ], xmm5;
        movntps [edi+96 ], xmm6;
        movntps [edi+112], xmm7;

        add esi, 128;
        add edi, 128;

        loop __copy_128b;

__tail_bytes:
        test edx, edx;
        jz __done;
        mov ecx, edx;
        rep movsb;

__done:
        pop edx;
        pop ecx;
        pop ebx;
        pop esi;
        pop edi;
    }

    return dst;
}

void *unaligned_memcpy_avx(void* dst, const void* src, size_t num)
{
    __asm
    {
        push edi;
        push esi;
        push ebx;
        push ecx;
        push edx;
        
        ; Handle unaligned bytes if address is not aligned
        mov edx, dst;
        mov ecx, 0x00000000;
        and edx, ALIGNED_MASK_256;
        test edx, edx;
        jz __already_aligned;

        ; Copy unaligned bytes
        mov edi, dst;
        mov esi, src;
        mov ecx, edx;
        rep movsb;
        sub num, ecx;

__already_aligned:
        mov edi, dst;
        mov esi, src;
        add edi, ecx;
        add esi, ecx;
        mov ebx, num;
        mov edx, num;

        and edx, 0x000000ff;
        and ebx, 0xffffff00;

        ; Use movsb if less than 256 bytes
        test ebx, ebx;
        jz __tail_bytes;

        ; Setup loop counter
        mov ecx, ebx;
        shr ecx, 8;

__copy_256b:
        vmovupd ymm0, [esi    ];
        vmovupd ymm1, [esi+32 ];
        vmovupd ymm2, [esi+64 ];
        vmovupd ymm3, [esi+96 ];
        vmovupd ymm4, [esi+128];
        vmovupd ymm5, [esi+160];
        vmovupd ymm6, [esi+192];
        vmovupd ymm7, [esi+224];

        vmovupd [edi    ], ymm0;
        vmovupd [edi+32 ], ymm1;
        vmovupd [edi+64 ], ymm2;
        vmovupd [edi+96 ], ymm3;
        vmovupd [edi+128], ymm4;
        vmovupd [edi+160], ymm5;
        vmovupd [edi+192], ymm6;
        vmovupd [edi+224], ymm7;

        add esi, 256;
        add edi, 256;
        loop __copy_256b;

__tail_bytes:
        test edx, edx;
        jz __done;
        mov ecx, edx;
        rep movsb;

__done:
        pop edx;
        pop ecx;
        pop ebx;
        pop esi;
        pop edi;
    }

    return dst;
}
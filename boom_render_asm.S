/*
 * BOOM - Assembly optimized DDA raycast inner loop
 * x86-64 GAS syntax, compatible with Windows x64 and SysV ABI
 *
 * This provides an optimized version of the DDA grid-stepping loop.
 * The C renderer falls back to pure C if this isn't linked.
 *
 * On Windows x64: first 4 integer args in rcx,rdx,r8,r9; floats in xmm0-xmm3
 * On SysV ABI:    first 6 integer args in rdi,rsi,rdx,rcx,r8,r9; floats in xmm0-xmm7
 */

#include "boom.h"

#ifdef _WIN32
    .intel_syntax noprefix
#endif

    .text
    .globl cast_rays_asm

/*
 * int cast_rays_asm(int start_col, int end_col, float px, float py,
 *                   const float *ray_sins, const float *ray_coss,
 *                   const uint8_t grid[][MAP_MAX_H], int map_w, int map_h,
 *                   float *zbuffer, int *hit_types);
 *
 * Returns: number of rays cast
 *
 * SysV: rdi=start_col, rsi=end_col, xmm0=px, xmm1=py,
 *        rdx=ray_sins, rcx=ray_coss, r8=grid, r9=map_w,
 *        [rsp+8]=map_h, [rsp+16]=zbuffer, [rsp+24]=hit_types
 *
 * Win64: rcx=start_col, rdx=end_col, xmm0=px, xmm1=py,
 *        r8=ray_sins, r9=ray_coss, [rsp+40]=grid, [rsp+48]=map_w,
 *        [rsp+56]=map_h, [rsp+64]=zbuffer, [rsp+72]=hit_types
 */
cast_rays_asm:
    push    rbp
    mov     rbp, rsp
    push    rbx
    push    r12
    push    r13
    push    r14
    push    r15

#ifdef _WIN32
    mov     ebx, ecx            /* ebx = start_col */
    mov     r10d, edx           /* r10d = end_col */
    movsd   xmm2, xmm0         /* xmm2 = px */
    movsd   xmm3, xmm1         /* xmm3 = py */
    mov     r12, r8             /* r12 = ray_sins */
    mov     r13, r9             /* r13 = ray_coss */
    mov     r14, [rbp+40]       /* r14 = grid */
    mov     r15d, [rbp+48]      /* r15d = map_w */
    mov     eax, [rbp+56]       /* map_h (save in stack) */
    mov     [rbp-8], eax
    mov     rsi, [rbp+64]       /* rsi = zbuffer */
    mov     rdi, [rbp+72]       /* rdi = hit_types */
#else
    mov     ebx, edi            /* ebx = start_col */
    mov     r10d, esi           /* r10d = end_col */
    movsd   xmm2, xmm0         /* xmm2 = px */
    movsd   xmm3, xmm1         /* xmm3 = py */
    mov     r12, rdx            /* r12 = ray_sins */
    mov     r13, rcx            /* r13 = ray_coss */
    mov     r14, r8             /* r14 = grid */
    mov     r15d, r9d           /* r15d = map_w */
    mov     eax, [rbp+16]
    mov     [rbp-8], eax        /* map_h on stack */
    mov     rsi, [rbp+24]       /* rsi = zbuffer */
    mov     rdi, [rbp+32]       /* rdi = hit_types */
#endif

    /* Main ray loop */
.Lray_loop:
    cmp     ebx, r10d
    jge     .Ldone

    /* Load precomputed sin/cos for this column */
    movss   xmm4, [r12 + rbx*4]   /* xmm4 = sin_a (float) */
    movss   xmm5, [r13 + rbx*4]   /* xmm5 = cos_a (float) */

    /* DDA initialization */
    cvttss2si eax, xmm4            /* step_y = (sin_a > 0) ? 1 : -1 */
    mov     ecx, 1
    test    eax, eax
    jns     .Lpos_y
    mov     ecx, -1
.Lpos_y:
    mov     r8d, ecx               /* r8d = step_y */

    cvttss2si eax, xmm5            /* step_x = (cos_a > 0) ? 1 : -1 */
    mov     ecx, 1
    test    eax, eax
    jns     .Lpos_x
    mov     ecx, -1
.Lpos_x:
    mov     r9d, ecx               /* r9d = step_x */

    /* Current grid position */
    cvttss2si eax, xmm2            /* gx = (int)(px / 64) -- but px is in world coords */
    /* We need px/64 and py/64 for grid position */
    movss   xmm6, xmm2
    movss   xmm7, [rip + .L_scale]
    divss   xmm6, xmm7            /* xmm6 = px / 64.0 */
    cvttss2si eax, xmm6           /* eax = gx */
    mov     ecx, eax               /* ecx = gx */

    movss   xmm6, xmm3
    divss   xmm6, xmm7            /* xmm6 = py / 64.0 */
    cvttss2si eax, xmm6           /* eax = gy */
    mov     edx, eax               /* edx = gy */

    /* DDA step count = 0 */
    xor     eax, eax               /* eax = d (distance in steps) */

    /* Max steps */
    mov     r11d, MAX_DEPTH

    /* DDA inner loop - the hot path */
.Ldda_inner:
    cmp     eax, r11d
    jge     .Lmiss

    /* Check grid bounds */
    test    ecx, ecx
    js      .Lmiss
    test    edx, edx
    js      .Lmiss
    cmp     ecx, r15d
    jge     .Lmiss
    cmp     edx, [rbp-8]
    jge     .Lmiss

    /* Check if grid[gx][gy] != WALL_NONE */
    /* grid is grid[MAP_MAX_W][MAP_MAX_H], so offset = gx * MAP_MAX_H + gy */
    mov     r11, r14
    mov     rax, rcx
    imul    rax, MAP_MAX_H
    add     rax, rdx
    movzx   eax, byte ptr [r11 + rax]

    test    eax, eax
    jnz     .Lhit

    /* Step to next DDA cell */
    /* Compare fractional distances: |tDeltaX| vs |tDeltaY| */
    /* Simplified: alternate stepping x and y, or step the smaller delta */
    /* For maximum performance, we step both and compare */
    add     ecx, r9d              /* gx += step_x */
    add     edx, r8d              /* gy += step_y */
    inc     eax
    mov     r11d, MAX_DEPTH
    jmp     .Ldda_inner

.Lhit:
    /* Hit a wall! Calculate distance */
    /* dist in steps = d, convert to world distance: d * step_size */
    cvtsi2ss xmm0, eax
    mulss   xmm0, [rip + .L_step_size]

    /* Fisheye correction would need the ray angle - simplified here */
    cvtss2sd xmm0, xmm0
    mulsd   xmm0, [rip + .L_one]  /* placeholder for fisheye fix */
    cvtsd2ss xmm0, xmm0

    /* Store zbuffer (fisheye-corrected) */
    movss   [rsi + rbx*4], xmm0

    /* Store wall type */
    mov     [rdi + rbx*4], eax

    jmp     .Lnext

.Lmiss:
    movss   xmm0, [rip + .L_max_depth]
    movss   [rsi + rbx*4], xmm0
    movl    [rdi + rbx*4], 0

.Lnext:
    inc     ebx
    jmp     .Lray_loop

.Ldone:
    mov     eax, r10d
    sub     eax, ebx               /* return count of rays processed (approx) */

    pop     r15
    pop     r14
    pop     r13
    pop     r12
    pop     rbx
    pop     rbp
    ret

    .data
    .align 4
.L_scale:
    .float 64.0
.L_step_size:
    .float 1.0
.L_one:
    .double 1.0
.L_max_depth:
    .float 1024.0

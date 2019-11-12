#ifndef DPMI_H_
#define DPMI_H_

#ifdef __DJGPP__
#include <dpmi.h>
#endif

#include "inttypes.h"

struct dpmi_regs {
	uint32_t edi, esi, ebp;
	uint32_t reserved;
	uint32_t ebx, edx, ecx, eax;
	uint16_t flags;
	uint16_t es, ds, fs, gs;
	uint16_t ip, cs, sp, ss;
};

uint16_t dpmi_alloc(unsigned int par, uint16_t *sel);
void dpmi_free(uint16_t sel);
void dpmi_int(int inum, struct dpmi_regs *regs);
void *dpmi_mmap(uint32_t phys_addr, unsigned int size);
void dpmi_munmap(void *addr);

#ifdef __WATCOMC__
#pragma aux dpmi_alloc = \
		"mov ax, 0x100" \
		"int 0x31" \
		"mov [edi], dx" \
		value[ax] \
		parm[ebx][edi];

#pragma aux dpmi_free = \
		"mov ax, 0x101" \
		"int 0x31" \
		parm[dx] \
		modify[ax];

#pragma aux dpmi_int = \
		"mov ax, 0x300" \
		"xor ecx, ecx" \
		"int 0x31" \
		parm[ebx][edi] \
		modify[ax ecx];

#pragma aux dpmi_mmap = \
		"mov ax, 0x800" \
		"mov cx, bx" \
		"shr ebx, 16" \
		"mov di, si" \
		"shr esi, 16" \
		"int 0x31" \
		"jnc mmap_skip_err" \
		"xor bx, bx" \
		"xor cx, cx" \
	"mmap_skip_err:" \
		"mov ax, bx" \
		"shl eax, 16" \
		"mov ax, cx" \
		value[eax] \
		parm[ebx][esi] \
		modify[cx di];

#pragma aux dpmi_munmap = \
		"mov ax, 0x801" \
		"mov cx, bx" \
		"shr ebx, 16" \
		"int 0x31" \
		parm[ebx] \
		modify[ax];
#endif	/* __WATCOMC__ */

#ifdef __DJGPP__
#define dpmi_int(inum, regs) __dpmi_int((inum), (__dpmi_regs*)(regs))
#endif

#endif	/* DPMI_H_ */

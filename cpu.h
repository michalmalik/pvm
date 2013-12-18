#ifndef __CPU_H__
#define __CPU_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <time.h>

#include "node.h"

#define PUSH(a)          p->mem[--p->sp] = a
#define POP()            p->mem[p->sp++]
#define OPO(op)         (op&0x3f)
#define OPD(op)         (op>>11)
#define OPS(op)         ((op>>6)&0x1f)
#define USINGNW(w)      ((w >= 0x10 && w <= 0x19))

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef int8_t s8;
typedef int16_t s16;
typedef int32_t s32;

enum _registers {
        rA, rB, rC, rD, rX, rY, rZ, rJ
};

enum _instructions {
        STO,
        ADD, SUB, MUL, DIV, MOD,
        NOT, AND, OR, XOR, SHL, SHR,
        MULS, DIVS, MODS,
        IFE, IFN, IFG, IFL, IFA, IFB,
        JMP, JTR, PUSH, POP, RET, RETI,
        IAR, INT, HWI, HWQ, HWN, HLT
};

struct cpu {
        u16 r[8];
        u16 sp, ip, ia, o;
        u16 mem[0x8000];
        u32 cycles;

        u32 halt;

        // Linked list of devices
        struct device *devices;
};

// union is not needed, we don't 
// need to access the mid and hid separately
struct device {
        struct device *next;

        u16 index; // hardware index
        u16 mid; // manufacter id
        u16 hid; // hardware id

        u32 (*_init)(struct cpu *);
        void (*_handle_interrupt)();
};

// We want to export these to the devices
void halt_cpu(struct cpu *p);

#endif
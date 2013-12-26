#ifndef __CPU_H__
#define __CPU_H__

#include "node.h"

#include <stdint.h>

#define PUSH(a)          p->mem[--p->sp] = a
#define POP()            p->mem[p->sp++]
#define OPO(op)         (op&0x3f)
#define OPD(op)         (op>>11)
#define OPS(op)         ((op>>6)&0x1f)
#define USINGNW(w)      ((w >= 0x10 && w <= 0x19))

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
        uint16_t r[8];
        uint16_t sp, ip, ia, o;
        uint16_t mem[0x8000];
        uint32_t cycles;

        uint32_t halt;

        // Linked list of devices
        struct device *devices;
};

// union is not needed, we don't 
// need to access the mid and hid separately
struct device {
        struct device *next;

        uint16_t index; // hardware index
        uint16_t mid; // manufacter id
        uint16_t hid; // hardware id

        uint32_t (*_init)(struct cpu *);
        void (*_handle_interrupt)();
};

// We want to export these to the devices
void halt_cpu(struct cpu *p);

#endif
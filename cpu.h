#ifndef __CPU_H__
#define __CPU_H__

#define zero(a)		(memset((a), 0, sizeof((a))))
#define count(a)	(sizeof((a))/sizeof((a)[0]))

#define OPO(op)         (op&0x3f)
#define OPD(op)         (op>>11)
#define OPS(op)         ((op>>6)&0x1f)
#define USINGNW(w)      ((w >= 0x10 && w <= 0x19))

typedef unsigned char u8;
typedef unsigned short u16;
typedef enum {
	FALSE, TRUE
} bool;

enum _registers {
        rA, rB, rC, rD, rX, rY, rZ, rJ
};

enum _instructions {
        STO,
        ADD, SUB, MUL, DIV, MOD,
        NOT, AND, OR, XOR, SHL, SHR,
        IFE, IFN, IFG, IFL, IFGE, IFLE,
        JMP, JTR, PUSH, POP, RET, RETI,
        IAR, INT, HWI, HWQ, HWN
};

struct cpu {
        u16 r[8];
        u16 sp, ip, ia;
        u8 of:1;
        u16 mem[0x8000];
        unsigned int cycles;

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

        unsigned int (*_init)(struct cpu *);
        void (*_handle_interrupt)(u16);
};

// We want to export these to the devices
void error(const char *format, ...);
void *zmalloc(size_t size);

#endif
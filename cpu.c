#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <time.h>

#define zero(a)     (memset((a),0,sizeof((a))))
#define count(a)    (sizeof((a))/sizeof((a)[0]))

// 1 Mhz
// nanoseconds
#define CPU_TICK                1000           

#define STACK_START             0x8000
#define STACK_LIMIT             0x2000

typedef unsigned char u8;
typedef unsigned short u16;

extern void disassemble(u16 *mem, u16 ip, char *out);

struct cpu {
        u16 r[8];
        u16 sp, ip, ia;
        u8 of:1;
        u16 mem[0x8000];
        int cycles;
};

static char *i_fn = NULL;
static char *o_fn = NULL;

void error(const char *format, ...) {
        char buf_1[512];
        char buf_2[256];
        va_list va;

        zero(buf_1);
        zero(buf_2);

        va_start(va, format);

        strcat(buf_1, "error: ");
        vsprintf(buf_2, format, va);
        strcat(buf_1, buf_2);   

        puts(buf_1);
        exit(1);
}

static void memory_dmp(struct cpu *p, const char *fn) {
        FILE *fp = NULL;
        size_t i = 0;

        if((fp = fopen(fn, "wb")) == NULL) {
                error("couldn't open/write to \"%s\"", fn);
        }

        while(i < 0x8000) {
                putc(p->mem[i]>>8, fp);
                putc(p->mem[i++]&0xff, fp);
        }

        fclose(fp);
}

static void debug(struct cpu *p) {
        printf("%2s: 0x%04X\n", "A", p->r[0]);
        printf("%2s: 0x%04X\n", "B", p->r[1]);
        printf("%2s: 0x%04X\n", "C", p->r[2]);
        printf("%2s: 0x%04X\n", "D", p->r[3]);
        printf("%2s: 0x%04X\n", "X", p->r[4]);
        printf("%2s: 0x%04X\n", "Y", p->r[5]);
        printf("%2s: 0x%04X\n", "Z", p->r[6]);
        printf("%2s: 0x%04X\n", "J", p->r[7]);
        printf("%2s: 0x%04X\n", "IA", p->ia);
        printf("%2s: 0x%04X\n", "SP", p->sp);
        printf("%2s: 0x%04X\n", "IP", p->ip);
        printf("%2s: 0x%04X\n", "OF", p->of);
        printf("%2s: %d\n", "CPU CYCLES", p->cycles);
        printf("\n");

        printf("Stack:\n");
        size_t i;
        for(i = 0; i < 0x40; i++) {
                printf("%04X ", p->mem[(STACK_START-0x40)+i]);
                if((i+1)%16==0) printf("\n");
        }
}

static void load(struct cpu *p, const char *fn) {
        FILE *fp = NULL;
        u8 b1 = 0, b2 = 0;
        u16 i = 0;

        if(!(fp = fopen(fn, "rb"))) {
                error("file \"%s\" not found or not readable", fn);
        }

        while(!feof(fp) && fscanf(fp, "%c%c", &b1, &b2) != EOF) {
                p->mem[i++] = ((b1<<8)|b2)&0xffff;
        }

        fclose(fp);
}

static u16 *getopr(struct cpu *p, u8 b, u16 *w) {
        u16 *o = NULL;

        if(b >= 0x10 && b <= 0x19) {
                w = p->mem+(p->ip++);
                p->cycles++;
        }

        switch(b) {
                // register
                case 0x00: case 0x01: case 0x02: case 0x03:
                case 0x04: case 0x05: case 0x06: case 0x07:
                        o = p->r+b;
                        break;
                // [register]
                case 0x08: case 0x09: case 0x0A: case 0x0B:
                case 0x0C: case 0x0D: case 0x0E: case 0x0F:
                        o = p->mem+p->r[b-0x08];
                        break;
                // [register+nextw]
                case 0x10: case 0x11: case 0x12: case 0x13:
                case 0x14: case 0x15: case 0x16: case 0x17:
                        o = p->mem+(p->r[b-0x10]+*w);
                        break;
                // nextw
                case 0x18:
                        o = w;
                        break;
                // [nextw]
                case 0x19:
                        o = p->mem+*w;
                        break;
                // SP
                case 0x1A:
                        o = &p->sp;
                        break;
                // IP
                case 0x1B:
                        o = &p->ip;
                        break;
                default: break;
        }
        return o;
}

static int word(u16 op) {
        u8 wc = 1;
        u16 o = op&0x3f;
        u8 d = op>>11;
        u8 s = (op>>6)&0x1f;

        if(d >= 0x10 && d <= 0x19) wc++;
        if(s >= 0x10 && s <= 0x19) wc++;

        return wc;
}

static void step(struct cpu *p) {
        char out[128] = {0};
        disassemble(p->mem, p->ip, out);
        printf("%s", out);

        int start_cyc = p->cycles;
        struct timespec slp = {0, 0};

        p->cycles++;

        u16 op = p->mem[p->ip++];

        u8 o = 0;
        u16 *d = 0, *s = 0, w = 0;
        u16 wc = 0;

        o = op&0x3f;
        d = getopr(p, op>>11, &w);
        s = getopr(p, (op>>6)&0x1f, &w);

        wc = word(*(p->mem+p->ip));

        switch(o) {
                // SET
                case 0: *d = *s; break;
                // ADD
                case 1: {
                        if(*s+*d>0xFFFF) p->of = 1; 
                        *d = (*s+*d)&0xffff; 
                        break;
                }
                // SUB
                case 2: *d = (*d-*s)&0xffff; break;
                // MUL
                case 3: *d = (*s*(*d))&0xffff; break;
                // DIV
                case 4: {
                        *d = (*d/(*s));
                        p->r[3] = (*d)%(*s);
                        p->cycles++;
                } break;
                // MOD
                case 5: {
                        *d = (*d)%(*s);
                } break;
                // NOT
                case 6: {
                        *d = ~(*d);
                        break;
                }
                // AND
                case 7: {
                        *d = (*d)&(*s);
                } break;
                // OR
                case 8: {
                        *d = (*d)|(*s);
                } break;
                // XOR
                case 9: {
                        *d = (*d)^(*s);
                } break;
                // SHL
                case 0x0A: {
                        *d = ((*d)<<(*s))&0xffff;
                } break;
                // SHR
                case 0x0B: {
                        *d = ((*d)>>(*s))&0xffff;
                } break;
                // IFE
                case 0x0C: {
                        if(*d != *s) p->ip += wc;
                } break;
                // IFN
                case 0x0D: {
                        if(*d == *s) p->ip += wc;
                } break;
                // IFG
                case 0x0E: {
                        if(*d <= *s) p->ip += wc;
                } break;
                // IFL
                case 0x0F: {
                        if(*d >= *s) p->ip += wc;
                } break;
                // IFGE
                case 0x10: {
                        if(*d < *s) p->ip += wc; 
                } break;
                // IFLE
                case 0x11: {
                        if(*d > *s) p->ip += wc;
                }
                // JMP
                case 0x12: {
                        p->ip = *d;
                } break;
                // JTR
                case 0x13: {
                        // push IP on stack
                        p->mem[--p->sp] = p->ip;
                        p->cycles++;
                        // jump
                        p->ip = *d; 
                } break;
                // PUSH
                case 0x14: {
                        p->mem[--p->sp] = *d;
                } break;
                // POP
                case 0x15: {
                        *d = p->mem[p->sp++];
                } break;
                // RET
                case 0x16: {
                        // pop IP from stack
                        p->ip = p->mem[p->sp++];
                } break;
                // RETI
                case 0x17: {
                        // pop IP from stack
                        p->ip = p->mem[p->sp++];
                        // set IA to 0, means we are returning
                        // from interrupt routine
                        p->ia = 0;
                } break;
                // IAR 
                case 0x18: {
                        // set IA to a value
                        // means we are expecting an interrupt routine
                        // to be entered
                        p->ia = *d;

                } break;
                // INT
                case 0x19: {
                        // set A to the message
                        p->r[0] = *d;
                        // push return IP to stack
                        p->mem[--p->sp] = p->ip;
                        // jump to IA
                        p->ip = p->ia;
                } break;
                default: break;
        }

        slp.tv_nsec = (p->cycles-start_cyc)*CPU_TICK;

        debug(p);

        nanosleep(&slp, NULL);
}

int main(int argc, char **argv) {
        if(argc < 3) {
                error("usage: %s <program> <memory_dump>", argv[0]);
        }

        i_fn = argv[1];
        o_fn = argv[2];

        static struct cpu p;
        zero(&p);
        zero(p.mem);

        load(&p, i_fn);

        p.cycles = 0;
        p.sp = STACK_START;

        int run = 0, c = 0;
        int w1 = 0, w2 = 0;
        do {
                if(p.sp < STACK_START-STACK_LIMIT-1) {
                        error("STACK_LIMIT(%04X) reached", STACK_LIMIT);
                }

                if(!run) {
                        c = getchar();
                } else {
                        step(&p);
                        continue;
                }

                if(c == 's') {
                        step(&p);       
                } else if(c == 'd') {
                        scanf("%04X", &w1);
                        printf("[0x%04X] = %04X\n", w1, p.mem[w1]);
                } else if(c == 'e') {
                        scanf("%04X %04X", &w1, &w2);
                        printf("([0x%04X] = %04X) -> %04X\n", w1, p.mem[w1], w2);
                        p.mem[w1] = w2;
                } else if(c == 'r') {
                        run = 1;
                }
        } while(p.mem[p.ip]);

        memory_dmp(&p, o_fn);

        return 0;
}
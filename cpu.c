#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

typedef unsigned char u8;
typedef unsigned short u16;

extern void disassemble(u16 *mem, u16 ip, char *out);

#define zero(a)     (memset((a),0,sizeof((a))))
#define count(a)    (sizeof((a))/sizeof((a)[0]))

struct cpu {
        u16 r[7];
        u16 sp, ip;
        u8 f : 3;
        u16 mem[0x8000];
        int end;
};

FILE *debug_fp = NULL;
char *i_fn = NULL;
char *o_fn = NULL;

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
        fprintf(debug_fp, "%2s: 0x%04X\n", "A", p->r[0]);
        fprintf(debug_fp, "%2s: 0x%04X\n", "B", p->r[1]);
        fprintf(debug_fp, "%2s: 0x%04X\n", "C", p->r[2]);
        fprintf(debug_fp, "%2s: 0x%04X\n", "D", p->r[3]);
        fprintf(debug_fp, "%2s: 0x%04X\n", "X", p->r[4]);
        fprintf(debug_fp, "%2s: 0x%04X\n", "Y", p->r[5]);
        fprintf(debug_fp, "%2s: 0x%04X\n", "Z", p->r[6]);
        fprintf(debug_fp, "%2s: 0x%04X\n", "SP", p->sp);
        fprintf(debug_fp, "%2s: 0x%04X\n", "IP", p->ip);
        fprintf(debug_fp, "%2s: 0x%02X\n", "E", p->f&1);
        fprintf(debug_fp, "%2s: 0x%02X\n", "S", (p->f&2)>>1);
        fprintf(debug_fp, "%2s: 0x%02X\n", "O", (p->f&4)>>2);
        fprintf(debug_fp, "\n");

        printf("%2s: 0x%04X\n", "A", p->r[0]);
        printf("%2s: 0x%04X\n", "B", p->r[1]);
        printf("%2s: 0x%04X\n", "C", p->r[2]);
        printf("%2s: 0x%04X\n", "D", p->r[3]);
        printf("%2s: 0x%04X\n", "X", p->r[4]);
        printf("%2s: 0x%04X\n", "Y", p->r[5]);
        printf("%2s: 0x%04X\n", "Z", p->r[6]);
        printf("%2s: 0x%04X\n", "SP", p->sp);
        printf("%2s: 0x%04X\n", "IP", p->ip);
        printf("%2s: 0x%04X\n", "E", p->f&1);
        printf("%2s: 0x%04X\n", "S", (p->f&2)>>1);
        printf("%2s: 0x%04X\n", "O", (p->f&4)>>2);
        printf("\n");
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

        if(b >= 0x15 && b <= 0x1D) {
                w = p->mem+(p->ip++);
        }

        switch(b) {
                case 0x00: case 0x01: case 0x02: case 0x03:
                case 0x04: case 0x05: case 0x06:
                        o = p->r+b;
                        break;
                case 0x07: case 0x08: case 0x09: case 0x0A:
                case 0x0B: case 0x0C: case 0x0D:
                        o = p->mem+p->r[b-0x07];
                        break;
                case 0x0E: case 0x0F: case 0x10: case 0x11: 
                case 0x12: case 0x13: case 0x14:
                        o = p->mem+(p->r[b-0x0E]+p->r[0]);
                        break;
                case 0x15: case 0x16: case 0x17: case 0x18:
                case 0x19: case 0x1A: case 0x1B:
                        o = p->mem+(p->r[b-0x15]+*w);
                        break;
                case 0x1C:
                        o = w;
                        break;
                case 0x1D:
                        o = p->mem+*w;
                        break;
                case 0x1E:
                        o = &p->sp;
                        break;
                case 0x1F:
                        o = &p->ip;
                        break;
                default: break;
        }
        return o;
}

static void step(struct cpu *p) {
        char out[128];
        zero(out);
        disassemble(p->mem, p->ip, out);
        fprintf(debug_fp, "%s", out);
        printf("%s", out);

        u16 op = p->mem[p->ip++];
        u8 o = 0;
        u16 *d = 0, *s = 0, w = 0;

        o = op&0x3f;
        d = getopr(p, op>>11, &w);
        s = getopr(p, (op>>6)&0x1f, &w);

        switch(o) {
                case 1: *d = *s; break;
                case 2: {
                        if(*d == *s) p->f |= 1;
                        if(*d != *s) p->f &= 2;
                        if(*d > *s) p->f |= 2;
                        if(*d < *s) p->f &= 1;
                } break;
                case 3: {
                        if(*s+*d>0xFFFF) {
                                p->f |= 4;
                        } 
                        *d = (*s+*d)&0xffff; 
                        break;
                }
                case 4: *d = (*d-*s)&0xffff; break;
                case 5: *d = (*s*(*d))&0xffff; break;
                case 6: {
                        *d = (*d/(*s));
                        p->r[3] = (*d)%(*s);
                } break;
                case 7: {
                        *d = (*d)%(*s);
                } break;
                case 8: {
                        *d = ~(*d);
                        break;
                }
                case 9: {
                        *d = (*d)&(*s);
                } break;
                case 0xA: {
                        *d = (*d)|(*s);
                } break;
                case 0xB: {
                        *d = (*d)^(*s);
                } break;
                case 0xC: {
                        *d = ((*d)<<(*s))&0xffff;
                } break;
                case 0xD: {
                        *d = ((*d)>>(*s))&0xffff;
                } break;
                case 0xE: {
                        p->ip = *d;;
                } break;
                case 0xF: {
                        if(p->f&1) p->ip = *d;
                } break;
                case 0x10: {
                        if(!(p->f&1)) p->ip = *d;
                } break;
                case 0x11: {
                        if(p->f&2) p->ip = *d;
                } break;
                case 0x12: {
                        if(!(p->f&2)) p->ip = *d;
                } break;
                case 0x13: {
                        if(p->f&1 || p->f&2) p->ip = *d;
                } break;
                case 0x14: {
                        if(p->f&1 || !(p->f&2)) p->ip = *d;
                } break;
                case 0x15: {
                        // push IP on stack
                        p->mem[p->sp--] = p->ip;
                        // jump
                        p->ip = *d; 
                } break;
                case 0x16: {
                        // pop IP from stack
                        p->ip = p->mem[++p->sp];
                        p->mem[p->sp] = 0;
                } break;
                case 0x17: {
                        p->mem[p->sp--] = *d;
                } break;
                case 0x18: {
                        *d = p->mem[++p->sp];
                        p->mem[p->sp] = 0;
                } break;
                case 0x19: {
                        p->end = 1;
                } break;
                default: break;
        }

        debug(p);

        printf("Stack:\n");
        size_t i;
        for(i = 0; i < 0x20; i++) {
                printf("%04X ", p->mem[0x7FFF-i]);
                if((i+1)%16==0) printf("\n");
        }
}

int main(int argc, char **argv) {
        if(argc < 3) {
                error("%s <program> <memory_dump>", argv[0]);
        }

        if((debug_fp = fopen("debug", "w")) == NULL) {
                error("debug file does not exist");
        }

        i_fn = argv[1];
        o_fn = argv[2];

        static struct cpu p;
        zero(&p);
        zero(p.mem);

        load(&p, i_fn);

        p.sp = 0x7FFF;
        p.end = 0;

        int c = 0;
        int w1 = 0, w2 = 0;
        do {
                // Step
                if(c == 's') {
                        step(&p);
                } else if(c == 'd') {
                        scanf("%04X", &w1);
                        printf("[0x%04X] = %04X\n", w1, p.mem[w1]);
                } else if(c == 'e') {
                        scanf("%04X %04X", &w1, &w2);
                        printf("([0x%04X] = %04X) -> %04X\n", w1, p.mem[w1], w2);
                        p.mem[w1] = w2;
                }
        } while(!p.end && (c = getchar()));

        memory_dmp(&p, o_fn);

        fclose(debug_fp);
	return 0;
}
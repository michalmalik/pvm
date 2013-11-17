#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>

/*

	TODO: Prettier assembler error messages.

*/

#define zero(a)     memset((a), 0, sizeof((a)))
#define count(a)    sizeof((a))/sizeof((a)[0])

typedef unsigned char u8;
typedef unsigned short u16;

union _mem {
	struct {
		u8 lo;
		u8 hi;
	} _b;
	u16 _w;
};

static union _mem MEM[0x8000];
static u16 IP = 0;

/*
	struct cdefine is a linked list

	WARNING:

	I am assuming DEFINES are NEVER forward referenced
*/
struct cdefine {
	struct cdefine *next;

	char name[128];
	u16 value;
};

struct file_asm {
	FILE *fp;
	char i_fn[64];
	char tokenstr[64];
	char linebuffer[256];
	
	int linenumber;

	char *lineptr;

	int token;
	u16 tokennum;

	struct cdefine *defines;
};

/*
	struct symbol and struct fix are 
	linked lists
*/
struct symbol {
	struct symbol *next;

	char name[128];
	u16 addr;
	u16 value;

	struct file_asm f;
};

struct fix {
	struct fix *next;

	char fn[64];
	char name[128];
	char linebuffer[256];
	int linenumber;

	u16 fix_addr;
	u16 const_val;
};

static struct file_asm *cf = NULL;

static struct symbol *symbols = NULL;
static struct fix *fixes = NULL;

static char *o_fn = NULL;
static char *sym_fn = NULL;

static const char *tn[] = {
	"A", "B", "C", "D", "X", "Y", "Z", "J",
	"SP", "IP", "IA",
	"STO",
	"ADD", "SUB", "MUL", "DIV", "MOD",
	"NOT", "AND", "OR", "XOR", "SHL", "SHR",
	"IFE", "IFN", "IFG", "IFL", "IFGE", "IFLE", "JMP", "JTR",
	"PUSH", "POP", "RET", "RETI", "IAR", "INT", 
	"DAT", 
	".", "#", ":", ",", "[", "]", "+",
	"(NUMBER)", "(STRING)", "(QUOTED STRING)", "(EOL)", "(EOF)"
};

enum {
	tA, tB, tC, tD, tX, tY, tZ, tJ,
	tSP, tIP, tIA,
	tSTO, 
	tADD, tSUB, tMUL, tDIV, tMOD,
	tNOT, tAND, tOR, tXOR, tSHL, tSHR,
	tIFE, tIFN, tIFG, tIFL, tIFGE, tIFLE, tJMP, tJTR,
	tPUSH, tPOP, tRET, tRETI, tIAR, tINT,
	tDAT,
	tDOT, tHASH, tCOLON, tCOMMA, tBS, tBE, tPLUS,
	tNUM, tSTR, tQSTR, tEOL, tEOF
};

#define LASTTOKEN	tEOF

void die(const char *format, ...) {
	char buf[512] = {0};
	va_list va;

	va_start(va, format);

	vsprintf(buf, format, va);

	puts(buf);
	exit(1);
}

void error(const char *format, ...) {
	char buf_1[512] = {0};
	char buf_2[256] = {0};
	va_list va;

	va_start(va, format);

	sprintf(buf_1, "%s: line %d, error: ", cf->i_fn, cf->linenumber);
	vsprintf(buf_2, format, va);
	strcat(buf_1, buf_2);	

	puts(buf_1);
	exit(1);
}

void init_asm(const char *i_fn) {
	struct file_asm fs;
	size_t i;

	fs.fp = NULL;
	zero(fs.i_fn);
	zero(fs.tokenstr);
	zero(fs.linebuffer);

	fs.linenumber = 0;

	fs.lineptr = fs.linebuffer;
	fs.token = 0;
	fs.tokennum = 0;

	fs.defines = NULL;

	strcpy(fs.i_fn, i_fn);

	if((fs.fp = fopen(i_fn, "r")) == NULL) {
		if(cf->fp) {
			die("%s: error: line %d, file \"%s\" does not exist \n\n%s", cf->i_fn, 
				cf->linenumber, fs.i_fn, cf->linebuffer);
		} else {
			die("error: file \"%s\" does not exist", fs.i_fn);
		}
	}

	*cf = fs;
}

void add_symbol(const char *name, u16 addr, u16 *value) {
	struct symbol *s = NULL;
	for(s = symbols; s; s = s->next) {
		if(!strcmp(name, s->name)) {
			error("symbol \"%s\" already defined here:\n%s: line %d: %s", name, s->f.i_fn, s->f.linenumber, s->f.linebuffer);
		}
	}
	s = (struct symbol *)malloc(sizeof(struct symbol));

	zero(s->name);

	s->addr = addr;
	strcpy(s->name, name);
	if(value) s->value = *value;
	s->f = *cf;

	s->next = symbols;
	symbols = s;
}

void fix_symbol(const char *name, u16 addr, u16 *const_val) {
	struct fix *f = (struct fix *)malloc(sizeof(struct fix));

	f->next = NULL;
	zero(f->fn);
	zero(f->name);
	zero(f->linebuffer);

	strcpy(f->name, name);
	strcpy(f->fn, cf->i_fn);
	strcpy(f->linebuffer, cf->linebuffer);

	f->fix_addr = addr;
	f->linenumber = cf->linenumber;
	if(const_val) f->const_val = *const_val;

	f->next = fixes;
	fixes = f;
}

void add_define(const char *name, u16 value) {
	struct cdefine *d = NULL;
	for(d = cf->defines; d; d = d->next) {
		if(!strcmp(d->name, name))
			error("define \"%s\" already defined", name);
	}

	d = (struct cdefine *)malloc(sizeof(struct cdefine));

	zero(d->name);

	strcpy(d->name, name);
	d->value = value;

	d->next = cf->defines;
	cf->defines = d;
}

struct symbol *get_symbol(const char *name) {
	struct symbol *s = NULL;
	for(s = symbols; s; s = s->next) {
		if(!strcmp(s->name, name)) return s;
	}
	return NULL;
}

struct cdefine *get_define(const char *name) {
	struct cdefine *d = NULL;
	for(d = cf->defines; d; d = d->next) {
		if(!strcmp(d->name, name)) return d;
	}
	return NULL;
}

int is_defined(const char *name) {
	struct symbol *s = NULL;
	struct cdefine *d = NULL;
	for(s = symbols; s; s = s->next) {
		if(!strcmp(s->name, name)) return 1;
	}
	for(d = cf->defines; d; d = d->next) {
		if(!strcmp(d->name, name)) return 1;
	}
	return 0;
}

void fix_symbols() {
	struct symbol *s = NULL;
	struct fix *f = NULL;
	for(f = fixes; f; f = f->next) {
		s = get_symbol(f->name);
		if(!s) {
			die("%s: error: line %d, symbol \"%s\" is not defined\n\n%s", f->fn, f->linenumber,
				 f->name, f->linebuffer);
		}
		// [symbol + const_val]
		// symbol is always represented as its address in memory
		// and const_val can be added to it
		MEM[f->fix_addr]._w += s->addr + f->const_val;
	}
}

void dump_symbols(const char *fn) {
	FILE *fp = NULL;
	struct symbol *s = NULL;

	if((fp = fopen(fn, "wb")) == NULL) {
		die("error: couldn't open/write to \"%s\"", fn);
	}

	fputs("Symbols\n-----------\n", fp);
	for(s = symbols; s; s = s->next) {
		fprintf(fp, "%s %s 0x%04X 0x%04X\n", s->f.i_fn, s->name, s->addr, s->value);
	}

	fclose(fp);
}

void free_defines_list() {
	struct cdefine *head = cf->defines, *tmp = NULL;
	while(head) {
		tmp = head;
		head = head->next;
		free(tmp);
	}
	free(head);
}

int next_token() {
	char c = 0, *x = 0;
	size_t i;
next_line:
	if(!*cf->lineptr) {
		if(feof(cf->fp)) return tEOF;
		if(fgets(cf->linebuffer, 256, cf->fp) == 0) return tEOF;
		cf->lineptr = cf->linebuffer;

		cf->linenumber++;
	}

	if(*cf->lineptr == '\n' || *cf->lineptr == '\r') {
		*cf->lineptr = 0;
		return tEOL;
	}

	while(*cf->lineptr <= ' ') {
		if(*cf->lineptr == 0) goto next_line;
		cf->lineptr++;
	}
	
	switch((c = *cf->lineptr++)) {
		case '.': return tDOT;
		case '#': return tHASH;
		case ':': return tCOLON;
		case ',': return tCOMMA;
		case '[': return tBS;
		case ']': return tBE;
		case '+': return tPLUS;
		case ';': {
			*cf->lineptr = 0;
			goto next_line;
		}
		case '"': {
			zero(cf->tokenstr);

			x = cf->tokenstr;
			for(;;) {
				switch((c = *cf->lineptr++)) {
					case '"': {
						return tQSTR;
						break;
					}
					default: {
						*x++ = c;
						break;
					}
				}
			}
			break;
		}
		default: {
			if(isalpha(c)) {
				zero(cf->tokenstr);

				cf->lineptr--;				
				x = cf->tokenstr;
				while(isalnum(*cf->lineptr) || *cf->lineptr == '_') {
					*x++ = *cf->lineptr++;
				}
				*x = 0;
				int i;
				for(i = 0; i < LASTTOKEN; i++) {
					if(!strcasecmp(tn[i], cf->tokenstr)) {
						return i;
					}
				}
				return tSTR;
			} else if(isdigit(c)) {
				cf->lineptr--;
				cf->tokennum = strtoul(cf->lineptr, &cf->lineptr, 0);
				return tNUM;				
			}
			break;	
		}
	}
	return -1;
}

int next() {
	return (cf->token = next_token());
}

void expect(int t) {
	if(next() != t) {
		error("expected: \"%s\", got: \"%s\"", tn[t], tn[cf->token]);		
	}
}

void assemble_label(const char *name, int *v) {
	// Symbols have higher priority
	struct symbol *s = get_symbol(name);
	struct cdefine *d = get_define(name);

	if(*v == -1) {
		if(s) *v = s->addr;
		else {
			if(d) *v = d->value;
			else {
				fix_symbol(name, IP+1, NULL);
				*v = 0;
			}
		}
	} else {
		if(s) *v += s->addr;
		else {
			if(d) *v += d->value;
			else {
				fix_symbol(name, IP+1, (u16 *)v);
				*v = 0;
			}
		}
	}
}

void assemble_o(u16 *o, int *v) {
	next();

	switch(cf->token) {
		// register
		case tA: case tB: case tC: case tD:
		case tX: case tY: case tZ: case tJ: {
			*o = cf->token&0x7;
		} break;

		// SP
		case tSP: {
			*o = 0x1A;
		} break;

		// IP
		case tIP: {
			*o = 0x1B;
		} break;

		// nextw
		case tNUM: {
			*o = 0x18;
			*v = cf->tokennum;
		} break;

		// nextw
		case tSTR: {
			*o = 0x18;
			assemble_label(cf->tokenstr, v);
		} break;

		default: {
			if(cf->token != tBS) error("expected operand\n\n%s", cf->linebuffer);
		} break;
	}

	if(cf->token == tBS) {
		next();

		if(cf->token <= 7) {
			*o = cf->token&7;
			next();
			if(cf->token == tPLUS) {
				next();

				// [register + nextw]
				if(cf->token == tNUM) {
					*o += 0x10;
					*v = cf->tokennum;
				}
				// [register + nextw]
				else if(cf->token == tSTR) {
					*o += 0x10;
					assemble_label(cf->tokenstr, v);
				} else {
					error("expected [register+nextw]\n\n%s", cf->linebuffer);
				}
				next();
			// [register]
			} else if(cf->token == tBE) {
				*o += 0x8;
			} 
		}

		if(cf->token == tNUM) {
			// [nextw]
			*v = cf->tokennum;
			*o = 0x19;
			next();
			if(cf->token == tPLUS) {
				next();
				// [nextw]
				if(cf->token == tNUM) {
					*v += cf->tokennum;
				// [register + nextw]
				} else if(cf->token <= 7) {
					*o = (cf->token&7)+0x10;
				// [nextw]
				} else if(cf->token == tSTR) {
					assemble_label(cf->tokenstr, v);
				} else {
					error("expected value\n\n%s", cf->linebuffer);
				}
				next();
			}
		}

		if(cf->token == tSTR) {
			*o = 0x19;
			*v = 0;
			int f_token = cf->token;
			int s_token = -1;
			int fdef = -1;
			int sdef = -1;

			char f_buf[128] = {0};
			char s_buf[128] = {0};
			strcpy(f_buf, cf->tokenstr);

			next();
			if(cf->token == tPLUS) {
				next();
				if(cf->token == tNUM) {
					*v += cf->tokennum;
				} else if(cf->token <= 7) {
					*o = (cf->token&7)+0x10;
				} else if(cf->token == tSTR) {
					s_token = cf->token;
					strcpy(s_buf, cf->tokenstr);
				} else {
					error("expected value\n\n%s", cf->linebuffer);
				}
				next();
			}

			// [defined + NON_STR]
			// [undefined + NON_STR]
			if(s_token == -1) {
				assemble_label(f_buf, v);
			} else {
				fdef = is_defined(f_buf);
				sdef = is_defined(s_buf);

				// [defined + defined]
				// [defined + undefined]
				if(fdef && (sdef || !sdef)) {
					assemble_label(f_buf, v);
					assemble_label(s_buf, v);
				} 
				// [undefined + defined]
				if(!fdef && sdef) {
					assemble_label(s_buf, v);
					assemble_label(f_buf, v);
				}
				// [undefined + undefined]
				if(!fdef && !sdef) {
					*v = -1;
					assemble_label(f_buf, v);
					assemble_label(s_buf, v);
				}
			}
		}
		
		if(cf->token != tBE) 
			error("expected ]\n\n%s", cf->linebuffer);
	}
} 

void assemble_i(int inst, u16 d, u16 s, int v1, int v2) {
	u16 o = 0;
	switch(inst) {
		case tSTO: o = 0; break;
		case tADD: o = 1; break;
		case tSUB: o = 2; break;
		case tMUL: o = 3; break;
		case tDIV: o = 4; break;
		case tMOD: o = 5; break;
		case tNOT: o = 6; break;
		case tAND: o = 7; break;
		case tOR: o = 8; break;
		case tXOR: o = 9; break;
		case tSHL: o = 0x0A; break;
		case tSHR: o = 0x0B; break;
		case tIFE: o = 0x0C; break;
		case tIFN: o = 0x0D; break;
		case tIFG: o = 0x0E; break;
		case tIFL: o = 0x0F; break;
		case tIFGE: o = 0x10; break;
		case tIFLE: o = 0x11; break;
		case tJMP: o = 0x12; break;
		case tJTR: o = 0x13; break;
		case tPUSH: o = 0x14; break;
		case tPOP: o = 0x15; break;
		case tRET: o = 0x16; break;
		case tRETI: o = 0x17; break;
		case tIAR: o = 0x18; break;
		case tINT: o = 0x19; break;
	}
	MEM[IP++]._w = (u16)(o|(s<<6)|(d<<11));
	if(v1 != -1) MEM[IP++]._w = v1&0xFFFF;		
	if(v2 != -1) MEM[IP++]._w = v2&0xFFFF;
}

int assemble() {
	u16 s = 0, d = 0;
	int v1 = -1, v2 = -1;
	int t;
	u16 b = 0;
	struct file_asm tf;
	size_t i;

	for(;;) {
		next();
again:
		v1 = -1;
		v2 = -1;
		t = cf->token;

		switch(t) {
			case tEOF: goto done;
			case tEOL: {
				next();
				goto again;
			}
			case tCOLON: {
				expect(tSTR);
				add_symbol(cf->tokenstr, IP, &IP);
				break;
			}
			case tHASH: {
				expect(tSTR);
				if(!strcasecmp(cf->tokenstr, "include")) {			
					expect(tQSTR);

					char buf[64] = {0};
					strcpy(buf, cf->tokenstr);

					tf = *cf;
					init_asm(buf);
					assemble();
					*cf = tf;
				} else if(!strcasecmp(cf->tokenstr, "define")) {
					char name[128] = {0};
					expect(tSTR);
					strcpy(name, cf->tokenstr);
					expect(tNUM);
					add_define(name, cf->tokennum);
				}
				break;
			}
			case tDOT: {
				char sym_name[128] = {0};
				int add_sym = 1;

				expect(tSTR);
				strcpy(sym_name, cf->tokenstr);

				expect(tDAT);
				next();

			write_mem:
				switch(cf->token) {
					case tNUM: {
						if(add_sym) add_symbol(sym_name, IP, &cf->tokennum);
						MEM[IP++]._w = cf->tokennum;
					} break;
					case tQSTR: {
						b = 0 | cf->tokenstr[0];
						if(add_sym) add_symbol(sym_name, IP, &b);

						for(i = 0; i < strlen(cf->tokenstr); i++) {
							MEM[IP++]._b.lo = cf->tokenstr[i];
						}
					} break;
					default: {
						error("expected value after DAT\n\n%s", cf->linebuffer);
					} break;
				}

				next();
				if(cf->token == tCOMMA) {
					add_sym = 0;
					next();
					goto write_mem;
				}	

				goto again;
			} break;
			
			case tSTO:
			case tADD: case tSUB: case tMUL: case tDIV:
			case tMOD: case tAND: case tOR: case tXOR:
			case tSHL: case tSHR: 
			case tIFE: case tIFN: case tIFG: case tIFL:
			case tIFGE: case tIFLE: 
			{
				assemble_o(&d, &v1);
				expect(tCOMMA);				
				assemble_o(&s, &v2);
				assemble_i(t, d, s, v1, v2);
				break;
			}
			case tNOT:
			case tJMP: case tJTR: case tPUSH: case tPOP: 
			case tIAR: case tINT: {
				assemble_o(&d, &v1);
				assemble_i(t, d, 0, v1, -1);
				break;	
			}

			case tRET: case tRETI: {
				assemble_i(t, 0, 0, -1, -1);
				break;
			}

			case tDAT: {
				next();

			_write_mem:
				switch(cf->token) {
					case tNUM: {
						MEM[IP++]._w = cf->tokennum;
					} break;
					case tQSTR: {
						for(i = 0; i < strlen(cf->tokenstr); i++) {
							MEM[IP++]._b.lo = cf->tokenstr[i];
						}
					} break;
					default: {
						error("expected value after DAT\n\n%s");
					} break;
				}

				next();
				if(cf->token == tCOMMA) {
					next();
					goto _write_mem;
				}

				goto again;
			}

			default: {
				error("invalid instruction \"%s\"", cf->tokenstr);
				break;
			}
		}
	}
done:
	fclose(cf->fp);
	free_defines_list();
	return 1;
}

void output(const char *fn) {
	FILE *fp = NULL;
	size_t i;

	if((fp = fopen(fn, "wb")) == NULL) {
		die("error: couldn't open/write to \"%s\"", fn);
	}

	for(i = 0; i < count(MEM); i++) {
		fprintf(fp, "%c%c", MEM[i]._b.hi, MEM[i]._b.lo);
	}

	fclose(fp);
}

void free_symbol_list() {
	struct symbol *head = symbols, *tmp = NULL;
	while(head) {
		tmp = head;
		head = head->next;
		free(tmp);
	}
	free(head);
}

void free_fix_list() {
	struct fix *head = fixes, *tmp = NULL;
	while(head) {
		tmp = head;
		head = head->next;
		free(tmp);
	}
	free(head);
}

int main(int argc, char **argv) {
	if(argc < 4) {
		die("usage: %s <source_file> <program> <symbols>", argv[0]);
	}

	zero(MEM);

	o_fn = argv[2];
	sym_fn = argv[3];

	// Init file_asm structure for the main file
	cf = (struct file_asm *)malloc(sizeof(struct file_asm));
	init_asm(argv[1]); 
	assemble();

	fix_symbols();
	dump_symbols(sym_fn);
	
	output(o_fn);

	free_symbol_list();
	free_fix_list();
	free(cf);
	return 0;
}
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>

typedef unsigned char u8;
typedef unsigned short u16;

#define zero(a)     (memset((a),0,sizeof((a))))
#define count(a)    (sizeof((a))/sizeof((a)[0]))

#define DEFINES_LIMIT	64

static u16 MEM[0x8000];
static u16 IP = 0;

// TODO: Take strings as values for defines
struct cdefine {
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

	// TODO: Delete counter from main struct
	u16 defn;
	// TODO: More defines
	struct cdefine defines[DEFINES_LIMIT];
};

struct symbol {
	struct symbol *next;

	char name[128];
	u16 ip;
	u16 num_value;
	char str_value[128];

	struct file_asm f;
};

struct fix {
	struct fix *next;

	char fn[64];
	char name[128];
	char str_value[128];
	char linebuffer[256];

	u16 fix_addr;
	u16 num_value;

	int linenumber;
};

static struct file_asm mf;
static struct file_asm *cf = &mf;

static struct symbol *symbols = NULL;
static struct fix *fixes = NULL;

static char *o_fn = NULL;
static char *sym_fn = NULL;

static const char *tn[] = {
	"A", "B", "C", "D", "X", "Y", "Z", "J",
	"SP", "IP",
	"STO",
	"ADD", "SUB", "MUL", "DIV", "MOD",
	"NOT", "AND", "OR", "XOR", "SHL", "SHR",
	"IFE", "IFN", "IFG", "IFL", "IFGE", "IFLE", "JMP", "JTR",
	"PUSH", "POP", "RET", "DAT", 
	".", "#", ":", ",", "[", "]", "+",
	"(NUMBER)", "(STRING)", "(QUOTED STRING)", "(EOF)"
};

enum _tokens {
	tA, tB, tC, tD, tX, tY, tZ, tJ,
	tSP, tIP,
	tSTO, 
	tADD, tSUB, tMUL, tDIV, tMOD,
	tNOT, tAND, tOR, tXOR, tSHL, tSHR,
	tIFE, tIFN, tIFG, tIFL, tIFGE, tIFLE, tJMP, tJTR,
	tPUSH, tPOP, tRET, tDAT,
	tDOT, tHASH, tCOLON, tCOMMA, tBS, tBE, tPLUS,
	tNUM, tSTR, tQSTR, tEOF
};

#define LASTTOKEN	tEOF

void die(const char *format, ...) {
	char buf[512];
	va_list va;

	zero(buf);

	va_start(va, format);

	vsprintf(buf, format, va);

	puts(buf);
	exit(1);
}

void error(const char *format, ...) {
	char buf_1[512];
	char buf_2[256];
	va_list va;

	zero(buf_1);
	zero(buf_2);

	va_start(va, format);

	sprintf(buf_1, "%s: line %d, error: ", cf->i_fn, cf->linenumber);
	vsprintf(buf_2, format, va);
	strcat(buf_1, buf_2);	

	puts(buf_1);
	exit(1);
}

void cleanstr(char *str) {
	size_t i;
	for(i = 0; i < strlen(str); i++) {
		if(str[i]=='\n') str[i]='\0';
	}
}

void clean_fs(struct file_asm *fs) {
	fclose(fs->fp);
	zero(fs->i_fn);
	zero(fs->tokenstr);
	zero(fs->linebuffer);
	fs->linenumber = 0;
	fs->lineptr = NULL;
	fs->token = 0;
	fs->tokennum = 0;
}

int init_asm(const char *i_fn) {
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

	fs.defn = 0;

	for(i = 0; i < count(fs.defines); i++) {
		zero(fs.defines[i].name);
		fs.defines[i].value = 0;
	}

	strcpy(fs.i_fn, i_fn);

	if((fs.fp = fopen(i_fn, "r")) == NULL) {
		if(cf->fp) {
			die("%s: error: line %d, file \"%s\" does not exist \nin '%s'", cf->i_fn, 
				cf->linenumber, fs.i_fn, cf->linebuffer);
		} else {
			die("error: file \"%s\" does not exist", fs.i_fn);
		}
	}

	*cf = fs;

	return 1;
}

void output(const char *fn) {
	FILE *fp = NULL;
	size_t i;

	if((fp = fopen(fn, "wb")) == NULL) {
		die("error: couldn't open/write to \"%s\"", fn);
	}

	for(i = 0; i < count(MEM); i++) {
		fputc((MEM[i]>>8)&0xFF, fp);
		fputc(MEM[i]&0xFF, fp);
	}
	fclose(fp);
}

void add_symbol(const char *name, u16 ip, u16 *num_value, char *str_value) {
	struct symbol *s = NULL;
	for(s = symbols; s; s = s->next) {
		if(!strcmp(name, s->name)) {
			error("symbol \"%s\" already defined", name);
		}
	}
	s = malloc(sizeof(struct symbol));

	zero(s->name);
	zero(s->str_value);

	s->ip = ip;
	strcpy(s->name, name);
	if(num_value) s->num_value = *num_value;
	if(str_value) strcpy(s->str_value, str_value);
	s->f = *cf;
	s->next = symbols;
	symbols = s;
}

void to_fix(const char *name, u16 ip, u16 *num_value, char *str_value) {
	struct fix *f = malloc(sizeof(struct fix));
	size_t i;

	zero(f->name);
	zero(f->str_value);
	zero(f->fn);
	zero(f->linebuffer);

	f->fix_addr = ip;
	strcpy(f->name, name);

	strcpy(f->fn, cf->i_fn);
	f->linenumber = cf->linenumber;
	strcpy(f->linebuffer, cf->linebuffer);

	if(num_value) f->num_value = *num_value;
	if(str_value) strcpy(f->str_value, str_value);
	f->next = fixes;
	fixes = f;
}

struct symbol *get_symbol(const char *name) {
	struct symbol *s = NULL;
	for(s = symbols; s; s = s->next) {
		if(!strcmp(s->name, name)) return s;
	}
	return NULL;
}

void fix_symbols() {
	struct symbol *s = NULL;
	struct fix *f = NULL;
	for(f = fixes; f; f = f->next) {
		s = get_symbol(f->name);
		if(!s) {
			die("%s: error: line %d, symbol \"%s\" is not defined, \"%s\"", f->fn, f->linenumber,
				 f->name, f->linebuffer);
		}
		MEM[f->fix_addr] += s->ip + f->num_value;
	}
}

void dump_symbols(const char *fn) {
	FILE *fp = NULL;
	struct symbol *s = NULL;

	if((fp = fopen(fn, "wb")) == NULL) {
		die("error: couldn't open/write to \"%s\"", fn);
	}

	for(s = symbols; s; s = s->next) {
		if(s->str_value[0]) fprintf(fp, "%s %s 0x%04X \"%s\"\n", s->f.i_fn, s->name, s->ip, s->str_value);
		else fprintf(fp, "%s %s 0x%04X 0x%04X\n", s->f.i_fn, s->name, s->ip, s->num_value);
	}

	fclose(fp);
}

void add_define(const char *name, u16 value) {
	size_t i;
	if(cf->defn >= DEFINES_LIMIT) error("DEFINES_LIMIT(%d) reached for this file", DEFINES_LIMIT);

	for(i = 0; i < DEFINES_LIMIT; i++) {
		if(!strcmp(cf->defines[i].name, name)) error("define \"%s\" already defined", name);
	}

	strcpy(cf->defines[cf->defn].name, name);
	cf->defines[cf->defn].value = value;

	cf->defn++;
}

struct cdefine *get_define(const char *name) {
	size_t i;
	for(i = 0; i < DEFINES_LIMIT; i++) {
		if(!strcmp(cf->defines[i].name, name)) return &cf->defines[i];
	}
	return NULL;
}

int is_defined(const char *name) {
	struct symbol *s;
	size_t i;
	for(s = symbols; s; s = s->next) {
		if(!strcmp(s->name, name)) return 1;
	}
	for(i = 0; i < DEFINES_LIMIT; i++) {
		if(!strcmp(cf->defines[i].name, name)) return 1;
	}
	return 0;
}

int next_token() {
	char c = 0, *x = 0;
	zero(cf->tokenstr);
next_line:
	if(!*cf->lineptr) {
		if(feof(cf->fp)) return tEOF;
		if(fgets(cf->linebuffer, 256, cf->fp) == 0) return tEOF;
		cf->lineptr = cf->linebuffer;
		cleanstr(cf->linebuffer);
		cf->linenumber++;
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
		if(s) *v = s->ip;
		else {
			if(d) *v = d->value;
			else {
				to_fix(name, IP+1, NULL, NULL);
				*v = 0;
			}
		}
	} else {
		if(s) *v += s->ip;
		else {
			if(d) *v += d->value;
			else {
				to_fix(name, IP+1, (u16 *)v, NULL);
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
			break;
		}

		// SP
		case tSP: {
			*o = 0x1A;
			break;
		}

		// IP
		case tIP: {
			*o = 0x1B;
			break;
		}

		// nextw
		case tNUM: {
			*o = 0x18;
			*v = cf->tokennum;
			break;
		}

		// nextw
		case tSTR: {
			*o = 0x18;
			assemble_label(cf->tokenstr, v);
			break;
		}

		default: {
			if(cf->token != tBS) error("expected [");
			break;
		}
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
					error("expected [register+nextw]");
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
					error("expected value");
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
					error("expected value");
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
			error("expected ]");
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
	}
	MEM[IP++] = (u16)(o|(s<<6)|(d<<11));
	if(v1 != -1) MEM[IP++] = v1&0xFFFF;		
	if(v2 != -1) MEM[IP++] = v2&0xFFFF;
}

void assemble_dat() {
	u16 b = 0;
	size_t i;

	next();
	while(cf->token == tCOMMA) {
		next();
		if(cf->token == tNUM) {
			MEM[IP++] = cf->tokennum;
		} else if(cf->token == tQSTR) {
			for(i = 0; i < strlen(cf->tokenstr); i++) {
				b = 0;
				b |= cf->tokenstr[i];
				MEM[IP++] = b; 
			}
			MEM[IP++] = 0;		
		}
		next();
	}
}

int assemble() {
	u16 s = 0, d = 0;
	int v1 = -1, v2 = -1;
	int t;
	u16 b = 0;
	size_t i;

	for(;;) {
		next();
again:
		v1 = -1;
		v2 = -1;
		t = cf->token;
		char sym[128];

		switch(t) {
			case tEOF: goto done;
			case tCOLON: {
				expect(tSTR);
				add_symbol(cf->tokenstr, IP, &IP, NULL);
				break;
			}
			case tHASH: {
				expect(tSTR);
				if(!strcasecmp(cf->tokenstr, "include")) {			
					expect(tQSTR);

					struct file_asm tf = *cf;

					char buf[64];
					zero(buf);
					strcpy(buf, cf->tokenstr);

					init_asm(buf);
					assemble();
					*cf = tf;
				} else if(!strcasecmp(cf->tokenstr, "define")) {
					char name[128];
					zero(name);
					expect(tSTR);
					strcpy(name, cf->tokenstr);
					expect(tNUM);
					add_define(name, cf->tokennum);
				}
				break;
			}
			case tDOT: {
				zero(sym);
				expect(tSTR);
				strcpy(sym, cf->tokenstr);
				expect(tDAT);
				next();
				if(cf->token == tNUM) {
					add_symbol(sym, IP, &cf->tokennum, NULL);
					MEM[IP++] = cf->tokennum;
					
					assemble_dat();
					goto again;
				} else if(cf->token == tQSTR) {
					add_symbol(sym, IP, NULL, cf->tokenstr);
					for(i = 0; i < strlen(cf->tokenstr); i++) {
						b = 0;
						b |= cf->tokenstr[i];
						MEM[IP++] = b; 
					}
					MEM[IP++] = 0;

					assemble_dat();
					goto again;
				} else {
					error("expected value after DAT");
				}
				break;
			}
			
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
			case tJMP: case tJTR: case tPUSH: case tPOP: {
				assemble_o(&d, &v1);
				assemble_i(t, d, 0, v1, -1);
				break;	
			}

			case tRET: {
				assemble_i(t, 0, 0, -1, -1);
				break;
			}
			default: {
				error("invalid instruction \"%s\"", cf->tokenstr);
				break;
			}
		}
	}
done:
	clean_fs(cf);
	return 1;
}

int main(int argc, char **argv) {
	if(argc < 4) {
		die("usage: %s <source_file> <program> <symbols>", argv[0]);
	}

	o_fn = argv[2];
	sym_fn = argv[3];

	zero(MEM);

	// Init file_asm structure for the main file
	init_asm(argv[1]); 

	assemble();
	fix_symbols();

	dump_symbols(sym_fn);
	output(o_fn);

	return 0;
}
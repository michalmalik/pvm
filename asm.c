#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>

#define zero(a)		memset((a), 0, sizeof((a)))

typedef unsigned char u8;
typedef unsigned short u16;
typedef enum {
	FALSE, TRUE
} bool;

static void free_defines_list();
static void free_memory();

union _mem {
	struct {
		u8 lo;
		u8 hi;
	} _b;
	u16 _w;
};

struct file_id {
	struct file_id *next;

	FILE *fp;
	unsigned int id;
};

struct cdefine {
	struct cdefine *next;

	char name[128];
	u16 value;
};

struct file_asm {
	struct file_id fid;

	char i_fn[64];
	char tokenstr[64];
	char linebuffer[256];
	
	int linenumber;

	char line[256];
	char *lineptr;

	int token;
	u16 tokennum;

	struct cdefine *defines;
};

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

static union _mem MEM[0x8000] = {0};
static u16 IP = 0;

static struct file_asm *cf = NULL;
static struct file_id *files = NULL;

static struct symbol *symbols = NULL;
static struct fix *fixes = NULL;

static const char *tn[] = {
	"A", "B", "C", "D", "X", "Y", "Z", "J",
	"SP", "IP", "IA",
	"STO",
	"ADD", "SUB", "MUL", "DIV", "MOD",
	"NOT", "AND", "OR", "XOR", "SHL", "SHR",
	"IFE", "IFN", "IFG", "IFL", "IFGE", "IFLE", "JMP", "JTR",
	"PUSH", "POP", "RET", "RETI", "IAR", "INT",
	"HWI", "HWQ", "HWN",
	"DAT", 
	".", "#", ":", ",", "[", "]", "+",
	"(NUMBER)", "(STRING)", "(QUOTED STRING)", "(EOL)", "(EOF)"
};

enum _tokens {
	tA, tB, tC, tD, tX, tY, tZ, tJ,
	tSP, tIP, tIA,
	tSTO, 
	tADD, tSUB, tMUL, tDIV, tMOD,
	tNOT, tAND, tOR, tXOR, tSHL, tSHR,
	tIFE, tIFN, tIFG, tIFL, tIFGE, tIFLE, tJMP, tJTR,
	tPUSH, tPOP, tRET, tRETI, tIAR, tINT,
	tHWI, tHWQ, tHWN,
	tDAT,
	tDOT, tHASH, tCOLON, tCOMMA, tBS, tBE, tPLUS,
	tNUM, tSTR, tQSTR, tEOL, tEOF
};

#define LASTTOKEN	tEOF

static void die(const char *format, ...) {
	char buf[512] = {0};
	va_list va;

	va_start(va, format);

	vsprintf(buf, format, va);

	puts(buf);

	free_defines_list();
	free_memory();
	exit(1);
}

static void error(const char *format, ...) {
	char buf_1[512] = {0};
	char buf_2[256] = {0};
	va_list va;
	
	va_start(va, format);

	sprintf(buf_1, "%s:%d, error: ", cf->i_fn, cf->linenumber);
	vsprintf(buf_2, format, va);
	strcat(buf_1, buf_2);	

	puts(buf_1);

	free_defines_list();
	free_memory();
	exit(1);
}

static void *zmalloc(size_t size) {
	void *block = NULL;

	if((block = malloc(size)) == NULL) {
		die("error: zmalloc fail on line %d in %s", __LINE__, __FILE__);
	}

	return memset(block, 0, size);
}	

static void add_file(const int id, FILE *fp) {
	struct file_id *fi = (struct file_id *)zmalloc(sizeof(struct file_id));

	fi->id = id;
	fi->fp = fp;

	fi->next = files;
	files = fi;
}

static void init_asm(const char *i_fn) {
	struct file_asm fs = {0};
	size_t i;

	fs.lineptr = fs.linebuffer;
	fs.defines = NULL;
	strcpy(fs.i_fn, i_fn);

	if((fs.fid.fp = fopen(i_fn, "r")) == NULL) {
		if(cf->fid.fp) {
			die("%s:%d, error: file \"%s\" does not exist \n\n%s", cf->i_fn, 
				cf->linenumber, fs.i_fn, cf->line);
		} else {
			die("error: file \"%s\" does not exist", fs.i_fn);
		}
	}

	if(!files) 
		fs.fid.id = 0;
	else 
		fs.fid.id = files->id+1;

	add_file(fs.fid.id, fs.fid.fp);

	*cf = fs;
}

static void add_symbol(const char *name, u16 addr, u16 *value) {
	struct symbol *s = NULL;
	for(s = symbols; s; s = s->next) {
		if(!strcmp(name, s->name)) {
			error("symbol \"%s\" already defined here:\n%s: line %d: %s", name, s->f.i_fn, s->f.linenumber, s->f.line);
		}
	}

	s = (struct symbol *)zmalloc(sizeof(struct symbol));

	s->addr = addr;
	strcpy(s->name, name);
	if(value) s->value = *value;
	s->f = *cf;

	s->next = symbols;
	symbols = s;
}

static void fix_symbol(const char *name, u16 addr, u16 *const_val) {
	struct fix *f = (struct fix *)zmalloc(sizeof(struct fix));

	strcpy(f->name, name);
	strcpy(f->fn, cf->i_fn);
	strcpy(f->linebuffer, cf->line);

	f->fix_addr = addr;
	f->linenumber = cf->linenumber;
	if(const_val) f->const_val = *const_val;

	f->next = fixes;
	fixes = f;
}

static void add_define(const char *name, u16 value) {
	struct cdefine *d = NULL;
	for(d = cf->defines; d; d = d->next) {
		if(!strcmp(d->name, name))
			error("define \"%s\" already defined", name);
	}

	d = (struct cdefine *)zmalloc(sizeof(struct cdefine));

	strcpy(d->name, name);
	d->value = value;

	d->next = cf->defines;
	cf->defines = d;
}

static struct symbol *find_symbol(const char *name) {
	struct symbol *s = NULL;
	for(s = symbols; s; s = s->next) {
		if(!strcmp(s->name, name)) return s;
	}
	return NULL;
}

static struct cdefine *find_define(const char *name) {
	struct cdefine *d = NULL;
	for(d = cf->defines; d; d = d->next) {
		if(!strcmp(d->name, name)) return d;
	}
	return NULL;
}

static int is_defined(const char *name) {
	struct symbol *s = NULL;
	struct cdefine *d = NULL;
	for(s = symbols; s; s = s->next) {
		if(!strcmp(s->name, name)) return TRUE;
	}
	for(d = cf->defines; d; d = d->next) {
		if(!strcmp(d->name, name)) return TRUE;
	}
	return FALSE;
}

static void fix_symbols() {
	struct symbol *s = NULL;
	struct fix *f = NULL;
	for(f = fixes; f; f = f->next) {
		s = find_symbol(f->name);
		if(!s) {
			die("%s:%d, error: symbol \"%s\" is not defined\n\n%s", f->fn, f->linenumber,
				 f->name, f->linebuffer);
		}
		// [symbol + const_val]
		// symbol is always represented as its address in memory
		// and const_val can be added to it
		MEM[f->fix_addr]._w += s->addr + f->const_val;
	}
}

static void print_symbols() {
	struct symbol *s = NULL;
	for(s = symbols; s; s = s->next) {
		printf("%s %s 0x%04X 0x%04X\n", s->f.i_fn, s->name, s->addr, s->value);
	}
}

static int next_token() {
	char c = 0, *x = NULL;
	size_t i;
	bool newline = FALSE;

next_line:
	if(!*cf->lineptr) {
		if(feof(cf->fid.fp)) return tEOF;
		if(fgets(cf->linebuffer, 256, cf->fid.fp) == 0) return tEOF;
		cf->lineptr = cf->linebuffer;
		cf->linenumber++;
		newline = TRUE;
	}

	if(*cf->lineptr == '\n' || *cf->lineptr == '\r') {
		*cf->lineptr = 0;
		return tEOL;
	}

	while(*cf->lineptr <= ' ') {
		if(*cf->lineptr == 0) goto next_line;
		cf->lineptr++;
	}

	// Copy clean line to cf->line for prettier
	// error messages
	if(newline) {
		zero(cf->line);
		strcpy(cf->line, cf->lineptr);
		for(i = 0; i < strlen(cf->line); i++) {
			if(cf->line[i]=='\r'||cf->line[i]=='\n')
				cf->line[i]='\0';
		}

		newline = FALSE;
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

static int next() {
	return (cf->token = next_token());
}

static void expect(int t) {
	if(next() != t) {
		error("expected: \"%s\", got: \"%s\"", tn[t], tn[cf->token]);		
	}
}

static void assemble_label(const char *name, int *v) {
	// Symbols have higher priority
	struct symbol *s = find_symbol(name);
	struct cdefine *d = find_define(name);

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

static void assemble_o(u16 *o, int *v) {
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
			if(cf->token != tBS) error("expected operand\n\n%s", cf->line);
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
					error("expected [register+nextw]\n\n%s", cf->line);
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
					error("expected value\n\n%s", cf->line);
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
					error("expected value\n\n%s", cf->line);
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
			error("expected ]\n\n%s", cf->line);
	}
} 

static void assemble_i(int inst, u16 d, u16 s, int v1, int v2) {
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
		case tHWI: o = 0x1A; break;
		case tHWQ: o = 0x1B; break;
		case tHWN: o = 0x1C; break;
	}
	MEM[IP++]._w = (u16)(o|(s<<6)|(d<<11));
	if(v1 != -1) MEM[IP++]._w = v1&0xFFFF;		
	if(v2 != -1) MEM[IP++]._w = v2&0xFFFF;
}

static int assemble() {
	u16 s = 0, d = 0;
	int v1 = -1, v2 = -1;
	int t;
	u16 b = 0;
	struct file_asm tf = {0};
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
				bool add_sym = TRUE;

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
						error("expected value after DAT\n\n%s", cf->line);
					} break;
				}

				next();
				if(cf->token == tCOMMA) {
					add_sym = FALSE;
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
			case tIAR: case tINT: 
			case tHWI: case tHWQ: case tHWN: {
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
						error("expected value after DAT\n\n%s", cf->line);
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
	free_defines_list();
	return 1;
}

static void output(const char *fn) {
	FILE *fp = NULL;
	size_t i;

	if((fp = fopen(fn, "wb")) == NULL) {
		die("error: couldn't open/write to \"%s\"", fn);
	}

	for(i = 0; i < 0x8000; i++) {
		fprintf(fp, "%c%c", MEM[i]._b.hi, MEM[i]._b.lo);
	}

	fclose(fp);
}

static void free_defines_list() {
	if(!cf) return;
	struct cdefine *head = cf->defines, *tmp = NULL;
	while(head) {
		tmp = head;
		head = head->next;
		free(tmp);
	}
	free(head);
	cf->defines = NULL;
}

static void free_memory() {
	struct symbol *shead = symbols, *stmp = NULL;
	struct fix *fhead = fixes, *ftmp = NULL;
	struct file_id *fihead = files, *fitmp = NULL;

	// Free symbol linked list
	while(shead) {
		stmp = shead;
		shead = shead->next;
		free(stmp); 
	}
	free(shead);
	symbols = NULL;

	// Free fix linked list
	while(fhead) {
		ftmp = fhead;
		fhead = fhead->next;
		free(ftmp);
	}
	free(fhead);
	fixes = NULL;

	// Free all files
	for(fitmp = files; fitmp; fitmp = fitmp->next) {
		fclose(fitmp->fp);
	}
	fitmp = NULL;
	
	while(fihead) {
		fitmp = fihead;
		fihead = fihead->next;
		free(fitmp);
	}
	free(fihead);
	fixes = NULL;

	// Free current file structure
	free(cf);
	cf = NULL;
}

int main(int argc, char **argv) {
	char i_fn[128] = {0};
	char o_fn[128] = {0};

	if(argc < 3) {
		die("usage: %s <source_file> <program>", argv[0]);
	}

	strcpy(i_fn, argv[1]);
	strcpy(o_fn, argv[2]);

	// Init file_asm structure for the main file
	cf = (struct file_asm *)zmalloc(sizeof(struct file_asm));

	init_asm(i_fn);

	assemble();
	fix_symbols();

	print_symbols();
	output(o_fn);

	free_memory();
	return 0;
}
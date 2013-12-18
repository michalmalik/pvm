#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>

#include "node.h"

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;

struct file_id {
	FILE *fp;
	u32 id;
};

struct define {
	char name[128];
	u16 value;
};

struct file_asm {
	struct file_id fid;

	char i_fn[64];
	char tokenstr[128];
	char linebuffer[256];
	
	u32 linenumber;

	char line[256];
	char *lineptr;

	u32 token;
	u16 tokennum;

	struct Node *defines;
};

struct symbol {
	char name[128];
	u16 addr;
	u16 value;

	struct file_asm f;
};

struct fix {
	char fn[64];
	char name[128];
	char linebuffer[256];
	u32 linenumber;

	u16 fix_addr;
	u16 const_val;
};

struct operand {
	u16 o;
	int w;
};

union _mem {
	struct {
		u8 lo;
		u8 hi;
	} _b;
	u16 _w;
};

static union _mem MEM[0x8000];
static u16 IP;

static struct file_asm *cf;

static struct Node *files;
static struct Node *symbols;
static struct Node *fixes;

static const char *tn[] = {
	"A", "B", "C", "D", "X", "Y", "Z", "J",
	"SP", "IP", "IA", "O",
	"STO",
	"ADD", "SUB", "MUL", "DIV", "MOD",
	"NOT", "AND", "OR", "XOR", "SHL", "SHR",
	"MULS", "DIVS", "MODS",
	"IFE", "IFN", "IFG", "IFL", "IFA", "IFB", "JMP", "JTR",
	"PUSH", "POP", "RET", "RETI", "IAR", "INT",
	"HWI", "HWQ", "HWN", "HLT",
	"DAT", 
	".", "#", ":", ",", "[", "]", "+",
	"(NUMBER)", "(STRING)", "(QUOTED STRING)", "(EOL)", "(EOF)"
};

enum {
	tA, tB, tC, tD, tX, tY, tZ, tJ,
	tSP, tIP, tIA, tO,
	tSTO, 
	tADD, tSUB, tMUL, tDIV, tMOD,
	tNOT, tAND, tOR, tXOR, tSHL, tSHR,
	tMULS, tDIVS, tMODS,
	tIFE, tIFN, tIFG, tIFL, tIFA, tIFB, tJMP, tJTR,
	tPUSH, tPOP, tRET, tRETI, tIAR, tINT,
	tHWI, tHWQ, tHWN, tHLT,
	tDAT,
	tDOT, tHASH, tCOLON, tCOMMA, tBS, tBE, tPLUS,
	tNUM, tSTR, tQSTR, tEOL, tEOF
};

#define LAST_TOKEN	tEOF

static void free_memory();

static void error(const char *format, ...) {
	char buf_1[512] = {0};
	char buf_2[256] = {0};
	va_list va;
	
	va_start(va, format);

	sprintf(buf_1, "%s:%d, error: ", cf->i_fn, cf->linenumber);
	vsprintf(buf_2, format, va);
	strcat(buf_1, buf_2);	

	puts(buf_1);

	free_memory();
	exit(1);
}	

static void add_file(const u32 id, FILE *fp) {
	struct Node *node = NEW_NODE(struct Node);
	struct file_id *fi = NEW_NODE(struct file_id);

	fi->id = id;
	fi->fp = fp;

	ADD_NODE(files, node, fi);
}

static void init_asm(const char *i_fn) {
	struct file_asm fs = {0}, tf = {0};

	fs.lineptr = fs.linebuffer;
	fs.defines = NULL;
	strcpy(fs.i_fn, i_fn);

	if((fs.fid.fp = fopen(i_fn, "r")) == NULL) {
		tf = *cf;
		free_memory();
		if(tf.fid.fp) {
			die("%s:%d, error: file \"%s\" does not exist \n\n%s", tf.i_fn, 
				tf.linenumber, fs.i_fn, tf.line);
		} else {
			die("error: file \"%s\" does not exist", fs.i_fn);
		}
	}

	if(!files) {
		fs.fid.id = 0;
	} else { 
		fs.fid.id = CAST_NODE(files, struct file_id)->id+1;
	}

	add_file(fs.fid.id, fs.fid.fp);
	*cf = fs;
}

static void add_symbol(const char *name, u16 value) {
	struct Node *node = NULL;
	struct symbol *s = NULL;

	ENUM_LIST(node, symbols) {
		s = CAST_NODE(node, struct symbol);
		if(!strcmp(s->name, name)) {
			error("symbol \"%s\" already defined here:\n%s:%d: %s", name, s->f.i_fn, 
				s->f.linenumber, s->f.line);
		}
	}

	node = NEW_NODE(struct Node);
	s = NEW_NODE(struct symbol);

	s->addr = IP;
	strcpy(s->name, name);
	s->value = value;
	s->f = *cf;

	ADD_NODE(symbols, node, s);
}

static void fix_symbol(const char *name, u16 const_val) {
	struct Node *node = NEW_NODE(struct Node);
	struct fix *f = NEW_NODE(struct fix);

	strcpy(f->name, name);
	strcpy(f->fn, cf->i_fn);
	strcpy(f->linebuffer, cf->line);

	f->fix_addr = IP+1;
	f->linenumber = cf->linenumber;
	f->const_val = const_val;

	ADD_NODE(fixes, node, f);
}

static void add_define(struct Node **list, const char *name, u16 value) {
	struct Node *node = NULL;
	struct define *d = NULL;

	ENUM_LIST(node, *list) {
		d = CAST_NODE(node, struct define);
		if(!strcmp(d->name, name))
			error("define \"%s\" already defined", name);
	}

	node = NEW_NODE(struct Node);
	d = NEW_NODE(struct define);

	strcpy(d->name, name);
	d->value = value;

	ADD_NODE(*list, node, d);
}

#define CMP_NODE_SYMBOL()	!strcmp(CAST_NODE(node, struct symbol)->name, name)
#define CMP_NODE_DEFINE()	!strcmp(CAST_NODE(node, struct define)->name, name)

static struct symbol *find_symbol(const char *name) {
	struct Node *node = NULL;

	ENUM_LIST(node, symbols) {
		if(CMP_NODE_SYMBOL())
			return node->block;
	}

	return NULL;
}

static struct define *find_define(const char *name) {
	struct Node *node = NULL;

	ENUM_LIST(node, cf->defines) {
		if(CMP_NODE_DEFINE()) 
			return node->block;
	}

	return NULL;
}

// Symbols have higher priority than defines
static u32 is_defined(const char *name) {
	struct Node *node = NULL;

	ENUM_LIST(node, symbols) {
		if(CMP_NODE_SYMBOL())
			return 1;
	}

	ENUM_LIST(node, cf->defines) {
		if(CMP_NODE_DEFINE())
			return 1;
	}

	return 0;
}

#undef CMP_NODE_SYMBOL
#undef CMP_NODE_DEFINE

static void fix_symbols() {
	struct Node *node = NULL;
	struct symbol *s = NULL;
	struct fix *f = NULL;

	ENUM_LIST(node, fixes) {
		f = CAST_NODE(node, struct fix);
		s = find_symbol(f->name);
		if(!s) {
			free_memory();
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
	struct Node *node = NULL;
	struct symbol *s = NULL;
	
	ENUM_LIST(node, symbols) {
		s = CAST_NODE(node, struct symbol);
		printf("%s %s 0x%04X 0x%04X\n", s->f.i_fn, s->name, s->addr, s->value);
	}
}

static u32 next_token() {
	char c = 0, *x = NULL;
	size_t i;
	u32 newline = 0;

next_line:
	if(!*cf->lineptr) {
		if(feof(cf->fid.fp)) return tEOF;
		if(fgets(cf->linebuffer, 256, cf->fid.fp) == 0) return tEOF;
		cf->lineptr = cf->linebuffer;
		cf->linenumber++;

		newline = 1;
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
		ZERO(cf->line);
		strcpy(cf->line, cf->lineptr);
		for(i = 0; i < strlen(cf->line); i++) {
			if(cf->line[i]=='\r'|| cf->line[i]=='\n')
				cf->line[i]='\0';
		}

		newline = 0;
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
			ZERO(cf->tokenstr);

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
		} break;
		default: {
			if(isalpha(c)) {
				ZERO(cf->tokenstr);

				cf->lineptr--;				
				x = cf->tokenstr;

				while(isalnum(*cf->lineptr) || *cf->lineptr == '_') {
					*x++ = *cf->lineptr++;
				}
				*x = 0;

				size_t i;
				for(i = 0; i < LAST_TOKEN; i++) {
					if(!strcasecmp(tn[i], cf->tokenstr)) {
						return i;
					}
				}
				return tSTR;
			// Handle both signed and unsigned integers
			} else if(isdigit(c) || c == '-') {
				cf->lineptr--;
				cf->tokennum = strtol(cf->lineptr, &cf->lineptr, 0);
				return tNUM;				
			}
		} break;
	}

	// Make sure we don't return
	// an existing token when it clearly
	// does not exist
	return LAST_TOKEN+1;
}

static u32 next() {
	return (cf->token = next_token());
}

static void expect(u32 t) {
	if(next() != t) {
		error("expected: \"%s\", got: \"%s\"", tn[t], tn[cf->token]);		
	}
}

static int assemble_label(const char *name, int w) {
	// Symbols have higher priority
	struct symbol *s = find_symbol(name);
	struct define *d = find_define(name);

	int rw = w;

	if(w == -1) {
		if(s) rw = s->addr;
		else {
			if(d) rw = d->value;
			else {
				fix_symbol(name, 0);
				rw = 0;
			}
		}
	} else {
		if(s) rw += s->addr;
		else {
			if(d) rw += d->value;
			else {
				fix_symbol(name, rw);
				rw = 0;
			}
		}
	}

	return rw;
}

static struct operand assemble_o() {
	u16 o = 0;
	int w = -1;

	next();
	switch(cf->token) {
		// register
		case tA: case tB: case tC: case tD:
		case tX: case tY: case tZ: case tJ: {
			o = cf->token&0x7;
		} break;

		// SP
		case tSP: {
			o = 0x1a;
		} break;

		// IP
		case tIP: {
			o = 0x1b;
		} break;

		// O
		case tO: {
			o = 0x1c;
		} break;

		// nextw
		case tNUM: {
			o = 0x18;
			w = cf->tokennum;
		} break;

		// nextw
		case tSTR: {
			o = 0x18;
			w = assemble_label(cf->tokenstr, w);
		} break;

		default: {
			if(cf->token != tBS) error("expected operand\n\n%s", cf->line);
		} break;
	}

	if(cf->token == tBS) {
		next();

		if(cf->token <= 7) {
			o = cf->token&7;
			next();
			if(cf->token == tPLUS) {
				next();

				// [register + nextw]
				if(cf->token == tNUM) {
					o += 0x10;
					w = cf->tokennum;
				}
				// [register + nextw]
				else if(cf->token == tSTR) {
					o += 0x10;
					w = assemble_label(cf->tokenstr, w);
				} else {
					error("expected [register+nextw]\n\n%s", cf->line);
				}
				next();
			// [register]
			} else if(cf->token == tBE) {
				o += 0x8;
			} 
		}

		if(cf->token == tNUM) {
			// [nextw]
			w = cf->tokennum;
			o = 0x19;
			
			next();
			if(cf->token == tPLUS) {
				next();
				// [nextw]
				if(cf->token == tNUM) {
					w += cf->tokennum;
				// [register + nextw]
				} else if(cf->token <= 7) {
					o = (cf->token&7)+0x10;
				// [nextw]
				} else if(cf->token == tSTR) {
					w = assemble_label(cf->tokenstr, w);
				} else {
					error("expected value\n\n%s", cf->line);
				}
				next();
			}
		}

		if(cf->token == tSTR) {
			o = 0x19;
			w = 0;
			
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
					w += cf->tokennum;
				} else if(cf->token <= 7) {
					o = (cf->token&7)+0x10;
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
				w = assemble_label(f_buf, w);
			} else {
				fdef = is_defined(f_buf);
				sdef = is_defined(s_buf);

				// [defined + defined]
				// [defined + undefined]
				if(fdef && (sdef || !sdef)) {
					w = assemble_label(f_buf, w);
					w = assemble_label(s_buf, w);
				} 
				// [undefined + defined]
				if(!fdef && sdef) {
					w = assemble_label(s_buf, w);
					w = assemble_label(f_buf, w);
				}
				// [undefined + undefined]
				if(!fdef && !sdef) {
					w = assemble_label(f_buf, -1);
					w = assemble_label(s_buf, w);
				}
			}
		}
		
		if(cf->token != tBE) 
			error("expected ]\n\n%s", cf->line);
	}

	struct operand op = {
		.o = o,
		.w = w
	};
	return op;
} 

static void assemble_i(int inst, struct operand d, struct operand s) {
	u16 o = inst - tSTO;
	MEM[IP++]._w = (u16)(o|(s.o<<6)|(d.o<<11));

	if(d.w != -1)
		MEM[IP++]._w = d.w & 0xffff;		
	if(s.w != -1)
		MEM[IP++]._w = s.w & 0xffff;
}

#define PREPROC(a)		!strcasecmp(cf->tokenstr, (a))
#define ZERO_OPR_ST(a)		(a).o = 0; (a).w = -1

static void assemble() {
	struct operand d, s;
	u32 t;
	u16 b = 0;
	size_t i;

	struct Node *node = NULL;
	struct Node *tdefines = NULL;
	struct define *def = NULL;
	struct file_asm tf = {0};

	for(;;) {
		next();
again:
		ZERO_OPR_ST(d);
		ZERO_OPR_ST(s);
		t = cf->token;

		switch(t) {
			case tEOF: return;
			case tEOL: {
				next();
				goto again;
			} break;
			case tCOLON: {
				expect(tSTR);
				add_symbol(cf->tokenstr, IP);
			} break;
			case tHASH: {
				char buffer[128];

				expect(tSTR);
				if(PREPROC("include")) {			
					expect(tQSTR);
					strcpy(buffer, cf->tokenstr);

					// Store the previous file_asm structure
					tf = *cf;

					init_asm(buffer);
					assemble();

					// Copy cf->defines to tdefines
					ENUM_LIST(node, cf->defines) {
						def = CAST_NODE(node, struct define);
						add_define(&tdefines, def->name, def->value);
					}
					free_node(&cf->defines);

					// Load the previous file_asm structure
					*cf = tf;

					// Copy tdefines back to cf->defines
					ENUM_LIST(node, tdefines) {
						def = CAST_NODE(node, struct define);
						add_define(&cf->defines, def->name, def->value);
					}
					free_node(&tdefines);
				} else if(PREPROC("define")) {
					expect(tSTR);
					strcpy(buffer, cf->tokenstr);
					expect(tNUM);
					add_define(&cf->defines, buffer, cf->tokennum);
				}		
			} break;
			case tDOT: {
				char sym_name[128] = {0};
				u32 add_sym = 1;

				expect(tSTR);
				strcpy(sym_name, cf->tokenstr);

				expect(tDAT);
				next();

			write_mem:
				switch(cf->token) {
					case tNUM: {
						if(add_sym) add_symbol(sym_name, cf->tokennum);
						MEM[IP++]._w = cf->tokennum;
					} break;
					case tQSTR: {
						b = 0 | cf->tokenstr[0];
						if(add_sym) add_symbol(sym_name, b);

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
			case tMULS: case tDIVS: case tMODS:
			case tIFE: case tIFN: case tIFG: case tIFL:
			case tIFA: case tIFB: 
			{
				d = assemble_o();
				expect(tCOMMA);				
				s = assemble_o();
				assemble_i(t, d, s);
			} break;
			case tNOT:
			case tJMP: case tJTR: case tPUSH: case tPOP: 
			case tIAR: case tINT: 
			case tHWI: case tHWQ: case tHWN: {
				d = assemble_o();
				assemble_i(t, d, s);
			} break;

			case tRET: case tRETI:
			case tHLT: {
				assemble_i(t, d, s);
			} break;

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
			} break;

			default: {
				error("invalid instruction \"%s\"", cf->tokenstr);
			} break;
		}
	}
}

#undef PREPROC
#undef ZERO_OPR_ST

static void output(const char *fn) {
	FILE *fp = NULL;
	size_t i;

	if((fp = fopen(fn, "wb")) == NULL) {
		free_memory();
		die("error: couldn't open/write to \"%s\"", fn);
	}

	for(i = 0; i < 0x8000; i++) {
		fprintf(fp, "%c%c", MEM[i]._b.hi, MEM[i]._b.lo);
	}

	fclose(fp);
}

static void free_memory() {
	if(!cf) return;

	struct Node *node = NULL;
	struct file_id *tmp = NULL;

	free_node(&symbols);
	free_node(&fixes);

	// Free all files
	ENUM_LIST(node, files) {
		tmp = CAST_NODE(node, struct file_id);
		fclose(tmp->fp);
	}
	
	free_node(&files);
	free_node(&cf->defines);

	// Free current file structure
	free(cf);
	cf = NULL;
}

int main(int argc, char **argv) {
	char i_fn[128] = {0};
	char o_fn[128] = {0};

	if(argc < 3) {
		free_memory();
		die("usage: %s <source_file> <program>", argv[0]);
	}

	strcpy(i_fn, argv[1]);
	strcpy(o_fn, argv[2]);

	// Init file_asm structure for the main file
	cf = NEW_NODE(struct file_asm);
	init_asm(i_fn);

	assemble();
	fix_symbols();

	print_symbols();
	output(o_fn);

	free_memory();
	return 0;
}
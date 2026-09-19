#include <string.h>
#include <stdint.h>

#include "port.h"

uint8_t msize(uint8_t* m) {
	uint16_t i = 0;
	for (;m[i] != 0x00;i++);
	return i;
}

// For delete comments in compilation
void trim_c(char* text,char c) {
	for (uint8_t i = 0; text[i] != '\0'; i++) {
		if (text[i] == c) {
			text[i] = '\0';
			break;
		}
	}
}

// ASCII
void lower_chars(char* text) {
	for (uint8_t c = 0; text[c] != '\0';c++) {
		if (text[c] >= 'A' && 'Z' >= text[c]) {
			text[c] += 0x20; // A = 0x41 | a = 0x61
		}
	}
}

char* str_char(char* txt, uint8_t byte) {
	while (*txt && *txt != byte) txt++;

	if (*txt == byte) return txt;

	return NULL;
}

int8_t atoi(char* txt) {
	int8_t num = 0;
	while (*txt && (*txt >= '0' && *txt <= '9')) {
		num = (num * 10)+(*txt - '0');
		txt++;
	}
	return num;
}

/*
    \n \r ' '
*/
void trim_end(char* text) {
	uint8_t end = 0;
	while(text[end+1]) end++;
	while(
		text[end] == '\n' ||
		text[end] == '\r' ||
		text[end] == ' '  ||
		text[end] == '\t' ||
		text[end] == ','
		) {
        text[end--] = '\0';
	}
}

void skip_chars(char* txt, uint8_t* i) {
	while (
		txt[*i] == ' '  ||
		txt[*i] == '\t' || // TAB - 0x09
		txt[*i] == ','  ||
		txt[*i] == '\n' ||
		txt[*i] == '\r'
	) (*i)++;
}

void skip__chars(char** txt) {
	uint8_t i = 0;
	while (
		(*txt)[i] == ' '  ||
		(*txt)[i] == '\t' ||
		(*txt)[i] == ','  ||
		(*txt)[i] == '\n' ||
		(*txt)[i] == '\r'
	) i++;
	*txt = &(*txt)[i];
}

// STOPS ON REAL BYTE! NOT \0 \n \r
void arg_len(char* txt, uint8_t* i, uint8_t byte) {
	while (
		txt[*i+1] != '\0' &&
		txt[*i+1] != ' '  &&
		txt[*i+1] != ','  &&
		txt[*i+1] != '\n' &&
		txt[*i+1] != '\r' &&
		txt[*i+1] != byte
	) (*i)++;
}

// Copying text in new m while symbols
// Returns byte \0 \n \r
uint8_t arg_copy(char* txt,char* newm,uint8_t byte) {
	uint8_t i = 0;
	while (
		txt[i] != '\0' &&
		txt[i] != ' '  &&
		txt[i] != '\n' &&
		txt[i] != '\r' &&
		txt[i] != byte &&
		txt[i] != ','
	) newm[i++] = txt[i];
	return i;
}

// At end NULL
void args_parser(char* text,SCANR_T args[]) {
	uint8_t argc = 0;
	uint8_t bufi = 0;

	args[argc].next = NULL;

	while (*text != '\0') {
		if (*text != ' '  &&
		*text != '\t'     &&
		*text != ','      &&
		*text != '\n'     &&
		*text != '\r')
		{
			// [] makes in one arg
			if (*text == '[') {


				while (*text != ']' && *text != '\0') {
					if (*text != ' ' && *text != '\t') {
						args[argc].txt[bufi++] = *text;
					}
					text++;
				}

				if (*text == ']') {
					args[argc].txt[bufi++] = ']';
					text++;
				}

				args[argc].txt[bufi] = '\0';
				bufi = 0;

				args[argc].next = &args[argc+1];
				argc++;
				args[argc].next = NULL;
			} else {
				args[argc].txt[bufi++] = *text;
				text++;
			}
		} else if (bufi > 0) {
			args[argc].txt[bufi] = '\0';
			bufi = 0;

			args[argc].next = &args[argc+1];
			argc++;
			args[argc].next = NULL;

			text++;
		} else {
			text++;
		}
	}

	if (bufi > 0) {
		args[argc].txt[bufi] = '\0';
		args[argc].next = NULL;
	} else if (argc > 0) {
		args[argc-1].next = NULL; // If reseted and new is bad
	}
}

int texto_num(char* txt, uint8_t size) {
	int num = 0;
	bool neg = (txt[0] == '-');
	uint8_t start = neg ? 1 : 0;
	for (uint8_t i = start; i < size; i++) {
		num = num * 10 + (txt[i] - '0');
	}
	return neg ? -num : num;
}

UNI_NUM_T uni_num(char* txt) {
	UNI_NUM_T ret = {};
	uint8_t i = 0;

	// HEX
	if (txt[i] == '0' && (txt[i+1] == 'x' || txt[i+1] == 'X')) {
		i += 2;

		uint8_t last = i;
		arg_len(txt,&i,0);

		if (i-last != 0) ret.has_num = true;

		uint8_t idx = 0;
		while (i >= last) {
			ret.num += hexindex(txt[i]) * stepen(16,idx);
			i--;
			idx++;
		}
	} else if (txt[i] >= '0' && '9' >= txt[i]) { // DECIMAL
		ret.has_num = true;

		uint8_t last = i;
		arg_len(txt,&i,0);
		i++;

		ret.num = texto_num(txt,i-last);
	} else {
		ret.has_num = false;
	}

	return ret;
}

const char hex_chars[] = "0123456789abcdef";

uint8_t hexindex(char hex) {
	uint8_t c = 0;
	for (; hex_chars[c] != '\0' && hex_chars[c] != hex; c++);
	return c;
}

int16_t stepen(int16_t num,uint16_t mon) {
	if (mon == 0) return 1;

	int16_t rnum = num;
	while (--mon) rnum = rnum*num;

	return rnum;
}

void asm_setsymbol(char* asmcode,SYM_T* sym,uint32_t* pc) {
	lower_chars(asmcode);
	trim_c(asmcode,';');
	trim_end(asmcode);
	if (asmcode[0] == '\0') return; // For comments and just empty
	COMP_T s = {.asmcode = asmcode,.bytes = 0,.sym = sym,.pc = pc,.mode = 1};

	GENED_T res = genbytecode(&s);
	*pc += res.gened;
	if (res.error) {
		uint8_t c = 0; // For labels
		arg_len(asmcode,&c,':');
		if (asmcode[c+1] == ':') {
			asmcode[++c] = '\0';
			for (uint8_t i = 0;i < sym->lastsymbol;i++) {
				if (strcmp(sym->symbols[i].text,asmcode) == 0) {
					sym->symbols[i].address = *pc;
					break;
		        	}
			}
        	}
	}
}

GENED_T asm_tobyte(char* asmcode, uint8_t* bytes, SYM_T* sym,uint32_t* pc) {
	COMP_T s = {.asmcode = asmcode,.bytes = bytes,.sym = sym,.pc = pc,.mode = 0};

	bool found = false;

	lower_chars(asmcode); // Symbols register TODO

	trim_c(asmcode,';');
	trim_end(asmcode);
	if (asmcode[0] == '\0') {
		goto nfound;
	}

	GENED_T res = genbytecode(&s);
	if (!res.error) {
		found = true;
	}

nfound:
	// For label:
	if (!found) {
		uint8_t i = 0;
		skip_chars(asmcode,&i);
		arg_len(asmcode,&i,':');
		if (asmcode[i+1] == ':' || asmcode[0] == '\0') {
			res.error = 2;
		} else {
			res.error = 1;
		}
		// Function on start trims ; and
		// if ; is first, asmcode[0] == '\0'
	}
	return res;
}

void asm_psymbol(char* buffer,SYM_T* SYM) {
	uint8_t i = 0;
	skip_chars(buffer,&i);
	char* ptr = &buffer[i];
	arg_len(buffer,&i,':');
	if (buffer[i+1] == ':') {
		buffer[i+1] = '\0';
		uint8_t idx = SYM->lastsymbol;
		strcpy(SYM->symbols[idx].text,ptr);
		SYM->lastsymbol = idx + 1;
	}
}

typedef struct {
	bool showbytes;
	bool debug;
} SETTINGS_CMP;

void asm_compile(FILE* file,FILE* bin,SETTINGS_CMP* settings) {
	char buffer[128] = {0};
	char* bufferptr = buffer; // Use pointer!!
	// bufferptr is pointer on char starting without \t ' '

	SYMBOL_T symbols[50] = {0}; // For "label:"
	SYM_T sym = {.symbols = symbols,.lastsymbol = 0};

	uint32_t pc = 0;

	// Symbol parser
	while (fgets(buffer,sizeof(buffer),file) != NULL) {
		asm_psymbol(bufferptr,&sym);
	}

	clearerr(file);
	fseek(file,0,SEEK_SET);

	while (fgets(buffer,sizeof(buffer),file) != NULL) {
		asm_setsymbol(buffer,&sym,&pc);
	}

	clearerr(file);
	fseek(file,0,SEEK_SET);

	pc = 0;

	uint8_t error = 0; // 1 = error
	uint16_t lineasm = 0;   // For error debug

	while (fgets(buffer,sizeof(buffer),file) != NULL) {
		skip__chars(&bufferptr);
		uint8_t byte[20] = {0};
		lineasm++;

		GENED_T b_c = asm_tobyte(bufferptr,byte,&sym,&pc);
		if (b_c.error == 1) {
			error = true;
			break;
		} else if (b_c.error != 2) { // 2 if empty
			pc += b_c.gened;
			fwrite(byte,b_c.gened,1,bin);
			if (settings->showbytes) {
				printf("Line: %u | ",lineasm);
				for (uint8_t i = 0; i < b_c.gened;i++) {
					printf("%X ",byte[i]);
				}
				printf("| %s |\n\r",bufferptr);
			}
		}
	}
	if (error) {
		printf("Error at line: %u | Text: [%s]\n\r",lineasm,buffer);
	} else {
		printf("Compiled!\n\r");
		printf("PC: %u\n\r",pc);
	}
}

// -1 is ok
int8_t arg_handler(uint8_t argc,char* argv[],SETTINGS_CMP* settings) { // argc is the first argument, not executefilename!
	for (uint8_t i = 0;i <= argc;i++) {
		if (argv[i] == NULL) break;
		char* text = argv[i];
		if (strcmp(text,"-bytes") == 0) {
			settings->showbytes = true;
		} else if (strcmp(text,"-debug") == 0) {
			settings->debug = true;
		} else {
			return i;
		}
	}
	return -1;
}

int main(int argc,char* argv[]) {
	if (argc < 3) return 1;

	// Compile settings
	SETTINGS_CMP settings = {.showbytes = false,.debug = false};
	arg_handler(argc-3,&argv[3],&settings); // atasm test bin -bytes

	FILE* file = fopen(argv[1],"r");
	FILE* bin =  fopen(argv[2],"w");

	if (file == NULL || bin == NULL) {
		perror("Failed to open file");
		return 1;
	}

	asm_compile(file,bin,&settings);

	fclose(file);
	fclose(bin);
	return 0;
}

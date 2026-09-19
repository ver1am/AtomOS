#include <stdint.h>

#include <stdio.h>
#include <string.h>

typedef struct __SCANR_T {
	union {
		int num;
		char txt[20];
	};

	bool type;
	struct __SCANR_T* next;
} SCANR_T;

typedef struct {
        char text[20];
        uint32_t address;
} SYMBOL_T;

typedef struct {
	SYMBOL_T* symbols;
	int8_t lastsymbol;
} SYM_T;

typedef struct {
        char* cmdf;
        uint8_t byte[3];
        uint8_t arg_t[3];
} CMD_T;

typedef struct {
	uint8_t gened;
	uint8_t error; /// 0 ok 1 error 2 ok but dont generate
} GENED_T;

typedef struct {
	char* asmcode; // Assembler code ex. mov eax, 0x05
	char* bytes;   // Result of generation
	SYM_T* sym;    // For symbols parse/paste
	uint32_t* pc;  // For symbols to calc addresses jumps
	bool mode;     // Zero if generate | One for symbols
} COMP_T;

extern CMD_T arc[];

extern uint8_t arc_count;

GENED_T genbytecode(COMP_T* settings); // mode | 0 for gen | 1 for calc size

// For portable
typedef struct {int32_t num; bool has_num;} UNI_NUM_T;
uint8_t hexindex(char hex);
int16_t stepen(int16_t num,uint16_t mon);
int texto_num(char* txt, uint8_t size);
UNI_NUM_T uni_num(char* txt);
void arg_len(char* txt, uint8_t* i, uint8_t byte);
uint8_t arg_copy(char* txt,char* newm,uint8_t byte);
void args_parser(char* text,SCANR_T* curr);
void trim_end(char* text);
char* str_char(char* txt, uint8_t byte);
int8_t atoi(char* txt);

#include <stdint.h>

#include <stdio.h>
#include <stdarg.h>
#include <string.h>

typedef struct {
	union {
		int num;
		char txt[20];
	};
	bool text; // True if text | False if number
	uint8_t bytes[5];
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
	uint8_t error;
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
uint8_t hexindex(char hex);
int16_t stepen(int16_t num,uint16_t mon);
int to_num(char* txt, uint8_t size);
void arg_len(char* txt, uint8_t* i, uint8_t byte);
uint8_t arg_copy(char* txt,char* newm,uint8_t byte);

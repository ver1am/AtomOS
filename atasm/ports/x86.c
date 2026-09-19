#include "port.h"

#ifdef x86

#define NO_REG 0xFF
#define RIP_REG 0xFE

typedef enum {
	REG_RAX = 0, REG_RCX, REG_RDX, REG_RBX,
	REG_ESP,     REG_RBP, REG_RSI, REG_RDI,
	REG_R8,      REG_R9,  REG_R10, REG_R11,
	REG_R12,     REG_R13, REG_R14, REG_R15,

	REG_AH = 0x20, REG_CH, REG_DH, REG_BH
} X86Reg;

typedef enum { OP_NONE = 0,OP_REG, OP_MEMORY, OP_IMMEDIATE } OpType;

typedef enum {
	ENC_LEGACY, // x86-64
	ENC_REX,
	ENC_VEX2,
	ENC_VEX3,
	ENC_EVEX
} EncType;

typedef struct {
	EncType enc_type;

	uint8_t legacy_prefix;
	uint8_t operand_size; // 8 16 32 64 128 (XMM) 256 (YMM) 512 (ZMM)
	uint8_t address_size;

	uint8_t opcode_len;
	uint8_t opcode[3];

	uint8_t has_modrm;
	uint8_t reg;     // 0-15

	OpType src_type;
	uint8_t src_reg; // 0-15

	uint8_t vvvv;
	uint8_t vex_l;
	uint8_t vex_pp;
	uint8_t vex_m_mmmm;

	uint8_t evex_z;
	uint8_t evex_aaa;
	uint8_t evex_b;

	bool is_rip_relative;
	struct {
		uint8_t base;
		uint8_t index;
		uint8_t scale; // 1 2 4 8
		int32_t disp;
	} mem;

	int64_t imm;
	uint8_t imm_size;
} INSCFG;

void constructor_bytes(INSCFG* cfg,uint8_t* bytes, uint8_t* out_len) {
	uint8_t ptr = 0;

	// Legacy prefixs
	if (cfg->address_size == 32) {
		bytes[ptr++] = 0x67;
	}

	if (cfg->legacy_prefix) bytes[ptr++] = cfg->legacy_prefix;
	if (cfg->operand_size == 16 && cfg->enc_type < ENC_VEX2) bytes[ptr++] = 0x66; // 16 bits mode

	bool is_high_reg = (cfg->reg >= REG_AH && cfg->reg <= REG_BH) ||
			(cfg->src_reg >=REG_AH && cfg->src_reg <= REG_BH);

	// REX
	if (cfg->enc_type == ENC_EVEX) {
		uint8_t r_inv = !((cfg->reg >> 3) & 1);
		uint8_t x_inv = !((cfg->mem.index >> 3) & 1);
		uint8_t b_inv  = !(cfg->src_type == OP_MEMORY ? ((cfg->mem.base >> 3) & 1) : ((cfg->src_reg >> 3) & 1));
		uint8_t r2_inv = !((cfg->reg >> 4) & 1);

		bytes[ptr++] = 0x62;
		bytes[ptr++] = (r_inv << 7) | (x_inv << 6) | (b_inv << 5) | (r2_inv << 4) | (cfg->vex_m_mmmm & 0x03);

		uint8_t w = (cfg->operand_size == 64) ? 1 : 0;
		uint8_t vvvv_inv = (~cfg->vvvv) & 0x0F;

		bytes[ptr++] = (w << 7) | (vvvv_inv << 3) | 0x04 | (cfg->vex_pp & 0x03);

		uint8_t z = cfg->evex_z & 1;
		uint8_t L_L2 = cfg->vex_l & 0x03; // L'L: 00=128, 01=256, 10=512
		uint8_t b = cfg->evex_b & 1;
		uint8_t v2_inv = !((cfg->vvvv >> 4) & 1);
		uint8_t aaa = cfg->evex_aaa & 0x07;

		bytes[ptr++] = (z << 7) | (L_L2 << 5) | (b << 4) | (v2_inv << 3) | aaa;
	} else if (cfg->enc_type == ENC_VEX2 || cfg->enc_type == ENC_VEX3) {
		uint8_t r_inv = !((cfg->reg >> 3) & 1);
		uint8_t x_inv = !((cfg->mem.index >> 3) & 1);
		uint8_t b_inv = !(cfg->src_type == OP_MEMORY ? ((cfg->mem.base >> 3) & 1) : ((cfg->src_reg >> 3) & 1));
		uint8_t vvvv_inv = (~cfg->vvvv) & 0x0F;

		if (cfg->enc_type == ENC_VEX2 && x_inv && b_inv && cfg->vex_m_mmmm == 1) {
			bytes[ptr++] = 0xC5;
			bytes[ptr++] = (r_inv << 7) | (vvvv_inv << 3) | (cfg->vex_l << 2) | (cfg->vex_pp & 3);
		} else {
			bytes[ptr++] = 0xC4;
			bytes[ptr++] = (r_inv << 7) | (x_inv << 6) | (b_inv << 5) | (cfg->vex_m_mmmm & 0x1F);
			uint8_t w = (cfg->operand_size == 64) ? 1 : 0;
			bytes[ptr++] = (w << 7) | (vvvv_inv << 3) | (cfg->vex_l << 2) | (cfg->vex_pp & 3);
		}
	} else {
		uint8_t rex = 0x40;
		if (cfg->operand_size == 64) rex |= 0x08;

		if (cfg->has_modrm) {
			if ((cfg->reg & 0x1F) >= 8) rex |= 0x04;

			if (cfg->src_type == OP_MEMORY) {
				if ((cfg->mem.index & 0x1F) >= 8 && cfg->mem.index != NO_REG)
					rex |= 0x02;
				if ((cfg->mem.base & 0x1F) >= 8 && cfg->mem.base != NO_REG)
					rex |= 0x01;
			} else if ((cfg->src_reg & 0x1F) >= 8)
				rex |= 0x01;
		} else {
			if ((cfg->reg & 0x1F) >= 8) rex |= 0x01; // No modrm
		}

		bool force_rex = (cfg->operand_size == 8) && !is_high_reg &&
			(((cfg->reg & 0x0F) >= 4 && (cfg->reg & 0x0F) <= 7) ||
			(cfg->src_type == OP_REG && (cfg->src_reg & 0x0F) >= 4 && (cfg->src_reg & 0x0F) <= 7));

		if ((rex != 0x40 || force_rex) && !is_high_reg) bytes[ptr++] = rex; // if changed or force
	}

	// OPCODE
	for (uint8_t i = 0;i < cfg->opcode_len;i++) {
		uint8_t b = cfg->opcode[i];
		if (!cfg->has_modrm && i == cfg->opcode_len -1) {
			b |= (cfg->reg & 7);
		}
		bytes[ptr++] = b;
	}

	if (cfg->has_modrm) {
		// ModR/M
		uint8_t modrm = 0;
		uint8_t sizeofs = 0;
		/* Mod
			11 - call eax              | REG
			00 - call [eax]            | MEMORY
			01 - call [eax+0x05]       |
			10 - call [eax+0x11223344] |
		*/
		modrm |= (cfg->reg & 7) << 3;

		if (cfg->src_type == OP_REG || cfg->src_type == OP_IMMEDIATE) {
			modrm |= 3 << 6;
			modrm |= cfg->src_reg & 7;
			bytes[ptr++] = modrm;
		} else if (cfg->is_rip_relative) {
			modrm |= (0 << 6);
			modrm |= 5;
			bytes[ptr++] = modrm;
			sizeofs = 4;
		} else if (cfg->src_type == OP_MEMORY) {
			bool need_sib = (cfg->mem.index != NO_REG) ||
					((cfg->mem.base & 7) == 4) ||
					(cfg->mem.base == NO_REG);

			if (cfg->mem.base == NO_REG) {
				modrm |= 0 << 6;
				sizeofs = 4;
			} else if (cfg->mem.disp == 0 && (cfg->mem.base & 7) != 5) {
				modrm |= 0 << 6;
				sizeofs = 0;
			} else if (cfg->mem.disp <= 127 && -128 <= cfg->mem.disp) { // -128 127
				modrm |= 1 << 6;
				sizeofs = 1;
			} else { // 4 bytes.
				modrm |= 2 << 6;
				sizeofs = 4;
			}

			if (need_sib) {
				modrm |= 4;
			} else {
				modrm |= (cfg->mem.base & 7);
			}

			bytes[ptr++] = modrm;

			if (need_sib) {
				// SIB
				uint8_t sib = 0;

				// You can use for scale real numbers! 0 2 4 8
				uint8_t scale_val = 0;
				if (cfg->mem.scale == 2)      scale_val = 1;
				else if (cfg->mem.scale == 4) scale_val = 2;
				else if (cfg->mem.scale == 8) scale_val = 3;

				sib |= (scale_val & 3) << 6;
				if (cfg->mem.index == NO_REG) sib |= 4 << 3; else
					sib |= (cfg->mem.index & 7) << 3;
				if (cfg->mem.base == NO_REG) sib |= 5; else
					sib |= cfg->mem.base & 7;

				bytes[ptr++] = sib;
			}
		}

		// Offset
		if (cfg->src_type == OP_MEMORY || cfg->is_rip_relative) {
			for (uint8_t j = 0; j < sizeofs; j++) {
					bytes[ptr++] = (uint8_t)((cfg->mem.disp >> (j * 8)) & 0xFF);
			}
		}

		/* Reg/Opcode
			Mod - 11
			mov edx,ebx = edx |000|111|

			0xFF, reg =
			inc - 000
			dec - 000
			call - 010 // close call
			call - 011 // far call
			jmp - 100 // close jmp
			jmp - 101 // far jmp
			push - 110
		*/
		/* R/M Register / Memory
			Mod - 11
			mov edx,ebx = ebx
			jmp edi = edi
		*/
	}

	// Immediate
	if (cfg->imm_size > 0) {
		for (uint8_t j = 0; j < cfg->imm_size; j++) {
			bytes[ptr++] = (uint8_t)((cfg->imm >> (j*8)) & 0xFF);
		}
	}

	// For symbols
	if (out_len) *out_len = ptr;
}

typedef struct {
	uint8_t reg;
	uint8_t operand_size;
} REGINFO;

typedef struct {
    const char* name;
    uint8_t reg;
    uint8_t operand_size;
} RegMapping;

static const RegMapping REG_TABLE[] = {
    // --- 64-bit registers ---
    {"rax", 0, 64},  {"rcx", 1, 64},  {"rdx", 2, 64},  {"rbx", 3, 64},
    {"rsp", 4, 64},  {"rbp", 5, 64},  {"rsi", 6, 64},  {"rdi", 7, 64},
    {"r8",  8, 64},  {"r9",  9, 64},  {"r10", 10, 64}, {"r11", 11, 64},
    {"r12", 12, 64}, {"r13", 13, 64}, {"r14", 14, 64}, {"r15", 15, 64},

    // --- 32-bit registers ---
    {"eax", 0, 32},  {"ecx", 1, 32},  {"edx", 2, 32},  {"ebx", 3, 32},
    {"esp", 4, 32},  {"ebp", 5, 32},  {"esi", 6, 32},  {"edi", 7, 32},
    {"r8d", 8, 32},  {"r9d", 9, 32},  {"r10d", 10, 32}, {"r11d", 11, 32},
    {"r12d", 12, 32}, {"r13d", 13, 32}, {"r14d", 14, 32}, {"r15d", 15, 32},

    // --- 16-bit registers ---
    {"ax",  0, 16},  {"cx",  1, 16},  {"dx",  2, 16},  {"bx",  3, 16},
    {"sp",  4, 16},  {"bp",  5, 16},  {"si",  6, 16},  {"di",  7, 16},
    {"r8w", 8, 16},  {"r9w", 9, 16},  {"r10w", 10, 16}, {"r11w", 11, 16},
    {"r12w", 12, 16}, {"r13w", 13, 16}, {"r14w", 14, 16}, {"r15w", 15, 16},

    // --- 8-bit Low registers ---
    {"al",   0, 8},  {"cl",   1, 8},  {"dl",   2, 8},  {"bl",   3, 8},
    {"spl",  4, 8},  {"bpl",  5, 8},  {"sil",  6, 8},  {"dil",  7, 8},
    {"r8b",  8, 8},  {"r9b",  9, 8},  {"r10b", 10, 8}, {"r11b", 11, 8},
    {"r12b", 12, 8}, {"r13b", 13, 8}, {"r14b", 14, 8}, {"r15b", 15, 8},

    // --- 8-bit High registers (AH, CH, DH, BH) ---
    {"ah", 0x20, 8}, {"ch", 0x21, 8}, {"dh", 0x22, 8}, {"bh", 0x23, 8}
};

#define REG_TABLE_SIZE (sizeof(REG_TABLE) / sizeof(REG_TABLE[0]))

REGINFO regtonum(const char* txt) {
    REGINFO res = { .reg = NO_REG, .operand_size = 0 };

    if (!txt) return res;

    for (size_t i = 0; i < REG_TABLE_SIZE; i++) {
        if (strcmp(txt, REG_TABLE[i].name) == 0) {
            res.reg = REG_TABLE[i].reg;
            res.operand_size = REG_TABLE[i].operand_size;
            return res;
        }
    }

    return res; // Not founded = reg = NO_REG (0xFF)
}

typedef struct {
	const char* name;
	OpType dest_type;        // Destination
	OpType src_type;         // Source
	uint8_t operand_size;    // 0 = any size, 8, 16, 32, 64

	uint8_t opcode[3];
	uint8_t opcode_len;

	bool has_modrm;          // Need ModR/M
	uint8_t modrm_ext;       // Extension for ModR/M
	bool opcode_plus_reg;    // Reg in opcode
} InstructionTemplate;

static const InstructionTemplate OPCODE_TABLE[] = {
	// --- 0-zero-bytes  ---
	{"nop",      OP_NONE, OP_NONE, 0,  {0x90}, 1, false, 0xFF, false},
	{"ret",      OP_NONE, OP_NONE, 0,  {0xC3}, 1, false, 0xFF, false},
	{"int3",     OP_NONE, OP_NONE, 0,  {0xCC}, 1, false, 0xFF, false},
	{"hlt",      OP_NONE, OP_NONE, 0,  {0xF4}, 1, false, 0xFF, false},
	{"leave",    OP_NONE, OP_NONE, 0,  {0xC9}, 1, false, 0xFF, false},
	{"syscall",  OP_NONE, OP_NONE, 0,  {0x0F, 0x05}, 2, false, 0xFF, false},

	// --- Стек (PUSH / POP) ---
	{"push",     OP_REG,       OP_NONE, 64, {0x50}, 1, false, 0xFF, true},
	{"pop",      OP_REG,       OP_NONE, 64, {0x58}, 1, false, 0xFF, true},
	{"push",     OP_IMMEDIATE, OP_NONE, 32, {0x68}, 1, false, 0xFF, false},

	// --- CALL / JMP ---
	{"call",     OP_REG,       OP_NONE, 32, {0xFF}, 1, true,  2,    false}, // call eax (/2)
	{"call",     OP_REG,       OP_NONE, 64, {0xFF}, 1, true,  2,    false}, // call rax (/2)
	{"call",     OP_IMMEDIATE, OP_NONE, 32, {0xE8}, 1, false, 0xFF, false}, // call imm32
	{"jmp",      OP_REG,       OP_NONE, 32, {0xFF}, 1, true,  4,    false}, // jmp eax (/4)
	{"jmp",      OP_REG,       OP_NONE, 64, {0xFF}, 1, true,  4,    false}, // jmp rax (/4)
	{"jmp",      OP_IMMEDIATE, OP_NONE, 32, {0xE9}, 1, false, 0xFF, false}, // jmp imm32

	// -- INC / DICK ---
	{"inc",      OP_REG,  OP_NONE, 32, {0xFF}, 1, true,  0,    false},
	{"inc",      OP_REG,  OP_NONE, 64, {0xFF}, 1, true,  0,    false},
	{"inc",      OP_REG,  OP_NONE, 8,  {0xFE}, 1, true,  0,    false},
	{"dec",      OP_REG,  OP_NONE, 32, {0xFF}, 1, true,  1,    false},
	{"dec",      OP_REG,  OP_NONE, 64, {0xFF}, 1, true,  1,    false},
	{"dec",      OP_REG,  OP_NONE, 8,  {0xFE}, 1, true,  1,    false},

	// --- MOV ---
	{"mov",      OP_REG,  OP_REG,       32, {0x8B}, 1, true,  0xFF, false},

	{"mov",      OP_REG,  OP_REG,       64, {0x8B}, 1, true,  0xFF, false},
	{"mov",      OP_REG,  OP_REG,       8,  {0x8A}, 1, true,  0xFF, false},
	{"mov",      OP_REG,  OP_MEMORY,    32, {0x8B}, 1, true,  0xFF, false},
	{"mov",      OP_REG,  OP_MEMORY,    64, {0x8B}, 1, true,  0xFF, false},
	{"mov",      OP_MEMORY,OP_REG,      32, {0x89}, 1, true,  0xFF, false},
	{"mov",      OP_MEMORY,OP_REG,      64, {0x89}, 1, true,  0xFF, false},
	{"mov",      OP_REG,  OP_IMMEDIATE, 32, {0xB8}, 1, false, 0xFF, true},  // B8 + reg
	{"mov",      OP_REG,  OP_IMMEDIATE, 64, {0xB8}, 1, false, 0xFF, true},  // B8 + reg
	{"mov",      OP_REG,  OP_IMMEDIATE, 8,  {0xB0}, 1, false, 0xFF, true},  // B0 + reg

	// --- ADD ---
	{"add",      OP_REG,  OP_REG,       32, {0x01}, 1, true,  0xFF, false},
	{"add",      OP_REG,  OP_REG,       64, {0x01}, 1, true,  0xFF, false},
	{"add",      OP_REG,  OP_REG,       8,  {0x00}, 1, true,  0xFF, false},
	{"add",      OP_REG,  OP_IMMEDIATE, 32, {0x81}, 1, true,  0,    false}, // /0
	{"add",      OP_REG,  OP_IMMEDIATE, 64, {0x81}, 1, true,  0,    false},

	// --- SUB ---
	{"sub",      OP_REG,  OP_REG,       32, {0x29}, 1, true,  0xFF, false},
	{"sub",      OP_REG,  OP_REG,       64, {0x29}, 1, true,  0xFF, false},
	{"sub",      OP_REG,  OP_REG,       8,  {0x28}, 1, true,  0xFF, false},
	{"sub",      OP_REG,  OP_IMMEDIATE, 32, {0x81}, 1, true,  5,    false}, // /5
	{"sub",      OP_REG,  OP_IMMEDIATE, 64, {0x81}, 1, true,  5,    false},

	// --- CMP ---
	{"cmp",      OP_REG,  OP_REG,       32, {0x39}, 1, true,  0xFF, false},
	{"cmp",      OP_REG,  OP_REG,       64, {0x39}, 1, true,  0xFF, false},
	{"cmp",      OP_REG,  OP_REG,       8,  {0x38}, 1, true,  0xFF, false},
	{"cmp",      OP_REG,  OP_IMMEDIATE, 32, {0x81}, 1, true,  7,    false}, // /7
	{"cmp",      OP_REG,  OP_IMMEDIATE, 64, {0x81}, 1, true,  7,    false},
	{"cmp",      OP_REG,  OP_IMMEDIATE, 8,  {0x80}, 1, true,  7,    false},

	// --- AND ---
	{"and",      OP_REG,  OP_REG,       32, {0x21}, 1, true,  0xFF, false},
	{"and",      OP_REG,  OP_REG,       64, {0x21}, 1, true,  0xFF, false},
	{"and",      OP_REG,  OP_IMMEDIATE, 32, {0x81}, 1, true,  4,    false}, // /4

	// --- OR ---
	{"or",       OP_REG,  OP_REG,       32, {0x09}, 1, true,  0xFF, false},
	{"or",       OP_REG,  OP_REG,       64, {0x09}, 1, true,  0xFF, false},
	{"or",       OP_REG,  OP_IMMEDIATE, 32, {0x81}, 1, true,  1,    false}, // /1

	// --- XOR ---
	{"xor",      OP_REG,  OP_REG,       32, {0x31}, 1, true,  0xFF, false},
	{"xor",      OP_REG,  OP_REG,       64, {0x31}, 1, true,  0xFF, false},
	{"xor",      OP_REG,  OP_IMMEDIATE, 32, {0x81}, 1, true,  6,    false}, // /6

	// --- TEST ---
	{"test",     OP_REG,  OP_REG,       32, {0x85}, 1, true,  0xFF, false},
	{"test",     OP_REG,  OP_REG,       64, {0x85}, 1, true,  0xFF, false},
	{"test",     OP_REG,  OP_IMMEDIATE, 32, {0xF7}, 1, true,  0,    false}, // /0

	// --- LEA ---
	{"lea",      OP_REG,  OP_MEMORY,    32, {0x8D}, 1, true,  0xFF, false},
	{"lea",      OP_REG,  OP_MEMORY,    64, {0x8D}, 1, true,  0xFF, false}
};

#define OPCODE_TABLE_SIZE (sizeof(OPCODE_TABLE) / sizeof(OPCODE_TABLE[0]))

const InstructionTemplate* find_instruction(
	const char* mnemonic,
	OpType dest_type,
	OpType src_type,
	uint8_t op_size
) {
	if (!mnemonic) return NULL;

	for (size_t i = 0; i < OPCODE_TABLE_SIZE; i++) {
		const InstructionTemplate* tmpl = &OPCODE_TABLE[i];

		if (strcmp(mnemonic, tmpl->name) != 0) continue;

		if (tmpl->dest_type != dest_type || tmpl->src_type != src_type) continue;

		if (tmpl->operand_size != 0 && op_size != 0 && tmpl->operand_size != op_size) continue;

		return tmpl;
	}

	return NULL;
}

typedef struct {
	uint8_t base;
	uint8_t index;
	uint8_t scale; // 1 2 4 8
	int32_t disp;
} __MEM;

__MEM kvad_parser(char* txt) {
	__MEM res = {.base = NO_REG, .index = NO_REG, .scale = 1,.disp = 0};
	char buf[32], *p = buf;

	for (int i = 0; txt[i];i++) {
		if (txt[i] != '[' && txt[i] != ']' && txt[i] != ' ' && txt[i] != '\t') {
			*p++ = txt[i];
		}
	}
	*p = '\0';

	p = buf;
	int8_t sign = 1;
	while (*p) {
		if (*p == '+') { sign = 1; p++; continue; }
		if (*p == '-') { sign = -1; p++; continue; }

		char term[32];
		char* end = p;
		while (*end && *end != '+' && *end != '-') end++;
		uint8_t len = end - p;
		for (uint8_t c = 0;c < len;c++) { // memcpy
			term[c] = p[c];
		}
		term[len] = '\0';

		char* star = str_char(term,'*');
		if (star) {
			*star = '\0';
			REGINFO r1 = regtonum(term);
			REGINFO r2 = regtonum(star + 1);

			if (r1.reg != NO_REG) {          // rax * 4
				res.index = r1.reg;
				res.scale = (uint8_t)uni_num(star + 1).num;
			} else if (r2.reg != NO_REG) {   // 4 * rax
				res.index = r2.reg;
				res.scale = (uint8_t)uni_num(term).num;
			} else {                         // 0x50 * 4 
				uint32_t n1 = uni_num(term).num;
				uint32_t n2 = uni_num(star + 1).num;
				res.disp += (int32_t)(n1 * n2) * sign;
			}
		} else {
			REGINFO r = regtonum(term);
			if (r.reg != NO_REG) {
				if (res.base == NO_REG) res.base = r.reg;
				else res.index = r.reg;
			} else {
				res.disp += (int32_t)uni_num(term).num * sign;
			}
		}
		p = end;
	}

	return res;
}

GENED_T genbytecode(COMP_T* s) { // mode | 0 for gen | 1 for calc size
	GENED_T result;
	SCANR_T args[10] = {0};

	args_parser(s->asmcode,args);

	// printf("Parser args:[0] %s | [1] %s\n\r",args[0].txt,args[1].txt);
	/*
		EncType enc_type;

		uint8_t legacy_prefix;
		uint8_t operand_size; // 8 16 32 64 128 (XMM) 256 (YMM) 512 (ZMM)
		uint8_t address_size;

		uint8_t opcode_len;
		uint8_t opcode[3];

		uint8_t has_modrm;
		uint8_t reg;     // 0-15

		OpType src_type;
		uint8_t src_reg; // 0-15

		uint8_t vvvv;
		uint8_t vex_l;
		uint8_t vex_pp;
		uint8_t vex_m_mmmm;

		uint8_t evex_z;
		uint8_t evex_aaa;
		uint8_t evex_b;

		bool is_rip_relative;
		struct {
			uint8_t base;
			uint8_t index;
			uint8_t scale; // 1 2 4 8
			int32_t disp;
		} mem;

		int64_t imm;
		uint8_t imm_size;
	*/

	const char* mnemonic = NULL; // mov
	OpType dest_type = OP_NONE;
	OpType src_type = OP_NONE;

	REGINFO dest_reg = { .reg = 0xFF, .operand_size = 0 };
	REGINFO src_reg = { .reg = 0xFF, .operand_size = 0 };
	__MEM mem = { .base = NO_REG, .index = NO_REG, .scale = 1, .disp = 0 };
	UNI_NUM_T imm_val = {0};

	uint8_t arg_idx = 0;

	for (SCANR_T* node = &args[0];node != NULL; node = node->next) {
		trim_end(node->txt);

		printf("[%s] | %u\n\r",node->txt,arg_idx);

		if (arg_idx == 0) {
			mnemonic = node->txt; // First is always mnemonic
		} else {
			if (node->txt[0] == '[') {
				src_type = OP_MEMORY;
				mem = kvad_parser(node->txt);
			} else {
				REGINFO r = regtonum(node->txt);
				UNI_NUM_T imm = uni_num(node->txt);

				if (arg_idx == 1) {
					if (r.reg != 0xFF) {
						dest_type = OP_REG;
						dest_reg = r;
					} else if (imm.has_num) {
						dest_type = OP_IMMEDIATE;
						imm_val = imm;
					}
				} else if (arg_idx == 2) {
					if (r.reg != 0xFF) {
						src_type = OP_REG;
						src_reg = r;
					} else if (imm.has_num) {
						src_type = OP_IMMEDIATE;
						imm_val = imm;
					}
				}
			}
		}
		arg_idx++;
	}

	// Generate struct

	const InstructionTemplate* tmpl = find_instruction(mnemonic,dest_type,src_type,dest_reg.operand_size);

	//printf("text: [%X] | reg: %u\n\r",cfg.opcode[0],r.reg);

	if (!tmpl) {
		return (GENED_T){ .error = 1};
	}

	INSCFG cfg = {0};
	cfg.operand_size = tmpl->operand_size;
	cfg.has_modrm = tmpl->has_modrm;
	cfg.opcode_len = tmpl->opcode_len;
	for (uint8_t i=0;i<tmpl->opcode_len;i++) cfg.opcode[i] = tmpl->opcode[i];

	cfg.mem.base = mem.base;
	cfg.mem.index = mem.index;
	cfg.mem.scale = mem.scale;
	cfg.mem.disp = mem.disp;

	cfg.src_type = src_type;

	if (dest_type == OP_REG) {
		cfg.reg = dest_reg.reg;
	}

	if (src_type == OP_REG) {
		cfg.src_reg = src_reg.reg;
	} else if (src_type == OP_IMMEDIATE) {
		cfg.imm = imm_val.num;
		cfg.imm_size = (cfg.operand_size == 32) ? 4 : (cfg.operand_size == 64 ? 4 : 1);
	}
	if (dest_type == OP_IMMEDIATE) {
		cfg.imm = imm_val.num;
		cfg.imm_size = (cfg.operand_size == 32) ? 4 : (cfg.operand_size == 64 ? 4 : 1);
	}

	if (tmpl->modrm_ext != 0xFF) {
		cfg.src_type = dest_type;
		cfg.reg = tmpl->modrm_ext;
		cfg.src_reg = dest_reg.reg;
	}

	if (tmpl->opcode_plus_reg) {
		cfg.opcode[cfg.opcode_len - 1] += (dest_reg.reg & 7);
	}

	uint8_t len = 0;
	uint8_t buffer[15] = {};

	if (!s->mode)
		constructor_bytes(&cfg,s->bytes,&len);
	else
		constructor_bytes(&cfg,buffer,&len);

	result.gened = len;

	return result;
}

#endif

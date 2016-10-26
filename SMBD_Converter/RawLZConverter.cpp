#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "RawLZConverter.h"


#define SMB2 0

#define SMBD 1

inline uint32_t readBigInt(FILE *file) {
	uint32_t c1 = getc(file) << 24;
	uint32_t c2 = getc(file) << 16;
	uint32_t c3 = getc(file) << 8;
	uint32_t c4 = getc(file);
	return (c1 | c2 | c3 | c4);
}

inline uint32_t readLittleInt(FILE *file) {
	uint32_t c1 = getc(file);
	uint32_t c2 = getc(file) << 8;
	uint32_t c3 = getc(file) << 16;
	uint32_t c4 = getc(file) << 24;
	return (c1 | c2 | c3 | c4);
}

inline void writeBigInt(FILE *file, uint32_t value) {
	putc((value << 24), file);
	putc((value << 16), file);
	putc((value << 8), file);
	putc((value), file);
}

inline void writeLittleInt(FILE *file, uint32_t value) {
	putc((value), file);
	putc((value >> 8), file);
	putc((value >> 16), file);
	putc((value >> 24), file);
}

typedef struct {
	int number;
	int offset;
}Item;

void parseRawLZ(char* filename) {
	int game = 0;

	// Using function pointers to read/write values keeps things game agnostic (can convert from SMBD to SMB2 or SMB2 to SMBD)
	uint32_t(*readInt)(FILE*);
	void(*writeInt)(FILE*, uint32_t);

	std::string outfilename = std::string(filename);

	FILE *original = fopen(filename, "rb");

	if (original == NULL) {
		printf("Error opening file\n");
		return;
	}

	fseek(original, 4, SEEK_SET);

	// Determine which game the raw lz is from and set function pointers/vars accordingly
	if (getc(original) == 0) {
		game = SMBD;
		outfilename += ".smb2";
		readInt = &readLittleInt;
		writeInt = &writeBigInt;
	}
	else {
		game = SMB2;
		outfilename += ".smbd";
		readInt = &readBigInt;
		writeInt = &writeLittleInt;

	}

	FILE *converted = fopen(outfilename.c_str(), "wb");

	fseek(original, 0, SEEK_SET);

	// Actually begin parsing/converting

	// Declare variables here for so they are seen when relavant

	Item collisionFields;
	Item startPositions;
	Item falloutY;
	Item goals;
	Item bumpers;
	Item jamabars;
	Item bananas;
	Item backgroundModels;
	Item levelModelA;
	Item levelModelB;
	// More Items I still need to figure out/waiting for Yohsi to document


	// Header
	writeInt(converted, readInt(original));
	writeInt(converted, readInt(original));

	// Collision Fields
	collisionFields.number = readInt(original);
	collisionFields.offset = readInt(original);
	writeInt(converted, collisionFields.number);
	writeInt(converted, collisionFields.offset);

	// Start Positions
	startPositions.offset = readInt(original);
	writeInt(converted, startPositions.offset);

	// Fallout Y
	falloutY.offset = readInt(original);
	writeInt(converted, falloutY.offset);

	// Number of start positions is determined by the fallout Y offset
	startPositions.number = (falloutY.offset - startPositions.offset) / 0x14;

	// Goals
	goals.number = readInt(original);
	goals.offset = readInt(original);
	writeInt(converted, goals.number);
	writeInt(converted, goals.offset);

	// Bumpers
	bumpers.number = readInt(original);
	bumpers.offset = readInt(original);
	writeInt(converted, bumpers.number);
	writeInt(converted, bumpers.offset);

	// Jamabars
	jamabars.number = readInt(original);
	jamabars.offset = readInt(original);
	writeInt(converted, jamabars.number);
	writeInt(converted, jamabars.offset);

	// Bananas
	bananas.number = readInt(original);
	bananas.offset = readInt(original);
	writeInt(converted, bananas.number);
	writeInt(converted, bananas.offset);

	// Dead Zone (0x20)
	for (int i = 0; i < 0x20 / 4; ++i) {
		writeInt(converted, readInt(original));
	}

	// Background Models
	backgroundModels.number = readInt(original);
	backgroundModels.offset = readInt(original);
	writeInt(converted, backgroundModels.number);
	writeInt(converted, backgroundModels.offset);

	// 3 zeros, 1 one, 7 zeros
	for (int i = 0; i < 11; ++i) {
		writeInt(converted, readInt(original));
	}

	// Level Model A
	levelModelA.number = readInt(original);
	levelModelA.offset = readInt(original);
	writeInt(converted, levelModelA.number);
	writeInt(converted, levelModelA.offset);

	// Level Model B
	levelModelB.number = readInt(original);
	levelModelB.offset = readInt(original);
	writeInt(converted, levelModelB.number);
	writeInt(converted, levelModelB.offset);

	// Dead Zone (0x800)
	for (int i = 0; i < 0x800 / 4; ++i) {
		writeInt(converted, readInt(original));
	}

	// End of header



	fclose(original);
	fclose(converted);
}
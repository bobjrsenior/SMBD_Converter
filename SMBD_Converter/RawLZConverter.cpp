#include "RawLZConverter.h"

inline void copyAsciiAligned(FILE *input, FILE *output, uint32_t offset) {
	uint32_t savePos = ftell(input);
	fseek(input, offset, SEEK_SET);
	fseek(output, offset, SEEK_SET);
	int c;
	do {
		c = getc(input);
		if (c == EOF) {
			break;
		}
		putc(c, output);
	} while (!feof(input) && !(c == 0 && ftell(input) % 4 == 0));
	fseek(input, savePos, SEEK_SET);
	fseek(output, savePos, SEEK_SET);
}

inline void copyAnimation(FILE *input, FILE *output, uint32_t offset, uint32_t(*readInt)(FILE*), void(*writeInt)(FILE*, uint32_t));

typedef struct {
	int number;
	int offset;
}Item;

void parseRawLZ(const char* filename) {
	int game = 0;

	// Using function pointers to read/write values keeps things game agnostic (can convert from SMBD to SMB2 or SMB2 to SMBD)
	uint32_t(*readInt)(FILE*);
	void(*writeInt)(FILE*, uint32_t);
	void(*writeNormalInt)(FILE*, uint32_t);

	uint16_t(*readShort)(FILE*);
	void(*writeShort)(FILE*, uint16_t);
	void(*writeNormalShort)(FILE*, uint16_t);

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
		readShort = &readLittleShort;

		writeInt = &writeBigInt;
		writeShort = &writeBigShort;

		writeNormalInt = &writeLittleInt;
		writeNormalShort = &writeLittleShort;
	}
	else {
		game = SMB2;
		outfilename += ".smbd";
		readInt = &readBigInt;
		readShort = &readBigShort;

		writeInt = &writeLittleInt;
		writeShort = &writeLittleShort;

		writeNormalInt = &writeBigInt;
		writeNormalShort = &writeBigShort;

	}

	FILE *converted = fopen(outfilename.c_str(), "wb");

	if (converted == NULL) {
		fclose(original);
		printf("Failed to open file: %s\n", strerror(errno));
		return;
	}

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
	Item falloutVolumes;
	Item backgroundModels;
	Item reflectiveModels;
	Item levelModelA;
	Item levelModelB;
	Item switches;
	Item wormholes;
	Item fog;
	Item mysteryThree;

#pragma region File_Header


	// Header (0x0, length = 0x8)
	writeInt(converted, readInt(original));
	writeInt(converted, readInt(original));

	// Collision Header (0x8, length = 0x8)
	collisionFields.number = readInt(original);
	collisionFields.offset = readInt(original);
	writeInt(converted, collisionFields.number);
	writeInt(converted, collisionFields.offset);

	// Start positions (0x10, length = 0x4)
	startPositions.offset = readInt(original);
	writeInt(converted, startPositions.offset);

	// Fallout Y (0x14, length = 0x4)
	falloutY.offset = readInt(original);
	writeInt(converted, falloutY.offset);

	// The number of start positions is the fallout Y offset - startPosition offset / sizeof(startPosition)
	startPositions.number = (falloutY.offset - startPositions.offset) / 0x14;

	// Goals (0x18, length = 0x8)
	goals.number = readInt(original);
	goals.offset = readInt(original);
	writeInt(converted, goals.number);
	writeInt(converted, goals.offset);

	// Bumpers (0x20, length = 0x8)
	bumpers.number = readInt(original);
	bumpers.offset = readInt(original);
	writeInt(converted, goals.number);
	writeInt(converted, goals.offset);

	// Jamabars (0x28, length = 0x8)
	jamabars.number = readInt(original);
	jamabars.offset = readInt(original);
	writeInt(converted, jamabars.number);
	writeInt(converted, jamabars.offset);

	// Bananas (0x30, length = 0x8)
	bananas.number = readInt(original);
	bananas.offset = readInt(original);
	writeInt(converted, bananas.number);
	writeInt(converted, bananas.offset);

	// Unknown/Null (0x38, length = 0x18)
	for (int i = 0; i < 0x18; i += 4) {
		writeInt(converted, readInt(original));
	}

	// Fallout Volumes (0x50, length = 0x8)
	falloutVolumes.number = readInt(original);
	falloutVolumes.offset = readInt(original);
	writeInt(converted, falloutVolumes.number);
	writeInt(converted, falloutVolumes.offset);

	// Background Models (0x58, length = 0x8)
	backgroundModels.number = readInt(original);
	backgroundModels.offset = readInt(original);
	writeInt(converted, backgroundModels.number);
	writeInt(converted, backgroundModels.offset);

	// Unknown/Null (0x60, length = 0xC)
	for (int i = 0; i < 0xC; i += 4) {
		writeInt(converted, readInt(original));
	}

	// One (0x6C, length = 0x4)
	writeInt(converted, readInt(original));

	// Reflective Models (0x70, length = 0x8)
	reflectiveModels.number = readInt(original);
	reflectiveModels.offset = readInt(converted);

	// Unknown/Null (0x78, length = 0x14)
	for (int i = 0; i < 0x14; i += 4) {
		writeInt(converted, readInt(original));
	}

	// Level Model A (0x8C, length = 0x8)
	levelModelA.number = readInt(original);
	levelModelA.offset = readInt(original);
	writeInt(converted, levelModelA.number);
	writeInt(converted, levelModelA.offset);

	// Level Model B (0x94, length = 0x8)
	levelModelB.number = readInt(original);
	levelModelB.offset = readInt(original);
	writeInt(converted, levelModelB.number);
	writeInt(converted, levelModelB.offset);

	// Unknown/Null (0x9C, length = 0x8)
	for (int i = 0; i < 0x8; i += 4) {
		writeInt(converted, readInt(original));
	}

	// Switches (0xA4, length = 0x8)
	switches.number = readInt(original);
	switches.offset = readInt(original);
	writeInt(converted, switches.number);
	writeInt(converted, switches.offset);

	// Unknown/Null (0xAC, length = 0x8)
	for (int i = 0; i < 0x8; i += 4) {
		writeInt(converted, readInt(original));
	}

	// Wormholes (0xB4, length = 0x8)
	wormholes.number = readInt(original);
	wormholes.offset = readInt(original);
	writeInt(converted, wormholes.number);
	writeInt(converted, wormholes.offset);

	// Fog (0xBC, length = 0x4)
	fog.offset = readInt(original);
	writeInt(converted, fog.offset);

	// Unknown/Null (0xC0, length = 0x14)
	for (int i = 0; i < 0x14; i += 4) {
		writeInt(converted, readInt(original));
	}

	// Mystery Three (0xD4, length = 0x4)
	mysteryThree.offset = readInt(original);
	writeInt(converted, mysteryThree.offset);

	// Unknown/Null (0xD8, length = 0x7C4)
	for (int i = 0; i < 0x7C4; i += 4) {
		writeInt(converted, readInt(original));
	}

#pragma endregion File_Header


#pragma region Start_Positions
	fseek(original, startPositions.offset, SEEK_SET);
	fseek(converted, startPositions.offset, SEEK_SET);

	// Loop through start positions
	for (int i = 0; i < startPositions.number; i++) {
		// Position (0x0, length = 0xC)
		writeInt(converted, readInt(original)); // X
		writeInt(converted, readInt(original)); // Y
		writeInt(converted, readInt(original)); // Z

		// Rotation (0xC, length = 0x8)
		writeShort(converted, readShort(original)); // X
		writeShort(converted, readShort(original)); // Y
		writeShort(converted, readShort(original)); // Z
		writeShort(converted, readShort(original)); // Padding/Null
	}
#pragma endregion Start_Positions

#pragma region Fallout_Y
	fseek(original, falloutY.offset, SEEK_SET);
	fseek(converted, falloutY.offset, SEEK_SET);

	// Fallout Y (0x0, length = 0x4)
	writeInt(converted, readInt(original));
#pragma endregion Fallout_Y



	fclose(original);
	fclose(converted);
}

inline void copyAnimation(FILE *input, FILE *output, uint32_t offset, uint32_t(*readInt)(FILE*), void(*writeInt)(FILE*, uint32_t)) {
	
	uint32_t savePos = ftell(input);

	fseek(input, offset, SEEK_SET);
	fseek(output, offset, SEEK_SET);

	
	Item rotX;
	Item rotY;
	Item rotZ;
	Item transX;
	Item transY;
	Item transZ;


	// Rotation Headers (X, Y, Z)
	rotX.number = readInt(input);
	rotX.offset = readInt(input);
	writeInt(output, rotX.number);
	writeInt(output, rotX.offset);

	rotY.number = readInt(input);
	rotY.offset = readInt(input);
	writeInt(output, rotY.number);
	writeInt(output, rotY.offset);

	rotZ.number = readInt(input);
	rotZ.offset = readInt(input);
	writeInt(output, rotZ.number);
	writeInt(output, rotZ.offset);

	// Translation Headers (X, Y, Z)
	transX.number = readInt(input);
	transX.offset = readInt(input);
	writeInt(output, transX.number);
	writeInt(output, transX.offset);

	transY.number = readInt(input);
	transY.offset = readInt(input);
	writeInt(output, transY.number);
	writeInt(output, transY.offset);

	transZ.number = readInt(input);
	transZ.offset = readInt(input);
	writeInt(output, transZ.number);
	writeInt(output, transZ.offset);

	// Copy the animation frames

	// Rotation X
	fseek(input, rotX.offset, SEEK_SET);
	fseek(output, rotX.offset, SEEK_SET);

	for (int i = 0; i < rotX.number; ++i) {
		// Animation Marker
		writeInt(output, readInt(input));

		// Time
		writeInt(output, readInt(input));

		// Displacement
		writeInt(output, readInt(input));

		// Dead Zone (0x8)
		writeInt(output, readInt(input));
		writeInt(output, readInt(input));
	}

	// Rotation Y
	fseek(input, rotY.offset, SEEK_SET);
	fseek(output, rotY.offset, SEEK_SET);

	for (int i = 0; i < rotY.number; ++i) {
		// Animation Marker
		writeInt(output, readInt(input));

		// Time
		writeInt(output, readInt(input));

		// Displacement
		writeInt(output, readInt(input));

		// Dead Zone (0x8)
		writeInt(output, readInt(input));
		writeInt(output, readInt(input));
	}

	// Rotation Z
	fseek(input, rotZ.offset, SEEK_SET);
	fseek(output, rotZ.offset, SEEK_SET);

	for (int i = 0; i < rotZ.number; ++i) {
		// Animation Marker
		writeInt(output, readInt(input));

		// Time
		writeInt(output, readInt(input));

		// Displacement
		writeInt(output, readInt(input));

		// Dead Zone (0x8)
		writeInt(output, readInt(input));
		writeInt(output, readInt(input));
	}

	// Translation X
	fseek(input, transX.offset, SEEK_SET);
	fseek(output, transX.offset, SEEK_SET);

	for (int i = 0; i < transX.number; ++i) {
		// Animation Marker
		writeInt(output, readInt(input));

		// Time
		writeInt(output, readInt(input));

		// Displacement
		writeInt(output, readInt(input));

		// Dead Zone (0x8)
		writeInt(output, readInt(input));
		writeInt(output, readInt(input));
	}

	// Translation Y
	fseek(input, transY.offset, SEEK_SET);
	fseek(output, transY.offset, SEEK_SET);

	for (int i = 0; i < transY.number; ++i) {
		// Animation Marker
		writeInt(output, readInt(input));

		// Time
		writeInt(output, readInt(input));

		// Displacement
		writeInt(output, readInt(input));

		// Dead Zone (0x8)
		writeInt(output, readInt(input));
		writeInt(output, readInt(input));
	}

	// Translation Z
	fseek(input, transZ.offset, SEEK_SET);
	fseek(output, transZ.offset, SEEK_SET);

	for (int i = 0; i < transZ.number; ++i) {
		// Animation Marker
		writeInt(output, readInt(input));

		// Time
		writeInt(output, readInt(input));

		// Displacement
		writeInt(output, readInt(input));

		// Dead Zone (0x8)
		writeInt(output, readInt(input));
		writeInt(output, readInt(input));
	}
	
	fseek(input, savePos, SEEK_SET);
	fseek(output, savePos, SEEK_SET);
}
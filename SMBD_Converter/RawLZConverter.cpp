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

inline uint16_t readBigShort(FILE *file) {
	uint16_t c1 = (uint16_t) getc(file) << 8;
	uint16_t c2 = (uint16_t) getc(file);
	return (c1 | c2);
}

inline uint16_t readLittleShort(FILE *file) {
	uint16_t c1 = (uint16_t) getc(file);
	uint16_t c2 = (uint16_t) getc(file) << 8;
	return (c1 | c2);
}

inline void writeBigInt(FILE *file, uint32_t value) {
	putc((value >> 24), file);
	putc((value >> 16), file);
	putc((value >> 8), file);
	putc((value), file);
}

inline void writeLittleInt(FILE *file, uint32_t value) {
	putc((value), file);
	putc((value >> 8), file);
	putc((value >> 16), file);
	putc((value >> 24), file);
}

inline void writeBigShort(FILE *file, uint16_t value) {
	putc((value >> 8), file);
	putc((value), file);
}

inline void writeLittleShort(FILE *file, uint16_t value) {
	putc((value), file);
	putc((value >> 8), file);
}

inline void copyAscii(FILE *input, FILE *output, uint32_t offset) {
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

void parseRawLZ(char* filename) {
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
	Item mysteryOne;
	Item backgroundModels;
	Item levelModelA;
	Item levelModelB;
	Item stageAnimations;
	Item mysteryTwo;
	Item mysteryThree;
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

	// Dead Zone (0x1C)8
	for (int i = 0; i < 0x18 / 4; ++i) {
		writeInt(converted, readInt(original));
	}

	// Unknown
	mysteryOne.number = readInt(original);
	mysteryOne.offset = readInt(original);
	writeInt(converted, mysteryOne.number);
	writeInt(converted, mysteryOne.offset);

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

	// Dead Zone (0x14)
	for (int i = 0; i < 0x14 / 4; ++i) {
		writeInt(converted, readInt(original));
	}

	// Stage Animation
	stageAnimations.offset = readInt(original);
	writeInt(converted, stageAnimations.offset);

	// Dead Zone (0x8)
	writeInt(converted, readInt(original));
	writeInt(converted, readInt(original));

	// Mystery Two
	mysteryTwo.offset = readInt(original);
	writeInt(converted, mysteryTwo.offset);

	// Dead Zone (0x14)
	for (int i = 0; i < 0x14 / 4; ++i) {
		writeInt(converted, readInt(original));
	}

	// Mystery Three
	mysteryThree.offset = readInt(original);
	writeInt(converted, mysteryThree.offset);

	// Dead Zone (0x7C4)
	for (int i = 0; i < 0x7E8 / 4; ++i) {
		writeInt(converted, readInt(original));
	}

	// End of header

	// Start Position
	
	fseek(original, startPositions.offset, SEEK_SET);
	fseek(converted, startPositions.offset, SEEK_SET);
	
	for (int i = 0; i < startPositions.number; ++i) {
		// Position
		writeInt(converted, readInt(original));
		writeInt(converted, readInt(original));
		writeInt(converted, readInt(original));

		// Rotation
		writeShort(converted, readShort(original));
		writeShort(converted, readShort(original));
		writeShort(converted, readShort(original));
		// Padding
		writeShort(converted, readShort(original));
	}

	// Fallout Y

	fseek(original, falloutY.offset, SEEK_SET);
	fseek(converted, falloutY.offset, SEEK_SET);
	writeInt(converted, readInt(original));

	// Order would be write collision fields next, but lets save that for later...

	// Goals

	fseek(original, goals.offset, SEEK_SET);
	fseek(converted, goals.offset, SEEK_SET);
	
	for (int i = 0; i < goals.number; ++i) {
		// Position
		writeInt(converted, readInt(original));
		writeInt(converted, readInt(original));
		writeInt(converted, readInt(original));

		// Rotation
		writeShort(converted, readShort(original));
		writeShort(converted, readShort(original));
		writeShort(converted, readShort(original));
		
		// Goal Type (order preserved regardless of endianess)
		writeNormalShort(converted, readShort(original));
	}
	
	// Bumpers

	fseek(original, bumpers.offset, SEEK_SET);
	fseek(converted, bumpers.offset, SEEK_SET);

	for (int i = 0; i < bumpers.number; ++i) {
		// Position
		writeInt(converted, readInt(original));
		writeInt(converted, readInt(original));
		writeInt(converted, readInt(original));

		// Rotation
		writeShort(converted, readShort(original));
		writeShort(converted, readShort(original));
		writeShort(converted, readShort(original));
		// Padding
		writeShort(converted, readShort(original));

		// Scale
		writeInt(converted, readInt(original));
		writeInt(converted, readInt(original));
		writeInt(converted, readInt(original));
	}

	// Jamabars

	fseek(original, jamabars.offset, SEEK_SET);
	fseek(converted, jamabars.offset, SEEK_SET);

	for (int i = 0; i < jamabars.number; ++i) {
		// Position
		writeInt(converted, readInt(original));
		writeInt(converted, readInt(original));
		writeInt(converted, readInt(original));

		// Rotation
		writeShort(converted, readShort(original));
		writeShort(converted, readShort(original));
		writeShort(converted, readShort(original));
		// Padding
		writeShort(converted, readShort(original));

		// Scale
		writeInt(converted, readInt(original));
		writeInt(converted, readInt(original));
		writeInt(converted, readInt(original));
	}
	
	// Bananas

	fseek(original, bananas.offset, SEEK_SET);
	fseek(converted, bananas.offset, SEEK_SET);

	for (int i = 0; i < bananas.number; ++i) {
		// Position
		writeInt(converted, readInt(original));
		writeInt(converted, readInt(original));
		writeInt(converted, readInt(original));

		// Banana Type
		writeInt(converted, readInt(original));
	}
	
	// Mystery One

	fseek(original, mysteryOne.offset, SEEK_SET);
	fseek(converted, mysteryOne.offset, SEEK_SET);

	for (int i = 0; i < mysteryOne.number; ++i) {
		// Marker?
		writeInt(converted, readInt(original));

		// 5 floats?
		writeInt(converted, readInt(original));
		writeInt(converted, readInt(original));
		writeInt(converted, readInt(original));
		writeInt(converted, readInt(original));
		writeInt(converted, readInt(original));

		// 2 4-byte zeros
		writeInt(converted, readInt(original));
		writeInt(converted, readInt(original));
	}
	
	// Background Models

	fseek(original, backgroundModels.offset, SEEK_SET);
	fseek(converted, backgroundModels.offset, SEEK_SET);

	for (int i = 0; i < backgroundModels.number; ++i) {
		// Background model marker
		writeInt(converted, readInt(original));

		// Model Name offset
		uint32_t modelNameOffset = readInt(original);
		writeInt(converted, modelNameOffset);
		copyAscii(original, converted, modelNameOffset);

		// Padding
		writeInt(converted, readInt(original));

		// Position
		writeInt(converted, readInt(original));
		writeInt(converted, readInt(original));
		writeInt(converted, readInt(original));

		// Rotation
		writeShort(converted, readShort(original));
		writeShort(converted, readShort(original));
		writeShort(converted, readShort(original));
		// Padding
		writeShort(converted, readShort(original));

		// Scale
		writeInt(converted, readInt(original));
		writeInt(converted, readInt(original));
		writeInt(converted, readInt(original));


		// BG Animation 1?
		uint32_t animation1Offset = readInt(original);
		writeInt(converted, animation1Offset);

		// BG Animation 2?
		uint32_t animation2Offset = readInt(original);
		writeInt(converted, animation2Offset);

		// Mystery 4
		uint32_t mysteryFour = readInt(original);
		writeInt(converted, mysteryFour);

		uint32_t savePos = ftell(original);

		if (animation1Offset != 0) {
			// Copy BG animation 1
			fseek(original, animation1Offset, SEEK_SET);
			fseek(converted, animation1Offset, SEEK_SET);

			// Zero
			writeInt(converted, readInt(original));

			// Always 0x41000000 (ascii A)?
			writeInt(converted, readInt(original));

			// Zero/Unknown (0x18)
			for (int j = 0; j < 0x18 / 4; ++j) {
				writeInt(converted, readInt(original));
			}

			// Animation
			copyAnimation(original, converted, ftell(original), readInt, writeInt);

			// Seek past animation
			fseek(original, 24, SEEK_CUR);
			fseek(converted, 24, SEEK_CUR);

			// Zero/Unknown (0x16)
			for (int j = 0; j < 0x16 / 4; ++j) {
				writeInt(converted, readInt(original));
			}
		}

		if (animation2Offset != 0) {
			// Copy BG animation 2
			fseek(original, animation2Offset, SEEK_SET);
			fseek(converted, animation2Offset, SEEK_SET);

			// Zero
			writeInt(converted, readInt(original));

			// Always 0x41000000 (ascii A)?
			writeInt(converted, readInt(original));

			// Zero/Unknown (0x18)
			for (int j = 0; j < 0x18 / 4; ++j) {
				writeInt(converted, readInt(original));
			}

			// Animation
			copyAnimation(original, converted, ftell(original), readInt, writeInt);

			// Seek past animation
			fseek(original, 24, SEEK_CUR);
			fseek(converted, 24, SEEK_CUR);

			// Zero/Unknown (0x16)
			for (int j = 0; j < 0x16 / 4; ++j) {
				writeInt(converted, readInt(original));
			}
		}

		if (mysteryFour != 0) {
			// Copy Mystery Four
			fseek(original, mysteryFour, SEEK_SET);
			fseek(converted, mysteryFour, SEEK_SET);

			// Zero/Unknown (0x10)
			for (int j = 0; j < 0x10 / 4; ++j) {
				writeInt(converted, readInt(original));
			}

			uint32_t oddOffset = readInt(original);
			writeInt(converted, oddOffset);

			// Zero/Unknown (0x1C)
			for (int j = 0; j < 0x1C / 4; ++j) {
				writeInt(converted, readInt(original));
			}

			fseek(original, oddOffset, SEEK_SET);
			fseek(converted, oddOffset, SEEK_SET);

			// Two floats
			writeInt(converted, readInt(original));
			writeInt(converted, readInt(original));
		}

		fseek(original, savePos, SEEK_SET);
		fseek(converted, savePos, SEEK_SET);
	}
	
	// Level Model As

	fseek(original, levelModelA.offset, SEEK_SET);
	fseek(converted, levelModelA.offset, SEEK_SET);

	for (int i = 0; i < levelModelA.number; ++i) {
		// Level Model A marker
		writeInt(converted, readInt(original));
		writeInt(converted, readInt(original));

		// Level Model offset
		uint32_t levelModelOffset = readInt(original);
		writeInt(converted, levelModelOffset);

		uint32_t savePos = ftell(original);

		// Level Model
		
		fseek(original, levelModelOffset - 8, SEEK_SET);
		fseek(converted, levelModelOffset - 8, SEEK_SET);

		// Header (All zero) (0xC)
		for (int j = 0; j < 0xC / 4; ++j) {
			writeInt(converted, readInt(original));
		}

		uint32_t modelNameOffset = readInt(original);
		writeInt(converted, modelNameOffset);
		copyAscii(original, converted, modelNameOffset);

		fseek(original, savePos, SEEK_SET);
		fseek(converted, savePos, SEEK_SET);


	}

	// Level Model Bs

	fseek(original, levelModelB.offset, SEEK_SET);
	fseek(converted, levelModelB.offset, SEEK_SET);

	for (int i = 0; i < levelModelB.number; ++i) {
		// Level Model offset
		uint32_t levelModelOffset = readInt(original);
		writeInt(converted, levelModelOffset);

		uint32_t savePos = ftell(original);

		// Level Model

		fseek(original, levelModelOffset, SEEK_SET);
		fseek(converted, levelModelOffset , SEEK_SET);

		// Level Model B Marker
		writeInt(converted, readInt(original));
		writeInt(converted, readInt(original));

		uint32_t levelModelAOffset = readInt(original);
		writeInt(converted, levelModelAOffset);
		//copyAscii(original, converted, modelNameOffset);

		fseek(original, savePos, SEEK_SET);
		fseek(converted, savePos, SEEK_SET);


	}
	
	// Collision Fields

	// Level Model Bs

	fseek(original, collisionFields.offset, SEEK_SET);
	fseek(converted, collisionFields.offset, SEEK_SET);

	for (int i = 0; i < collisionFields.number; ++i) {
		//if (i == 1) { return; }
		// Center Animation Position
		writeInt(converted, readInt(original));
		writeInt(converted, readInt(original));
		writeInt(converted, readInt(original));

		// Initial Animation Rotation
		writeShort(converted, readShort(original));
		writeShort(converted, readShort(original));
		writeShort(converted, readShort(original));
		// Padding
		writeShort(converted, readShort(original));

		// Offset to animation data
		uint32_t animationOffset = readInt(original);
		writeInt(converted, animationOffset);
		if (animationOffset != 0) {
			// Write animation data
			copyAnimation(original, converted, animationOffset, readInt, writeInt);
		}

		// Dead Zone (May include model name reference) (0x10)
		for (int j = 0; j < 0xC / 4; ++j) {
			writeInt(converted, readInt(original));
		}

		// Triangle Collision Data
		uint32_t triangleDataOffset = readInt(original);
		writeInt(converted, triangleDataOffset);

		// Collision Grid Pointers
		uint32_t collisionGridPointerOffsets = readInt(original);
		writeInt(converted, collisionGridPointerOffsets);

		// Collision Grid X/Z Start value
		writeInt(converted, readInt(original));
		writeInt(converted, readInt(original));

		// Collision Grid X/Z Step value
		writeInt(converted, readInt(original));
		writeInt(converted, readInt(original));

		// 0x10 0x10
		writeInt(converted, readInt(original));
		writeInt(converted, readInt(original));

		// Copy of header for what gets animated with the collision field (0x458)

		// Number of Goals
		writeInt(converted, readInt(original));

		// Offset to Goals
		writeInt(converted, readInt(original));

		// Number of Bumpers
		writeInt(converted, readInt(original));

		// Offset to Bumpers
		writeInt(converted, readInt(original));

		// Number of Jamabars
		writeInt(converted, readInt(original));

		// Offset to Jamabars
		writeInt(converted, readInt(original));

		// Number of Bananas
		writeInt(converted, readInt(original));
		
		// Offset to Bananas
		writeInt(converted, readInt(original));

		// Unknown/Zero (0x30)
		for (int j = 0; j < 0x30 / 4; ++j) {
			writeInt(converted, readInt(original));
		}

		// Number of Level Models
		writeInt(converted, readInt(original));

		// Offset to Level Models
		writeInt(converted, readInt(original));

		// Mystery Six
		uint32_t mystery6Offset = 0;// readInt(original);
		//writeInt(converted, mystery6Offset);

		// Next part of header copy (0x18)
		for (int j = 0; j < 0x18 / 4; ++j) {
			writeInt(converted, readInt(original));
		}

		// Mystery Five
		uint32_t myserty5Offset = readInt(original);
		//printf("TELL: %d\n", ftell(original));
		//return;
		writeInt(converted, myserty5Offset);

		// Rest of header copy (0x3D0)
		for (int j = 0; j < 0x3E4 / 4; ++j) {
			writeInt(converted, readInt(original));
		}

		uint32_t savePos = ftell(original);

		// Copy Mystery 5
		if (myserty5Offset != 0) {
			fseek(original, myserty5Offset, SEEK_SET);
			fseek(converted, myserty5Offset, SEEK_SET);

			// Zero/Unknown
			writeInt(converted, readInt(original));

			// 3 Floats
			writeInt(converted, readInt(original));
			writeInt(converted, readInt(original));
			writeInt(converted, readInt(original));
		}

		if (mystery6Offset != 0) {
			fseek(original, mystery6Offset, SEEK_SET);
			fseek(converted, mystery6Offset, SEEK_SET);
			// 4 Floats
			writeInt(converted, readInt(original));
			writeInt(converted, readInt(original));
			writeInt(converted, readInt(original));
			writeInt(converted, readInt(original));

		}

		// Handle the collision grid before the collision triangles to find which offset they end at
		fseek(original, collisionGridPointerOffsets, SEEK_SET);
		fseek(converted, collisionGridPointerOffsets, SEEK_SET);

		uint32_t triangleCollisionEnd = readInt(original);
		fseek(original, -4, SEEK_CUR);

		do {
			uint32_t collisionGridOffset = readInt(original);

			// A collision Grid offset won't be this big, but an ascii value will be which is next in the file order
			if (collisionGridOffset >= 0x01000000) {
				break;
			}

			writeInt(converted, collisionGridOffset);

			// 0 == No triangles here
			if (collisionGridOffset != 0) {
				uint32_t savePos2 = ftell(original);

				// Go to the collision grid
				fseek(original, collisionGridOffset, SEEK_SET);
				fseek(converted, collisionGridOffset, SEEK_SET);

				// Copy all the collision grid numbers for this particular grid
				uint16_t collisionTriangleNumber;
				do {
					collisionTriangleNumber = readShort(original);
					writeShort(converted, collisionTriangleNumber);
				} while (collisionTriangleNumber != 0xFFFF); // End of grid marker

				fseek(original, savePos2, SEEK_SET);
				fseek(converted, savePos2, SEEK_SET);
			}

		} while (1);

		// Handle Collision Triangles
		fseek(original, triangleDataOffset, SEEK_SET);
		fseek(converted, triangleDataOffset, SEEK_SET);

		while ((uint32_t) ftell(original) < triangleCollisionEnd) {
			// X1 Position
			writeInt(converted, readInt(original));
			writeInt(converted, readInt(original));
			writeInt(converted, readInt(original));

			// Normal (X, Y, Z)
			writeInt(converted, readInt(original));
			writeInt(converted, readInt(original));
			writeInt(converted, readInt(original));

			// Rotation from XY plane (X, Y, Z)
			writeShort(converted, readShort(original));
			writeShort(converted, readShort(original));
			writeShort(converted, readShort(original));
			// Padding
			writeShort(converted, readShort(original));

			// Distance to other vertecies (DX2X1, DY2Y1, DX3X1, DY3Y1)
			writeInt(converted, readInt(original));
			writeInt(converted, readInt(original));
			writeInt(converted, readInt(original));
			writeInt(converted, readInt(original));

			// Tangent (X, Y)
			writeInt(converted, readInt(original));
			writeInt(converted, readInt(original));

			// Bitangent (X, Y)
			writeInt(converted, readInt(original));
			writeInt(converted, readInt(original));
		}

		fseek(original, savePos, SEEK_SET);
		fseek(converted, savePos, SEEK_SET);
	}
	
	/// Time for some mysteries...

	// stageAnimations
	if (stageAnimations.offset != 0) {
		copyAnimation(original, converted, stageAnimations.offset, readInt, writeInt);
	}

	// Mystery Two
	if (mysteryTwo.offset != 0) {
		fseek(original, mysteryTwo.offset, SEEK_SET);
		fseek(converted, mysteryTwo.offset, SEEK_SET);
		// Dead Zone (Endianess doesn't matter here) (0x24)
		for (int i = 0; i < 0x24 / 0x4; ++i) {
			writeNormalInt(converted, readInt(original));
		}
	}

	// Mystery Three
	if (mysteryThree.offset != 0) {
		fseek(original, mysteryThree.offset, SEEK_SET);
		fseek(converted, mysteryThree.offset, SEEK_SET);
		// Dead Zone (0xC)
		for (int i = 0; i < 0xC / 0x4; ++i) {
			writeInt(converted, readInt(original));
		}

		// Zero/Padding
		writeShort(converted, readShort(original));

		// Some marker?
		writeShort(converted, readShort(original));

		// Dead Zone (0x14)
		for (int i = 0; i < 0x14 / 0x4; ++i) {
			writeInt(converted, readInt(original));
		}
	}

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
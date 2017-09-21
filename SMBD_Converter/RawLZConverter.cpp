#include "RawLZConverter.h"

inline void copyAsciiAligned(FILE *input, FILE *output, uint32_t offset) {
	uint32_t savePos = ftell(input);
	fseek(input, offset, SEEK_SET);
	fseek(output, offset, SEEK_SET);

	// Copy characters until at the end of file
	// or at the end of the ascii and 4 byte aligned
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

inline void copyAnimation(FILE *input, FILE *output, uint32_t offset, uint32_t(*readInt)(FILE*), void(*writeInt)(FILE*, uint32_t), int numKeys);

inline void copyBackgroundAnimation(FILE *input, FILE *output, uint32_t offset, uint32_t(*readInt)(FILE*), void(*writeInt)(FILE*, uint32_t), int numKeys);

inline void copyEffects(FILE *input, FILE *output, uint32_t offset, uint32_t(*readInt)(FILE*), void(*writeInt)(FILE*, uint32_t));

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
	Item fogAnimation;
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

	// Unknown/Null (0xAC, length = 0x4)
	for (int i = 0; i < 0x4; i += 4) {
		writeInt(converted, readInt(original));
	}

	// Fog Animation (0xB0, length = 0x4)
	fogAnimation.offset = readInt(original);
	writeInt(converted, fogAnimation.offset);

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

#pragma region Goals
	fseek(original, goals.offset, SEEK_SET);
	fseek(converted, goals.offset, SEEK_SET);

	// Loop through goals
	for (int i = 0; i < goals.number; i++) {
		// Position (0x0, length = 0xC)
		writeInt(converted, readInt(original)); // X
		writeInt(converted, readInt(original)); // Y
		writeInt(converted, readInt(original)); // Z

		// Rotation (0xC, length = 0x6)
		writeShort(converted, readShort(original)); // X
		writeShort(converted, readShort(original)); // Y
		writeShort(converted, readShort(original)); // Z

		// Goal Type (0x12, length = 2, marker)
		writeNormalShort(converted, readShort(original)); // Goal Type
	}
#pragma endregion Goals

#pragma region Bumpers
	fseek(original, bumpers.offset, SEEK_SET);
	fseek(converted, bumpers.offset, SEEK_SET);

	// Loop through bumpers
	for (int i = 0; i < bumpers.number; i++) {
		// Position (0x0, length = 0xC)
		writeInt(converted, readInt(original)); // X
		writeInt(converted, readInt(original)); // Y
		writeInt(converted, readInt(original)); // Z

		// Rotation (0xC, length = 0x8)
		writeShort(converted, readShort(original)); // X
		writeShort(converted, readShort(original)); // Y
		writeShort(converted, readShort(original)); // Z
		writeShort(converted, readShort(original)); // Padding/Null

		// Scale (0x14, length = 0xC)
		writeInt(converted, readInt(original)); // X
		writeInt(converted, readInt(original)); // Y
		writeInt(converted, readInt(original)); // Z
	}
#pragma endregion Bumpers

#pragma region Jamabars
	fseek(original, jamabars.offset, SEEK_SET);
	fseek(converted, jamabars.offset, SEEK_SET);

	// Loop through jamabars
	for (int i = 0; i < jamabars.number; i++) {
		// Position (0x0, length = 0xC)
		writeInt(converted, readInt(original)); // X
		writeInt(converted, readInt(original)); // Y
		writeInt(converted, readInt(original)); // Z

		// Rotation (0xC, length = 0x8)
		writeShort(converted, readShort(original)); // X
		writeShort(converted, readShort(original)); // Y
		writeShort(converted, readShort(original)); // Z
		writeShort(converted, readShort(original)); // Padding/Null

		// Scale (0x14, length = 0xC)
		writeInt(converted, readInt(original)); // X
		writeInt(converted, readInt(original)); // Y
		writeInt(converted, readInt(original)); // Z
	}
#pragma endregion Jamabars

#pragma region Bananas
	fseek(original, bananas.offset, SEEK_SET);
	fseek(converted, bananas.offset, SEEK_SET);

	// Loop through bananas
	for (int i = 0; i < bananas.number; i++) {
		// Position (0x0, length = 0xC)
		writeInt(converted, readInt(original)); // X
		writeInt(converted, readInt(original)); // Y
		writeInt(converted, readInt(original)); // Z

		// Banana Type (0xC, length = 4)
		writeInt(converted, readInt(original));
	}
#pragma endregion Bananas

#pragma region Fallout_Volume
	fseek(original, falloutVolumes.offset, SEEK_SET);
	fseek(converted, falloutVolumes.offset, SEEK_SET);

	// Loop through falloutVolumes
	for (int i = 0; i < falloutVolumes.number; i++) {
		// Center Position (0x0, length = 0xC)
		writeInt(converted, readInt(original)); // X
		writeInt(converted, readInt(original)); // Y
		writeInt(converted, readInt(original)); // Z

		// Size (0x14, length = 0xC)
		writeInt(converted, readInt(original)); // X
		writeInt(converted, readInt(original)); // Y
		writeInt(converted, readInt(original)); // Z

		// Rotation (0xC, length = 0x8)
		writeShort(converted, readShort(original)); // X
		writeShort(converted, readShort(original)); // Y
		writeShort(converted, readShort(original)); // Z
		writeShort(converted, readShort(original)); // Padding/Null

		
	}
#pragma endregion Fallout_Volume

#pragma region Switch
	fseek(original, switches.offset, SEEK_SET);
	fseek(converted, switches.offset, SEEK_SET);

	// Loop through switches
	for (int i = 0; i < switches.number; i++) {
		// Position (0x0, length = 0xC)
		writeInt(converted, readInt(original)); // X
		writeInt(converted, readInt(original)); // Y
		writeInt(converted, readInt(original)); // Z

		// Rotation (0xC, length = 0x6)
		writeShort(converted, readShort(original)); // X
		writeShort(converted, readShort(original)); // Y
		writeShort(converted, readShort(original)); // Z

		// Switch type (0x12, length = 0x2)
		writeShort(converted, readShort(original)); // Switch Type

		// Animation Group IDs affected (0x14, length = 0x2)
		writeShort(converted, readShort(original)); // Group IDs

		// Padding/Null (0x16, length = 0x2)
		writeShort(converted, readShort(original));
	}
#pragma endregion Switch

#pragma region Wormhole
	fseek(original, wormholes.offset, SEEK_SET);
	fseek(converted, wormholes.offset, SEEK_SET);

	// Loop through wormholes
	for (int i = 0; i < wormholes.number; i++) {
		// Position (0x0, length = 0xC)
		writeInt(converted, readInt(original)); // X
		writeInt(converted, readInt(original)); // Y
		writeInt(converted, readInt(original)); // Z

		// Rotation (0xC, length = 0x8)
		writeShort(converted, readShort(original)); // X
		writeShort(converted, readShort(original)); // Y
		writeShort(converted, readShort(original)); // Z
		writeShort(converted, readShort(original)); // Padding/Null

		// Offset to destination wormwhole (0x14, length = 0x4)
		writeInt(converted, readInt(original));
	}
#pragma endregion Wormhole

#pragma region Fog
	fseek(original, fog.offset, SEEK_SET);
	fseek(converted, fog.offset, SEEK_SET);

	// Fog Id (0x0, length = 0x4)
	writeInt(converted, readInt(original));
	
	// Distance (0x4, length = 0x8)
	writeInt(converted, readInt(original)); // Start Distance
	writeInt(converted, readInt(original)); // End Distance

	// Color (0xC, length = 0xC)
	writeInt(converted, readInt(original)); // Red
	writeInt(converted, readInt(original)); // Green
	writeInt(converted, readInt(original)); // Blue

	// Unknown/Null (0x18, length = 0xC)
	for (int i = 0; i < 0xC; i += 4) {
		writeInt(converted, readInt(original));
	}

#pragma endregion Fog

#pragma region Fog_Animation
	fseek(original, fogAnimation.offset, SEEK_SET);
	fseek(converted, fogAnimation.offset, SEEK_SET);

	// Animation (0x0, length = 0x40)
	copyAnimation(original, converted, fogAnimation.offset, readInt, writeInt, 5);
#pragma endregion Fog_Animation

#pragma region Mystery_Three
	fseek(original, mysteryThree.offset, SEEK_SET);
	fseek(converted, mysteryThree.offset, SEEK_SET);

	// Position? (0x0, length = 0xC)
	writeInt(converted, readInt(original));
	writeInt(converted, readInt(original));
	writeInt(converted, readInt(original));

	// Padding/Null (0xC, length = 0x2)
	writeShort(converted, readShort(original));

	// Some Marker (0xE, length = 0x2);
	writeNormalShort(converted, readShort(original));

	// Unknown/Null (0x10, length = 0x14)
	for (int i = 0; i < 0x14; i += 4) {
		writeInt(converted, readInt(original));
	}

#pragma endregion Mystery_Three

#pragma region Level_Model_A
	fseek(original, levelModelA.offset, SEEK_SET);
	fseek(converted, levelModelA.offset, SEEK_SET);

	// Level Model A Symbol (0x0, length = 0x8)
	writeInt(converted, readInt(original)); // 0
	writeInt(converted, readInt(original)); // 1

	
	{
		// Offset to Level Model A (Actual) (0x0, length = 0x4)
		uint32_t levelModelAOffset = readInt(original);
		writeInt(converted, levelModelAOffset);

		// Seek to the actual Level Model A
		fseek(original, levelModelAOffset, SEEK_SET);
		fseek(converted, levelModelAOffset, SEEK_SET);

		// Null (0x0, length = 4)
		writeInt(converted, readInt(original));
		
		// Model Name offset (0x4, length = 4)
		uint32_t modelNameOffset = readInt(original);
		writeInt(converted, modelNameOffset);

		// Null/Padding (0x8, length = 8)
		writeInt(converted, readInt(original));
		writeInt(converted, readInt(original));

		// Copy model name
		copyAsciiAligned(original, converted, modelNameOffset);
	}

#pragma endregion Level_Model_A

#pragma region Level_Model_B
	fseek(original, levelModelB.offset, SEEK_SET);
	fseek(converted, levelModelB.offset, SEEK_SET);

	// Offset to Level Model A (0x0, length = 0x4)
	writeInt(converted, readInt(original)); // Not saved because the Level Model A readion covers it

#pragma endregion Level_Model_B

#pragma region Reflective_Level_Model
	fseek(original, reflectiveModels.offset, SEEK_SET);
	fseek(converted, reflectiveModels.offset, SEEK_SET);

	{
		// Model Name offset (0x0, length = 0x4)
		uint32_t modelNameOffset = readInt(original);
		writeInt(converted, modelNameOffset);

		// Null/Padding (0x4, length = 0x8)
		writeInt(converted, readInt(original));
		writeInt(converted, readInt(original));

		// Copy model name
		copyAsciiAligned(original, converted, modelNameOffset);
	}
#pragma endregion Reflective_Level_Model

#pragma region Background_Model
	fseek(original, backgroundModels.offset, SEEK_SET);
	fseek(converted, backgroundModels.offset, SEEK_SET);

	// Background Model Symbol (0x0, length = 0x4)
	writeInt(converted, readInt(original));

	{
		// Model Name Offset (0x4, length = 0x4)
		uint32_t modelNameOffset = readInt(original);
		writeInt(converted, modelNameOffset);

		// Null/Padding (0x8, length = 0x4)
		writeInt(converted, readInt(original));

		// Position (0xC, length = 0xC)
		writeInt(converted, readInt(original)); // X
		writeInt(converted, readInt(original)); // Y
		writeInt(converted, readInt(original)); // Z

		// Rotation (0x18, length = 0x8)
		writeShort(converted, readShort(original)); // X
		writeShort(converted, readShort(original)); // Y
		writeShort(converted, readShort(original)); // Z
		writeShort(converted, readShort(original)); // Padding

		// Scale (0x20, length = 0xC)
		writeInt(converted, readInt(original)); // X
		writeInt(converted, readInt(original)); // Y
		writeInt(converted, readInt(original)); // Z

		// Animation One (0x2C, length = 0x4)
		uint32_t backgroundAnimationOffsetOne = readInt(original);
		writeInt(converted, backgroundAnimationOffsetOne);

		// Animation Two (0x30, length = 0x4)
		uint32_t backgroundAnimationOffsetTwo = readInt(original);
		writeInt(converted, backgroundAnimationOffsetTwo);

		// Effects (0x34, length = 0x4)
		uint32_t effectsOffset = readInt(original);
		writeInt(converted, effectsOffset);

		// Copy model name
		copyAsciiAligned(original, converted, modelNameOffset);

		// Copy animation one
		copyBackgroundAnimation(original, converted, backgroundAnimationOffsetOne, readInt, writeInt, 6); // TODO

		// Copy animation two
		copyBackgroundAnimation(original, converted, backgroundAnimationOffsetTwo, readInt, writeInt, 6); // TODO

		// Copy effects
		copyEffects(original, converted, effectsOffset, readInt, writeInt);
	}
#pragma endregion Background_Model


	fclose(original);
	fclose(converted);
}

inline void copyAnimation(FILE *input, FILE *output, uint32_t offset, uint32_t(*readInt)(FILE*), void(*writeInt)(FILE*, uint32_t), int numKeys) {
	uint32_t savePos = ftell(input);

	// Sanity Check (or moreso a sanity warning...)
	if (numKeys > 8) {
		printf("Warning: %d animation keys is more than the expected allowed number\n", numKeys);
	}

	fseek(input, offset, SEEK_SET);
	fseek(output, offset, SEEK_SET);

	Item *keyList = (Item *)malloc(sizeof(Item) * numKeys);

	// Gather keys (0x0, length = 0x8 * numKeys
	for (int i = 0; i < numKeys; i++) {
		keyList[i].number = readInt(input);
		keyList[i].offset = readInt(input);
		writeInt(output, keyList[i].number);
		writeInt(output, keyList[i].offset);
	}

	// Unknown/Null (0x8 * numKeys, length = 0x40 - (0x8 * numKeys)
	for (int i = numKeys * 0x8; i < 0x40; i += 4) {
		writeInt(output, readInt(input));
	}

	for (int i = 0; i < numKeys; i++) {
		if (keyList[i].number > 0 && keyList[i].offset != 0) {
			fseek(input, keyList[i].offset, SEEK_SET);
			fseek(output, keyList[i].offset, SEEK_SET);

			// Easing (0x0, length = 0x4)
			writeInt(output, readInt(input));

			// Time (0x4, length = 0x4)
			writeInt(output, readInt(input));

			// Value (0x8, length = 0x4)
			writeInt(output, readInt(input));

			// Unknown/Null (0xC, length = 0x8)
			for (int j = 0; j < 0x8; j += 4) {
				writeInt(output, readInt(input));
			}
		}
	}
	
	fseek(input, savePos, SEEK_SET);
	fseek(output, savePos, SEEK_SET);
	free(keyList);
}

inline void copyBackgroundAnimation(FILE *input, FILE *output, uint32_t offset, uint32_t(*readInt)(FILE*), void(*writeInt)(FILE*, uint32_t), int numKeys) {
	uint32_t savePos = ftell(input);
	fseek(input, offset, SEEK_SET);
	fseek(output, offset, SEEK_SET);

	// Unknown/Null (0x0, length = 0x4)
	writeInt(output, readInt(input));

	// Animation Loop point (0x4, length = 0x4)
	writeInt(output, readInt(input));

	// Unknown/Null (0x8, length = 0xC)
	for (int i = 0; i < 0xC; i += 4) {
		writeInt(output, readInt(input));
	}

	// Animation (0x14, length = 0x40)
	copyAnimation(input, output, ftell(input), readInt, writeInt, numKeys);

	fseek(input, savePos, SEEK_SET);
	fseek(output, savePos, SEEK_SET);
}

inline void copyEffects(FILE *input, FILE *output, uint32_t offset, uint32_t(*readInt)(FILE*), void(*writeInt)(FILE*, uint32_t)) {
	uint32_t savePos = ftell(input);
	fseek(input, offset, SEEK_SET);
	fseek(output, offset, SEEK_SET);

	// TODO

	fseek(input, savePos, SEEK_SET);
	fseek(output, savePos, SEEK_SET);
}
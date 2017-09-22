#include "RawLZConverter.h"
// TODO Read put Item readings into separate methods
static void copyAsciiAligned(FILE *input, FILE *output, uint32_t offset) {
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

typedef struct {
	int number;
	int offset;
}Item;

static void copyAnimation(FILE *input, FILE *output, uint32_t offset, int numKeys);

static void copyBackgroundAnimation(FILE *input, FILE *output, uint32_t offset, int numKeys);

static void copyEffects(FILE *input, FILE *output, uint32_t offset);

static Item readItem(FILE *input, FILE *output);

static void copyStartPositions(FILE *original, FILE *converted, Item item);
static void copyFalloutY(FILE *original, FILE *converted, Item item);
static void copyGoals(FILE *original, FILE *converted, Item item);
static void copyBumpers(FILE *original, FILE *converted, Item item);
static void copyJamabars(FILE *original, FILE *converted, Item item);
static void copyBananas(FILE *original, FILE *converted, Item item);
static void copyFalloutVolumes(FILE *original, FILE *converted, Item item);
static void copyBackgroundModels(FILE *original, FILE *converted, Item item);
static void copyReflectiveModels(FILE *original, FILE *converted, Item item);
static void copyLevelModelAs(FILE *original, FILE *converted, Item item);
static void copyLevelModelBs(FILE *original, FILE *converted, Item item);
static void copySwitches(FILE *original, FILE *converted, Item item);
static void copyFogAnimation(FILE *original, FILE *converted, Item item);
static void copyWormholes(FILE *original, FILE *converted, Item item);
static void copyFog(FILE *original, FILE *converted, Item item);
static void copyMysteryThrees(FILE *original, FILE *converted, Item item);
static void copyCollisionFields(FILE *original, FILE *converted, Item item);


// Using function pointers to read/write values keeps things game agnostic (can convert from SMBD to SMB2 or SMB2 to SMBD)
static uint32_t(*readInt)(FILE*);
static void(*writeInt)(FILE*, uint32_t);
static void(*writeNormalInt)(FILE*, uint32_t);

static uint16_t(*readShort)(FILE*);
static void(*writeShort)(FILE*, uint16_t);
static void(*writeNormalShort)(FILE*, uint16_t);


void parseRawLZ(const char* filename) {
	int game = 0;


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
	collisionFields = readItem(original, converted);

	// Start positions (0x10, length = 0x4)
	startPositions = readItem(original, converted);

	// Fallout Y (0x14, length = 0x4)
	falloutY.offset = readInt(original);
	writeInt(converted, falloutY.offset);

	// The number of start positions is the fallout Y offset - startPosition offset / sizeof(startPosition)
	startPositions.number = (falloutY.offset - startPositions.offset) / 0x14;

	// Goals (0x18, length = 0x8)
	goals = readItem(original, converted);

	// Bumpers (0x20, length = 0x8)
	bumpers = readItem(original, converted);

	// Jamabars (0x28, length = 0x8)
	jamabars = readItem(original, converted);

	// Bananas (0x30, length = 0x8)
	bananas = readItem(original, converted);

	// Unknown/Null (0x38, length = 0x18)
	for (int i = 0; i < 0x18; i += 4) {
		writeInt(converted, readInt(original));
	}

	// Fallout Volumes (0x50, length = 0x8)
	falloutVolumes = readItem(original, converted);

	// Background Models (0x58, length = 0x8)
	backgroundModels = readItem(original, converted);

	// Unknown/Null (0x60, length = 0xC)
	for (int i = 0; i < 0xC; i += 4) {
		writeInt(converted, readInt(original));
	}

	// One (0x6C, length = 0x4)
	writeInt(converted, readInt(original));

	// Reflective Models (0x70, length = 0x8)
	reflectiveModels = readItem(original, converted);

	// Unknown/Null (0x78, length = 0x14)
	for (int i = 0; i < 0x14; i += 4) {
		writeInt(converted, readInt(original));
	}

	// Level Model A (0x8C, length = 0x8)
	levelModelA = readItem(original, converted);

	// Level Model B (0x94, length = 0x8)
	levelModelB = readItem(original, converted);

	// Unknown/Null (0x9C, length = 0x8)
	for (int i = 0; i < 0x8; i += 4) {
		writeInt(converted, readInt(original));
	}

	// Switches (0xA4, length = 0x8)
	switches = readItem(original, converted);

	// Unknown/Null (0xAC, length = 0x4)
	for (int i = 0; i < 0x4; i += 4) {
		writeInt(converted, readInt(original));
	}

	// Fog Animation (0xB0, length = 0x4)
	fogAnimation.offset = readInt(original);
	writeInt(converted, fogAnimation.offset);

	// Wormholes (0xB4, length = 0x8)
	wormholes = readItem(original, converted);

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

	// Start copying the rest of the file
	// Order shouldn't matter
	// By having everything in separate methods,
	// it avoids code duplication in the collision header
	copyStartPositions(original, converted, startPositions);

	copyFalloutY(original, converted, falloutY);

	copyGoals(original, converted, goals);

	copyBumpers(original, converted, bumpers);

	copyJamabars(original, converted, jamabars);

	copyBananas(original, converted, bananas);

	copyFalloutVolumes(original, converted, falloutVolumes);

	copySwitches(original, converted, switches);

	copyWormholes(original, converted, wormholes);

	copyFog(original, converted, fog);

	copyFogAnimation(original, converted, fogAnimation);

	copyMysteryThrees(original, converted, mysteryThree);

	copyLevelModelAs(original, converted, levelModelA);

	copyLevelModelBs(original, converted, levelModelB);

	copyReflectiveModels(original, converted, reflectiveModels);

	copyBackgroundModels(original, converted, backgroundModels);

	copyCollisionFields(original, converted, collisionFields);


	fclose(original);
	fclose(converted);
}

static void copyAnimation(FILE *input, FILE *output, uint32_t offset, int numKeys) {
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

static void copyBackgroundAnimation(FILE *input, FILE *output, uint32_t offset, int numKeys) {
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
	copyAnimation(input, output, ftell(input), numKeys);

	fseek(input, savePos, SEEK_SET);
	fseek(output, savePos, SEEK_SET);
}

static void copyEffects(FILE *input, FILE *output, uint32_t offset) {
	uint32_t savePos = ftell(input);
	fseek(input, offset, SEEK_SET);
	fseek(output, offset, SEEK_SET);

	// TODO

	fseek(input, savePos, SEEK_SET);
	fseek(output, savePos, SEEK_SET);
}

static Item readItem(FILE *input, FILE *output) {
	Item newItem;

	newItem.number = readInt(input);
	writeInt(output, newItem.number);
	newItem.offset = readInt(input);
	writeInt(output, newItem.offset);

	return newItem;
}

static void copyStartPositions(FILE *original, FILE *converted, Item item) {
	fseek(original, item.offset, SEEK_SET);
	fseek(converted, item.offset, SEEK_SET);

	// Loop through start positions
	for (int i = 0; i < item.number; i++) {
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
}
static void copyFalloutY(FILE *original, FILE *converted, Item item) {
	fseek(original, item.offset, SEEK_SET);
	fseek(converted, item.offset, SEEK_SET);

	// Fallout Y (0x0, length = 0x4)
	writeInt(converted, readInt(original));
}
static void copyGoals(FILE *original, FILE *converted, Item item) {
	fseek(original, item.offset, SEEK_SET);
	fseek(converted, item.offset, SEEK_SET);

	// Loop through goals
	for (int i = 0; i < item.number; i++) {
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
}
static void copyBumpers(FILE *original, FILE *converted, Item item) {
	fseek(original, item.offset, SEEK_SET);
	fseek(converted, item.offset, SEEK_SET);

	// Loop through bumpers
	for (int i = 0; i < item.number; i++) {
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
}
static void copyJamabars(FILE *original, FILE *converted, Item item) {
	fseek(original, item.offset, SEEK_SET);
	fseek(converted, item.offset, SEEK_SET);

	// Loop through jamabars
	for (int i = 0; i < item.number; i++) {
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
}
static void copyBananas(FILE *original, FILE *converted, Item item) {
	fseek(original, item.offset, SEEK_SET);
	fseek(converted, item.offset, SEEK_SET);

	// Loop through bananas
	for (int i = 0; i < item.number; i++) {
		// Position (0x0, length = 0xC)
		writeInt(converted, readInt(original)); // X
		writeInt(converted, readInt(original)); // Y
		writeInt(converted, readInt(original)); // Z

		// Banana Type (0xC, length = 4)
		writeInt(converted, readInt(original));
	}
}
static void copyFalloutVolumes(FILE *original, FILE *converted, Item item) {
	fseek(original, item.offset, SEEK_SET);
	fseek(converted, item.offset, SEEK_SET);

	// Loop through falloutVolumes
	for (int i = 0; i < item.number; i++) {
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
}
static void copyBackgroundModels(FILE *original, FILE *converted, Item item) {
	fseek(original, item.offset, SEEK_SET);
	fseek(converted, item.offset, SEEK_SET);

	for (int i = 0; i < item.number; i++) {
		// Background Model Symbol (0x0, length = 0x4)
		writeInt(converted, readInt(original));

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
		copyBackgroundAnimation(original, converted, backgroundAnimationOffsetOne, 6); // TODO

		 // Copy animation two
		copyBackgroundAnimation(original, converted, backgroundAnimationOffsetTwo, 6); // TODO

		// Copy effects
		copyEffects(original, converted, effectsOffset);
	}
}
static void copyReflectiveModels(FILE *original, FILE *converted, Item item) {
	fseek(original, item.offset, SEEK_SET);
	fseek(converted, item.offset, SEEK_SET);

	for (int i = 0; i < item.number; i++) {
		// Model Name offset (0x0, length = 0x4)
		uint32_t modelNameOffset = readInt(original);
		writeInt(converted, modelNameOffset);

		// Null/Padding (0x4, length = 0x8)
		writeInt(converted, readInt(original));
		writeInt(converted, readInt(original));

		// Copy model name
		copyAsciiAligned(original, converted, modelNameOffset);
	}
}
static void copyLevelModelAs(FILE *original, FILE *converted, Item item) {
	fseek(original, item.offset, SEEK_SET);
	fseek(converted, item.offset, SEEK_SET);

	for (int i = 0; i < item.number; i++) {
		// Level Model A Symbol (0x0, length = 0x8)
		writeInt(converted, readInt(original)); // 0
		writeInt(converted, readInt(original)); // 1

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
}
static void copyLevelModelBs(FILE *original, FILE *converted, Item item) {
	fseek(original, item.offset, SEEK_SET);
	fseek(converted, item.offset, SEEK_SET);

	for (int i = 0; i < item.number; i++) {
		// Offset to Level Model A (0x0, length = 0x4)
		writeInt(converted, readInt(original)); // Not saved because the Level Model A readion covers it
	}
}
static void copySwitches(FILE *original, FILE *converted, Item item) {
	fseek(original, item.offset, SEEK_SET);
	fseek(converted, item.offset, SEEK_SET);

	// Loop through switches
	for (int i = 0; i < item.number; i++) {
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
}
static void copyFogAnimation(FILE *original, FILE *converted, Item item) {
	fseek(original, item.offset, SEEK_SET);
	fseek(converted, item.offset, SEEK_SET);

	// Animation (0x0, length = 0x40)
	copyAnimation(original, converted, item.offset, 5);
}
static void copyWormholes(FILE *original, FILE *converted, Item item) {
	fseek(original, item.offset, SEEK_SET);
	fseek(converted, item.offset, SEEK_SET);

	// Loop through wormholes
	for (int i = 0; i < item.number; i++) {
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
}
static void copyFog(FILE *original, FILE *converted, Item item) {
	fseek(original, item.offset, SEEK_SET);
	fseek(converted, item.offset, SEEK_SET);

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
}
static void copyMysteryThrees(FILE *original, FILE *converted, Item item) {
	fseek(original, item.offset, SEEK_SET);
	fseek(converted, item.offset, SEEK_SET);

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
}
static void copyCollisionFields(FILE *original, FILE *converted, Item item) {
	fseek(original, item.offset, SEEK_SET);
	fseek(converted, item.offset, SEEK_SET);

	for (int i = 0; i < item.number; i++) {
		// Center of Rotation (0x0, length = 0xC)
		writeInt(converted, readInt(original)); // X
		writeInt(converted, readInt(original)); // Y
		writeInt(converted, readInt(original)); // Z

		// Initial Rotation (0xC, length = 0x6)
		writeShort(converted, readShort(original)); // X
		writeShort(converted, readShort(original)); // Y
		writeShort(converted, readShort(original)); // Z

		// Animatione loop type/seesaw (0x12, length = 0x2)
		writeShort(converted, readShort(original));

		// Animation Offset (0x14, length = 0x4)
		uint32_t animationOffset = readInt(original);
		writeInt(converted, animationOffset);

		// Unknown/Null (0x18, length = 0xC)
		for (int j = 0; j < 0xC; j += 4) {
			writeInt(converted, readInt(original));
		}

		// Collision triangle information (0x24, length = 0x20)
		uint32_t collisionTriangleListOffset = readInt(original);
		writeInt(converted, collisionTriangleListOffset);

		uint32_t collisionTriangleGridOffset = readInt(original);
		writeInt(converted, collisionTriangleGridOffset);
		// TODO
		uint32_t gridStartXInt = readInt(original);
		writeInt(converted, gridStartXInt);

		uint32_t gridStartZInt = readInt(original);
		writeInt(converted, gridStartXInt);

		uint32_t gridEndXInt = readInt(original);
		writeInt(converted, gridStartXInt);

		uint32_t gridEndZInt = readInt(original);
		writeInt(converted, gridStartXInt);

		// Goals (0x44, length = 0x8)
		writeInt(converted, readInt(original));
		writeInt(converted, readInt(original));

	}
}
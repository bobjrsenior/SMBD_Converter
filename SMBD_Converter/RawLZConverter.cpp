#include "RawLZConverter.h"

static void copyAsciiAligned(FILE *input, FILE *output, uint32_t offset) {
	if (offset == 0) return;
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


static void copyAnimation(FILE *input, FILE *output, uint32_t offset, int numKeys, int minLength);

static void copyBackgroundAnimation(FILE *input, FILE *output, uint32_t offset, int numKeys, int minLength);

static void copyEffects(FILE *input, FILE *output, uint32_t offset);
static void copyEffectOne(FILE *input, FILE *output, Item item);
static void copyEffectTwo(FILE *input, FILE *output, Item item);
static void copyTextureScroll(FILE *input, FILE *output, uint32_t offset);

static uint32_t copyCollisionTriangleGrid(FILE *input, FILE *output, uint32_t offset, uint32_t xStepCount, uint32_t zStepCount);

static void copyCollisionTriangles(FILE *input, FILE *output, uint32_t offset, uint32_t maxIndex);

static Item readItem(FILE *input, FILE *output);

static void copyStartPositions(FILE *original, FILE *converted, Item item);
static void copyFalloutY(FILE *original, FILE *converted, Item item);
static void copyGoals(FILE *original, FILE *converted, Item item);
static void copyBumpers(FILE *original, FILE *converted, Item item);
static void copyJamabars(FILE *original, FILE *converted, Item item);
static void copyBananas(FILE *original, FILE *converted, Item item);
static void copyConeCollisions(FILE *original, FILE *converted, Item item);
static void copyCylinderCollisions(FILE *original, FILE *converted, Item item);
static void copySphereCollisions(FILE *original, FILE *converted, Item item);
static void copyFalloutVolumes(FILE *original, FILE *converted, Item item);
static void copyBackgroundModels(FILE *original, FILE *converted, Item item);
static void copyMysteryEights(FILE *original, FILE *converted, Item item);
static void copyMysteryTwelves(FILE *original, FILE *converted, Item item);
static void copyReflectiveModels(FILE *original, FILE *converted, Item item);
static void copyModelDuplicates(FILE *original, FILE *converted, Item item);
static void copyLevelModelAs(FILE *original, FILE *converted, Item item);
static void copyLevelModelBs(FILE *original, FILE *converted, Item item);
static void copySwitches(FILE *original, FILE *converted, Item item);
static void copyFogAnimation(FILE *original, FILE *converted, Item item);
static void copyWormholes(FILE *original, FILE *converted, Item item);
static void copyFog(FILE *original, FILE *converted, Item item);
static void copyMysteryThrees(FILE *original, FILE *converted, Item item);
static void copyMysteryFive(FILE *original, FILE *converted, Item item);
static void copyMysteryElevens(FILE *original, FILE *converted, Item item);
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
		printf("Error opening file: %s\n", filename);
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
	Item coneCollisions;
	Item cylinderCollisions;
	Item sphereCollisions;
	Item falloutVolumes;
	Item backgroundModels;
	Item mysteryEight;
	Item mysteryTwelve;
	Item reflectiveModels;
	Item modelDuplicates;
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
	startPositions.offset = readInt(original);
	writeInt(converted, startPositions.offset);

	// Fallout Y (0x14, length = 0x4)
	falloutY.offset = readInt(original);
	writeInt(converted, falloutY.offset);

	// The number of start positions is the fallout Y offset - startPosition offset / sizeof(startPosition)
	startPositions.number = (falloutY.offset - 0x89C) / 0x14;

	// Goals (0x18, length = 0x8)
	goals = readItem(original, converted);

	// Bumpers (0x20, length = 0x8)
	bumpers = readItem(original, converted);

	// Jamabars (0x28, length = 0x8)
	jamabars = readItem(original, converted);

	// Bananas (0x30, length = 0x8)
	bananas = readItem(original, converted);

	// Cone Collisions (0x38, length = 0x8)
	coneCollisions = readItem(original, converted);

	// Sphere Collisions (0x40, length = 0x8)
	sphereCollisions = readItem(original, converted);

	// Cylinder Collisions (0x48, length = 0x8)
	cylinderCollisions = readItem(original, converted);

	// Fallout Volumes (0x50, length = 0x8)
	falloutVolumes = readItem(original, converted);

	// Background Models (0x58, length = 0x8)
	backgroundModels = readItem(original, converted);

	// Mystery Eight (0x60, length = 0x8)
	mysteryEight = readItem(original, converted);

	// Mystery Twelve (0x68, length = 0x4)
	mysteryTwelve.offset = readInt(original);
	writeInt(converted, mysteryTwelve.offset);

	// One, but not always one (0x6C, length = 0x4)
	writeInt(converted, readInt(original));

	// Reflective Models (0x70, length = 0x8)
	reflectiveModels = readItem(original, converted);

	// Unknown/Null (0x78, length = 0xC)
	for (int i = 0; i < 0xC; i += 4) {
		writeInt(converted, readInt(original));
	}
	
	// Model Duplicates (0x84, length = 0x8)
	modelDuplicates = readItem(original, converted);

	// Level Model A (0x8C, length = 0x8)
	levelModelA = readItem(original, converted);

	// Level Model B (0x94, length = 0x8)
	levelModelB = readItem(original, converted);

	// Unknown/Null (0x9C, length = 0xC)
	for (int i = 0; i < 0xC; i += 4) {
		writeInt(converted, readInt(original));
	}
	
	// Switches (0xA8, length = 0x8)
	switches = readItem(original, converted);

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

	copyConeCollisions(original, converted, coneCollisions);

	copyCylinderCollisions(original, converted, cylinderCollisions);

	copySphereCollisions(original, converted, sphereCollisions);

	copyFalloutVolumes(original, converted, falloutVolumes);
	
	copySwitches(original, converted, switches);

	copyWormholes(original, converted, wormholes);
	
	copyFog(original, converted, fog);
	
	copyFogAnimation(original, converted, fogAnimation);
	
	copyMysteryThrees(original, converted, mysteryThree);
	
	copyLevelModelAs(original, converted, levelModelA);
	
	copyLevelModelBs(original, converted, levelModelB);

	copyReflectiveModels(original, converted, reflectiveModels);

	copyModelDuplicates(original, converted, modelDuplicates);

	copyBackgroundModels(original, converted, backgroundModels);

	copyMysteryEights(original, converted, mysteryEight);

	copyMysteryTwelves(original, converted, mysteryTwelve);
	
	copyCollisionFields(original, converted, collisionFields);

	fclose(original);
	fclose(converted);
}

static void copyAnimation(FILE *input, FILE *output, uint32_t offset, int numKeys, int minLength) {
	if (offset == 0) return;
	uint32_t savePos = ftell(input);

	fseek(input, offset, SEEK_SET);
	fseek(output, offset, SEEK_SET);

	Item *keyList = (Item *)malloc(sizeof(Item) * numKeys);

	// Gather keys (0x0, length = 0x8 * numKeys
	for (int i = 0; i < numKeys; i++) {
		keyList[i].number = readInt(input);
		keyList[i].offset = readInt(input);
		if (keyList[i].offset == 0x3D1C) {
			//puts("Found it");
		}
		writeInt(output, keyList[i].number);
		writeInt(output, keyList[i].offset);
	}

	// Unknown/Null (0x8 * numKeys, length = minLength - (0x8 * numKeys)
	if (numKeys == 6 && minLength == 0x123456) {
		// (Collision Header Animation Only)

		// Three Floats? (0x30, length = 0xC)
		writeInt(output, readInt(input));
		writeInt(output, readInt(input));
		writeInt(output, readInt(input));

		// Unknown/Null (0x3C, length = 0x2)
		writeShort(output, readShort(input));

		// Some Short (0x3E, length = 0x2)
		writeShort(output, readShort(input));
	}
	else {
		for (int i = numKeys * 0x8; i < minLength; i += 4) {
			writeInt(output, readInt(input));
		}
	}
	
	for (int i = 0; i < numKeys; i++) {
		if (keyList[i].number > 0 && keyList[i].offset != 0) {
			
			fseek(input, keyList[i].offset, SEEK_SET);
			fseek(output, keyList[i].offset, SEEK_SET);

			for (int j = 0; j < keyList[i].number; j++) {
				// Easing (0x0, length = 0x4)
				writeInt(output, readInt(input));

				// Time (0x4, length = 0x4)
				writeInt(output, readInt(input));

				// Value (0x8, length = 0x4)
				writeInt(output, readInt(input));

				// Unknown/Null (0xC, length = 0x8)
				for (int k = 0; k < 0x8; k += 4) {
					writeInt(output, readInt(input));
				}
			}
		}
	}
	
	fseek(input, savePos, SEEK_SET);
	fseek(output, savePos, SEEK_SET);
	free(keyList);
}

static void copyBackgroundAnimation(FILE *input, FILE *output, uint32_t offset, int numKeys, int minLength) {
	if (offset == 0) return;
	uint32_t savePos = ftell(input);
	fseek(input, offset, SEEK_SET);
	fseek(output, offset, SEEK_SET);

	// Unknown/Null (0x0, length = 0x4)
	writeInt(output, readInt(input));

	// Animation Loop point (0x4, length = 0x4)
	writeInt(output, readInt(input));

	// Animation (0x8, length = 0x58)
	copyAnimation(input, output, ftell(input), numKeys, minLength);

	fseek(input, savePos, SEEK_SET);
	fseek(output, savePos, SEEK_SET);
}

static void copyEffects(FILE *input, FILE *output, uint32_t offset) {
	if (offset == 0) return;
	uint32_t savePos = ftell(input);
	fseek(input, offset, SEEK_SET);
	fseek(output, offset, SEEK_SET);

	// Effect 1 (0x0, length = 0x8)
	Item effectOne = readItem(input, output);
	
	// Effect 2 (0x8, length = 0x8)
	Item effectTwo = readItem(input, output);

	// Texture Scroll (0x10, length = 0x4)
	uint32_t textureScrollOffset = readInt(input);
	writeInt(output, textureScrollOffset);

	// Unknown/Null (0x14, length = 0x1C)
	for (int i = 0; i < 0x1C; i += 4) {
		writeInt(output, readInt(input));
	}

	// Copy Effects
	copyEffectOne(input, output, effectOne);

	copyEffectTwo(input, output, effectTwo);

	copyTextureScroll(input, output, textureScrollOffset);

	fseek(input, savePos, SEEK_SET);
	fseek(output, savePos, SEEK_SET);
}

static void copyEffectOne(FILE *input, FILE *output, Item item) {
	uint32_t savePos = ftell(input);
	fseek(input, item.offset, SEEK_SET);
	fseek(output, item.offset, SEEK_SET);

	for (int i = 0; i < item.number; i++) {
		// Unknown (0x0, length = 0xC)
		writeInt(output, readInt(input)); // X?
		writeInt(output, readInt(input)); // Y?
		writeInt(output, readInt(input)); // Z?

		// Unknown (0xC, length = 0x8)
		writeShort(output, readShort(input)); // X?
		writeShort(output, readShort(input)); // Y?
		writeShort(output, readShort(input)); // Z?
		writeNormalShort(output, readShort(input)); // Marker?
	}
	
	fseek(input, savePos, SEEK_SET);
	fseek(output, savePos, SEEK_SET);
}
static void copyEffectTwo(FILE *input, FILE *output, Item item) {
	uint32_t savePos = ftell(input);
	fseek(input, item.offset, SEEK_SET);
	fseek(output, item.offset, SEEK_SET);

	for (int i = 0; i < item.number; i++) {
		// Unknown (0x0, length = 0xC)
		writeInt(output, readInt(input)); // X?
		writeInt(output, readInt(input)); // Y?
		writeInt(output, readInt(input)); // Z?

		// Unknown/Null (0xC, length = 0x4)
		// There is a discrepancy on the size of this (ie: is it 4 bytes or 1 byte then 3 bytes)
		writeInt(output, readInt(input));
	}

	fseek(input, savePos, SEEK_SET);
	fseek(output, savePos, SEEK_SET);
}
static void copyTextureScroll(FILE *input, FILE *output, uint32_t offset) {
	if (offset == 0) return;

	uint32_t savePos = ftell(input);
	fseek(input, offset, SEEK_SET);
	fseek(output, offset, SEEK_SET);

	// Speed (0x0, length = 0x8)
	writeInt(output, readInt(input)); // X
	writeInt(output, readInt(input)); // Y
	
	fseek(input, savePos, SEEK_SET);
	fseek(output, savePos, SEEK_SET);
}

static uint32_t copyCollisionTriangleGrid(FILE *input, FILE *output, uint32_t offset, uint32_t xStepCount, uint32_t zStepCount) {
	if (offset == 0) return 0;
	uint32_t savePos = ftell(input);
	fseek(input, offset, SEEK_SET);
	fseek(output, offset, SEEK_SET);

	uint16_t maxIndex = 0;
	int totalSteps = xStepCount * zStepCount;
	for (int i = 0; i < totalSteps; i++) {
		// Grid Pointer (0x0, length = 0x4)
		uint32_t gridPointer = readInt(input);
		writeInt(output, gridPointer);
		if (gridPointer != 0) {
			// Copy the actual grid...
			uint32_t savePos2 = ftell(input);
			fseek(input, gridPointer, SEEK_SET);
			fseek(output, gridPointer, SEEK_SET);

			do {
				uint16_t index = readShort(input);
				writeShort(output, index);
				if (index == 0xFFFF) break;
				if (index > maxIndex) maxIndex = index;
			} while (1);

			fseek(input, savePos2, SEEK_SET);
			fseek(output, savePos2, SEEK_SET);
		}
	}

	fseek(input, savePos, SEEK_SET);
	fseek(output, savePos, SEEK_SET);

	return (int)maxIndex;
}

static void copyCollisionTriangles(FILE *input, FILE *output, uint32_t offset, uint32_t maxIndex) {
	if (offset == 0) return;
	uint32_t savePos = ftell(input);
	fseek(input, offset, SEEK_SET);
	fseek(output, offset, SEEK_SET);
	// Collision Triangles are 0 indexed
	// Because of that a max index of ie 10 must include 10 here
	for (uint32_t i = 0; i <= maxIndex; i++) {
		// Position 1 (0x0, length = 0xC)
		writeInt(output, readInt(input)); // X
		writeInt(output, readInt(input)); // Y
		writeInt(output, readInt(input)); // Z

		// Normal (0xC, length = 0xC)
		writeInt(output, readInt(input)); // X
		writeInt(output, readInt(input)); // Y
		writeInt(output, readInt(input)); // Z

		// Roation From XY Plane (0x18, length = 0x8)
		writeShort(output, readShort(input)); // X
		writeShort(output, readShort(input)); // Y
		writeShort(output, readShort(input)); // Z
		writeShort(output, readShort(input)); // Padding

		// Distance (0x20, length = 0x10)
		writeInt(output, readInt(input)); // DX2X1
		writeInt(output, readInt(input)); // DY2Y1
		writeInt(output, readInt(input)); // DX3X1
		writeInt(output, readInt(input)); // DY3Y1

		// Tangent (0x30, length = 0x8)
		writeInt(output, readInt(input)); // X
		writeInt(output, readInt(input)); // Y

		// Bitangent (0x38, length = 0x8)
		writeInt(output, readInt(input)); // X
		writeInt(output, readInt(input)); // Y
	}

	fseek(input, savePos, SEEK_SET);
	fseek(output, savePos, SEEK_SET);
}

static Item readItem(FILE *input, FILE *output) {
	Item newItem;

	newItem.number = readInt(input);
	newItem.offset = readInt(input);
	writeInt(output, newItem.number);
	writeInt(output, newItem.offset);

	return newItem;
}

static void copyStartPositions(FILE *original, FILE *converted, Item item) {
	if (item.offset == 0) return;
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
	if (item.offset == 0) return;
	fseek(original, item.offset, SEEK_SET);
	fseek(converted, item.offset, SEEK_SET);

	// Fallout Y (0x0, length = 0x4)
	writeInt(converted, readInt(original));
}
static void copyGoals(FILE *original, FILE *converted, Item item) {
	if (item.offset == 0) return;
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
	if (item.offset == 0) return;
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
	if (item.offset == 0) return;
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
	if (item.offset == 0) return;
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
static void copyConeCollisions(FILE *original, FILE *converted, Item item) {
	if (item.offset == 0) return;
	fseek(original, item.offset, SEEK_SET);
	fseek(converted, item.offset, SEEK_SET);

	// Loop through mystery sevens
	for (int i = 0; i < item.number; i++) {
		// Base Center Position (0x0, length = 0xC)
		writeInt(converted, readInt(original)); // X
		writeInt(converted, readInt(original)); // Y
		writeInt(converted, readInt(original)); // Z

		// Rotation (0xC, length = 0x8)
		writeShort(converted, readShort(original)); // X
		writeShort(converted, readShort(original)); // Y
		writeShort(converted, readShort(original)); // Z
		writeShort(converted, readShort(original)); // Padding

		// Radius One (0x14, length = 0x4)
		writeInt(converted, readInt(original));

		// Height (0x18, length = 0x4)
		writeInt(converted, readInt(original));

		// Radius Two (0x1C, length = 0x4)
		writeInt(converted, readInt(original));
	}
}
static void copyCylinderCollisions(FILE *original, FILE *converted, Item item) {
	if (item.offset == 0) return;
	fseek(original, item.offset, SEEK_SET);
	fseek(converted, item.offset, SEEK_SET);

	// Loop through mystery sevens
	for (int i = 0; i < item.number; i++) {
		// Center Position (0x0, length = 0xC)
		writeInt(converted, readInt(original)); // X
		writeInt(converted, readInt(original)); // Y
		writeInt(converted, readInt(original)); // Z

		// Radius (0xC, length = 0x4)
		writeInt(converted, readInt(original));

		// Height (0x10, length = 0x4)
		writeInt(converted, readInt(original));

		// Rotation (0x14, length = 0x8)
		writeShort(converted, readShort(original)); // X
		writeShort(converted, readShort(original)); // Y
		writeShort(converted, readShort(original)); // Z
		writeShort(converted, readShort(original)); // Padding
	}
}
static void copySphereCollisions(FILE *original, FILE *converted, Item item) {
	if (item.offset == 0) return;
	fseek(original, item.offset, SEEK_SET);
	fseek(converted, item.offset, SEEK_SET);

	// Loop through sphere collisions
	for (int i = 0; i < item.number; i++) {
		// Center Position (0x0, length = 0xC)
		writeInt(converted, readInt(original)); // X
		writeInt(converted, readInt(original)); // Y
		writeInt(converted, readInt(original)); // Z

		// Radius (0xC, length = 0x4)
		writeInt(converted, readInt(original));

		// Unknown/Null (0x10, length = 0x4)
		writeInt(converted, readInt(original));
	}
}
static void copyFalloutVolumes(FILE *original, FILE *converted, Item item) {
	if (item.offset == 0) return;
	fseek(original, item.offset, SEEK_SET);
	fseek(converted, item.offset, SEEK_SET);
	//if (item.number == 0 && item.offset != 0) {
	//	item.number = 1; // TODO 8 bytes if offset, no no count?
	//}
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
	if (item.offset == 0) return;
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
		copyBackgroundAnimation(original, converted, backgroundAnimationOffsetOne, 8, 0x0);

		 // Copy animation two
		copyBackgroundAnimation(original, converted, backgroundAnimationOffsetTwo, 11, 0x0);

		// Copy effects
		copyEffects(original, converted, effectsOffset);
	}
}
static void copyMysteryEights(FILE *original, FILE *converted, Item item) {
	if (item.offset == 0) return;
	fseek(original, item.offset, SEEK_SET);
	fseek(converted, item.offset, SEEK_SET);

	for (int i = 0; i < item.number; i++) {
		// Marker (0x1F) (0x0, length = 0x4)
		writeInt(converted, readInt(original));

		// Model Name Offset (0x4, length = 0x4)
		uint32_t modelNameOffset = readInt(original);
		writeInt(converted, modelNameOffset);

		// Unknown/Null (0x8, length = 0x18)
		for (int j = 0; j < 0x18; j += 4) {
			writeInt(converted, readInt(original));
		}

		// Three Floats (0x20, length = 0xC)
		writeInt(converted, readInt(original));
		writeInt(converted, readInt(original));
		writeInt(converted, readInt(original));

		// Unknown/Null (0x2C, length = 0x10)
		writeInt(converted, readInt(original));
		writeInt(converted, readInt(original));
		writeInt(converted, readInt(original));

		copyAsciiAligned(original, converted, modelNameOffset);
	}
}
static void copyMysteryTwelves(FILE *original, FILE *converted, Item item) {
	if (item.offset == 0) return;
	fseek(original, item.offset, SEEK_SET);
	fseek(converted, item.offset, SEEK_SET);
	
	// 5 somthing sets? (0x0, length = 0x28)
	Item setOne = readItem(original, converted); // Set 1
	Item setTwo = readItem(original, converted); // Set 2
	Item setThree = readItem(original, converted); // Set 3
	Item setFour = readItem(original, converted); // Set 4
	Item setFive = readItem(original, converted); // Set 5

	// Unknown/Null (0x28, length = 0xC8)
	for (int i = 0; i < 0xC8; i += 4) {
		writeInt(converted, readInt(original));
	}

	// Copy Set 1 Data
	fseek(original, setOne.offset, SEEK_SET);
	fseek(converted, setOne.offset, SEEK_SET);
	for (int i = 0; i < setOne.number; i++) {
		// One (0x0, length = 0x4)
		writeInt(converted, readInt(original));

		// Four Floats (0x4, length = 10)
		writeInt(converted, readInt(original));
		writeInt(converted, readInt(original));
		writeInt(converted, readInt(original));
		writeInt(converted, readInt(original));
	}

	// Copy Set 2 Data
	fseek(original, setTwo.offset, SEEK_SET);
	fseek(converted, setTwo.offset, SEEK_SET);
	for (int i = 0; i < setTwo.number; i++) {
		// One (0x0, length = 0x4)
		writeInt(converted, readInt(original));

		// Float (0x4, length = 0x4)
		writeInt(converted, readInt(original));

		// Unknown/Null (0x8, length = 0xC)
		writeInt(converted, readInt(original));
		writeInt(converted, readInt(original));
		writeInt(converted, readInt(original));
	}

	// Copy Set 3 Data
	fseek(original, setThree.offset, SEEK_SET);
	fseek(converted, setThree.offset, SEEK_SET);
	for (int i = 0; i < setThree.number; i++) {
		// One (0x0, length = 0x4)
		writeInt(converted, readInt(original));

		// Four Floats (0x4, length = 10)
		writeInt(converted, readInt(original));
		writeInt(converted, readInt(original));
		writeInt(converted, readInt(original));
		writeInt(converted, readInt(original));
	}

	// Copy Set 5 Data
	fseek(original, setFive.offset, SEEK_SET);
	fseek(converted, setFive.offset, SEEK_SET);
	for (int i = 0; i < setFive.number; i++) {
		// Three Floats (0x0, length = 0xC)
		writeInt(converted, readInt(original));
		writeInt(converted, readInt(original));
		writeInt(converted, readInt(original));
		
		// Four Shorts? (0xC, length = 0x8)
		writeShort(converted, readShort(original));
		writeShort(converted, readShort(original));
		writeShort(converted, readShort(original));
		writeShort(converted, readShort(original));
	}

	// Copy Set 4 Data
	fseek(original, setFour.offset, SEEK_SET);
	fseek(converted, setFour.offset, SEEK_SET);
	for (int i = 0; i < setFour.number; i++) {
		Item innerSet[3];
		// Read Sets 1-3 (0x0, length = 0x18)
		for (int j = 0; j < 3; j++) {
			innerSet[j] = readItem(original, converted);
		}
		uint32_t savePos = ftell(original);

		// 1/2/3/4/5/6
		for (int j = 0; j < 3; j++) {
			fseek(original, innerSet[j].offset, SEEK_SET);
			fseek(converted, innerSet[j].offset, SEEK_SET);

			for (int k = 0; k < innerSet[j].number; k++) {
				// One (0x0, length = 0x4)
				writeInt(converted, readInt(original));

				// Four Floats? (0x4, length = 0x10)
				writeInt(converted, readInt(original));
				writeInt(converted, readInt(original));
				writeInt(converted, readInt(original));
				writeInt(converted, readInt(original));
			}
		}
		fseek(original, savePos, SEEK_SET);
		fseek(converted, savePos, SEEK_SET);
	}
}
static void copyReflectiveModels(FILE *original, FILE *converted, Item item) {
	if (item.offset == 0) return;
	fseek(original, item.offset, SEEK_SET);
	fseek(converted, item.offset, SEEK_SET);

	for (int i = 0; i < item.number; i++) {
		// Model Name offset (0x0, length = 0x4)
		uint32_t modelNameOffset = readInt(original);
		writeInt(converted, modelNameOffset);

		// Null/Padding (0x4, length = 0x8)
		writeInt(converted, readInt(original));
		writeInt(converted, readInt(original));

		if (modelNameOffset != 0) {
			// Copy model name
			copyAsciiAligned(original, converted, modelNameOffset);
		}
	}
}
static void copyModelDuplicates(FILE *original, FILE *converted, Item item) {
	if (item.offset == 0) return;
	fseek(original, item.offset, SEEK_SET);
	fseek(converted, item.offset, SEEK_SET);
	
	for (int i = 0; i < item.number; i++) {
		// Offset to level model A (0x0, length = 0x4)
		writeInt(converted, readInt(original));

		// Position? (0x4, length = 0xC)
		writeInt(converted, readInt(original)); // X?
		writeInt(converted, readInt(original)); // Y?
		writeInt(converted, readInt(original)); // Z?

		// Rotation?? (0x10, length = 0x8)
		writeShort(converted, readShort(original)); // Unknown X?
		writeShort(converted, readShort(original)); // Has Value Y
		writeShort(converted, readShort(original)); // Unknown Z?
		writeShort(converted, readShort(original)); // Unknown Padding?

		// Scale? (0x18, length = 0xC)
		writeInt(converted, readInt(original));
		writeInt(converted, readInt(original));
		writeInt(converted, readInt(original));
	}
}
static void copyLevelModelAs(FILE *original, FILE *converted, Item item) {
	if (item.offset == 0) return;
	fseek(original, item.offset, SEEK_SET);
	fseek(converted, item.offset, SEEK_SET);

	for (int i = 0; i < item.number; i++) {
		// Level Model A Symbol (0x0, length = 0x8)
		writeInt(converted, readInt(original)); // 0
		writeInt(converted, readInt(original)); // 1

		// Offset to Level Model A (Actual) (0x0, length = 0x4)
		uint32_t levelModelAOffset = readInt(original);
		writeInt(converted, levelModelAOffset);

		uint32_t savePos = ftell(original);
		// Seek to the actual Level Model A
		fseek(original, levelModelAOffset, SEEK_SET);
		fseek(converted, levelModelAOffset, SEEK_SET);

		// Null (0x0, length = 4)
		writeInt(converted, readInt(original));

		// Model Name offset (0x4, length = 4)
		uint32_t modelNameOffset = readInt(original);
		writeInt(converted, modelNameOffset);

		// Null/Padding (0x8, length = 4)
		writeInt(converted, readInt(original));

		// Float (Sometimes 0x41F00000) (0xC, length = 0x4)
		writeInt(converted, readInt(original));

		if (modelNameOffset != 0) {
			// Copy model name
			copyAsciiAligned(original, converted, modelNameOffset);
		}

		fseek(original, savePos, SEEK_SET);
		fseek(converted, savePos, SEEK_SET);
	}
}
static void copyLevelModelBs(FILE *original, FILE *converted, Item item) {
	if (item.offset == 0) return;
	fseek(original, item.offset, SEEK_SET);
	fseek(converted, item.offset, SEEK_SET);

	for (int i = 0; i < item.number; i++) {
		// Offset to Level Model A (0x0, length = 0x4)
		writeInt(converted, readInt(original)); // Not saved because the Level Model A readion covers it
	}
}
static void copySwitches(FILE *original, FILE *converted, Item item) {
	if (item.offset == 0) return;
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
	if (item.offset == 0) return;
	const int numKeys = 6;
	uint32_t savePos = ftell(original);

	fseek(original, item.offset, SEEK_SET);
	fseek(converted, item.offset, SEEK_SET);
	Item keyList[6];

	// Gather keys (0x0, length = 0x8 * numKeys
	for (int i = 0; i < numKeys; i++) {
		keyList[i].number = readInt(original);
		keyList[i].offset = readInt(original);
		writeInt(converted, keyList[i].number);
		writeInt(converted, keyList[i].offset);
	}

	// Unknown/Null (0x8 * numKeys, length = 0x30 - (0x8 * numKeys)
	for (int i = numKeys * 0x8; i < 0x30; i += 4) {
		writeInt(converted, readInt(original));
	}

	for (int i = 0; i < numKeys; i++) {
		if (keyList[i].number > 0 && keyList[i].offset != 0) {

			fseek(original, keyList[i].offset, SEEK_SET);
			fseek(converted, keyList[i].offset, SEEK_SET);

			for (int j = 0; j < keyList[i].number; j++) {
				// Easing (0x0, length = 0x4)
				writeInt(converted, readInt(original));

				// Time (0x4, length = 0x4)
				writeInt(converted, readInt(original));

				// Value (0x8, length = 0x4)
				writeInt(converted, readInt(original));

				// Unknown/Null (0xC, length = 0x8)
				for (int k = 0; k < 0x8; k += 4) {
					writeInt(converted, readInt(original));
				}
			}
		}
	}

	fseek(original, savePos, SEEK_SET);
	fseek(converted, savePos, SEEK_SET);
}
static void copyWormholes(FILE *original, FILE *converted, Item item) {
	if (item.offset == 0) return;
	fseek(original, item.offset, SEEK_SET);
	fseek(converted, item.offset, SEEK_SET);

	// Loop through wormholes
	for (int i = 0; i < item.number; i++) {
		// Wormhole ID (1) (0x0, length = 0x4)
		// Every second wormhole has this as a marker (no endianness change)
		if (i % 2 == 0) {
			writeInt(converted, readInt(original));
		}
		else {
			writeNormalInt(converted, readInt(original));
		}

		// Position (0x4, length = 0xC)
		writeInt(converted, readInt(original)); // X
		writeInt(converted, readInt(original)); // Y
		writeInt(converted, readInt(original)); // Z

		// Rotation (0x10, length = 0x8)
		writeShort(converted, readShort(original)); // X
		writeShort(converted, readShort(original)); // Y
		writeShort(converted, readShort(original)); // Z
		writeShort(converted, readShort(original)); // Padding/Null

		// Offset to destination wormwhole (0x18, length = 0x4)
		writeInt(converted, readInt(original));
	}
}
static void copyFog(FILE *original, FILE *converted, Item item) {
	if (item.offset == 0) return;
	fseek(original, item.offset, SEEK_SET);
	fseek(converted, item.offset, SEEK_SET);

	// Fog Id Marker (0x0, length = 0x4, marker)
	writeNormalInt(converted, readInt(original));

	// Distance (0x4, length = 0x8)
	writeNormalInt(converted, readInt(original)); // Start Distance
	writeNormalInt(converted, readInt(original)); // End Distance

	// Color (0xC, length = 0xC)
	writeNormalInt(converted, readInt(original)); // Red
	writeNormalInt(converted, readInt(original)); // Green
	writeNormalInt(converted, readInt(original)); // Blue

	// Unknown/Null (0x18, length = 0xC)
	for (int i = 0; i < 0xC; i += 4) {
		writeInt(converted, readInt(original));
	}
}
static void copyMysteryThrees(FILE *original, FILE *converted, Item item) {
	if (item.offset == 0) return;
	fseek(original, item.offset, SEEK_SET);
	fseek(converted, item.offset, SEEK_SET);
	
	// Position? (0x0, length = 0xC)
	writeInt(converted, readInt(original));
	writeInt(converted, readInt(original));
	writeInt(converted, readInt(original));

	// Some Symbol (0xC, length = 0x4)
	writeShort(converted, readShort(original)); // Unknown/Null
	writeShort(converted, readShort(original)); // The marker value

	// Unknown/Null (0x10, length = 0x14)
	for (int i = 0; i < 0x14; i += 4) {
		writeInt(converted, readInt(original));
	}
}
static void copyMysteryFive(FILE *original, FILE *converted, Item item) {
	if (item.offset == 0) return;
	fseek(original, item.offset, SEEK_SET);
	fseek(converted, item.offset, SEEK_SET);

	// Unknown/Null (0x0, length = 0x4)
	writeInt(converted, readInt(original));

	// Unknown/Floats (0x4, length = 0xC)
	writeInt(converted, readInt(original));
	writeInt(converted, readInt(original));
	writeInt(converted, readInt(original));
}
static void copyMysteryElevens(FILE *original, FILE *converted, Item item) {
	if (item.offset == 0) return;
	fseek(original, item.offset, SEEK_SET);
	fseek(converted, item.offset, SEEK_SET);

	// Float? (0x0, length = 0x4)
	writeInt(converted, readInt(original));

	// Unknown/Null (0x4, length = 0x4)
	writeInt(converted, readInt(original));
}
static void copyCollisionFields(FILE *original, FILE *converted, Item item) {
	if (item.offset == 0) return;
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

		// Conveyor Speed (0x18, length = 0xC)
		writeInt(converted, readInt(original)); // X
		writeInt(converted, readInt(original)); // Y
		writeInt(converted, readInt(original)); // Z

		// Collision triangle information (0x24, length = 0x8)
		uint32_t collisionTriangleListOffset = readInt(original);
		writeInt(converted, collisionTriangleListOffset);

		uint32_t collisionTriangleGridOffset = readInt(original);
		writeInt(converted, collisionTriangleGridOffset);

		// Grid Paramters (0x2C, length = 0x10)
		writeInt(converted, readInt(original)); // X Start

		writeInt(converted, readInt(original)); // Z Start

		writeInt(converted, readInt(original)); // X Step

		writeInt(converted, readInt(original)); // Z Step
		
		// Grid Step Counts (0x3C, length = 0x8)
		uint32_t xStepCount = readInt(original);
		writeInt(converted, xStepCount);

		uint32_t zStepCount = readInt(original);
		writeInt(converted, zStepCount);

		// Time for a duplicate of most of the file header
		// Technically the lists can point to different places...
		Item goals;
		Item bumpers;
		Item jamabars;
		Item bananas;
		Item coneCollisions;
		Item sphereCollisions;
		Item cylinderCollisions;
		Item falloutVolumes;
		Item reflectiveModels;
		Item modelDuplicates;
		Item levelModelB;
		Item switches;
		Item wormholes;
		Item mysteryFive;
		Item mysteryEleven;

		// Goals (0x44, length = 0x8)
		goals = readItem(original, converted);
		
		// Bumpers (0x4C, length = 0x8)
		bumpers = readItem(original, converted);
		
		// Jamabars (0x54, length = 0x8)
		jamabars = readItem(original, converted);

		// Bananas (0x5C, length = 0x8)
		bananas = readItem(original, converted);

		// Cone Collisions (0x64, length = 0x8)
		coneCollisions = readItem(original, converted);

		// Sphere Collisions (0x6C, length = 0x8)
		sphereCollisions = readItem(original, converted);

		// Cylinder Collisions (0x74, length = 0x8)
		cylinderCollisions = readItem(original, converted);

		// Fallout Volumes (0x7C, length = 0x8)
		falloutVolumes = readItem(original, converted);
		
		// Reflective Models (0x84, length = 0x8)
		reflectiveModels = readItem(original, converted);

		// Model Duplicates (0x8C, length = 0x8)
		modelDuplicates = readItem(original, converted);

		// Level Model Type Bs (0x94, length = 0x8)
		levelModelB = readItem(original, converted);

		// Unknown/Null (0x9C, length = 0x8)
		for (int j = 0; j < 0x8; j += 4) {
			writeInt(converted, readInt(original));
		}

		// Animation Group ID (0xA4, length = 0x4, marker?)
		writeShort(converted, readShort(original));
		writeShort(converted, readShort(original));

		// Switches (0xA8, length = 0x8)
		switches = readItem(original, converted);

		// Unknown/Null (0xB0, length = 0x4)
		writeInt(converted, readInt(original));

		// Mystery Five (0xB4, length = 0x4)
		mysteryFive.offset = readInt(original);
		writeInt(converted, mysteryFive.offset);

		// Seesaw (0xB8, length = 0xC)
		writeInt(converted, readInt(original)); // Sensitivity
		writeInt(converted, readInt(original)); // Reset Stiffness
		writeInt(converted, readInt(original)); // Bounds of Rotation
		
		// Wormholes (0xC4, length = 0x8)
		wormholes = readItem(original, converted);

		// Initial Animation State (0xCC, length = 0x4)
		writeInt(converted, readInt(original));

		// Unknown/Null (0xD0, length = 0x4)
		writeInt(converted, readInt(original));

		// Animation Loop Point (0xD4, length = 0x4)
		writeInt(converted, readInt(original));

		// Mystery Eleven (0xD8, length = 0x4)
		mysteryEleven.offset = readInt(original);
		writeInt(converted, mysteryEleven.offset);

		// Unknown/Null (0xDC, length = 0x3C0)
		for (int j = 0; j < 0x3C0; j += 4) {
			writeInt(converted, readInt(original));
		}

		uint32_t savePos = ftell(original);

		// Copy the fun items...
		copyAnimation(original, converted, animationOffset, 6, 0x123456);
		
		copyBumpers(original, converted, bumpers);
		
		copyJamabars(original, converted, jamabars);

		copyBananas(original, converted, bananas);

		copyConeCollisions(original, converted, coneCollisions);

		copySphereCollisions(original, converted, sphereCollisions);

		copyCylinderCollisions(original, converted, cylinderCollisions);

		copyFalloutVolumes(original, converted, falloutVolumes);
		
		copyReflectiveModels(original, converted, reflectiveModels);

		copyModelDuplicates(original, converted, modelDuplicates);

		copyLevelModelBs(original, converted, levelModelB);

		copySwitches(original, converted, switches);

		copyMysteryFive(original, converted, mysteryFive);

		copyWormholes(original, converted, wormholes);

		copyMysteryElevens(original, converted, mysteryEleven);
		
		// Only convert exactly the grid specified
		// and only up to the last used collision triangle index
		// This avoids making assumptions about file order
		// in order to approximate section size
		uint32_t maxTriangleIndex = copyCollisionTriangleGrid(original, converted, collisionTriangleGridOffset, xStepCount, zStepCount);
		copyCollisionTriangles(original, converted, collisionTriangleListOffset, maxTriangleIndex);
		
		fseek(original, savePos, SEEK_SET);
		fseek(converted, savePos, SEEK_SET);
	}
}
#include "GMAConverter.h"
#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif

#include <string>
#include "FunctionsAndDefines.h"

typedef struct {
	uint32_t modelOffsetFromBase;
	uint32_t nameOffsetFromNames;

}Model;

inline void copyAscii(FILE *input, FILE *output, uint32_t offset) {
	fseek(input, offset, SEEK_SET);
	fseek(output, offset, SEEK_SET);
	int c;
	do {
		c = getc(input);
		if (c == EOF) {
			break;
		}
		putc(c, output);
	} while (!feof(input) && c != 0);
}

inline int compareModels(const void* a, const void* b) {
	uint32_t aVal = *(((uint32_t*)a) + 1);
	uint32_t bVal = *(((uint32_t*)b) + 1);
	return aVal - bVal;
}

inline void copyChunk(FILE* input, FILE* output, uint32_t chunkSize, uint32_t(*readInt)(FILE*), void(*writeInt)(FILE*, uint32_t), uint16_t(*readShort)(FILE*), void(*writeShort)(FILE*, uint16_t));

void parseGMA(char* filename) {
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
	FILE* converted;

	std::string inputFile(filename);

	std::string outputFile;

	Model models[256];

	if (original == NULL) {
		printf("Error opening file\n");
		return;
	}

	uint32_t fileLength;
	fseek(original, 0, SEEK_END);
	fileLength = ftell(original);
	fseek(original, 0, SEEK_SET);


	// Check which game it is
	if (readBigShort(original) != 0)  {
		game = SMBD;
		outputFile = inputFile + ".smb2";
		// Open the output file
		converted = fopen(outputFile.c_str(), "wb");

		// Set correct IO functions
		readInt = &readLittleInt;
		readShort = &readLittleShort;

		writeInt = &writeBigInt;
		writeShort = &writeBigShort;

		writeNormalInt = &writeLittleInt;
		writeNormalShort = &writeLittleShort;
	}
	else {
		game = SMB2;
		fseek(original, 0, SEEK_SET);
		outputFile = inputFile + ".smbd";
		// Open the output file
		converted = fopen(outputFile.c_str(), "wb");

		// Set correct IO functions
		readInt = &readBigInt;
		readShort = &readBigShort;

		writeInt = &writeLittleInt;
		writeShort = &writeLittleShort;

		writeNormalInt = &writeBigInt;
		writeNormalShort = &writeBigShort;
	}

	fseek(original, 0, SEEK_SET);

	// Num models
	uint32_t numModels = readInt(original);
	writeInt(converted, numModels);

	// Model Data Base Offset
	uint32_t modelBaseOffset = readInt(original);
	writeInt(converted, modelBaseOffset);

	// Copy models offsets
	for (int i = 0; i < (int)numModels; ++i) {
		// Model offset from model base
		models[i].modelOffsetFromBase = readInt(original);
		writeInt(converted, models[i].modelOffsetFromBase);

		// Name offset from name base
		models[i].nameOffsetFromNames = readInt(original);
		writeInt(converted, models[i].nameOffsetFromNames);
	}

	// Sort models by name offset to ensure we can add trailing zeros
	qsort(models, numModels, sizeof(Model), &compareModels);

	// Copy models names
	uint32_t nameOffset = ftell(original);
	for (int i = 0; i < (int)numModels; ++i) {
		copyAscii(original, converted, nameOffset + models[i].nameOffsetFromNames);
	}

	// Copy padding until we are at the model base
	while (ftell(original) < (int) modelBaseOffset) {
		putc(getc(original), converted);
	}


	// Copy Models
	for (int i = 0; i < (int)numModels; ++i) {
		fseek(original, modelBaseOffset + models[i].modelOffsetFromBase, SEEK_SET);
		fseek(converted, modelBaseOffset + models[i].modelOffsetFromBase, SEEK_SET);

		// ASCII (int) "GCMF"
		writeInt(converted, readInt(original));

		// Unknown uint32
		writeInt(converted, readInt(original));

		// 4 Unknown Floats (0x10)
		for (int j = 0; j < 0x10 / 4; ++j) {
			writeInt(converted, readInt(original));
		}

		// Number of textures
		uint16_t numTextures = readShort(original);
		writeShort(converted, numTextures);

		// Number of textures in section 1
		uint16_t numTexturesSection1 = readShort(original);
		writeShort(converted, numTexturesSection1);

		// Number of textures in section 2
		uint16_t numTexturesSection2 = readShort(original);
		writeShort(converted, numTexturesSection2);

		// Number of header blobs (unknown)?
		putc(getc(original), converted);

		// Check byte (0x00)
		putc(getc(original), converted);

		// End of header offset (unknown)?
		writeInt(converted, readInt(original));

		// Check uint32 (0)
		writeInt(converted, readInt(original));

		// Check int32 (-1)
		writeInt(converted, readInt(original));

		// Check int32 (-1)
		writeInt(converted, readInt(original));

		// Check uint32 (0)
		writeInt(converted, readInt(original));

		// Check uint32 (0)
		writeInt(converted, readInt(original));

		// Check uint32 (0)
		writeInt(converted, readInt(original));

		// Check uint32 (0)
		writeInt(converted, readInt(original));


		// Copy texture headers
		for (int j = 0; j < numTextures; ++j) {
			// Unknown int (lat byte = clamping: 0x80 CLAMP BOTH, 0x84 CLAMP T 0x90 CLAMP S, 0x94 NO CLAMP)
			writeInt(converted, readInt(original));

			// TPL texture number
			writeShort(converted, readShort(original));

			// Unknown uint16
			writeShort(converted, readShort(original));

			// Check uint32 (0)
			writeInt(converted, readInt(original));

			// Unknown uint16
			writeShort(converted, readShort(original));

			// Texture index
			writeShort(converted, readShort(original));

			// Unknown uint32
			writeInt(converted, readInt(original));

			// Check uint32 (0)
			writeInt(converted, readInt(original));

			// Check uint32 (0)
			writeInt(converted, readInt(original));

			// Check uint32 (0)
			writeInt(converted, readInt(original));
		}

		// Section Header
		// 5 Unknown uint32 (0x14)
		for (int j = 0; j < 0x14 / 4; ++j) {
			writeInt(converted, readInt(original));
		}

		// 4 Unknown uint16 (0x8)
		for (int j = 0; j < 0x8 / 2; ++j) {
			writeShort(converted, readShort(original));
		}

		// Vertex flags
		writeInt(converted, readInt(original));

		// Check int32 (-1)
		writeInt(converted, readInt(original));

		// Check int32 (-1)
		writeInt(converted, readInt(original));

		// Chunk 1 size
		uint32_t chunk1Size = readInt(original);
		writeInt(converted, chunk1Size);

		// Chunk 2 size
		uint32_t chunk2Size = readInt(original);
		writeInt(converted, chunk2Size);

		// 4 Unknown float (0x10)
		for (int j = 0; j < 0x10 / 4; ++j) {
			writeInt(converted, readInt(original));
		}

		// Unknown uint32
		writeInt(converted, readInt(original));

		// 7 Check int32 (0) (0x1C)
		for (int j = 0; j < 0x1C / 4; ++j) {
			writeInt(converted, readInt(original));
		}

		copyChunk(original, converted, chunk1Size, readInt, writeInt, readShort, writeShort);
		copyChunk(original, converted, chunk2Size, readInt, writeInt, readShort, writeShort);
	}

	fclose(original);
	fclose(converted);
}

inline void copyChunk(FILE* input, FILE* output, uint32_t chunkSize, uint32_t(*readInt)(FILE*), void(*writeInt)(FILE*, uint32_t), uint16_t(*readShort)(FILE*), void(*writeShort)(FILE*, uint16_t)) {
	if (chunkSize == 0) { return; }
	uint32_t initialPos = ftell(input);
	
	// Filler
	putc(getc(input), output);
	
	// Make sure there is enough space left for another section (min size: 39 (0x27))
	while(ftell(input) - initialPos < chunkSize - 39) {
		// Type
		uint8_t type = (uint8_t) getc(input);
		putc(type, output);

		// Error checking/there may be a type 99 to support
		if (type != 0x98) {
			printf("NOT 98: %d\nAT: %#04x", type, ftell(input));
			return;
		}

		// Num verteces in part
		uint16_t numVerts = readShort(input);
		writeShort(output, numVerts);

		// Copy verteces in part
		for (int j = 0; j < numVerts; ++j) {
			// Position (X, Y, Z)
			writeInt(output, readInt(input));
			writeInt(output, readInt(input));
			writeInt(output, readInt(input));

			// Normal (I, J, K)
			writeInt(output, readInt(input));
			writeInt(output, readInt(input));
			writeInt(output, readInt(input));

			// Color RGBA
			writeInt(output, readInt(input));

			// Texture (S, T)
			writeInt(output, readInt(input));
			writeInt(output, readInt(input));
		}
	}

	// Trailing zeros
	while (ftell(input) - initialPos < chunkSize) {
		putc(getc(input), output);
	}
}
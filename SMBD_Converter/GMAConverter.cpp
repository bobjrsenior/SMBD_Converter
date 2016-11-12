#include "GMAConverter.h"

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



}
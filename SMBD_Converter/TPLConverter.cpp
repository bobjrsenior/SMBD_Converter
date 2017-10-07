#include "TPLConverter.h"
#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "FunctionsAndDefines.h"
#include <string>

#define CMPR 14
#define I8 1

typedef struct {
	uint32_t encoding;
	uint32_t originalOffset;
	uint32_t convertedOffset;
	uint16_t width;
	uint16_t height;
	uint16_t unknown;
	uint16_t always1234;
}Texture;

void copyTexture(FILE* input, FILE* output, uint32_t offset, uint32_t encoding);

void copyCompressedTexture(FILE* input, FILE* output, uint32_t offset, uint32_t encoding);

void parseTPL(char* filename) {
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

	Texture textures[256];

	if (original == NULL) {
		printf("Error opening file\n");
		return;
	}

	uint32_t fileLength;
	fseek(original, 0, SEEK_END);
	fileLength = ftell(original);
	fseek(original, 0, SEEK_SET);

	// Check which game it is (SMBD starts with the ascii "XTPL")
	if (getc(original) == 'X' && getc(original) == 'T' && getc(original) == 'P' && getc(original) == 'L') {
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
		// Initial header
		putc('X', converted);
		putc('T', converted);
		putc('P', converted);
		putc('L', converted);

		// Set correct IO functions
		readInt = &readBigInt;
		readShort = &readBigShort;

		writeInt = &writeLittleInt;
		writeShort = &writeLittleShort;

		writeNormalInt = &writeBigInt;
		writeNormalShort = &writeBigShort;
	}

	

	// Get num textures
	int numTextures = readInt(original);
	int deadTextures = 0;
	// Write num textures later
	fseek(converted, 4, SEEK_CUR);

	if (numTextures > 256) {
		numTextures = 256;
		printf("Number of textures truncated to 256\n");
	}

	// Read in texture headers (we will write the the textre headers once we know the new offsets)
	for (int i = 0; i < numTextures; ++i) {
		// Encoding
		textures[i].encoding = readInt(original);

		// Offset to data
		textures[i].originalOffset = readInt(original);

		// Width
		textures[i].width = readShort(original);

		// Height
		textures[i].height = readShort(original);

		// Level Count (mip maps)
		textures[i].unknown = readShort(original);

		// Always 0x1234?
		textures[i].always1234 = readShort(original);
	}

	// Sekip past the texture headers in the converted file
	fseek(converted, numTextures * 0x10, SEEK_CUR);

	int difference = textures[0].originalOffset - ftell(converted);
	for (int i = 0; i < difference; ++i) {
		putc(0, converted);
	}

	// Copy the texture data
	for (int i = 0; i < numTextures; ++i) {
		// Seek to the texture
		fseek(original, textures[i].originalOffset, SEEK_SET);
		textures[i].convertedOffset = ftell(converted);

		if (game == SMB2) {
			if (textures[i].encoding == CMPR) {
				// Secondary CMPR Marker
				writeNormalInt(converted, 0x0C000000);
				
			}
			else if (textures[i].encoding == I8) {
				writeNormalInt(converted, 0x1A000000);
			}
			else {
				++deadTextures;
				continue;
			}

			// Width
			writeShort(converted, textures[i].width);
			// Padding
			writeShort(converted, 0);

			// Height
			writeShort(converted, textures[i].height);
			// Padding
			writeShort(converted, 0);

			// 4 or 5 ?
			writeInt(converted, 5);

			// Is it compressed (0x10000000 = compressed, 0x00000000 = uncompressed)
			writeInt(converted, 0);

			// Length of data
			uint32_t dataLength;
			// Calculated based on the offset of the next texture (or the end of the file)
			if (i < numTextures - 1) {
				dataLength = textures[i + 1].originalOffset - textures[i].originalOffset;
			}
			else {
				dataLength = fileLength - textures[i].originalOffset;
			}
			writeInt(converted, dataLength);

			// Length of data + 0xD (with part of header?), 0 if uncompressed
			writeInt(converted, 0);

			// Zero/Padding?
			writeInt(converted, 0);

			// Data
			for (int j = 0; j < (int) dataLength; ++j) {
				putc(getc(original), converted);
			}
		}
	}

	fclose(original);
	if (game == SMBD) {
		fseek(converted, 0, SEEK_SET);
	}
	else if (game == SMB2) {
		fseek(converted, 4, SEEK_SET);
	}

	writeInt(converted, numTextures - deadTextures);

	for (int i = 0; i < numTextures; ++i) {
		if (textures[i].encoding != CMPR && textures[i].encoding != I8) {
			continue;
		}
		// Encoding
		writeInt(converted, textures[i].encoding);

		// Data Offset
		writeInt(converted, textures[i].convertedOffset);

		// Width
		writeShort(converted, textures[i].width);

		// Height
		writeShort(converted, textures[i].height);

		// Unknown
		writeShort(converted, textures[i].unknown);

		// 0x1234
		writeNormalShort(converted, textures[i].always1234);
	}
	fclose(converted);
}

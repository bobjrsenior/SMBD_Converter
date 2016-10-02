#include <stdio.h>
#include <string>
#include <iostream>
#include <stdint.h>

typedef struct {
	uint32_t encoding;
	uint32_t offset;
	uint16_t width;
	uint16_t height;
	uint16_t unknown;
}Texture;

typedef struct {
	uint32_t modelOffsetFromBase;
	uint32_t nameOffsetFromStartOfNames;
	uint32_t unknown1;
	float unknown2;
	float unknown3;
	float unknown4;
	float unknown5;
	uint32_t numTextures;
	uint32_t numTexturesSection1;
	uint32_t numTexturesSection2;
	uint8_t numHeaderBlobs; //Unknown
	uint8_t checkByte; //0
	uint32_t endOfHeaderOffset; //Unknown
	//Checks
	std::string modelName;

}Model;

void parseTPL(char* filename);

void parseGMA(char* filename);

int main(int argc, char*argv[]) {
	if (argc == 1) {
		return 0;
	}
	std::string filenameParam(argv[1]);

	std::string fileType = filenameParam.substr(filenameParam.length() - 3, 3);

	if (fileType == "tpl") {
		parseTPL(argv[1]);
	}
	else if (fileType == "gma") {
		parseGMA(argv[1]);
	}

	return 0;
	
}

void parseTPL(char* filename) {
	const int CMPR = 14;
	const int I8 = 1;


	std::string inputFile(filename);

	std::string outputFile = inputFile + ".gc";
	int numTextures = 0;
	uint32_t fileSize = 0;
	Texture textures[256];
	int newOffsets[256];

	FILE* input = fopen(filename, "rb");

	if (input == NULL) {
		std::cout << "FAILED TO OPEN" << std::endl;
		return;
	}

	fseek(input, 0, SEEK_END);
	fileSize = ftell(input);
	fseek(input, 0, SEEK_SET);

	FILE* output = fopen(outputFile.c_str(), "wb");

	// Skip the XTPL marker
	fseek(input, 4, SEEK_SET);

	// Get the number of textures
	numTextures = getc(input) + (getc(input) << 8) + (getc(input) << 16) + (getc(input) << 24);

	//Retrieve the Textures headers
	for (int i = 0; i < numTextures; ++i) {
		textures[i].encoding = getc(input) + (getc(input) << 8) + (getc(input) << 16) + (getc(input) << 24);
		textures[i].offset = getc(input) + (getc(input) << 8) + (getc(input) << 16) + (getc(input) << 24);
		textures[i].width = getc(input) + (getc(input) << 8);
		textures[i].height = getc(input) + (getc(input) << 8);
		textures[i].unknown = getc(input) + (getc(input) << 8);

		// 0x1234
		getc(input);
		getc(input);
	}

	std::cout << "READ TEXTURE HEADERS" << std::endl;

	fseek(output, 4, SEEK_SET);
	fseek(output, 16 * numTextures, SEEK_CUR);


	for (int i = 0; i < numTextures; i++) {
		newOffsets[i] = ftell(output);
		fseek(input, textures[i].offset, SEEK_SET);

		// Designates texture start (Value depends on texture type?)
		fseek(input, 4, SEEK_CUR);

		// Copy of texture width and height
		fseek(input, 8, SEEK_CUR);

		// Unknown texture headers
		if (textures[i].encoding == CMPR) {
			fseek(input, 20, SEEK_CUR);
		}
		else if (textures[i].encoding == I8) {
			fseek(input, 20, SEEK_CUR);
		}// Unsupported/Unknown
		else {
			// Unknown
			fseek(input, 20, SEEK_CUR);
		}

		// Copy texture data
		while (((uint32_t) ftell(input)) < fileSize || (i < numTextures - 1 && ((uint32_t) ftell(input)) < textures[i + 1].offset)) {
			if (textures[i].encoding == CMPR) {
				/*
				uint16_t pallete1 = getc(input) + (getc(input) << 8);
				putc((pallete1 >> 8) & 0xFF, output);
				putc(pallete1 & 0xFF, output);

				uint16_t pallete2 = getc(input) + (getc(input) << 8);
				putc((pallete2 >> 8) & 0xFF, output);
				putc(pallete2 & 0xFF, output);
				*/

				
				uint32_t palletes = (getc(input)) + (getc(input) << 8) + (getc(input) << 16) + (getc(input) << 24);
				putc((palletes >> 8) & 0xFF, output);
				putc((palletes) & 0xFF, output);
				putc((palletes >> 24) & 0xFF, output);
				putc((palletes >> 16) & 0xFF, output);
				

				uint32_t pixels = (getc(input)) + (getc(input) << 8) + (getc(input) << 16) + (getc(input) << 24);
				putc((pixels >> 24) & 0xFF, output);
				putc((pixels >> 16) & 0xFF, output);
				putc((pixels >> 8) & 0xFF, output);
				putc((pixels) & 0xFF, output);
			}
			else if (textures[i].encoding == I8) {
				uint8_t intensity = getc(input);
				putc(intensity, output);
				
			}
			else {
				uint8_t intensity = getc(input);
				putc(intensity, output);
			}
		}
	}

	std::cout << "COPIED TEXTURES" << std::endl;

	fseek(output, 0, SEEK_SET);
	putc((numTextures >> 24) & 0xFF, output);
	putc((numTextures >> 16) & 0xFF, output);
	putc((numTextures >> 8) & 0xFF, output);
	putc(numTextures & 0xFF, output);

	for (int i = 0; i < numTextures; ++i) {
		uint32_t encoding = textures[i].encoding;
		putc((encoding >> 24) & 0xFF, output);
		putc((encoding >> 16) & 0xFF, output);
		putc((encoding >> 8) & 0xFF, output);
		putc(encoding & 0xFF, output);

		uint32_t offset = newOffsets[i];
		putc((offset >> 24) & 0xFF, output);
		putc((offset >> 16) & 0xFF, output);
		putc((offset >> 8) & 0xFF, output);
		putc(offset & 0xFF, output);

		uint16_t width = textures[i].width;
		putc((width >> 8) & 0xFF, output);
		putc(width & 0xFF, output);

		uint16_t height = textures[i].height;
		putc((height >> 8) & 0xFF, output);
		putc(height & 0xFF, output);

		uint16_t unknown = textures[i].unknown;
		putc((unknown >> 8) & 0xFF, output);
		putc(unknown & 0xFF, output);

		// 0x1234
		putc(18, output);
		putc(52, output);
	}
	std::cout << ftell(output) << std::endl;
	std::cout << "WROTE TEXTURES" << std::endl;
	fclose(input);
	fclose(output);
}

void parseGMA(char* filename) {
	std::string inputFile(filename);

	std::string outputFile = inputFile + ".gc";
	int numTextures = 0;
	uint32_t fileSize = 0;

	Model models[256];

	FILE* input = fopen(filename, "rb");

	if (input == NULL) {
		std::cout << "FAILED TO OPEN" << std::endl;
		return;
	}

	fseek(input, 0, SEEK_END);
	fileSize = ftell(input);
	fseek(input, 0, SEEK_SET);

	FILE* output = fopen(outputFile.c_str(), "wb");

	uint32_t numModels = getc(input) + (getc(input) << 8) + (getc(input) << 16) + (getc(input) << 24);
	putc((numModels >> 24) & 0xFF, output);
	putc((numModels >> 16) & 0xFF, output);
	putc((numModels >> 8) & 0xFF, output);
	putc(numModels & 0xFF, output);
	

	uint32_t modelDataBaseOffset = getc(input) + (getc(input) << 8) + (getc(input) << 16) + (getc(input) << 24);
	putc((numModels >> 24) & 0xFF, output);
	putc((numModels >> 16) & 0xFF, output);
	putc((numModels >> 8) & 0xFF, output);
	putc(numModels & 0xFF, output);

	for (int i = 0; i < (int) numModels; ++i) {
		uint32_t modelOffsetFromBase = getc(input) + (getc(input) << 8) + (getc(input) << 16) + (getc(input) << 24);
		models[i].modelOffsetFromBase = modelOffsetFromBase;

		uint32_t nameOffsetFromStartOfNames = getc(input) + (getc(input) << 8) + (getc(input) << 16) + (getc(input) << 24);
		models[i].nameOffsetFromStartOfNames = nameOffsetFromStartOfNames;
	}

	fseek(output, 8 * numModels, SEEK_CUR);
	uint32_t asciiOffset = ftell(input);

	for (int i = 0; i < (int) numModels; ++i) {
		char modelName[512];
		int index = 0;
		char c = getc(input);
		while(c != 0 && index < 510) {
			modelName[index++] = c;
			putc(c, output);
			c = getc(input);
		}
		putc(0, output);
		modelName[index] = 0;
		std::string modelNameStr(modelName);
	}


	// 0s for padding/alignment
	while (ftell(input) < modelDataBaseOffset) {
		putc(getc(input), output);
	}

	// GCMF
	putc('G', output);
	putc('C', output);
	putc('N', output);
	putc('F', output);

	// It it FMCG on xbox
	fseek(input, 4, SEEK_CUR);

	for (int i = 0; i < numModels; ++i) {
		/// Model Header

		uint32_t unknown1 = getc(input) + (getc(input) << 8) + (getc(input) << 16) + (getc(input) << 24);
		putc((unknown1 >> 24) & 0xFF, output);
		putc((unknown1 >> 16) & 0xFF, output);
		putc((unknown1 >> 8) & 0xFF, output);
		putc(unknown1 & 0xFF, output);

		uint32_t unknown2 = getc(input) + (getc(input) << 8) + (getc(input) << 16) + (getc(input) << 24);
		putc((unknown1 >> 24) & 0xFF, output);
		putc((unknown1 >> 16) & 0xFF, output);
		putc((unknown1 >> 8) & 0xFF, output);
		putc(unknown1 & 0xFF, output);

		uint32_t unknown3 = getc(input) + (getc(input) << 8) + (getc(input) << 16) + (getc(input) << 24);
		putc((unknown1 >> 24) & 0xFF, output);
		putc((unknown1 >> 16) & 0xFF, output);
		putc((unknown1 >> 8) & 0xFF, output);
		putc(unknown1 & 0xFF, output);

		uint32_t unknown4 = getc(input) + (getc(input) << 8) + (getc(input) << 16) + (getc(input) << 24);
		putc((unknown1 >> 24) & 0xFF, output);
		putc((unknown1 >> 16) & 0xFF, output);
		putc((unknown1 >> 8) & 0xFF, output);
		putc(unknown1 & 0xFF, output);

		uint32_t unknown5 = getc(input) + (getc(input) << 8) + (getc(input) << 16) + (getc(input) << 24);
		putc((unknown1 >> 24) & 0xFF, output);
		putc((unknown1 >> 16) & 0xFF, output);
		putc((unknown1 >> 8) & 0xFF, output);
		putc(unknown1 & 0xFF, output);

		uint16_t numTextures = getc(input) + (getc(input) << 8);
		putc((unknown1 >> 8) & 0xFF, output);
		putc(unknown1 & 0xFF, output);
		
		uint16_t numTexturesSection1 = getc(input) + (getc(input) << 8);
		putc((numTexturesSection1 >> 8) & 0xFF, output);
		putc(numTexturesSection1 & 0xFF, output);

		uint16_t numTexturesSection2 = getc(input) + (getc(input) << 8);
		putc((numTexturesSection2 >> 8) & 0xFF, output);
		putc(numTexturesSection2 & 0xFF, output);

		// Number of header blobs/Unknown
		putc(getc(input), output);

		// 0 (For alignment probably)
		putc(getc(input), output);

		// Unknown
		uint32_t endOfHeaderOffset = getc(input) + (getc(input) << 8) + (getc(input) << 16) + (getc(input) << 24);
		putc((endOfHeaderOffset >> 24) & 0xFF, output);
		putc((endOfHeaderOffset >> 16) & 0xFF, output);
		putc((endOfHeaderOffset >> 8) & 0xFF, output);
		putc(endOfHeaderOffset & 0xFF, output);

		uint32_t endOfHeaderOffset = getc(input) + (getc(input) << 8) + (getc(input) << 16) + (getc(input) << 24);
		putc((endOfHeaderOffset >> 24) & 0xFF, output);
		putc((endOfHeaderOffset >> 16) & 0xFF, output);
		putc((endOfHeaderOffset >> 8) & 0xFF, output);
		putc(endOfHeaderOffset & 0xFF, output);

		uint32_t zero1 = getc(input) + (getc(input) << 8) + (getc(input) << 16) + (getc(input) << 24);
		putc((zero1 >> 24) & 0xFF, output);
		putc((zero1 >> 16) & 0xFF, output);
		putc((zero1 >> 8) & 0xFF, output);
		putc(zero1 & 0xFF, output);

		uint32_t negativeOne = getc(input) + (getc(input) << 8) + (getc(input) << 16) + (getc(input) << 24);
		putc((negativeOne >> 24) & 0xFF, output);
		putc((negativeOne >> 16) & 0xFF, output);
		putc((negativeOne >> 8) & 0xFF, output);
		putc(negativeOne & 0xFF, output);


		// Negative one in XGMA?
		uint32_t zero2 = getc(input) + (getc(input) << 8) + (getc(input) << 16) + (getc(input) << 24);
		putc((zero2 >> 24) & 0xFF, output);
		putc((zero2 >> 16) & 0xFF, output);
		putc((zero2 >> 8) & 0xFF, output);
		putc(zero2 & 0xFF, output);

		uint32_t zero3 = getc(input) + (getc(input) << 8) + (getc(input) << 16) + (getc(input) << 24);
		putc((zero3 >> 24) & 0xFF, output);
		putc((zero3 >> 16) & 0xFF, output);
		putc((zero3 >> 8) & 0xFF, output);
		putc(zero3 & 0xFF, output);

		uint32_t zero4 = getc(input) + (getc(input) << 8) + (getc(input) << 16) + (getc(input) << 24);
		putc((zero4 >> 24) & 0xFF, output);
		putc((zero4 >> 16) & 0xFF, output);
		putc((zero4 >> 8) & 0xFF, output);
		putc(zero4 & 0xFF, output);

		uint32_t zero5 = getc(input) + (getc(input) << 8) + (getc(input) << 16) + (getc(input) << 24);
		putc((zero5 >> 24) & 0xFF, output);
		putc((zero5 >> 16) & 0xFF, output);
		putc((zero5 >> 8) & 0xFF, output);
		putc(zero5 & 0xFF, output);

		uint32_t zero6 = getc(input) + (getc(input) << 8) + (getc(input) << 16) + (getc(input) << 24);
		putc((zero6 >> 24) & 0xFF, output);
		putc((zero6 >> 16) & 0xFF, output);
		putc((zero6 >> 8) & 0xFF, output);
		putc(zero6 & 0xFF, output);

		/// Textures
		for (int j = 0; j < numTextures; ++j) {

			// Unknown/last byte = clamping
			uint32_t clamping = getc(input) + (getc(input) << 8) + (getc(input) << 16) + (getc(input) << 24);
			putc((clamping >> 24) & 0xFF, output);
			putc((clamping >> 16) & 0xFF, output);
			putc((clamping >> 8) & 0xFF, output);
			putc(clamping & 0xFF, output);


			uint16_t tplTextureNumber = getc(input) + (getc(input) << 8);
			putc((tplTextureNumber >> 8) & 0xFF, output);
			putc(tplTextureNumber & 0xFF, output);

			// Probably padding/Unknown
			uint16_t unknown1 = getc(input) + (getc(input) << 8);
			putc((unknown1 >> 8) & 0xFF, output);
			putc(unknown1 & 0xFF, output);

			uint32_t zero1 = getc(input) + (getc(input) << 8) + (getc(input) << 16) + (getc(input) << 24);
			putc((zero1 >> 24) & 0xFF, output);
			putc((zero1 >> 16) & 0xFF, output);
			putc((zero1 >> 8) & 0xFF, output);
			putc(zero1 & 0xFF, output);

			uint32_t unknown2 = getc(input) + (getc(input) << 8) + (getc(input) << 16) + (getc(input) << 24);
			putc((unknown2 >> 24) & 0xFF, output);
			putc((unknown2 >> 16) & 0xFF, output);
			putc((unknown2 >> 8) & 0xFF, output);
			putc(unknown2 & 0xFF, output);


			// Probably padding/Unknown
			uint16_t unknown3 = getc(input) + (getc(input) << 8);
			putc((unknown3 >> 8) & 0xFF, output);
			putc(unknown3 & 0xFF, output);

			uint16_t textureIndex = getc(input) + (getc(input) << 8);
			putc((textureIndex >> 8) & 0xFF, output);
			putc(textureIndex & 0xFF, output);

			uint32_t zero2 = getc(input) + (getc(input) << 8) + (getc(input) << 16) + (getc(input) << 24);
			putc((zero2 >> 24) & 0xFF, output);
			putc((zero2 >> 16) & 0xFF, output);
			putc((zero2 >> 8) & 0xFF, output);
			putc(zero2 & 0xFF, output);

			uint32_t zero3 = getc(input) + (getc(input) << 8) + (getc(input) << 16) + (getc(input) << 24);
			putc((zero3 >> 24) & 0xFF, output);
			putc((zero3 >> 16) & 0xFF, output);
			putc((zero3 >> 8) & 0xFF, output);
			putc(zero3 & 0xFF, output);

			uint32_t zero4 = getc(input) + (getc(input) << 8) + (getc(input) << 16) + (getc(input) << 24);
			putc((zero4 >> 24) & 0xFF, output);
			putc((zero4 >> 16) & 0xFF, output);
			putc((zero4 >> 8) & 0xFF, output);
			putc(zero4 & 0xFF, output);
		}

	}


	fclose(input);
	fclose(output);
}
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

void parseTPL(char* filename);

int main(int argc, char*argv[]) {
	if (argc == 1) {
		return 0;
	}
	std::string filenameParam(argv[1]);

	std::string fileType = filenameParam.substr(filenameParam.length() - 3, 3);

	if (fileType == "tpl") {
		parseTPL(argv[1]);
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
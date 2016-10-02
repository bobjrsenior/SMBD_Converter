#include <stdio.h>
#include <string>
#include <iostream>

typedef struct {
	int encoding;
	int offset;
	short width;
	short height;
	short unknown;
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
	std::string inputFile(filename);

	std::string outputFile = inputFile + ".gc";
	int numTextures = 0;
	int fileSize = 0;
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

	fseek(input, 4, SEEK_SET);


	numTextures = getc(input) + (getc(input) << 8) + (getc(input) << 16) + (getc(input) << 24);

	std::cout << ftell(input) << std::endl;
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

	fseek(output, 16 * numTextures, SEEK_SET);


	for (int i = 0; i < numTextures; i++) {
		newOffsets[i] = ftell(output);
		fseek(input, textures[i].offset, SEEK_SET);

		// 0C Designates texture start
		fseek(input, 4, SEEK_SET);
		// Copy of texture width and height
		fseek(input, 8, SEEK_SET);

		// Unknown
		fseek(input, 40, SEEK_SET);

		while (ftell(input) < fileSize || (i < numTextures - 1 && ftell(input) < textures[i + 1].offset)) {
			putc(getc(input), output);
		}
	}

	std::cout << "COPIED TEXTURES" << std::endl;

	fseek(output, 0, SEEK_SET);
	putc((numTextures >> 24) & 0xFF, output);
	putc((numTextures >> 16) & 0xFF, output);
	putc((numTextures >> 8) & 0xFF, output);
	putc(numTextures & 0xFF, output);

	for (int i = 0; i < numTextures; ++i) {
		int encoding = textures[i].encoding;
		putc((encoding >> 24) & 0xFF, output);
		putc((encoding >> 16) & 0xFF, output);
		putc((encoding >> 8) & 0xFF, output);
		putc(encoding & 0xFF, output);

		int offset = newOffsets[i];
		putc((offset >> 24) & 0xFF, output);
		putc((offset >> 16) & 0xFF, output);
		putc((offset >> 8) & 0xFF, output);
		putc(offset & 0xFF, output);

		short width = textures[i].width;
		putc((width >> 8) & 0xFF, output);
		putc(width & 0xFF, output);

		short height = textures[i].height;
		putc((height >> 8) & 0xFF, output);
		putc(height & 0xFF, output);

		short unknown = textures[i].unknown;
		putc((unknown >> 8) & 0xFF, output);
		putc(unknown & 0xFF, output);

		// 0x1234
		putc(18, output);
		putc(52, output);
	}

	std::cout << "WROTE TEXTURES" << std::endl;
	fclose(input);
	fclose(output);
}
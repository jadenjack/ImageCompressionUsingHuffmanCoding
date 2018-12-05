#pragma once

#include <crtdbg.h>
#include <math.h>
#include <iostream>
#include <vector>
#include <algorithm>
#include <map>
#include <string>
#include <fstream>


using namespace std;

const char* fileName = "lena256.raw";
const char* fileName_compressed = "lena256_compressed.raw";
const char* fileName_out = "lena256_decom.raw";

const int HEIGHT = 256;
const int WIDTH = 256;
const int PIXEL_MAX = 255;

class Tree {
	public:
		Tree() {
			count = 0;
			index = 0;
			left = NULL;
			right = NULL;
		}
		Tree(int _count, int _index) {
			count = _count;
			index = _index;
		}
		Tree(int _count, int _index, Tree* _left, Tree* _right) {
			count = _count;
			index = _index;
			left = _left;
			right = _right;
		}
		int count;
		int index;
		Tree* left;
		Tree* right;
};

bool compareModel(const Tree& left, const Tree& right) {
	return left.count < right.count;
}

void fileRead(const char* filename, unsigned char arr[][WIDTH], int height, int width)
{
	FILE* fp = fopen(filename, "rb");
	for (int h = 0; h < height; h++)
	{
		fread(arr[h], sizeof(unsigned char), width, fp);
	}

	fclose(fp);
}

void fileWrite(const char* filename, unsigned char* arr, int width)
{
	FILE* fp_out = fopen(filename, "wb");
	fwrite(arr, sizeof(unsigned char), width, fp_out);
	fclose(fp_out);
}
void fileWrite(const char* filename, unsigned char** arr, int height, int width)
{
	FILE* fp_out = fopen(filename, "wb");
	for (int h = 0; h < height; h++)
	{
		fwrite(arr[h], sizeof(unsigned char), width, fp_out);
	}

	fclose(fp_out);
}

void treeToTable(map<int, string>* mappingTable, map<string, int>* demappingTable, Tree* head, string bit) {

	if (head == NULL)
		return;

	if (head->index > 0) {
		(*mappingTable)[head->index] = bit;
		(*demappingTable)[bit] = head->index;
	}
	treeToTable(mappingTable, demappingTable, head->left, bit + '0');
	treeToTable(mappingTable, demappingTable, head->right, bit + '1');
}

void makeMappingTable(vector<Tree> hist, map<int, string>* mappingTable, map<string, int>* demappingTable) {
	
	Tree* head = NULL;
	//make tree
	while (true) {
		Tree* left = new Tree(hist.front());
		hist.erase(hist.begin());
		Tree* right = new Tree(hist.front());
		hist.erase(hist.begin());

		Tree* parentTree = new Tree();
		parentTree->count = left->count + right->count;
		parentTree->index = -1;
		parentTree->left = left;
		parentTree->right = right;
		if (hist.size() == 0) {
			head = parentTree;
			break;
		}
		else {
			hist.push_back(*parentTree);
			sort(hist.begin(), hist.end(), compareModel);
		}
	}
	
	treeToTable(mappingTable,demappingTable, head, "");
}

string compress(unsigned char img[][WIDTH], map<int, string>* mappingTable) {
	string compressedImage = "";
	for (int i = 0; i < HEIGHT; i++) {
		for (int j = 0; j < WIDTH; j++) {
			compressedImage += (*mappingTable)[img[i][j]];
		}
	}
	return compressedImage;
}

void decompress(unsigned char** img_decom, string compressedImage, map<string, int>* demappingTable) {
	
	int bitIndex = 0;
	string currentBit = "";

	for (int h = 0; h < HEIGHT; h++) {
		for (int w = 0; w < WIDTH; w++) {

			//find by key
			while (true) {
				currentBit += compressedImage[bitIndex++];
				if ((*demappingTable).count(currentBit) == 1) {
					img_decom[h][w] = (*demappingTable)[currentBit];
					currentBit = "";
					break;
				}
				if (bitIndex >= compressedImage.size())
					return;
			}

		}
	}

}

int main() {

	unsigned char img[HEIGHT][WIDTH];
	vector<Tree> hist;

	cout << "file read" << endl;
	fileRead(fileName, img, HEIGHT, WIDTH);
	for (int i = 0; i <= PIXEL_MAX; i++) {
		Tree newModel(0,i,NULL,NULL);
		hist.push_back(newModel);
	}
	cout << "counting histogram of pixels" << endl;
	for (int i = 0; i < HEIGHT; i++) {
		for (int j = 0; j < WIDTH; j++) {
			hist[img[i][j]].count++;
		}
	}

	sort(hist.begin(), hist.end(), compareModel);
	while (hist.front().count == 0) {
		hist.erase(hist.begin());
	}

	map<int, string>* mappingTable = new map<int,string>;
	map<string, int>* demappingTable = new map<string, int>;
	cout << "making mapping & demapping Table" << endl;
	makeMappingTable(hist,mappingTable, demappingTable);

	cout << "compressiong" << endl;
	string compressedImage = compress(img, mappingTable);

	//compressed string을 비트단위로 저장하기 위한 절차.
	const int compressedBitSize = compressedImage.size() / 8;
	unsigned char* compressedBit = new unsigned char[compressedBitSize];
	memset(compressedBit, 0, compressedBitSize);
	unsigned char c = 0;
	int bitIndex = 0;
	int cIndex = 0;
	for (int i = 0; i < compressedImage.size(); i++) {
		c += (((int)compressedImage[i] - 48) << (7 - cIndex));
		cIndex++;
		if (cIndex >=8 || i == compressedImage.size() - 1) {
			compressedBit[bitIndex] = c;
			c = 0;
			cIndex = 0;
			bitIndex++;
		}
	}
	fileWrite(fileName_compressed, compressedBit, compressedBitSize);


	//비트단위로 저장했던 compressed string을 다시 string으로만든다.
	compressedImage = "";
	for (int i = 0; i < compressedBitSize; i++) {
		for (int j = 0; j < 8; j++) {
			if ((compressedBit[i] / (1 << (7 - j)) == 1)) {
				compressedImage += '1';
				compressedBit[i] -= (1 << (7 - j));
			}
			else {
				compressedImage += '0';
			}
		}
	}

	unsigned char** img_decom;
	img_decom = new unsigned char*[HEIGHT];
	for (int i = 0; i < HEIGHT; i++) {
		img_decom[i] = new unsigned char[WIDTH];
		memset(img_decom[i], 0, sizeof(unsigned char) * WIDTH);
	}
	cout << "decompressing" << endl;
	decompress(img_decom, compressedImage, demappingTable);
	cout << "writing decompressed file" << endl;
	fileWrite(fileName_out, img_decom, HEIGHT, WIDTH);
	return 0;
}
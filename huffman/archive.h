#pragma once
#include "huffman_types.h"

int createTarball(int fileCount, char* files[], const char* tempName);
void extractTarball(const char* tempName);
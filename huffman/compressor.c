#include "compressor.h"
#include "huffman_core.h"

void encodeStream(const char* inputFile, const char* outputFile) {
    FILE* in = fopen(inputFile, "rb");
    if (!in) return;

    HuffmanContext* ctx = (HuffmanContext*)malloc(sizeof(HuffmanContext));
    memset(ctx, 0, sizeof(HuffmanContext));

    unsigned char* ioBuf = (unsigned char*)malloc(BUF_SIZE);
    size_t bytesRead;

    while ((bytesRead = fread(ioBuf, 1, BUF_SIZE, in)) > 0) {
        for (size_t i = 0; i < bytesRead; i++) {
            ctx->freq[ioBuf[i]]++;
            ctx->totalChars++;
        }
    }

    if (ctx->totalChars == 0) {
        fclose(in); free(ioBuf); free(ctx);
        FILE* emptyOut = fopen(outputFile, "wb");
        if (emptyOut) fclose(emptyOut);
        return;
    }

    buildHuffmanEnvironment(ctx);
    rewind(in);

    FILE* out = fopen(outputFile, "wb");

    fwrite(&ctx->totalChars, sizeof(uint32_t), 1, out);
    fwrite(ctx->freq, sizeof(unsigned int), MAX_CHAR, out);

    unsigned char* outBuf = (unsigned char*)malloc(BUF_SIZE);
    size_t outPos = 0;
    unsigned char bitBuffer = 0; int bitCount = 0;

    while ((bytesRead = fread(ioBuf, 1, BUF_SIZE, in)) > 0) {
        for (size_t i = 0; i < bytesRead; i++) {
            char* c = ctx->codes[ioBuf[i]];
            for (int j = 0; c[j] != '\0'; j++) {
                bitBuffer = (bitBuffer << 1) | (c[j] - '0');
                bitCount++;
                if (bitCount == 8) {
                    outBuf[outPos++] = bitBuffer;
                    bitBuffer = 0; bitCount = 0;
                    if (outPos == BUF_SIZE) {
                        fwrite(outBuf, 1, BUF_SIZE, out);
                        outPos = 0;
                    }
                }
            }
        }
    }

    if (bitCount > 0) {
        bitBuffer <<= (8 - bitCount);
        outBuf[outPos++] = bitBuffer;
    }
    if (outPos > 0) {
        fwrite(outBuf, 1, outPos, out);
    }

    fclose(in); fclose(out);
    freeTree(ctx->root);
    free(ioBuf); free(outBuf); free(ctx);
}

void decodeStream(const char* inputFile, const char* outputFile) {
    FILE* in = fopen(inputFile, "rb");
    if (!in) return;

    fseek(in, 0, SEEK_END);
    if (ftell(in) == 0) {
        fclose(in);
        FILE* emptyOut = fopen(outputFile, "wb");
        if (emptyOut) fclose(emptyOut);
        return;
    }
    rewind(in);

    HuffmanContext* ctx = (HuffmanContext*)malloc(sizeof(HuffmanContext));
    memset(ctx, 0, sizeof(HuffmanContext));

    if (fread(&ctx->totalChars, sizeof(uint32_t), 1, in) != 1) { fclose(in); free(ctx); return; }
    if (fread(ctx->freq, sizeof(unsigned int), MAX_CHAR, in) != MAX_CHAR) { fclose(in); free(ctx); return; }

    buildHuffmanEnvironment(ctx);

    FILE* out = fopen(outputFile, "wb");
    Node* current = ctx->root;

    unsigned char* inBuf = (unsigned char*)malloc(BUF_SIZE);
    unsigned char* outBuf = (unsigned char*)malloc(BUF_SIZE);
    size_t outPos = 0, bytesRead;
    uint32_t count = 0;

    while (count < ctx->totalChars && (bytesRead = fread(inBuf, 1, BUF_SIZE, in)) > 0) {
        for (size_t j = 0; j < bytesRead && count < ctx->totalChars; j++) {
            unsigned char ch = inBuf[j];
            for (int i = 7; i >= 0 && count < ctx->totalChars; i--) {
                int bit = (ch >> i) & 1;
                current = bit ? current->right : current->left;

                if (!current) {
                    printf("\n[严重错误] 数据流损坏，终止解压！\n");
                    goto FINISH_DECODE;
                }

                if (!current->left && !current->right) {
                    outBuf[outPos++] = current->ch;
                    current = ctx->root;
                    count++;
                    if (outPos == BUF_SIZE) {
                        fwrite(outBuf, 1, BUF_SIZE, out);
                        outPos = 0;
                    }
                }
            }
        }
    }

FINISH_DECODE:
    if (outPos > 0) fwrite(outBuf, 1, outPos, out);
    fclose(in); fclose(out);
    freeTree(ctx->root);
    free(inBuf); free(outBuf); free(ctx);
}
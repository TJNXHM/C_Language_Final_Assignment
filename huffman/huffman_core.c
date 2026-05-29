#include "huffman_core.h"

static Node* createNode(unsigned char ch, unsigned int freq) {
    Node* node = (Node*)malloc(sizeof(Node));
    if (!node) return NULL;
    node->ch = ch; node->freq = freq;
    node->left = node->right = NULL;
    return node;
}

static MinHeap* createMinHeap(int capacity) {
    MinHeap* heap = (MinHeap*)malloc(sizeof(MinHeap));
    if (!heap) return NULL;
    heap->data = (Node**)malloc(capacity * sizeof(Node*));
    heap->size = 0;
    return heap;
}

static void swapNode(Node** a, Node** b) { Node* temp = *a; *a = *b; *b = temp; }

static void heapify(MinHeap* heap, int i) {
    int smallest = i;
    int left = 2 * i + 1, right = 2 * i + 2;
    if (left < heap->size && heap->data[left]->freq < heap->data[smallest]->freq) smallest = left;
    if (right < heap->size && heap->data[right]->freq < heap->data[smallest]->freq) smallest = right;
    if (smallest != i) {
        swapNode(&heap->data[i], &heap->data[smallest]);
        heapify(heap, smallest);
    }
}

static Node* extractMin(MinHeap* heap) {
    if (heap->size == 0) return NULL;
    Node* temp = heap->data[0];
    heap->data[0] = heap->data[heap->size - 1];
    heap->size--;
    heapify(heap, 0);
    return temp;
}

static void insertHeap(MinHeap* heap, Node* node) {
    int i = heap->size++;
    heap->data[i] = node;
    while (i && heap->data[i]->freq < heap->data[(i - 1) / 2]->freq) {
        swapNode(&heap->data[i], &heap->data[(i - 1) / 2]);
        i = (i - 1) / 2;
    }
}

static void generateCodesRecursive(Node* root, char* code, int depth, HuffmanContext* ctx) {
    if (!root) return;
    if (!root->left && !root->right) {
        code[depth] = '\0';
        strcpy(ctx->codes[root->ch], code);
        return;
    }
    code[depth] = '0'; generateCodesRecursive(root->left, code, depth + 1, ctx);
    code[depth] = '1'; generateCodesRecursive(root->right, code, depth + 1, ctx);
}

void buildHuffmanEnvironment(HuffmanContext* ctx) {
    MinHeap* heap = createMinHeap(MAX_CHAR);
    for (int i = 0; i < MAX_CHAR; i++) {
        if (ctx->freq[i] > 0) {
            insertHeap(heap, createNode((unsigned char)i, ctx->freq[i]));
        }
    }
    if (heap->size == 0) { free(heap->data); free(heap); return; }
    if (heap->size == 1) {
        Node* singleNode = extractMin(heap);
        Node* top = createNode(0, singleNode->freq);
        top->left = singleNode;
        insertHeap(heap, top);
    }
    while (heap->size > 1) {
        Node* left = extractMin(heap);
        Node* right = extractMin(heap);
        Node* top = createNode(0, left->freq + right->freq);
        top->left = left; top->right = right;
        insertHeap(heap, top);
    }
    ctx->root = extractMin(heap);
    free(heap->data); free(heap);
    char code[MAX_CHAR];
    generateCodesRecursive(ctx->root, code, 0, ctx);
}

void freeTree(Node* root) {
    if (!root) return;
    freeTree(root->left); freeTree(root->right);
    free(root);
}
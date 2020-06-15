#ifndef __TREE_H__
#define __TREE_H__

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <arpa/inet.h>

typedef struct node {
    int index;
    struct node *left;  //Bit de 0;
    struct node *right; //Bit de 1;
} Tree;

Tree *initNode();
Tree *init_tree();
int get_bit_from_position(uint32_t n, int i);
int get_number_of_bits(uint32_t n);
Tree *add_node(Tree *root, int index, uint32_t n, int get_number_of_bits, int get_bit_from_position);
Tree *deleteTree(Tree *root);
void longestMatchingPrefix(Tree *root, int *final_index, uint32_t n, int current_level);

#endif
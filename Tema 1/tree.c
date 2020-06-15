#include "tree.h"

Tree *initNode() {
    Tree *node = (Tree*)malloc(sizeof(struct node));
    node->index = -1;
    node->left = NULL;
    node->right = NULL;
    return node;
} //Creates an empty node

Tree *init_tree(Tree *root) {
    Tree *new_node = initNode();
    root = new_node;
    return root;
} //Creates an empty tree

int get_bit_from_position (uint32_t n,int i) {
    uint32_t mask = 1;
    if(i > 32)
        return 0;
    mask = mask << (32 - i);
    if ((n & mask) == 0)
        return 0;
    else
        return 1;
} //Returns the bit from the given positon (1 for MSB, 32 for LSB)

int get_number_of_bits(uint32_t n) {
    int count = 0;
    while(n) {
        count += n & 1;
        n >>= 1;
    }
    return count;
} //Returns then number of bits set to 1

Tree *add_node(Tree *root, int index, uint32_t n, int number_of_bits, int position) {

    if(position == number_of_bits) {
        root->index = index;
        return root;
    }
    int bit = get_bit_from_position(n, position + 1);
    if(bit == 0) {
        if(root->left == NULL)
            root->left = initNode();
        root->left = add_node(root->left, index, n, number_of_bits, position + 1);
    }
    else if(bit == 1){
        if(root->right == NULL)
            root->right = initNode();
        root->right = add_node(root->right, index , n, number_of_bits, position + 1);
    }
    return root;
} //Adds a node to a specific level in the tree based on the number_of_bits

Tree* deleteTree(Tree *root) {
    if(root == NULL)
        return root;
    root->left = deleteTree(root->left);
    root->right = deleteTree(root->right);
    free(root);
    return NULL;
} //Deletes the entire tree

void longestMatchingPrefix(Tree *root, int *final_index, uint32_t n, int current_level) {
    int bit = get_bit_from_position(n, current_level + 1);
    if(root->index != -1)
        *final_index=root->index;
    if(current_level == 32)
        return;
    if(root == NULL)
        return;
    if(bit == 0) {
        if(root->left != NULL) {
            longestMatchingPrefix(root->left, final_index,n, current_level + 1); 
             
        }
        return;
    }
    if(bit == 1) {
        if(root->right != NULL) {
            longestMatchingPrefix(root->right, final_index,n, current_level + 1);
        }
        return; 
    }
} //Returns the index in the route table which matches the best with the address n
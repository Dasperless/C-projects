#ifndef BST_H
#define BST_H

#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct BSTNode {
    int key;
    int value;
    struct BSTNode *left;
    struct BSTNode *right;
} BSTNode;

typedef struct BST {
    BSTNode *root;
    size_t size;
} BST;

typedef void (*bst_visit_fn)(int key, int value);

BST *bst_create(void);
void bst_destroy(BST *tree);

/* Inserts key/value. An existing key has its value updated (size unchanged). */
bool bst_insert(BST *tree, int key, int value);
bool bst_search(const BST *tree, int key, int *out_value);
/* Deletes key. Handles leaf, one-child and two-child (in-order successor) cases. */
bool bst_delete(BST *tree, int key);

void bst_inorder(const BST *tree, bst_visit_fn visit);
void bst_preorder(const BST *tree, bst_visit_fn visit);
void bst_postorder(const BST *tree, bst_visit_fn visit);

/* Height in nodes on the longest root-to-leaf path; empty tree is 0. */
size_t bst_height(const BST *tree);
size_t bst_size(const BST *tree);

#ifdef __cplusplus
}
#endif

#endif /* BST_H */

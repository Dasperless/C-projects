#include "bst.h"

#include <stdlib.h>

BST *bst_create(void) {
    return (BST *)calloc(1, sizeof(BST));
}

static void free_subtree(BSTNode *n) {
    if (n == NULL) {
        return;
    }
    free_subtree(n->left);
    free_subtree(n->right);
    free(n);
}

void bst_destroy(BST *tree) {
    if (tree == NULL) {
        return;
    }
    free_subtree(tree->root);
    free(tree);
}

bool bst_insert(BST *tree, int key, int value) {
    if (tree == NULL) {
        return false;
    }
    BSTNode **link = &tree->root;
    while (*link != NULL) {
        if (key == (*link)->key) {
            (*link)->value = value;
            return true;
        }
        link = (key < (*link)->key) ? &(*link)->left : &(*link)->right;
    }
    BSTNode *n = (BSTNode *)malloc(sizeof(BSTNode));
    if (n == NULL) {
        return false;
    }
    n->key = key;
    n->value = value;
    n->left = NULL;
    n->right = NULL;
    *link = n;
    tree->size++;
    return true;
}

bool bst_search(const BST *tree, int key, int *out_value) {
    if (tree == NULL) {
        return false;
    }
    const BSTNode *cur = tree->root;
    while (cur != NULL) {
        if (key == cur->key) {
            if (out_value != NULL) {
                *out_value = cur->value;
            }
            return true;
        }
        cur = (key < cur->key) ? cur->left : cur->right;
    }
    return false;
}

bool bst_delete(BST *tree, int key) {
    if (tree == NULL) {
        return false;
    }
    BSTNode **link = &tree->root;
    while (*link != NULL && (*link)->key != key) {
        link = (key < (*link)->key) ? &(*link)->left : &(*link)->right;
    }
    BSTNode *victim = *link;
    if (victim == NULL) {
        return false;
    }
    if (victim->left == NULL) {
        *link = victim->right; /* case 0 and case 1 (right child only) */
    } else if (victim->right == NULL) {
        *link = victim->left; /* case 1 (left child only) */
    } else {
        /* case 2: detach the in-order successor (leftmost of right subtree) */
        BSTNode **succ_link = &victim->right;
        while ((*succ_link)->left != NULL) {
            succ_link = &(*succ_link)->left;
        }
        BSTNode *succ = *succ_link;
        *succ_link = succ->right;
        succ->left = victim->left;
        succ->right = victim->right;
        *link = succ;
    }
    free(victim);
    tree->size--;
    return true;
}

static void inorder(const BSTNode *n, bst_visit_fn visit) {
    if (n == NULL) {
        return;
    }
    inorder(n->left, visit);
    visit(n->key, n->value);
    inorder(n->right, visit);
}

static void preorder(const BSTNode *n, bst_visit_fn visit) {
    if (n == NULL) {
        return;
    }
    visit(n->key, n->value);
    preorder(n->left, visit);
    preorder(n->right, visit);
}

static void postorder(const BSTNode *n, bst_visit_fn visit) {
    if (n == NULL) {
        return;
    }
    postorder(n->left, visit);
    postorder(n->right, visit);
    visit(n->key, n->value);
}

void bst_inorder(const BST *tree, bst_visit_fn visit) {
    if (tree != NULL && visit != NULL) {
        inorder(tree->root, visit);
    }
}

void bst_preorder(const BST *tree, bst_visit_fn visit) {
    if (tree != NULL && visit != NULL) {
        preorder(tree->root, visit);
    }
}

void bst_postorder(const BST *tree, bst_visit_fn visit) {
    if (tree != NULL && visit != NULL) {
        postorder(tree->root, visit);
    }
}

static size_t height(const BSTNode *n) {
    if (n == NULL) {
        return 0;
    }
    size_t l = height(n->left);
    size_t r = height(n->right);
    return 1 + (l > r ? l : r);
}

size_t bst_height(const BST *tree) {
    return tree == NULL ? 0 : height(tree->root);
}

size_t bst_size(const BST *tree) {
    return tree == NULL ? 0 : tree->size;
}

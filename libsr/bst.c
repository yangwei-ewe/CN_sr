/**
 * @file bst.c
 * @author yangwei-ewe (jaychiang0127@gmail.com)
 * @brief
 * @version 0.1
 * @date 2024-06-14
 *
 * @copyright @楊偉 Copyright 2024
 */

#include "bst.h"
#include "defines.h"  // max() min() and WND_SIZE
#include "sr.h"

 /**
  * @brief a private function to alloc bst_node*
  *
  * @param pkg pkg
  * @return bst_node*
  */
bst_node* _bst_mknode(sr_pkg* pkg);

void bst_init(bst_t** bst, sr_obj* obj) {
    if (*bst) {
        free(*bst);
    }
    TEST((*bst = (bst_t*)malloc(sizeof(bst_t))) == NULL, "bst_init(): failed");
    **bst = (bst_t){ NULL, obj->wnd_size, 0, PTHREAD_MUTEX_INITIALIZER };
    return;
}

void bst_update_wnd_size(bst_t* bst, int wnd_size) {
    pthread_mutex_lock(&bst->bst_lock);
    bst->def_wnd_size = wnd_size;
    pthread_mutex_unlock(&bst->bst_lock);
    return;
}

void bst_push(bst_t* bst, sr_pkg* pkg) {
    if (bst->cur_wnd_size >= bst->def_wnd_size) {
        fputs("bst if full", stderr);
        return;
    }
    bst_node* new_node = _bst_mknode(pkg);
    pthread_mutex_lock(&bst->bst_lock);
    if (bst->cur_wnd_size == 0) {
        bst->root = new_node;
        bst->cur_wnd_size++;
        pthread_mutex_unlock(&bst->bst_lock);
        return;
    }

    bst_node* current = bst->root;
    bst_node* parent = NULL;
    while (current != NULL) {
        parent = current;
        if (pkg->seq_num < current->pkg->seq_num) {
            current = current->left;
        }
        else {
            current = current->right;
        }
    }
    if (pkg->seq_num < parent->pkg->seq_num) {
        parent->left = new_node;
    }
    else {
        parent->right = new_node;
    }
    bst->cur_wnd_size++;
    pthread_mutex_unlock(&bst->bst_lock);
    return;
}

sr_pkg* bst_pop(bst_t* bst) {
    TEST((bst == NULL), "bst_pop(): is null, where it shouldn't be.");
    if (bst->cur_wnd_size == 0) {
        return NULL;
    }
    pthread_mutex_lock(&bst->bst_lock);
    bst_node* parent = NULL;
    bst_node* curr = bst->root;

    while (curr->left != NULL) {
        parent = curr;
        curr = curr->left;
    }

    if (parent == NULL) {
        bst->root = curr->right;
    }
    else {
        parent->left = curr->right;
    }
    bst->cur_wnd_size--;
    pthread_mutex_unlock(&bst->bst_lock);
    sr_pkg* pkg = curr->pkg;
    free(curr);
    return pkg;
}

sr_pkg* bst_find(bst_t* bst, int seq_num) {
    pthread_mutex_lock(&bst->bst_lock);
    bst_node* ptr = bst->root;

    while (ptr != NULL) {
        if (seq_num == ptr->pkg->seq_num) {
            pthread_mutex_unlock(&bst->bst_lock);
            return ptr->pkg;
        }
        else if (seq_num < ptr->pkg->seq_num) {
            ptr = ptr->left;
        }
        else {
            ptr = ptr->right;
        }
    }
    pthread_mutex_unlock(&bst->bst_lock);
    return NULL;
}

int bst_is_pushable(bst_t* bst) {
    pthread_mutex_lock(&bst->bst_lock);
    if (bst->cur_wnd_size < bst->def_wnd_size) {
        pthread_mutex_unlock(&bst->bst_lock);
        return 1;
    }
    pthread_mutex_unlock(&bst->bst_lock);
    return 0;
}

void _free_node(bst_node* node) {
    if (node == NULL) {
        return;
    }
    _free_node(node->left);
    _free_node(node->right);
    free(node);
    return;
}

void free_bst(bst_t** bst) {
    pthread_mutex_lock(&(*bst)->bst_lock);
    _free_node((*bst)->root);
    pthread_mutex_unlock(&(*bst)->bst_lock);
    pthread_mutex_destroy(&(*bst)->bst_lock);
    free(*bst);
    *bst = NULL;
    return;
}

bool bst_is_not_empty(bst_t* bst) {
    return bst->cur_wnd_size;
}

inline bst_node* _bst_mknode(sr_pkg* pkg) {
    bst_node* new_node;
    TEST((new_node = (bst_node*)malloc(sizeof(bst_node))) == NULL, "bst_mknode: failed to allocate memory");
    *new_node = (bst_node){ pkg, NULL, NULL };
    return new_node;
}

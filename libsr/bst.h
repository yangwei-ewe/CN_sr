/**
 * @file bst.h
 * @author yangwei-ewe (jaychiang0127@gmail.com)
 * @brief
 * @version 0.1
 * @date 2024-06-23
 *
 * @copyright @楊偉 Copyright 2024
 */
#ifndef __BST_H_
#define __BST_H_
#include <pthread.h>
#include <stdbool.h>

typedef struct bst_node bst_node;
typedef struct sr_pkg sr_pkg;
typedef struct sr_obj sr_obj;

typedef struct bst_node {
    sr_pkg* pkg;
    bst_node* left;
    bst_node* right;
} bst_node;

typedef struct bst_t {
    bst_node* root;
    size_t def_wnd_size;
    size_t cur_wnd_size;
    pthread_mutex_t bst_lock;
} bst_t;

/**
 * @brief binary search tree initalzer, please make sure to call free_bst() before terminal.
 *
 * @param bst pointer to store bst handle
 * @param obj a pointer to sr_obj
 */
void bst_init(bst_t** bst, sr_obj* obj);

/**
 * @brief Updates the window size of the bst.
 *
 * @param bst a pointer to the bst handle.
 * @param wnd_size the new window size to be updated.
 */
void bst_update_wnd_size(bst_t* bst, int wnd_size);

/**
 * @brief free the bst
 *
 * @param bst a pointer to the bst handle.
 */
void free_bst(bst_t** bst);

/**
 * @brief push packge into bst
 *
 * @param bst a pointer to the bst handle.
 * @param pkg a pointer to the pkg.
 */
void bst_push(bst_t* bst, sr_pkg* pkg);

/**
 * @brief find the pkg in bst
 *
 * @param bst a pointer to the bst handle
 * @param seq_num sequence number
 * @return the pointer to pkg, NULL if unfound
 */
sr_pkg* bst_find(bst_t* bst, int seq_num);

/**
 * @brief Removes and returns the smallest sequence number package from the binary search tree.
 *
 * @param bst A pointer to the bst handle.
 * @return A pointer to the removed package with the smallest sequence number, it should not be failed.
 */
sr_pkg* bst_pop(bst_t* bst);

/**
 * @brief a bool function to check bst is reach wnd size or not.
 *
 * @param bst
 * @return int
 */
int bst_is_pushable(bst_t* bst);

/**
 * @brief Checks if the binary search tree (BST) is not empty.
 *
 * @param bst A pointer to the BST handle.
 * @return True if the bst is not empty, false otherwise.
 */
bool bst_is_not_empty(bst_t* bst);
#endif
#ifndef _NODE_H
#define _NODE_H

#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

/* arg list */
typedef struct ArgList ArgList;
struct ArgList {
    int idx, capacity;
    char *val[10];
};

ArgList *new_arg_list();
void push_arg(ArgList *list, char *arg);

/* node */
typedef struct Node Node;
struct Node {
    ArgList *args;
    bool in_append, out_append;
    char *in_path, *out_path;

    Node *next;
};

Node *new_node(ArgList *args, char *in_path, char *out_path, bool in_append, bool out_append);

/* node linked list */
typedef struct NodeList NodeList;
struct NodeList {
    Node *begin, *end;
};

NodeList *new_node_list();
void free_node_list(NodeList *list);
void push_node(NodeList *list, Node *new_node);

/* function declaration */
void print_tree(NodeList *cur_node);

#endif
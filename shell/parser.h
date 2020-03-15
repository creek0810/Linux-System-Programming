#ifndef _PARSER_H
#define _PARSER_H

#include <stdbool.h>
#include "scanner.h"

/* type def */
typedef struct Node Node;
typedef enum NodeType NodeType;
typedef struct NodeList NodeList;

struct Node {
    // ND_COMMAND
    char **arg;
    bool in_append, out_append;
    char *in_path, *out_path;

    // ND_PIPE
    Node *lhs, *rhs;
};

struct NodeList {
    Node *content;
    NodeList* next;
};

/* function declaration */
void print_tree(NodeList *cur_node);
NodeList *parse(Token *head);

#endif
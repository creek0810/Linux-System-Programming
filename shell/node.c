#include "node.h"

/* node function */
Node *new_node(ArgList *args, char *in_path, char *out_path, bool in_append, bool out_append) {
    Node *cur_node = calloc(1, sizeof(Node));

    cur_node->args = args;
    cur_node->in_append = in_append;
    cur_node->out_append = out_append;
    cur_node->in_path = in_path;
    cur_node->out_path = out_path;
    return cur_node;
}

Node *free_node(Node *cur_node) {
    // TODO: free args
    free(cur_node->in_path);
    free(cur_node->out_path);

    Node *next = cur_node->next;
    free(cur_node);
    return next;
}

/* node list function */
NodeList *new_node_list() {
    return calloc(1, sizeof(NodeList));
}

void push_node(NodeList *list, Node *new_node) {
    if(!(list->begin)) {
        list->begin = new_node;
        list->end = new_node;
    } else {
        list->end->next = new_node;
        list->end = new_node;
    }
}

void free_node_list(NodeList *list) {
    Node *cur_node = list->begin;
    while(cur_node) {
        cur_node = free_node(cur_node);
    }
    free(list);
}

/* arg list function */
ArgList *new_arg_list() {
    ArgList *cur_list = calloc(1, sizeof(ArgList));
    cur_list->idx = 0;
    cur_list->capacity = 2;
    // cur_list->val = calloc(1, sizeof(char*) * cur_list->capacity);
    return cur_list;
}

void push_arg(ArgList *list, char *arg) {
    // TODO: realloc
    if(list->idx >= list->capacity) {
    }
    list->val[(list->idx)++] = arg;
}

/* help function */

bool starts_with(char *str, char *tar) {
    return strcmp(str, tar) == 0;
}

char *convert_str_to_word(char *str) {
    // buffer = 50
    int str_len = strlen(str);
    char *word = calloc(1, sizeof(char) * (str_len + 50));
    // start from 1 and end at str_len - 1 to skip "
    for(int i=1, w=0; i<str_len-1; i++, w++) {
        word[w] = str[i];
    }
    return word;
}

/* parse function */
/*
char **parse_arg() {
    char **arg = calloc(10, sizeof(char*));
    int cnt = 0;
    // TODO: realloc if exceed 10
    while(cur_token) {
        if(cur_token->type == TK_OP) {
            break;
        }
        if(cur_token->type == TK_STR) {
            arg[cnt++] = convert_str_to_word(cur_token->str);
        } else {
            arg[cnt++] = cur_token->str;
        }
        cur_token = cur_token->next;
    }
    return arg;
}
*/

/* debug function */
void print_cmd(Node *cur_node) {
    printf("command: ");
    for(int i=0 ;i<10; i++) {
        if(!cur_node->args->val[i]) break;
        printf(" %s", cur_node->args->val[i]);
    }
    printf("\n");
    if(cur_node->in_path) printf("in: %s\n", cur_node->in_path);
    if(cur_node->out_path) printf("out: %s\n", cur_node->out_path);
    printf("\n");
}

void print_tree(NodeList *list) {
    printf("-----------parser------------\n");
    Node *cur_node = list->begin;
    while(cur_node) {
        print_cmd(cur_node);
        cur_node = cur_node->next;
    }
    printf("\n");
}


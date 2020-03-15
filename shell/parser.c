#include "parser.h"


/* global */
Token *cur_token = NULL;

/* node list function */
NodeList *append_node_list(NodeList *cur_node, Node *new_node_content) {
    NodeList *new_node = calloc(1, sizeof(NodeList));
    new_node->content = new_node_content;
    if(cur_node) cur_node->next = new_node;
    return new_node;
}

/* new node function */
Node *new_cmd_node(char **arg) {
    Node *new_node = calloc(1, sizeof(Node));
    new_node->arg = arg;
    return new_node;
}

/* help function */
void next_token() {
    cur_token = cur_token->next;
}

bool starts_with(char *str, char *tar) {
    return strcmp(str, tar) == 0;
}

char *convert_str_to_word(char *str) {
    // buffer = 50
    int str_len = strlen(str);
    char *word = calloc(1, sizeof(char) * (str_len + 50));
    // start from 1 and end at str_len - 1 to skip "
    for(int i=1, w=0; i<str_len-1; i++, w++) {
        if(str[i] == ' ') {
            word[w++] = '\\';
        }
        word[w] = str[i];
    }
    return word;
}

/* parse function */
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

/*
    <command> {(<|>|<<|>>) <file>}*
*/
Node *parse_cmd() {
    Node *cur_node = new_cmd_node(parse_arg());
    // TODO: add | token type
    while(cur_token && cur_token->str[0] != '|') {
        if(starts_with(cur_token->str, ">>")) {
            next_token();
            cur_node->out_path = cur_token->str;
            cur_node->out_append = true;
            continue;
        }
        if(starts_with(cur_token->str, ">")) {
            next_token();
            cur_node->out_path = cur_token->str;
            continue;
        }
        if(starts_with(cur_token->str, "<<")) {
            next_token();
            cur_node->in_path = cur_token->str;
            cur_node->in_append = true;
            continue;
        }
        if(starts_with(cur_token->str, "<")) {
            next_token();
            cur_node->in_path = cur_token->str;
            continue;
        }
        next_token();
    }
    return cur_node;
}

/*
    <pipe> <cmd>
*/
NodeList *parse_pipe() {
    NodeList *head = append_node_list(NULL, parse_cmd());
    NodeList *cur_node = head;
    while(cur_token && cur_token->str[0] == '|') {
        next_token();
        cur_node = append_node_list(cur_node, parse_cmd());
    }
    return head;
}

/* debug function */
void print_cmd(Node *cur_node) {
    printf("command: ");
    for(int i=0 ;i<10; i++) {
        if(!cur_node->arg[i]) break;
        printf(" %s", cur_node->arg[i]);
    }
    printf("\n");
    if(cur_node->in_path) printf("in: %s\n", cur_node->in_path);
    if(cur_node->out_path) printf("out: %s\n", cur_node->out_path);
    printf("\n");
}

void print_tree(NodeList *cur_node) {
    printf("-----------parser------------\n");
    while(cur_node) {
        print_cmd(cur_node->content);
        cur_node = cur_node->next;
    }
    printf("\n");
}

NodeList *parse(Token *head) {
    cur_token = head;
    // start pare
    return parse_pipe();
}

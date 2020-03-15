#ifndef _SCANNER_H
#define _SCANNER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pwd.h>
#include <unistd.h>

/* type def */
typedef struct Token Token;
typedef enum TokenType TokenType;
enum TokenType {
    TK_WORD,
    TK_OP,
    TK_STR
};

struct Token {
    TokenType type;
    char *str;
    Token *next;
};

/* function declaration */
Token *tokenize(char *cmd);
void print_token(Token *head);

#endif
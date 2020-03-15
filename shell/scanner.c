#include "scanner.h"

Token *new_token(TokenType type, Token **prev_token, char *str, int len) {
    Token *cur_token = calloc(1, sizeof(Token));
    cur_token->type = type;
    // warning: don't forget to calc '\0'
    cur_token->str = calloc(1, sizeof(char) * (len + 1));
    strncpy(cur_token->str, str, len);
    if(*prev_token) (*prev_token)->next = cur_token;
    (*prev_token) = cur_token;
    return cur_token;
}

int op(char *cmd, int loc, int len) {
    if(cmd[loc] == '|') {
        return loc + 1;
    }
    // we only need to split << and >>
    if(loc + 1 < len && cmd[loc] == cmd[loc + 1]) return loc + 2;
    return loc + 1;
}

int word(char *cmd, int loc, int len) {
    while(loc < len) {
        if(cmd[loc] == '|' || cmd[loc] == '<' ||
           cmd[loc] == '>' || cmd[loc] == ' '
        ) {
            break;
        }
        loc ++;
    }
    return loc;
}

int str(char *cmd, int loc, int len) {
    loc ++;
    while(loc < len && cmd[loc] != '\"') {
        loc ++;
    }
    return loc + 1;
}

Token *tokenize(char *cmd) {
    int cur_loc = 0;
    int str_len = strlen(cmd);
    Token *head = NULL, *cur_token = NULL;
    while(cur_loc < str_len) {
        int new_loc;
        switch(cmd[cur_loc]) {
            case ' ': // skip
                cur_loc++;
                break;
            case '<':
            case '>':
            case '|': {
                int new_loc = op(cmd, cur_loc, str_len);
                Token *token = new_token(TK_OP, &cur_token, cmd + cur_loc, new_loc - cur_loc);
                if(!head) head = token;
                cur_loc = new_loc;
                break;
            }
            case '\"': {
                int new_loc = str(cmd, cur_loc, str_len);
                Token *token = new_token(TK_STR, &cur_token, cmd + cur_loc, new_loc - cur_loc);
                if(!head) head = token;
                cur_loc = new_loc;
                break;
            }
            default: {
                int new_loc = word(cmd, cur_loc, str_len);
                Token *token = new_token(TK_WORD, &cur_token, cmd + cur_loc, new_loc - cur_loc);
                if(!head) head = token;
                cur_loc = new_loc;
                break;
            }
        }
    }
    return head;
}

/* debug function */
void print_token(Token *head) {
    printf("-----------scanner---------\n");
    while(head) {
        printf("%d: %s\n", head->type, head->str);
        head = head->next;
    }
}
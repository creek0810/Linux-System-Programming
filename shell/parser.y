%{
    #include <stdio.h>
    #include <stdbool.h>
    #include <stdlib.h>
    #include <unistd.h>
    #include <fcntl.h>

    #include "node.h"
    #include "interactive.h"

    #define PIPE_SIZE 50

    extern bool IS_INTERACTIVE;
    extern int yylex();
    extern FILE *yyin;
    extern int yylineno;

    bool IS_EXECUTING = false;
    /* pipe idx */
    #define READ 0
    #define WRITE 1
    bool HAS_CHILD = false;

    void run_cmd(NodeList *list);
    void free_node_list(NodeList *list);
    char *expand_quote(char *str);
    void yyerror(const char* msg) {
        printf("ash: %s", msg);
    }
%}

%union {
    char *str;
    char **args;
    NodeList *nodeList;
    Node *node;
    ArgList *argList;
}

// redirection
%token RED_IN RED_OUT RED_A_OUT

// pipe
%token PIPE

// basic type
%token <str>QUOTE

%token EOL

/* non terminal */
%type <nodeList> stmt_list
%type <node> redirection
%type <str> path
%type <argList> cmd

%%
program:
       | error EOL {
           yyerrok;
           if(IS_INTERACTIVE) {
               printf("\n");
               print_bar();
           }
       }
       | program EOL {
           if(IS_INTERACTIVE) {
               print_bar();
           }
       }
       | program stmt_list EOL {
           run_cmd($2);
           free_node_list($2);
       }
       ;

stmt_list: redirection { $$ = new_node_list(); push_node($$, $1); }
         | stmt_list PIPE redirection { push_node($$, $3); }
         ;


redirection: cmd { $$ = new_node($1, NULL, NULL, false, false); }
           | cmd RED_IN path { $$ = new_node($1, $3, NULL, false, false); }
           | cmd RED_OUT path { $$ = new_node($1, NULL, $3, false, false); }
           | cmd RED_A_OUT path { $$ = new_node($1, NULL, $3, false, true); }

           | cmd RED_IN path RED_OUT path { $$ = new_node($1, $3, $5, false, false); }
           | cmd RED_OUT path RED_IN path { $$ = new_node($1, $5, $3, false, false); }

           | cmd RED_IN path RED_A_OUT path { $$ = new_node($1, $3, $5, false, true); }
           | cmd RED_A_OUT path RED_IN path { $$ = new_node($1, $5, $3, false, true); }
           ;

path: QUOTE { $$ = expand_quote($1); }
    ;

cmd: QUOTE { $$ = new_arg_list(); push_arg($$, expand_quote($1)); }
   | cmd QUOTE{ push_arg($$, expand_quote($2)); }
   ;


%%

bool is_ansi_esc() {
    /*
    \a
    alert (bell)

    \b
    backspace

    \e
    \E
    an escape character (not ANSI C)

    \f
    form feed

    \n
    newline

    \r
    carriage return

    \t
    horizontal tab

    \v
    vertical tab

    \\
    backslash

    \'
    single quote

    \"
    double quote

    \?
    question mark

    \nnn
    the eight-bit character whose value is the octal value nnn (one to three octal digits)

    \xHH
    the eight-bit character whose value is the hexadecimal value HH (one or two hex digits)

    \uHHHH
    the Unicode (ISO/IEC 10646) character whose value is the hexadecimal value HHHH (one to four hex digits)

    \UHHHHHHHH
    the Unicode (ISO/IEC 10646) character whose value is the hexadecimal value HHHHHHHH (one to eight hex digits)

    \cx
    a control-x character
    */



}

/* execute function */
char* expand_quote(char *str) {
    /* TODO: support special char */
    char *new_str = calloc(1, sizeof(char) * strlen(str) - 1);
    if(str[0] == '\'' || str[0] == '\"') {
        strncpy(new_str, str+1, strlen(str) - 2);
    } else {
        // get home info
        const char *home = getpwuid(getuid())->pw_dir;

        /* TODO: check if \\ need to be removed */
        for(int i=0, j=0; i<strlen(str); i++) {
            if(str[i] == '~') {
                int new_len = strlen(str) - 1 + strlen(home);
                new_str = realloc(new_str, sizeof(char) * new_len);
                strcpy(new_str+j, home);
                j += strlen(home);
            }else if(str[i] != '\\') {
                new_str[j++] = str[i];
            }
        }
    }
    return new_str;
}

void cd(char *direction) {
    if(chdir(direction) != 0) {
        printf("cd: no such file or directory: %s\n", direction);
    }
}

void run_cmd(NodeList *list) {
    IS_EXECUTING = true;
    /* init pipe */
    // cur pipe for child to process
    int pipes[PIPE_SIZE][2] = {0};
    pid_t pid[PIPE_SIZE] = {0};

    int cmd_idx = 0;

    Node *cur_node = list->begin;

    while(cur_node) {
        /* built-it function */
        if(strcmp("exit", cur_node->args->val[0]) == 0) {
            exit(0);
        } else if(strcmp("cd", cur_node->args->val[0]) == 0) {
            cd(cur_node->args->val[1]);
            cmd_idx++;
            cur_node = cur_node->next;
            continue;
        }

        /* normal cmd */
        pipe(pipes[cmd_idx]);
        pid[cmd_idx]= fork();

        if(pid[cmd_idx] < 0) {
            // error forking
            perror("error shell");
        } else if(pid[cmd_idx] != 0) {
            // parent process
            close(pipes[cmd_idx][WRITE]);
            int status;
            // warning: avoid closing wrong pipe
            if(!cmd_idx) {
                // close(pipes[cmd_idx-1][READ]);
            }
        } else {
            // child process
            // set ctrl-c handler
            signal(SIGINT, sigint_kill_handler);
            // Child process
            close(pipes[cmd_idx][READ]);
            // warning: redirection has higher priority than pipe
            if(cmd_idx) {
                dup2(pipes[cmd_idx-1][READ], STDIN_FILENO);
                close(pipes[cmd_idx-1][READ]);
            }
            if(cur_node->next) {
                dup2(pipes[cmd_idx][WRITE], STDOUT_FILENO);
            }
            close(pipes[cmd_idx][WRITE]);

            // redirection
            if(cur_node->in_path) {
                int fd = open(cur_node->in_path, O_RDONLY);
                dup2(fd, STDIN_FILENO);
            }

            if(cur_node->out_path) {

                int fd;
                if(cur_node->out_append) {
                    fd = open(cur_node->out_path, O_WRONLY | O_CREAT | O_APPEND, 0644);
                } else {
                    fd = open(cur_node->out_path, O_WRONLY | O_CREAT, 0644);
                }
                dup2(fd, STDOUT_FILENO);
            }

            cur_node->args->val[cur_node->args->idx] = NULL;
            // start execute
            if (execvp(cur_node->args->val[0], cur_node->args->val) == -1) {
                perror("error shell");
            }
            exit(0);
        }
        cur_node = cur_node->next;
        cmd_idx++;
    }
    // recover all subprocess and close pipe
    for(int i=0; i<cmd_idx; i++) {
        // close(pipes[i][READ]);
        waitpid(pid[i], NULL, 0);
    }

    if(IS_INTERACTIVE) {
        print_bar();
    }
    IS_EXECUTING = false;
}

%{
    #include <stdio.h>
    #include <stdlib.h>

    #include "node.h"
    #include "parser.h"
    char *clean_str(char *ori);
%}

ESC "\\t"|"\\n"|"\\ "|"\\|"|"\\&"|"\\;"|"\\("|"\\)"|"\\<"|"\\>"

    /* quote */
GEN_QUOTE ({ESC}|[^\t\n |&;()<>])*
SINGLE_QUOTE \'[^']*\'
DOUBLE_QUOTE \"[^"]*\"
QUOTE ({GEN_QUOTE}|{SINGLE_QUOTE}|{DOUBLE_QUOTE})

    /* config */
%option yylineno

%%

    /* quote */
{QUOTE}                 { yylval.str = strdup(yytext); return QUOTE; }

    /* pipeline */
"|"                     { return PIPE; }
"|&"
    /* redirection */
"<"                     { return RED_IN; }
">"                     { return RED_OUT; }
">>"                    { return RED_A_OUT; }
    /* list of commands */
"&&"
"||"
";"
"&"

    /* comment */
"#".*                       {}

    /* another meta char */
\n                          { return EOL; }
[\t ]                       {}
[()]                        {}

%%
int yywrap() {
    return 1;
}

void print_token(char *file_name) {
    yyin = fopen(file_name, "r");
    int token;
    while(token = yylex()) {
        if(token == QUOTE) {
            printf("quote: %s\n", yytext);
        } else {
            printf("op: %s\n", yytext);
        }
    }
    fclose(yyin);
}
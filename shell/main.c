#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdbool.h>

#include "node.h"
#include "parser.h"
#include "tokenizer.h"
#include "interactive.h"

extern int yyparse();

/* main shell function */
int main() {
    /* TODO: support script mode */
    set_interactive_mode();
    print_bar();
    yyparse();
}

#include "interactive.h"

extern bool IS_EXECUTING;
/* public global var */
bool IS_INTERACTIVE = false;

/* private global var */
struct passwd *user_info;
int home_len;


/* ctrl-c handler */

void sigint_ignore_handler() {
    if(!IS_EXECUTING) {
        printf("\n");
        print_bar();
        fflush(stdout);
    }
}

void sigint_kill_handler() {
    printf("\n");
    kill(getpid(), SIGKILL);
}


/* public function */
void set_interactive_mode() {
    IS_INTERACTIVE = true;

    // init user info
    user_info = getpwuid(getuid());
    home_len = strlen(user_info->pw_dir);

    // parent process should ignore ctrl-c
    signal(SIGINT, sigint_ignore_handler);
}

void print_bar() {
    char cwd[1024];
    getcwd(cwd, sizeof(cwd));
    if(strncmp(cwd, user_info->pw_dir, home_len) == 0) {
        printf("\033[1;36m%s\033[0m \033[1;32m~%s\033[0m$ ", user_info->pw_name, cwd + home_len);
    } else {
        printf("\033[1;36m%s\033[0m \033[1;32m%s\033[0m$ ", user_info->pw_name, cwd);
    }
}


#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/wait.h>

#include "scanner.h"
#include "parser.h"

#define DEBUG 0

#define READ 0
#define WRITE 1

Token *head = NULL;
bool has_child = false;

void sigint_ignore_handler() {
    // if no child then clean
    if(!has_child) {
        printf("\n> ");
        fflush(stdout);
    }
    return;
}

void sigint_kill_handler() {
    kill(getpid(), SIGKILL);
}

void execute(NodeList *cur_node) {
    /* close shell */
    if(strcmp("exit", cur_node->content->arg[0]) == 0) {
        kill(getpid(), SIGKILL);
    }

    /* init pipe */
    // cur pipe for child to process
    int pipes[2][2];
    int cur_pipe = 0;
    bool is_first = true;

    while(cur_node) {
        int prev_pipe = (cur_pipe + 1) % 2;
        pipe(pipes[cur_pipe]);

        pid_t pid = fork();
        has_child = true;
        if (pid == 0) {
            // set ctrl-c handler
            signal(SIGINT, sigint_kill_handler);
            // Child process
            close(pipes[cur_pipe][READ]);
            // warning: redirection has higher priority than pipe
            if(!is_first) {
                dup2(pipes[prev_pipe][READ], STDIN_FILENO);
                close(pipes[prev_pipe][READ]);
            }
            if(cur_node->next) {
                dup2(pipes[cur_pipe][WRITE], STDOUT_FILENO);
            }
            close(pipes[cur_pipe][WRITE]);
           

            // redirection
            if(cur_node->content->in_path) {
                int fd = open(cur_node->content->in_path, O_RDONLY);
                dup2(fd, STDIN_FILENO);
            }

            if(cur_node->content->out_path) {

                int fd;
                if(cur_node->content->out_append) {
                    fd = open(cur_node->content->out_path, O_WRONLY | O_CREAT | O_APPEND, 0644);
                } else {
                    fd = open(cur_node->content->out_path, O_WRONLY | O_CREAT, 0644);
                }
                dup2(fd, STDOUT_FILENO);
            }

            // start execute
            if (execvp(cur_node->content->arg[0], cur_node->content->arg) == -1) {
                perror("error shell");
            }
            exit(0);
        } else if (pid < 0) {
            // Error forking
            perror("error shell");
        } else {
            // Parent process
            close(pipes[cur_pipe][WRITE]);
            int status;
            waitpid(pid, &status, 0);
            close(pipes[prev_pipe][READ]);
            has_child = false;
        }
        cur_node = cur_node->next;
        is_first = false;
        cur_pipe = (cur_pipe + 1) % 2;

    }
}

int main() {
    // parent process should ignore ctrl-c
    signal(SIGINT, sigint_ignore_handler);

    // split by ' ', '<', '>', '|'
    char *cmd = NULL;
    size_t size;
    printf("> ");
    while(getline(&cmd, &size, stdin)) {
        cmd[strlen(cmd) - 1] = 0;
        // scanner
        head = tokenize(cmd);
        if(DEBUG) print_token(head);
        // parser
        NodeList *ast = parse(head);
        if(DEBUG) print_tree(ast);
        // execute
        execute(ast);

        printf("> ");
     }
}

#include <dirent.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>

#define INIT_DEPTH 2
int is_last_size = INIT_DEPTH;
bool *is_last;

char *join_path(char *a, char *b) {
    int new_len = strlen(a) + strlen(b) + 2;
    char *new_path = calloc(1, sizeof(char) * new_len);
    strcat(new_path, a);
    strcat(new_path, "/");
    strcat(new_path, b);
    return new_path;
}

struct dirent *get_next_dir(DIR *d) {
    /* skip . .. */
    struct dirent *cur_dir = readdir(d);
    while(cur_dir != NULL && cur_dir->d_name[0] == '.') {
        cur_dir = readdir(d);
    }
    return cur_dir;
}

void print(char *name, int depth, bool is_last[]) {
    // print prefix
    for(int i = 0; i < depth; i++) {
        if(is_last[i]) {
            printf("    ");
        } else {
            printf("│   ");
        }
    }
    if(is_last[depth]) {
        printf("└── %s\n", name);
    } else {
        printf("├── %s\n", name);
    }
}

void print_cur_dir(char *path, int depth) {
    /* expand is_last array */
    if(depth >= is_last_size) {
        is_last_size *= 2;
        bool *new_ptr = (bool*)realloc(is_last, sizeof(bool) * is_last_size);
        if(!new_ptr) {
            perror("Memory leak!");
            exit(1);
        }
        is_last = new_ptr;
    }
    DIR *d = opendir(path);
    if(d) {
        /* use next dir to determine if we need to print └ */
        struct dirent *cur_dir, *next_dir = get_next_dir(d);
        while((cur_dir = next_dir) != NULL) {
            next_dir = get_next_dir(d);
            /* cur_dir is the latest */
            if(next_dir == NULL) {
                is_last[depth] = true;
            }

            print(cur_dir->d_name, depth, is_last);

            /* print folder content */
            if(cur_dir->d_type == DT_DIR) {;
                char *new_path = join_path(path, cur_dir->d_name);
                print_cur_dir(new_path, depth + 1);
                free(new_path);
            }
        }
        // clear is_last array
        is_last[depth] = false;
        closedir(d);
    }
}

int main() {
    is_last = calloc(1, sizeof(bool) * INIT_DEPTH);
    printf(".\n");
    print_cur_dir(".", 0);
}
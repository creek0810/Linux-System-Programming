#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <unistd.h>

#define INIT_DEPTH 2

int SKIP_HIDDEN = 1;

/* statistics var */
int TOTAL_SIZE = 0;
int DIR_NUM = 0;
int FILE_NUM = 0;
int SOFT_LINK_NUM = 0;

/* recursive var */
int is_last_size = INIT_DEPTH;
bool *is_last;

/* path list */
typedef struct PathList PathList;
struct PathList {
    // capacity = path memory size
    // size = the number of path
    int capacity, top;
    char **path;
};

PathList *new_path_list() {
    PathList *result = calloc(1, sizeof(PathList));
    // init size is 16
    result->capacity = 16;
    result->path = calloc(1, sizeof(char*) * 16);
    return result;
}

void append_path_list(PathList *cur_list, char *path) {
    // realloc
    if(cur_list->capacity < cur_list->top + 1) {
        cur_list->capacity <<= 1;
        char **new_ptr = realloc(cur_list->path, sizeof(char*) * cur_list->capacity);
        if(new_ptr == NULL) {
            printf("realloc error\n");
            exit(1);
        };
        cur_list->path = new_ptr;
    }
    // warning: don't forget '\0'
    int path_len = strlen(path);
    char *copy_path = calloc(1, sizeof(char) * (path_len + 1));
    strncpy(copy_path, path, path_len);
    cur_list->path[cur_list->top++] = copy_path;
}

PathList *sort(char *path) {
    PathList *result = new_path_list();
    // get all file name
    DIR *d = opendir(path);
    if(d) {
        struct dirent *cur_dir = readdir(d);
        while ((cur_dir = readdir(d)) != NULL) {
            // skip hidden file
            if(SKIP_HIDDEN && cur_dir->d_name[0] == '.') {
                continue;
            }
            append_path_list(result, cur_dir->d_name);
        }
    }
    // bubble sort
    for(int i = 0; i < result->top; i++) {
      for(int j = i + 1; j < result->top; j++){
         if(strcmp(result->path[i], result->path[j]) > 0) {
            char *tmp = result->path[i];
            result->path[i] = result->path[j];
            result->path[j] = tmp;
         }
      }
    }
    return result;
}

void print(char *name, int depth, mode_t file_type) {
    // print prefix
    for(int i = 0; i < depth; i++) {
        if(is_last[i]) {
            printf("    ");
        } else {
            printf("│   ");
        }
    }
    if(is_last[depth]) {
        printf("└── ");
    } else {
        printf("├── ");
    }
    // choose ansi color
    if(S_ISDIR(file_type)) {
        printf("\033[1;96m%s\033[0m", name);
    } else if(S_ISLNK(file_type)) {
        printf("\033[1;95m%s\033[0m", name);
    } else if(access(name, X_OK) == 0) {
        printf("\033[1;91m%s\033[0m", name);
    } else {
        printf("%s", name);
    }

}

void print_list(PathList *list, int depth) {
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

    /*
        S_IFMT: type of file
        S_IFBLK: block special
        S_IFCHR: character special
        S_IFIFO: FIFO special
        S_IFREG: regular
        S_IFDIR: directory
        S_IFLNK: symbolic link
    */
    struct stat buf;
    for(int i = 0; i < list->top; i++) {
        // update is last array
        if(i == list->top - 1) {
            is_last[depth] = true;
        }
        // print and calc
        lstat(list->path[i], &buf);

        print(list->path[i], depth, buf.st_mode);
        if(S_ISLNK(buf.st_mode)) {
            char ori_path[500];
            readlink(list->path[i], ori_path, sizeof(ori_path));
            SOFT_LINK_NUM += 1;
            printf(" -> %s\n", ori_path);
        } else if(S_ISREG(buf.st_mode)) {
            FILE_NUM += 1;
            printf("\n");
        } else if(S_ISDIR(buf.st_mode)) {
            DIR_NUM += 1;
            printf("\n");
            PathList *new_path_list = sort(list->path[i]);
            // recursive
            chdir(list->path[i]);
            print_list(new_path_list, depth + 1);
            chdir("..");
        }
        // calc total size
        TOTAL_SIZE += buf.st_size;
    }
    // clear is_last array
    is_last[depth] = false;
}



int main() {
    is_last = calloc(1, sizeof(bool) * INIT_DEPTH);
    printf(".\n");
    PathList *list = sort(".");
    print_list(list, 0);
    printf("\n%d directories, %d files, %d soft links\n", DIR_NUM, FILE_NUM, SOFT_LINK_NUM);
    printf("size: %d\n", TOTAL_SIZE);
}
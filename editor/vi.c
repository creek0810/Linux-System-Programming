#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>
#include <ctype.h>
#include <ncurses.h>
#include <string.h>

#define ESC 27
#define BACKSPACE 8
#define DEL 127

/* struct and enum */
typedef enum editor_mode_t editor_mode_t;
typedef struct Editor Editor;
typedef struct Line Line;
enum editor_mode_t {
    NORMAL_MODE,
    INSERT_MODE,
    COMMAND_MODE,
};
struct Editor {
    int width, height; // screen size
    int row, col; // cursor offset
    int min_line, max_line; // line num at top and bottom screen
    editor_mode_t mode;
    WINDOW *pad;
    // file info
    Line *file_start, *file_end;
    Line *cur_line;
    int file_line_cnt;
} editor;

struct Line {
    Line *prev, *next;
    char *str;
    size_t size;
    int len;
};



/* init function */
void init_terminal() {
    initscr();
    noecho();
    cbreak();
    keypad(stdscr, TRUE);
}

void load_file(char *file_name) {
    // init file_start, file_end, file_line_cnt, cur_line
    FILE *fp = fopen(file_name, "r+");
    char *buffer = NULL;
    size_t buffer_size = 0;

    // start_line
    if(getline(&buffer, &buffer_size, fp) != -1) {
        // alloc Line instance
        Line *tmp = calloc(1, sizeof(Line));
        char *copy_str = calloc(1, buffer_size);
        memcpy(copy_str, buffer, buffer_size);
        tmp->size = buffer_size;
        tmp->str = copy_str;
        tmp->len = strlen(copy_str);
        // build double linked list
        editor.cur_line = editor.file_start = editor.file_end = tmp;
        editor.file_line_cnt += 1;
    }

    while(getline(&buffer, &buffer_size, fp) != -1) {
        // alloc Line instance
        Line *tmp = calloc(1, sizeof(Line));
        char *copy_str = calloc(1, buffer_size);
        memcpy(copy_str, buffer, buffer_size);
        tmp->size = buffer_size;
        tmp->str = copy_str;
        tmp->len = strlen(copy_str);
        // build double linked list
        editor.file_end->next = tmp;
        tmp->prev = editor.file_end;
        editor.file_end = tmp;
        editor.file_line_cnt += 1;
    }
    free(buffer);
}


void init_editor(char *file_name) {
    /* init editor info about file */
    load_file(file_name);
    /* init editor info pad */
    // TODO: determine the buffer size for pad
    editor.pad = newpad(editor.file_line_cnt, COLS);
    keypad(editor.pad, TRUE);

    /* init screen size */
    editor.width = COLS;
    editor.height = LINES;
    // screen top and bottom
    editor.min_line = 0;
    editor.max_line = LINES - 1;
    // cursor offset
    editor.row = editor.col = 0;
    // another attr
    editor.mode = NORMAL_MODE;

    // show file
    Line *cur_line = editor.file_start;
    while(cur_line) {
        for(int i=0; i<cur_line->len; i++) {
            waddch(editor.pad, cur_line->str[i]);
        }
        cur_line = cur_line->next;
    }
    prefresh(editor.pad, 0, 0, 0, 0, editor.height - 1, editor.width - 1);
}

/* mode action */
void normal_mode_action(int ch) {
    switch(ch){
        case KEY_LEFT:
        case 'h':
            editor.col = (editor.col - 1 < 0) ? 0 : editor.col - 1;
            break;
        case KEY_RIGHT:
        case 'l':
            // editor.col = (editor.col + 1 < editor.width) ? editor.col + 1 : editor.col;
            editor.col = (editor.col + 2 < editor.cur_line->len) ? editor.col + 1 : editor.col;
            break;
        case KEY_UP:
        case 'k':
            // at screen top
            if(editor.row == editor.min_line) {
                // not at pad top
                if(editor.min_line != 0) {
                    // scroll down
                    editor.max_line -= 1;
                    editor.min_line -= 1;
                    // move row
                    editor.row -= 1;
                    editor.cur_line = editor.cur_line->prev;
                }
            } else {
                editor.row -= 1;
                editor.cur_line = editor.cur_line->prev;
            }
            break;
        case KEY_DOWN:
        case 'j': // down
            // TODO: deal with next line is empty problem
            // at screen bottom
            if(editor.row == editor.max_line) {
                // not at pad bottom
                if(editor.max_line != editor.file_line_cnt - 1) {
                    // scroll down
                    editor.max_line += 1;
                    editor.min_line += 1;
                    // move row
                    editor.row += 1;
                    editor.cur_line = editor.cur_line->next;
                }
            } else {
                editor.row += 1;
                editor.cur_line = editor.cur_line->next;
            }
            break;
        case 'i': // insert mode
            editor.mode = INSERT_MODE;
            break;
        case ':': // command mode
            editor.mode = COMMAND_MODE;
            break;
    }
    wmove(editor.pad, editor.row, editor.col);
    prefresh(editor.pad, editor.min_line, 0, 0, 0, editor.height - 1, editor.width - 1);
}

void insert_mode_action(int ch) {
    switch(ch) {
        case ESC:
            editor.mode = NORMAL_MODE;
            break;
        case BACKSPACE:
        case DEL:
            editor.col = (editor.col - 1 < 0) ? 0 : editor.col - 1;
            move(editor.row, editor.col);
            echochar(' ');
            editor.col = (editor.col - 1 < 0) ? 0 : editor.col - 1;
            break;
        default:
            waddch(editor.pad, ch);
            editor.col += 1;
            break;
    }
    prefresh(editor.pad, editor.min_line, 0, 0, 0, editor.height - 1, editor.width - 1);
}

void command_mode_action(int ch) {
    switch(ch) {
        case ESC:
            editor.mode = NORMAL_MODE;
            // printf("change to normal mode\n");
            break;
        default:
            break;
            // printf("command: %c\n", ch);
    }
}

char read_keyboard() {
    int ch = wgetch(editor.pad);
    switch(editor.mode) {
        case NORMAL_MODE:
            normal_mode_action(ch);
            break;
        case INSERT_MODE:
            insert_mode_action(ch);
            break;
        case COMMAND_MODE:
            command_mode_action(ch);
            break;
    };
    return ch;
}

/* main function */
int main(int argc, char *argv[]) {
    if(argc != 2) {
        // printf("no input file\n");
        exit(1);
    }
    /* init */
    init_terminal();
    init_editor(argv[1]);
    while(1) {
        if(read_keyboard() == 'q') {
            break;
        }
    }
    endwin();
}
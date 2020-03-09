#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>
#include <ctype.h>
#include <ncurses.h>
#include <string.h>

#define ESC 27
#define DEL 127
#define NEWLINE 0xA

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
    editor.pad = newpad(editor.file_line_cnt + 100, COLS);
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
        waddstr(editor.pad, cur_line->str);
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

void insert_ch(int ch) {
    Line *cur_line = editor.cur_line;
    if((unsigned long)(cur_line->len + 2) > cur_line->size) {
        // TODO: check if realloc error
        cur_line->size *= 2;
        cur_line->str = (char*)realloc(cur_line->str, sizeof(char) * cur_line->size);
    }
    int x = editor.col;
    memmove(cur_line->str + x + 1, cur_line->str + x, cur_line->len - x);
    cur_line->str[x] = ch;
    cur_line->len += 1;
    editor.col += 1;
    // redraw
    mvwaddstr(editor.pad, editor.row, 0, cur_line->str);
    // move cursor
    wmove(editor.pad, editor.row, editor.col);
    prefresh(editor.pad, 0, 0, 0, 0, editor.height - 1, editor.width - 1);
}

void move_str(Line *ori_line, int ori_loc, Line *cur_line, int cur_loc) {
    int len = strlen(ori_line->str + ori_loc);
    // TODO: realloc must!!!
    if(len > cur_line->size) {
    }
    // copy to cur_line
    cur_line->len = cur_line->len + len - 1;
    strncpy(cur_line->str + cur_loc, ori_line->str + ori_loc, len);
    // clear ori_line
    ori_line->len = ori_line->len - len + 1;
    ori_line->str[ori_loc] = '\n';
    memset(ori_line->str + ori_line->len, 0, ori_line->size - ori_line->len);
}

void delete_ch() {
    Line *cur_line = editor.cur_line;
    if(editor.col != 0) {
        // TODO: full clear
        int x = editor.col;
        memmove(cur_line->str + x - 1, cur_line->str + x, cur_line->len - x);
        cur_line->str[cur_line->len - 1] = 0;
        editor.col -= 1;
        cur_line->len -= 1;
        mvwaddstr(editor.pad, editor.row, 0, cur_line->str);
    } else if(editor.row != 0){
        // concate str
        move_str(editor.cur_line, 0, cur_line->prev, cur_line->prev->len - 1);
        // go to previous line
        wdeleteln(editor.pad);
        // rebuild linked
        cur_line->prev->next = cur_line->next;
        if(cur_line->next) {
            cur_line->next->prev = cur_line->prev;
        }
        Line *free_line = cur_line;
        editor.cur_line = editor.cur_line->prev;
        free(free_line);
        editor.file_line_cnt -= 1;
        editor.row -= 1;
        // skip newline
        editor.col = editor.cur_line->len - 2;
        // TODO: redraw cur line
        mvwaddstr(editor.pad, editor.row, 0, editor.cur_line->str);
    }
    // move cursor
    wmove(editor.pad, editor.row, editor.col);
    prefresh(editor.pad, 0, 0, 0, 0, editor.height - 1, editor.width - 1);
}

void new_line() {
    // TODO: check if pad is overflow
    Line *tmp = calloc(1, sizeof(Line));
    // build linked
    tmp->next = editor.cur_line->next;
    editor.cur_line->next->prev = tmp;

    tmp->prev = editor.cur_line;
    editor.cur_line->next = tmp;



    // move prev line content to next
    tmp->len = 0;
    tmp->str = calloc(1, sizeof(char) * 32);
    tmp->size = 32;


    // move_str(editor.cur_line->str + editor.col, tmp);
    move_str(editor.cur_line, editor.col, tmp, 0);

    // TODO: update prev str
    // update and draw on screen
    // redraw origin line
    mvwaddstr(editor.pad, editor.row, 0, editor.cur_line->str);
    // draw new line
    winsertln(editor.pad);
    waddstr(editor.pad, tmp->str);
    // update editor attr

    editor.cur_line = tmp;
    editor.file_line_cnt += 1;
    // TODO: check scroll
    editor.row += 1;
    editor.col = 0;
    wmove(editor.pad, editor.row, editor.col);


    prefresh(editor.pad, editor.min_line, 0, 0, 0, editor.height - 1, editor.width - 1);
}


void insert_mode_action(int ch) {
    switch(ch) {
        case ESC:
            editor.mode = NORMAL_MODE;
            break;
        case KEY_BACKSPACE:
        case DEL:
            delete_ch();
            break;
        case KEY_ENTER:
        case NEWLINE:
            new_line();
            break;
        default:
            insert_ch(ch);
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

void save_file() {
    // TODO: check name
    FILE *fp = fopen("test.out", "w+");
    Line *cur_line = editor.file_start;
    while(cur_line) {
        fprintf(fp, "%s", cur_line->str);
        cur_line = cur_line->next;
    }
    fclose(fp);
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
    save_file();
    endwin();
}
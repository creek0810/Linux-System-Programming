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
typedef struct File File;
enum editor_mode_t {
    NORMAL_MODE,
    INSERT_MODE,
    COMMAND_MODE,
};

struct File {
    Line *start, *end;
    int line_num;
    char *path;
};

struct Line {
    Line *prev, *next;
    char *str;
    int size;
    int len;
};

struct Editor {
    int width, height; // screen size
    int row, col; // cursor offset
    int min_line, max_line; // line num at top and bottom screen
    editor_mode_t mode;
    WINDOW *pad;
    File file;
    Line *cur_line;
} editor;

/* line function */
Line *insert_line(char *str, Line *prev_line, Line *next_line) {
    Line *cur_line = calloc(1, sizeof(Line));
    // build linked
    // warning: note if prev_line, next_line is NULL or not
    if(prev_line) prev_line->next = cur_line;
    if(next_line) next_line->prev = cur_line;
    cur_line->prev = prev_line;
    cur_line->next = next_line;
    // init size and len
    cur_line->len = strlen(str);
    int i = 1;
    while(i <= cur_line->len) i <<= 1;
    cur_line->size = i;
    // init str part
    cur_line->str = calloc(1, cur_line->size);
    strncpy(cur_line->str, str, cur_line->len);
    return cur_line;
}

/* file function */
void save_file(char *path) {
    FILE *fp = fopen(path, "w+");
    Line *cur_line = editor.file.start;
    while(cur_line) {
        fprintf(fp, "%s", cur_line->str);
        cur_line = cur_line->next;
    }
    fclose(fp);
}

/* draw screen function */
void render_pad() {
    Line *cur_line = editor.file.start;
    wmove(editor.pad, 0, 0);
    while(cur_line) {
        waddstr(editor.pad, cur_line->str);
        cur_line = cur_line->next;
    }
    wmove(editor.pad, editor.row, editor.col);
}

void move_end() {

}

int end_col(Line *cur_line) {
    int len = cur_line->len;
    if(len == 0) return 0;
    if(cur_line->str[len - 1] == '\n') {
        len = (len - 1 < 0) ? 0 : len - 1;
    }
    // len to col
    return len = (len - 1 < 0) ? 0 : len - 1;
}

void move_up() {
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

    // check col
    int end = end_col(editor.cur_line);
    editor.col = (editor.col > end) ? end : editor.col;
}

void move_down() {
    // at screen bottom
    if(editor.row == editor.max_line) {
        // not at pad bottom
        if(editor.max_line != editor.file.line_num - 1) {
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

    int end = end_col(editor.cur_line);
    editor.col = (editor.col > end) ? end : editor.col;
}

void update_scroll() {
    // TODO: update scroll
    wmove(editor.pad, editor.row, editor.col);
    prefresh(editor.pad, editor.min_line, 0, 0, 0, editor.height - 1, editor.width - 1);
}

/* init function */
void init_terminal() {
    initscr();
    noecho();
    cbreak();
    keypad(stdscr, TRUE);
}

void init_file(char *path) {
    // init editor.file
    memset(&editor.file, 0, sizeof(File));
    editor.file.path = path;

    /* start load file */
    FILE *fp = fopen(path, "r+");
    if(fp == NULL){
        // new file
        editor.file.end = editor.file.start = editor.cur_line =
            insert_line("", NULL, NULL);
        editor.file.line_num = 1;
        return;
    }

    /* load file and construct File data structure */
    char *buffer = NULL;
    size_t buffer_size = 0;
    // start_line
    if(getline(&buffer, &buffer_size, fp) != -1) {
        editor.file.end = editor.file.start = editor.cur_line =
            insert_line(buffer, NULL, NULL);
        editor.file.line_num = 1;
    }

    // a var helping establish File data structure
    Line *file_cur_line = editor.file.start;
    while(getline(&buffer, &buffer_size, fp) != -1) {
        editor.file.end = file_cur_line =
            insert_line(buffer, file_cur_line, NULL);
        editor.file.line_num += 1;
    }
    free(buffer);
}

void init_editor(char *path) {
    /* init editor info pad */
    init_file(path);
    // TODO: determine the buffer size for pad
    editor.pad = newpad(editor.file.line_num + 100, COLS);
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
    render_pad();
    prefresh(editor.pad, 0, 0, 0, 0, editor.height - 1, editor.width - 1);
}

/* mode action */
bool normal_mode_action(int ch) {
    switch(ch){
        case KEY_LEFT:
        case 'h':
            editor.col = (editor.col - 1 < 0) ? 0 : editor.col - 1;
            break;
        case KEY_RIGHT:
        case 'l':
            if(editor.col + 1 <= end_col(editor.cur_line))
                editor.col += 1;
            break;
        case KEY_UP:
        case 'k':
            move_up();
            break;
        case KEY_DOWN:
        case 'j': // down
            move_down();
            break;
        case 'I': // insert mode
            editor.col = 0;
        case 'i': // insert mode
            editor.mode = INSERT_MODE;
            break;
        case ':': // command mode
            editor.mode = COMMAND_MODE;
            break;
    }
    update_scroll();
    return false;
}

void insert_ch(int ch) {
    Line *cur_line = editor.cur_line;
    // warning: don't forget to count '\0'
    if(cur_line->len + 2 > cur_line->size) {
        cur_line->size *= 2;
        char *new_ptr = (char*)realloc(cur_line->str, sizeof(char) * cur_line->size);
        if(new_ptr == NULL) {
            endwin();
            printf("realloc error!\n");
            exit(1);
        }
        cur_line->str = new_ptr;
    }
    int x = editor.col;
    memmove(cur_line->str + x + 1, cur_line->str + x, cur_line->len - x);
    cur_line->str[x] = ch;
    cur_line->len += 1;
    editor.col += 1;
    // redraw
    mvwaddstr(editor.pad, editor.row, 0, cur_line->str);
    // move cursor
    update_scroll();
}

void delete_ch() {
    Line *cur_line = editor.cur_line;
    if(editor.col == 0 && editor.row == 0) {
        // skip
    } else if(editor.col == 0) {
        // go back to previous line
        editor.file.line_num -= 1;
        editor.row -= 1;
        editor.col = editor.cur_line->prev->len - 1;

        // update previous line str
        int new_size = cur_line->prev->size;
        int new_len = cur_line->prev->len + cur_line->len - 1; // skip prev \n
        while(new_size <= new_len) new_size <<= 1;

        // realloc
        if(new_size != cur_line->prev->size) {
            char *new_ptr = (char*)realloc(cur_line->prev->str, new_size);
            if(new_ptr == NULL) {
                endwin();
                printf("realloc error!\n");
                exit(1);
            }
            cur_line->prev->str = new_ptr;
        }
        // warning: to replace \n, so we need to - 1
        char *update_loc = cur_line->prev->str + cur_line->prev->len - 1;
        strncpy(update_loc, cur_line->str, cur_line->len);
        cur_line->prev->size = new_size;
        cur_line->prev->len = new_len;

        // go to previous line
        wdeleteln(editor.pad);

        // rebuild linked
        cur_line->prev->next = cur_line->next;
        if(cur_line->next) cur_line->next->prev = cur_line->prev;
        editor.cur_line = editor.cur_line->prev;
        free(cur_line);

        // redraw cur line
        mvwaddstr(editor.pad, editor.row, 0, editor.cur_line->str);
    } else {
        int x = editor.col;
        memmove(cur_line->str + x - 1, cur_line->str + x, cur_line->len - x);
        cur_line->len -= 1;
        memset(cur_line->str + cur_line->len, 0, cur_line->size - cur_line->len);
        editor.col -= 1;
        mvwaddstr(editor.pad, editor.row, 0, cur_line->str);
    }
    // move cursor
    update_scroll();
}

void insert_newline() {
    // TODO: check if pad is overflow
    editor.cur_line =
        insert_line(editor.cur_line->str + editor.col, editor.cur_line, editor.cur_line->next);
    int reset_size = editor.cur_line->prev->size - editor.col;
    memset(editor.cur_line->prev->str + editor.col, 0, reset_size);
    // update prev line
    editor.cur_line->prev->str[editor.col] = '\n';
    editor.cur_line->prev->len = strlen(editor.cur_line->prev->str);

    editor.file.line_num += 1;

    // update ori line and new line on screen
    mvwaddstr(editor.pad, editor.row, 0, editor.cur_line->prev->str);
    winsertln(editor.pad);
    waddstr(editor.pad, editor.cur_line->str);
    // draw cursor and update
    editor.col = 0;
    editor.row += 1;
    update_scroll();
}

bool insert_mode_action(int ch) {
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
            insert_newline();
            break;
        default:
            insert_ch(ch);
            break;
    }
    prefresh(editor.pad, editor.min_line, 0, 0, 0, editor.height - 1, editor.width - 1);
    return false;
}

bool command_mode_action(int ch) {
    // TODO: read q q! wq
    switch(ch) {
        case ESC:
            editor.mode = NORMAL_MODE;
            break;
        case 'q':
            save_file("test.out");
            return true;
    }
    return false;
}

// the return value represent close editor or not
bool read_keyboard() {
    int ch = wgetch(editor.pad);
    switch(editor.mode) {
        case NORMAL_MODE:
            return normal_mode_action(ch);
        case INSERT_MODE:
            return insert_mode_action(ch);
        case COMMAND_MODE:
            return command_mode_action(ch);
    };
}

/* main function */
int main(int argc, char *argv[]) {
    if(argc != 2) {
        printf("pls enter a file path\n");
        exit(1);
    }
    /* init */
    init_terminal();
    init_file(argv[1]);
    init_editor(argv[1]);

    while(1) {
        if(read_keyboard()) {
            break;
        }
    }
    endwin();
}
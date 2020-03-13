#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <ncurses.h>
#include <string.h>

#define ESC 27
#define DEL 127
#define NEWLINE 10

#define TAB_SIZE 4
#define COMMAND_BUFFER_SIZE 50
#define NORMAL_BUFFER_SIZE 50

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
    WINDOW *pad, *status_bar;
    int pad_height;
    File file;
    Line *cur_line;

    char cmd_buffer[COMMAND_BUFFER_SIZE];
    int cmd_cnt;

    char pre_normal;
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

Line *delete_line(Line *cur_line) {
    Line *result;
    // warning: return cur_line->next if it exist, or we will return cur_line->prev
    if(cur_line->prev) {
        cur_line->prev->next = cur_line->next;
        result = cur_line->prev;
    }
    if(cur_line->next) {
        cur_line->next->prev = cur_line->prev;
        result = cur_line->next;
    }
    // free
    free(cur_line->str);
    free(cur_line);
    return result;
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
}

void update_pad() {
    // avoid show cursor on main pad on command mode
    if(editor.mode == COMMAND_MODE) return;
    // update max_line and min_line
    if(editor.row < editor.min_line) {
        editor.max_line -= editor.min_line - editor.row;
        editor.min_line = editor.row;
    } else if(editor.row > editor.max_line) {
        editor.min_line += editor.row - editor.max_line;
        editor.max_line = editor.row;
    }
    // move cursor to main pad and refresh
    wmove(editor.pad, editor.row, editor.col);
    prefresh(editor.pad, editor.min_line, 0, 0, 0, editor.height - 1, editor.width - 1);
}

void update_status_bar() {
    char status_info[200] = {0};
    switch (editor.mode) {
        case NORMAL_MODE:
            sprintf(status_info, "%*s %d,%d\n",
                editor.width - 20, " ", editor.row + 1, editor.col + 1);
            break;
        case INSERT_MODE:
            sprintf(status_info, "-- INSERT -- %*s %d,%d  %d/%d\n",
                editor.width - 30, " ", editor.row + 1, editor.col + 1,
                editor.row + 1, editor.file.line_num);
            break;
        case COMMAND_MODE:
            sprintf(status_info, ":%s\n", editor.cmd_buffer);
            break;
    }
    mvwaddstr(editor.status_bar, 0, 0, status_info);
    // command mode shows cursor on status_bar pad and before \n
    if(editor.mode == COMMAND_MODE) wmove(editor.status_bar, 0, editor.cmd_cnt + 1);
    prefresh(editor.status_bar, 0, 0, LINES-1, 0, LINES, COLS);
}

/* init function */
void init_terminal() {
    initscr();
    noecho();
    cbreak();
    keypad(stdscr, TRUE);
    set_escdelay(0);
}

void init_file(char *path) {
    // TODO: deal with tab showing problem
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
    editor.pre_normal = 0;
    /* init editor info pad */
    init_file(path);
    // 100 means pad buffer
    editor.pad = newpad(editor.file.line_num + 100, COLS);
    keypad(editor.pad, TRUE);
    editor.pad_height = editor.file.line_num + 100;
    /* init cmd pad */
    editor.status_bar = newpad(1, COLS);
    keypad(editor.status_bar, TRUE);

    /* init main pad size */
    editor.width = COLS;
    editor.height = LINES - 1;
    // screen top and bottom
    editor.min_line = 0;
    editor.max_line = editor.height - 1;

    // cursor offset
    editor.row = editor.col = 0;
    // another attr
    editor.mode = NORMAL_MODE;

    // show file
    render_pad();
    update_status_bar();
    update_pad();
}

/* find char function */
int last_char(Line *cur_line) {
    if(cur_line->len == 0) return 0;
    int last_idx = cur_line->len - 1;
    // skip \n
    if(last_idx > 0 && cur_line->str[last_idx] == '\n') {
        last_idx --;
    }
    return last_idx;
}

int first_char(Line *cur_line) {
    int i = 0;
    while(i < cur_line->len && cur_line->str[i] == ' ') {
        i++;
    }
    return (i < cur_line->len) ? i : last_char(cur_line);
}

/* undefined */
void adjust_terminal() {
    // TODO: resize pad
    if(editor.height != LINES - 1) {
        editor.height = LINES - 1;
        editor.max_line = editor.min_line + editor.height - 1;
    }
    if(editor.width != COLS) {
        editor.width = COLS;
    }
    update_pad();
}

void prepend_newline() {
    editor.cur_line = insert_line("\n", editor.cur_line->prev, editor.cur_line);
    if(editor.row == 0) editor.file.start = editor.cur_line;
    editor.file.line_num += 1;
    winsertln(editor.pad);
}

/* action function */
void move_up() {
    if(editor.row > 1) {
        editor.row -= 1;
        editor.cur_line = editor.cur_line->prev;
        int end = last_char(editor.cur_line);
        editor.col = (editor.col > end) ? end : editor.col;
    }
}

void move_down() {
    if(editor.row + 1 < editor.file.line_num) {
        editor.row += 1;
        editor.cur_line = editor.cur_line->next;
        int end = last_char(editor.cur_line);
        editor.col = (editor.col > end) ? end : editor.col;
    }
}

/* mode action */
void insert_newline();
void delete_ch();

void insert_mode_change(int ch) {
    editor.mode = INSERT_MODE;
    switch(ch){
        case 'i':
            break;
        case 'I': // insert mode (find first non space char)
            editor.col = first_char(editor.cur_line);
            break;
        case 'o':
            editor.col = last_char(editor.cur_line) + 1;
            insert_newline();
            break;
        case 'O':
            editor.col = 0;
            prepend_newline();
            break;
        case 'a': // insert mode
            // if col line not empty
            if(last_char(editor.cur_line) == 0) {
                if(editor.cur_line->str[0] != '\n' && editor.cur_line->str[0] != 0) {
                    editor.col += 1;
                }
            } else {
                editor.col += 1;
            }
            break;
        case 'A': // insert mode
            if(last_char(editor.cur_line) == 0) {
                if(editor.cur_line->str[0] != '\n' && editor.cur_line->str[0] != 0) {
                    editor.col = last_char(editor.cur_line) + 1;
                }
            } else {
                editor.col = last_char(editor.cur_line) + 1;
            }
            break;
    }
}

bool normal_mode_action(int ch) {
    switch(ch){
        // move cursor
        case KEY_LEFT:
        case 'h':
            editor.col = (editor.col - 1 < 0) ? 0 : editor.col - 1;
            break;
        case KEY_RIGHT:
        case 'l':
            if(editor.col + 1 <= last_char(editor.cur_line))
                editor.col += 1;
            break;
        case KEY_UP:
        case 'k':
            move_up();
            break;
        case KEY_DOWN:
        case 'j':
            move_down();
            break;
        case 'x': // delete cur char
            // check if the line is empty
            if(editor.cur_line->str[editor.col] != '\n' &&
                editor.cur_line->str[editor.col] != 0) {
                editor.col += 1;
                delete_ch();
                // deal with end of line problem
                if(editor.col > last_char(editor.cur_line)) {
                    editor.col = last_char(editor.cur_line);
                }
            }
            break;
        case 'd':
            // TODO: buffer the delete line and support p P action
            if(editor.pre_normal == 'd') {
                editor.pre_normal = 0;
                wdeleteln(editor.pad);
                editor.file.line_num -= 1;
                editor.cur_line = delete_line(editor.cur_line);
                // TODO: maybe it can be a function
                // update file start or end
                if(editor.row == 0) {
                    editor.file.start = editor.cur_line;
                } else if(editor.row == editor.file.line_num) {
                    editor.row -= 1;
                    editor.file.end = editor.cur_line;
                }
            } else {
                editor.pre_normal = 'd';
            }
            break;
        // jump line
        case 'g':
            if(editor.pre_normal == 'g') {
                editor.cur_line = editor.file.start;
                editor.row = 0;
                editor.pre_normal = 0;
            } else {
                editor.pre_normal = 'g';
            }
            break;
        case 'G':
            editor.row = editor.file.line_num - 1;
            editor.cur_line = editor.file.end;
            editor.col = last_char(editor.cur_line);
            break;
        case 'a': // change to insert mode
        case 'A':
        case 'o':
        case 'O':
        case 'i':
        case 'I':
            insert_mode_change(ch);
            break;
        case ':': // change to command mode
            editor.mode =  COMMAND_MODE;
            break;

    }
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
        // update file
        if(editor.file.line_num == editor.row + 1) {
            editor.file.end = editor.cur_line;
        }
    } else {
        int x = editor.col;
        memmove(cur_line->str + x - 1, cur_line->str + x, cur_line->len - x);
        cur_line->len -= 1;
        memset(cur_line->str + cur_line->len, 0, cur_line->size - cur_line->len);
        editor.col -= 1;
        mvwaddstr(editor.pad, editor.row, 0, cur_line->str);
        // deal with eof
        if(editor.row == editor.file.line_num - 1) {
            waddch(editor.pad, '\n');
        }
    }
}

void insert_newline() {
    // realloc pad
    if(editor.file.line_num + 1 > editor.pad_height) {
        editor.pad_height += 100;
        wresize(editor.pad, editor.pad_height, COLS);
    }
    editor.cur_line =
        insert_line(editor.cur_line->str + editor.col, editor.cur_line, editor.cur_line->next);
    int reset_size = editor.cur_line->prev->size - editor.col;
    memset(editor.cur_line->prev->str + editor.col, 0, reset_size);
    // update prev line
    editor.cur_line->prev->str[editor.col] = '\n';
    editor.cur_line->prev->len = strlen(editor.cur_line->prev->str);

    // update file end
    if(editor.file.line_num == editor.row + 1) {
        editor.file.end = editor.cur_line;
    }
    editor.file.line_num += 1;

    // update ori line and new line on screen
    mvwaddstr(editor.pad, editor.row, 0, editor.cur_line->prev->str);
    winsertln(editor.pad);
    waddstr(editor.pad, editor.cur_line->str);
    // draw cursor and update
    editor.col = 0;
    editor.row += 1;
}

bool insert_mode_action(int ch) {
    switch(ch) {
        case KEY_LEFT:
            editor.col = (editor.col - 1 < 0) ? 0 : editor.col - 1;
            break;
        case KEY_RIGHT:
            // insert mode can move on \n
            if(editor.col + 1 <= last_char(editor.cur_line) + 1)
                editor.col += 1;
            break;
        case KEY_UP:
            move_up();
            break;
        case KEY_DOWN:
            move_down();
            break;
        case ESC:
            editor.mode = NORMAL_MODE;
            if(editor.col > last_char(editor.cur_line)) {
                editor.col = last_char(editor.cur_line);
            }
            break;
        case KEY_BACKSPACE:
        case DEL:
            delete_ch();
            break;
        case KEY_ENTER:
        case NEWLINE:
            insert_newline();
            break;
        case '\t':
        case KEY_STAB:
            for(int i=0; i<TAB_SIZE; i++) {
                insert_ch(' ');
            }
            break;
        default:
            insert_ch(ch);
            break;
    }
    return false;
}

bool run_command() {
    // TODO: if file is dirty, it needs to use q!
    if(strcmp(editor.cmd_buffer, "w") == 0) {
        save_file("test.out");
        return false;
    }
    // line jump
    if('0' <= editor.cmd_buffer[0] && '9' >= editor.cmd_buffer[0]) {
        // get line num, be care for exceed number
        int line_num = atoi(editor.cmd_buffer);
        if(line_num >= editor.file.line_num) {
            line_num = editor.file.line_num - 1;
        } else if(line_num > 0){ // warning: row is 0 base
            line_num --;
        }

        editor.row = line_num;
        // update cur line
        int cur_num = 0;
        editor.cur_line = editor.file.start;
        while(cur_num < line_num) {
            editor.cur_line = editor.cur_line->next;
            cur_num ++;
        }
        return false;
    }
    if(strcmp(editor.cmd_buffer, "wq") == 0) {
        save_file("test.out");
    }
    return true;
}

bool command_mode_action(int ch) {
    switch(ch) {
        case KEY_ENTER:
        case NEWLINE: {
            editor.mode = NORMAL_MODE;
            bool result = run_command();
            // clear command buffer
            editor.cmd_cnt = 0;
            memset(editor.cmd_buffer, 0, COMMAND_BUFFER_SIZE * sizeof(char));
            return result;
        }
        case ESC:
            // clear buffer and change to normal mode
            editor.mode = NORMAL_MODE;
            // clear command buffer
            editor.cmd_cnt = 0;
            memset(editor.cmd_buffer, 0, COMMAND_BUFFER_SIZE * sizeof(char));
            break;
        case KEY_BACKSPACE:
        case DEL:
            if(editor.cmd_cnt == 0) {
                // clear buffer and change to normal mode
                editor.mode = NORMAL_MODE;
                // clear command buffer
                editor.cmd_cnt = 0;
                memset(editor.cmd_buffer, 0, COMMAND_BUFFER_SIZE * sizeof(char));
            } else {
                editor.cmd_cnt -= 1;
                editor.cmd_buffer[editor.cmd_cnt] = 0;
            }
            break;
        default:
            editor.cmd_buffer[editor.cmd_cnt++] = ch;
    }
    return false;
}

// the return value represent close editor or not
bool read_keyboard() {
    int ch = wgetch(editor.pad);
    adjust_terminal();
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
    init_editor(argv[1]);
    /* main loop */
    while(read_keyboard() == false) {
        update_status_bar();
        update_pad();
    }
    endwin();
}

#ifndef _INTERACTIVE_H
#define _INTERACTIVE_H

#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <pwd.h>

bool IS_INTERACTIVE;

void set_interactive_mode();
void print_bar();
void sigint_kill_handler();

#endif
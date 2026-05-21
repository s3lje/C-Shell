#ifndef __BUILTINS_H
#define __BUILTINS_H

int chd(char** args);
int help(char** args);
int quit(char** args);

int num_builtins();

static char* builtin_str[] = {
    "cd",
    "help",
    "quit"
};

static int (*builtin_func[])(char**) = {
    &chd,
    &help,
    &quit
};

#endif

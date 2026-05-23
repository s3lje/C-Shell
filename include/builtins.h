#ifndef __BUILTINS_H
#define __BUILTINS_H

int chd(char** args);
int help(char** args);
int quit(char** args);
int num_builtins();

extern char* builtin_str[];
extern int (*builtin_func[])(char**);

#endif

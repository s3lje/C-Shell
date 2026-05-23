#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "builtins.h"

char *builtin_str[] = {
    "cd",
    "help",
    "exit"
};

int (*builtin_func[])(char**) = {
    &chd,
    &help,
    &quit
};

int num_builtins(){
    return sizeof(builtin_str) / sizeof(char*); 
}

/**
 * Function for changing directory.
 */
int chd(char** args){
    if (args[1] == NULL){
        fprintf(stderr, "c-shell: expected argument to \"cd\"\n"); 
    } else {
        if (chdir(args[1]) != 0){
            perror("c-shell");
        }
    }
    return 1;
}

/**
 * Prints a help menu to the user.
 */
int help(char** args){
    printf("C-SHELL\n");
    printf("Type a program name followed by its arguments, and hit enter.\n");
    printf("The following are built in:\n");

    for (int i = 0; i < num_builtins(); i++){
        printf("\t%s\n", builtin_str[i]);
    }

    printf("Use the man command for information on other programs.\n"); 
    return 1; 
}

/**
 * Quits the shell.
 */
int quit(char** args){
    return EXIT_SUCCESS;
}

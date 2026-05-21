#include <stdio.h>
#include <stdint.h>

void    shell_loop();
char*   read_line();
char**  split_line();
int     exec();

#include <stdlib.h>
int main(int argc, char** argv){
    

    shell_loop();


    return EXIT_SUCCESS;
}

void shell_loop(){
    char*   line;
    char**  args;
    int     status;

    do{
        printf("> ");
        line   = read_line();
        args   = split_line();
        status = exec();

        free(line);
        free(args);
    } while (status);
}

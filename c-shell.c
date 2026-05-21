#include <stdio.h>
#include <stdint.h>

#define LINE_BUFFERSIZE 1024

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

char* read_line(){
    int   bufferSize = LINE_BUFFERSIZE;
    int   pos        = 0;
    char* buffer     = malloc(sizeof(char) * bufferSize);
    int c;

    if (!buffer){
        fprintf(stderr, "read_line: allocation error...\n");
        return EXIT_FAILURE;
    }

    while (1){
        c = getchar();
        if (c == EOF || c == '\n'){ // replace end of line with terminator
            buffer[pos] = '\0';
            return buffer;
        } else {
            buffer[pos] = c;
        }
        pos++;

        if (pos >= bufferSize){
            bufferSize += LINE_BUFFERSIZE;
            buffer = realloc(buffer, bufferSize);
            if (!buffer) {
                fprintf(stderr, "read_line: re-allocation error...\n"); 
                return EXIT_FAILURE;
            }
        }
    }
}


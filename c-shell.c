#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define LINE_BUFSIZE 1024
#define TOK_BUFSIZE  64
#define TOK_DELIM    " \t\r\n\a"

void    shell_loop();
char*   read_line();
char**  parse_line(char*);
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
        args   = parse_line(line);
        status = exec();

        free(line);
        free(args);
    } while (status);
}

char* read_line(){
    int   bufferSize = LINE_BUFSIZE;
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
            bufferSize += LINE_BUFSIZE;
            buffer = realloc(buffer, bufferSize);
            if (!buffer) {
                fprintf(stderr, "read_line: re-allocation error...\n"); 
                return EXIT_FAILURE;
            }
        }
    }
}

char** parse_line(char* line){
    int bufferSize  = TOK_BUFSIZE;
    int pos         = 0;
    char** tokens   = malloc(bufferSize * sizeof(char*));
    char*  token;

    if (!tokens){
        fprintf(stderr, "parse_line: allocation error...\n");
        return EXIT_FAILURE;
    }

    token = strtok(line, TOK_DELIM);
    while (token != NULL){
        tokens[pos] = token;
        pos++;

        if (pos >= bufferSize){
            bufferSize += TOK_BUFSIZE;
            tokens = realloc(tokens, bufferSize * sizeof(char*));
            if (!tokens){
                fprintf(stderr, "parse_line: allocation error...\n");
                return EXIT_FAILURE;
            }
        }

        token = strtok(NULL, TOK_DELIM);
    }

    tokens[pos] = NULL;
    return tokens; 
}

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#define LINE_BUFSIZE 1024
#define TOK_BUFSIZE  64
#define TOK_DELIM    " \t\r\n\a"

void    shell_loop();
char*   read_line();
char**  parse_line(char*);
int     launch_bin(char**);
int     exec(char**);

//// Built-ins ////////////////////////////////////////////
int chd(char** args);
int help(char** args);
int quit(char** args);

char* builtin_str[] = {
    "cd",
    "help",
    "exit",
};

int (*builtin_func[]) (char**) = {
    &chd,
    &help,
    &quit
};

int num_builtins(){
    return sizeof(builtin_str) / sizeof(char *);
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

//////////////////////////////////////////////////////////////




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
    int len         = 0;
    char** tokens   = malloc(bufferSize * sizeof(char*));

    if (!tokens){
        fprintf(stderr, "parse_line: allocation error...\n");
        return EXIT_FAILURE;
    }
    
    char* start = line;
    char* p     = line;

    while (*p){
        if (strchr(TOK_DELIM, *p)){
            if (p > start){ // found token
                len = p - start;
                tokens[pos] = malloc(len+1);
                if (!tokens){
                    fprintf(stderr, "parse_line: allocation error...\n");
                    return EXIT_FAILURE;
                }

                strncpy(tokens[pos], start, len);
                tokens[pos++][len] = '\0';

                if (pos >= bufferSize){
                    bufferSize += TOK_BUFSIZE;
                    tokens = realloc(tokens, bufferSize * sizeof(char*));
                    if (!tokens){
                        fprintf(stderr, "parse_line: allocation error...\n");
                        return EXIT_FAILURE;
                    }
                }
            }
            start = p + 1;
        }
        p++;
    }
    
    // Handle last token 
    if (p > start) {
        len = p - start;
        tokens[pos] = malloc(len+1);

        if (!tokens[pos]){
            fprintf(stderr, "parse_line: allocation error...\n");
            return EXIT_FAILURE; 
        }

        strncpy(tokens[pos], start, len);
        tokens[pos++][len] = '\0';
    }

    tokens[pos] = NULL;
    return tokens;
}

int launch_bin(char** args){
    pid_t pid;
    pid_t wpid;
    int status;

    pid = fork();
    if (pid == 0){
        // Child process
        if (execvp(args[0], args) == -1){
            perror("launch_bin");
        }
        return EXIT_FAILURE;
    } else if (pid < 0){
        // Error forking
        perror("launch_bin");
    } else {
        // Parent process
        do {
            wpid = waitpid(pid, &status, WUNTRACED);
        } while (!WIFEXITED(status) && !WIFSIGNALED(status));
    }

    return 1;
}

int exec(char** args){
    if (args[0] == NULL){
        // Empty command entered
        return 1;
    }

    for (int i = 0; i < num_builtins(); i++){
        if (strcmp(args[0], builtin_str[i]) == 0){
            return (*builtin_func[i])(args);
        }
    }

    return launch_bin(args); 
}

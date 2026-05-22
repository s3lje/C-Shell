#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>
#include <pwd.h>
#include <limits.h>
#include "include/builtins.h"

#define LINE_BUFSIZE 1024
#define TOK_BUFSIZE  64
#define TOK_DELIM    " \t\r\n\a"

void    shell_loop();
char*   read_line();
char**  parse_line(char*);
char**  split_pipes(char*, int*);
int     launch_bin(char**);
int     exec(char**);
char*   shorten_path(const char*);

int main(){

    shell_loop();

    return EXIT_SUCCESS;
}

void shell_loop(){
    char*   line;
    char**  args;
    int     cmd_num; 
    char**  cmds;
    char    cwd[PATH_MAX];
    int     status; 
    char*   display_cwd;
    const char* username = getenv("USER");

    if (username == NULL){
        fprintf(stderr, "Username not found... Defaulting to user.\n");
        username = "user"; 
    }

    do{
        if (getcwd(cwd, sizeof(cwd)) != NULL){
            display_cwd = shorten_path(cwd);
            printf("%s | %s\n[]---> ", username, display_cwd);
            
            free(display_cwd);
        } else {
            printf("%s []---> ", username);
        }

        line   = read_line();
        args   = NULL; 
        cmds   = split_pipes(line, &cmd_num);

        if (cmd_num == 1){
            args = parse_line(cmds[0]);
            if (!args[0]) { free(args); free(cmds); continue; }
            status = exec(args);
        } else if (cmd_num == 2){
            status = 1; 
            int fd[2];
            if (pipe(fd) == -1){
                perror("pipe");
                exit(EXIT_FAILURE);
            }
            
            pid_t pid1 = fork();
            if (pid1 == -1){
                perror("fork");
                exit(EXIT_FAILURE); 
            }
            if (pid1 == 0){
                close(fd[0]);
                dup2(fd[1], STDOUT_FILENO);
                close(fd[1]);

                args = parse_line(cmds[0]);
                execvp(args[0], args);
                perror("execpv (cmd1)");
                exit(EXIT_FAILURE);
            }

            pid_t pid2 = fork();
            if (pid2 == -1){
                perror("fork");
                exit(EXIT_FAILURE);
            }

            if (pid2 == 0){
                close(fd[1]);
                dup2(fd[0], STDIN_FILENO);
                close(fd[0]);

                args = parse_line(cmds[1]);
                execvp(args[0], args); 
                perror("execpv (cmd2)");
                exit(EXIT_FAILURE); 
            }

            close(fd[0]);
            close(fd[1]);
            waitpid(pid1, NULL, 0);
            waitpid(pid2, NULL, 0); 
        } else { // more than two commands
            int pipe_num = cmd_num - 1;
            int pipes[pipe_num][2];

            for (int i = 0; i < pipe_num; i++){
                if (pipe(pipes[i]) == -1){
                    perror("pipe");
                    exit(EXIT_FAILURE);
                }
            }

            for (int i = 0; i < cmd_num; i++){
                pid_t pid = fork();
                if (pid == -1){
                    perror("fork");
                    exit(EXIT_FAILURE);
                }

                if (pid == 0){
                    if (i > 0)
                        dup2(pipes[i-1][0], STDIN_FILENO);
                    if (i < cmd_num - 1)
                        dup2(pipes[i][1], STDOUT_FILENO);

                    for (int j = 0; j < pipe_num; j++){
                        close(pipes[j][0]);
                        close(pipes[j][1]);
                    }

                    args = parse_line(cmds[i]);
                    execvp(args[0], args);
                    perror("execvp");
                    exit(EXIT_FAILURE);
                }
            }

            for (int i = 0; i < pipe_num; i++){
                close(pipes[i][0]);
                close(pipes[i][1]);
            }
            for (int i = 0; i < cmd_num; i++){
                waitpid(-1, NULL, 0); // wait for any child to finish
            }

        }

        free(line);
        if (args) {
            free(args);
            args = NULL; 
        }
        free(cmds); 
    } while (status);
}

char* read_line(){
    int   bufferSize = LINE_BUFSIZE;
    int   pos        = 0;
    char* buffer     = malloc(sizeof(char) * bufferSize);
    int c;

    if (!buffer){
        fprintf(stderr, "read_line: allocation error...\n");
        exit(EXIT_FAILURE);
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
                exit(EXIT_FAILURE);
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
        exit(EXIT_FAILURE);
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
                    exit(EXIT_FAILURE);
                }

                strncpy(tokens[pos], start, len);
                tokens[pos++][len] = '\0';

                if (pos >= bufferSize){
                    bufferSize += TOK_BUFSIZE;
                    tokens = realloc(tokens, bufferSize * sizeof(char*));
                    if (!tokens){
                        fprintf(stderr, "parse_line: allocation error...\n");
                        exit(EXIT_FAILURE);
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
            exit(EXIT_FAILURE); 
        }

        strncpy(tokens[pos], start, len);
        tokens[pos++][len] = '\0';
    }

    tokens[pos] = NULL;
    return tokens;
}

char** split_pipes(char* input, int* cmd_num){
    char**  cmds;
    char*   token;
    int i = 0;

    *cmd_num = 1;
    for (int i = 0; input[i]; i++){
        if (input[i] == '|') (*cmd_num)++;
    }

    cmds = malloc(*cmd_num * sizeof(char*));
    token = strtok(input, "|"); 
    
    while (token != NULL && i < *cmd_num){
        char* trimmed = token;
        while (*trimmed == ' ' || *trimmed == '\t')
            trimmed++;
        char* end = trimmed + strlen(trimmed) - 1;
        while (end > trimmed && (*end == ' ' || *end == '\t' || *end == '\n'))
            end--;
        *(end + 1) = '\0';

        cmds[i++] = strdup(trimmed);
        token = strtok(NULL, "|");
    }
    return cmds; 
}

int launch_bin(char** args){
    pid_t pid;
    pid_t wpid;
    int status;

    pid = fork();
    if (pid == 0){
        // Child process
        if (execvp(args[0], args) == -1){
            fprintf(stderr, "----------------------------------------\n");
            perror("c-shell");
            fprintf(stderr, "----------------------------------------\n"); 
        }
        exit(EXIT_FAILURE);
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

char* shorten_path(const char* full_path){
    char* home = getenv("HOME");

    if (!home){
        return strdup(full_path);
    }

    size_t home_len = strlen(home);
    size_t path_len = strlen(full_path);

    if (path_len >= home_len && strncmp(full_path, home, home_len) == 0){
        if (path_len == home_len){
            return strdup("~");
        }

        if (full_path[home_len] == '/'){
            size_t remaining_len = path_len - home_len;
            char* short_path = malloc(remaining_len + 2);

            if (!short_path) return strdup(full_path);

            sprintf(short_path, "~%s", full_path + home_len); 
            return short_path; 
        }
    }

    return strdup(full_path); 
}

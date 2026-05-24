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
int     launch_piped(char**, int);
int     exec_cmd(char**);
char*   shorten_path(const char*);
void    free_args(char**);
void    free_cmds(char**, int); 

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
        fprintf(stderr, "Warning: Username not found... Defaulting to user.\n");
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
        cmds   = split_pipes(line, &cmd_num);

        if (cmd_num == 1){
            args = parse_line(cmds[0]);
            if (!args[0]) { free(args); free(cmds); continue; }
            status = exec_cmd(args);
        } else {
            status = launch_piped(cmds, cmd_num); 
        }

        free(line);
        free_cmds(cmds, cmd_num);  
    } while (status);
}


char *read_line(void) {
    int   cap = LINE_BUFSIZE;
    int   pos = 0;
    int   c;
    char *buf = malloc(cap);
    if (!buf) {
        perror("malloc");
        exit(EXIT_FAILURE);
    }

    while ((c = getchar()) != EOF && c != '\n') {
        buf[pos++] = (char)c;
        if (pos >= cap) {
            cap *= 2;
            buf = realloc(buf, cap);
            if (!buf) {
                perror("realloc");
                exit(EXIT_FAILURE);
            }
        }
    }

    buf[pos] = '\0';
    return buf;
}

char** parse_line(char* line){
    int bufferSize  = TOK_BUFSIZE;
    int pos         = 0;
    char** tokens   = malloc(bufferSize * sizeof(char*));

    if (!tokens){
        fprintf(stderr, "parse_line: allocation error...\n");
        exit(EXIT_FAILURE);
    }
    
    char* p     = line;

    while (*p){
        while (*p && strchr(TOK_DELIM, *p)) 
            p++;
        if (!*p)
            break;

        char* tokenStart;
        int   tokenLen;

        if (*p == '"' || *p == '\''){
            char quote = *p;
            p++;
            tokenStart = p;
            while (*p && *p != quote)
                p++;
            tokenLen = p - tokenStart;
            if (*p == quote)
                p++;
        } else {
            tokenStart = p;
            while(*p && !strchr(TOK_DELIM, *p))
                p++;
            tokenLen = p - tokenStart;
        }

        tokens[pos] = malloc(tokenLen+1);
        if (!tokens[pos]){
            fprintf(stderr, "parse_line: allocation error...\n");
            exit(EXIT_FAILURE);
        }

        strncpy(tokens[pos], tokenStart, tokenLen);
        tokens[pos++][tokenLen] = '\0';

        if (pos >= bufferSize){
            bufferSize += TOK_BUFSIZE;
            tokens = realloc(tokens, bufferSize * sizeof(char*));
            if (!tokens){
                fprintf(stderr, "parse_line: allocation error"); 
                exit(EXIT_FAILURE); 
            }
        }
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
int launch_piped(char **cmds, int cmd_num) {
    int pipe_num = cmd_num - 1;
    int pipes[pipe_num][2];

    for (int i = 0; i < pipe_num; i++) {
        if (pipe(pipes[i]) == -1) {
            perror("pipe");
            return 1;
        }
    }

    for (int i = 0; i < cmd_num; i++) {
        pid_t pid = fork();
        if (pid == -1) {
            perror("fork");
            return 1;
        }

        if (pid == 0) {
            if (i > 0)
                dup2(pipes[i - 1][0], STDIN_FILENO);
            if (i < cmd_num - 1)
                dup2(pipes[i][1], STDOUT_FILENO);

            for (int j = 0; j < pipe_num; j++) {
                close(pipes[j][0]);
                close(pipes[j][1]);
            }

            char **args = parse_line(cmds[i]);
            execvp(args[0], args);
            perror("execvp");
            exit(EXIT_FAILURE);
        }
    }

    for (int i = 0; i < pipe_num; i++) {
        close(pipes[i][0]);
        close(pipes[i][1]);
    }
    for (int i = 0; i < cmd_num; i++)
        wait(NULL);

    return 1;
}
int exec_cmd(char **args) {
    if (!args[0])
        return 1;

    for (int i = 0; i < num_builtins(); i++) {
        if (strcmp(args[0], builtin_str[i]) == 0)
            return (*builtin_func[i])(args);
    }

    /* External command */
    pid_t pid = fork();
    if (pid == -1) {
        perror("fork");
        return 1;
    }

    if (pid == 0) {
        if (execvp(args[0], args) == -1) {
            fprintf(stderr, "c-shell: %s: command not found\n", args[0]);
            exit(EXIT_FAILURE);
        }
    }

    int status;
    waitpid(pid, &status, WUNTRACED);
    return 1;
}

////// HELPERS ///////////////////////////
void free_args(char **args) {
    if (!args) return;
    for (int i = 0; args[i]; i++)
        free(args[i]);
    free(args);
}

void free_cmds(char **cmds, int n) {
    if (!cmds) return;
    for (int i = 0; i < n; i++)
        free(cmds[i]);
    free(cmds);
}

char *shorten_path(const char *path) {
    const char *home     = getenv("HOME");
    size_t      home_len = home ? strlen(home) : 0;

    if (home && strncmp(path, home, home_len) == 0) {
        if (path[home_len] == '/' || path[home_len] == '\0') {
            char *short_path;
            if (asprintf(&short_path, "~%s", path + home_len) != -1)
                return short_path;
        }
    }
    return strdup(path);
}

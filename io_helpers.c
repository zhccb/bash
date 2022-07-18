#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <signal.h>
#include <sys/wait.h>


#include "builtins.h"
#include "io_helpers.h"
#include "variables.h"

char p_th[1024] = "./";

// ===== Output helpers =====

/* Prereq: str is a NULL terminated string
 */
void display_message(char *str) {
    write(STDOUT_FILENO, str, strnlen(str, MAX_STR_LEN));
}


/* Prereq: pre_str, str are NULL terminated string
 */
void display_error(char *pre_str, char *str) {
    write(STDERR_FILENO, pre_str, strnlen(pre_str, MAX_STR_LEN));
    write(STDERR_FILENO, str, strnlen(str, MAX_STR_LEN));
    write(STDERR_FILENO, "\n", 1);
}


// ===== Input tokenizing =====

/* Prereq: in_ptr points to a character buffer of size > MAX_STR_LEN
 * Return: number of bytes read
 */
ssize_t get_input(char *in_ptr) {
    int retval = read(STDIN_FILENO, in_ptr, MAX_STR_LEN+1); // Not a sanitizer issue since in_ptr is allocated as MAX_STR_LEN+1
    int read_len = retval;
    if (retval == -1) {
        read_len = 0;
    }
    if (read_len > MAX_STR_LEN) {
        read_len = 0;
        retval = -1;
        write(STDERR_FILENO, "ERROR: input line too long\n", strlen("ERROR: input line too long\n"));
        int junk = 0;
        while((junk = getchar()) != EOF && junk != '\n');
    }
    in_ptr[read_len] = '\0';
    return retval;
}

/* Prereq: in_ptr is a string, tokens is of size >= len(in_ptr)
 * Warning: in_ptr is modified
 * Return: number of tokens.
 */
size_t tokenize_input(char *in_ptr, char **tokens) {
    // TODO, uncomment the next line.
    char *curr_ptr = strtok (in_ptr, DELIMITERS);
    size_t token_count = 0;

    while (curr_ptr != NULL) {  
        tokens[token_count] = curr_ptr;
        token_count++;
        curr_ptr = strtok(NULL, DELIMITERS);
    }
    tokens[token_count] = NULL;
    return token_count;
}

size_t split_cmd(char *in_ptr, char **tokens) {
    char *curr_ptr = strtok (in_ptr, "|");
    size_t token_count = 0;

    while (curr_ptr != NULL) { 
        tokens[token_count] = curr_ptr;
        token_count++;
        curr_ptr = strtok (NULL, "|");
    }
    tokens[token_count] = NULL;
    return token_count;
}

char *get_path() {return p_th;}

size_t exe(int token_count, char **token_arr){
     // Command execution
    if (token_count >= 1) {
        bn_ptr builtin_fn = check_builtin(token_arr[0]);
        if (builtin_fn != NULL) {
            for(int i = 1; i < token_count; i++){
                if(token_arr[i][0] == '$'){
                    token_arr[i] = get_value(token_arr[i]);}
            }

            ssize_t err = builtin_fn(token_arr);
            if (err == - 1) {
                display_error("ERROR: Builtin failed: ", token_arr[0]);
            }
        }else if (strstr(token_arr[0], "=") != NULL){
            char *name_value[2];
            name_value[0] = strtok(token_arr[0], "=");
            name_value[1] = name_value[0] + strlen(name_value[0]) + 1;
            strtok(name_value[1], "\n");
            create_variable(name_value[0], name_value[1]);
        }else {
            // display_error("ERROR: Unrecognized command: ", token_arr[0]);
            pid_t p = fork();
            if(p == 0){
                if(execvp(token_arr[0],token_arr) == -1){
                    display_error("ERROR: Unrecognized command: ", token_arr[0]);
                    exit(1);
                }
            } wait(NULL);
            return 0;
        }
    }
    return 0;
}

     void handler(int sig) {
        display_message("\n");
    }

    size_t unkill(){
        struct sigaction act;
        act.sa_handler = handler;
        act.sa_flags = 0;
        sigemptyset(&act.sa_mask);
        sigaction(SIGINT,&act,NULL);
        return 0;
    }
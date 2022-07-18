#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <dirent.h> 
#include <poll.h>
#include <errno.h>
#include <signal.h>

#include "builtins.h"
#include "io_helpers.h"
#include "variables.h"


// ====== Command execution =====

/* Return: index of builtin or -1 if cmd doesn't match a builtin
 */
bn_ptr check_builtin(const char *cmd) {
    ssize_t cmd_num = 0;
    while (cmd_num < BUILTINS_COUNT &&
           strncmp(BUILTINS[cmd_num], cmd, MAX_STR_LEN) != 0) {
        cmd_num += 1;
    }
    return BUILTINS_FN[cmd_num];
}


// ===== Builtins =====

/* Prereq: tokens is a NULL terminated sequence of strings.
 * Return 0 on success and -1 on error ... but there are no errors on echo. 
 */
ssize_t bn_echo(char **tokens) {
    ssize_t index = 1;

    if (tokens[index] != NULL) {
        display_message(tokens[index]);
        index += 1;
    }
    while (tokens[index] != NULL) {
        // TODO:
         display_message(" ");
         display_message(tokens[index]);
        index += 1;
    }
    display_message("\n");

    return 0;
}



ssize_t bn_cat(char **tokens) {
    FILE *f;
    int d = 0;
    if(tokens[1] == NULL){
        struct pollfd fds;
        fds.fd = 0;     
        fds.events = POLLIN; 
        int ret = poll(&fds, 1, 10); // in your project, set this to 10, not 3000.
        if (ret == 0) {
            display_error("ERROR: No input source provided","");
            clearerr(stdin);
            return -1;
        } else {
            f = stdin;
            d += 1;
        }
    }
    if(d == 0){
        char path[strlen(tokens[1]) + 1 + strlen(get_path())];
        strcpy(path, get_path());
        strcat(path, tokens[1]);
        f = fopen(path, "r");
        if(f == NULL){
            display_error("ERROR: Cannot open file: ", tokens[1]);
            return -1;
        }
    }
    
    char c[2];
    while(fread(&c[0], 1, 1, f)){
        c[1] ='\0';
        display_message(c);
    }

   
    if(d == 0 && fclose(f) != 0){
        display_error("ERROR: Cannot close file: ", tokens[1]);
        return -1;
    }else{clearerr(stdin);}
    return 0;
}


ssize_t bn_wc(char **tokens) {
    FILE *f;
    int d = 0, word_count = 0, char_count = 0, newline_count = 0, w = 0;
    char ch, s[256];
    if(tokens[1] == NULL){
        struct pollfd fds;
        fds.fd = 0;     
        fds.events = POLLIN; 
        int ret = poll(&fds, 1, 10); // in your project, set this to 10, not 3000.
        if (ret == 0) {
            display_error("ERROR: No input source provided","");
            clearerr(stdin);
            return -1;
        } else {
            f = stdin;
            d += 1;
        }
    }

    if(d == 0){
        char path[strlen(tokens[1]) + 1 + strlen(get_path())];  
        strcpy(path, get_path());  
        strcat(path, tokens[1]);  
        f = fopen(path, "r");  
        if(f == NULL){  
            display_error("ERROR: Cannot open file: ", tokens[1]);  
            return -1;
        }  
    }  
      
    while(fread(&ch, sizeof(char), 1, f)){  
        char_count++;
        if(ch != ' ' && ch != '\t' && ch != '\n' && ch != '\r'){
            w = 1;
        }else if (ch == '\n' || ch == '\r') {
            newline_count++;
            if (w) {
                word_count++;
                w = 0;
            }
        }else if (ch == ' ' || ch == '\t') {
            if (w) {
                word_count++;
                w = 0;
            }
        }
    }
    if (w) {
        word_count++;
        w = 0;
    }

    sprintf(s, "word count %d\n", word_count);
    display_message(s);

    sprintf(s, "character count %d\n", char_count);
    display_message(s);

    sprintf(s, "newline count %d\n", newline_count);
    display_message(s);

    if (d == 0){
    if(fclose(f) != 0){
        display_error("ERROR: Cannot close file: ", tokens[1]);
        return -1;}
    }else{clearerr(stdin);}
    return 0;
}


ssize_t bn_cd(char **tokens){
    char t[1024] = "/", *temp_way = t + 1, temp[1024];
    if(tokens[1] == NULL){strcpy(get_path(), "./");return 0;}
    char *dir = strtok(tokens[1], "/");

    while (dir != NULL) {
        int i = 0;
        while(i < strlen(dir) && dir[i] == '.'){i++;}
        if (i != strlen(dir) || i < 3) {
            strcat(temp_way, dir);
            strcat(temp_way, "/");
        }
        else {
            for (int a = 0; a < i - 1; a++)
                strcat(temp_way, "../");
        }
        dir = strtok(NULL, "/");
    }

    struct stat sd;
    if (tokens[1][0] != '/') {
        strcpy(temp, get_path());
        strcat(get_path(), temp_way);
        if (!(stat(get_path(), &sd) == 0 && S_ISDIR(sd.st_mode))) {
            display_error("ERROR: Invalid path", "");
            strcpy(get_path(), temp);
            return -1;
        } 
    }
    else {
        if (!(stat(temp_way-1, &sd) == 0 && S_ISDIR(sd.st_mode))) {
            display_error("ERROR: Invalid path", "");
            return -1;
        } strcpy(get_path(), temp_way-1);
    }
    return 0;
}



size_t h1_p(char *temp, char*sub, int dep, int rec){
    if (rec && dep == 0) return 0;
    char path[1024];
    struct dirent *d;
    DIR *dir = opendir(temp);
    if (dir == NULL) {
        display_error("ERROR: Invalid path", "");
        return -1;
    }
    // if(rec){display_message(".");}
    // if(rec){display_message("..");}
    while ((d = readdir(dir)) != NULL) {
        if(rec && sub == NULL){
        if (strcmp(d->d_name, ".") == 0){
            display_message(".");
            display_message("\n");
            continue;
        }else if(strcmp(d->d_name, "..") == 0) {
            display_message("..");
            display_message("\n");
            continue;}
        }
        
        if (sub == NULL) {
            display_message(d->d_name);
            display_message("\n");
        }else {
            if (strstr(d->d_name, sub) != NULL) {
                display_message(d->d_name);
                display_message("\n");
            }
        }
        if (rec && d->d_type == DT_DIR) {
            strcpy(path, temp);
            strcat(path, "/"); 
            strcat(path, d->d_name);
            h1_p(path, sub, dep - 1, 1);
        }
    }
    closedir(dir);
    return 0;
}


ssize_t bn_ls(char **tokens){

    char *path = NULL, *sub = NULL, *rec = NULL, temp[1024];
    int depth = -1, i = 1;
    
    strcpy(temp, get_path());

    while(tokens[i]) {
        if (strcmp(tokens[i], "--f") == 0) {sub = tokens[i+1];i++;}
        else if (strcmp(tokens[i], "--rec") == 0) {rec = tokens[i+1];i++;}
        else if (strcmp(tokens[i], "--d") == 0) {depth = strtol(tokens[i+1], NULL, 10);i++;} 
        else {path = tokens[i];}i++;
    }

    if (rec != NULL && depth == -1){
        display_error("ERROR: Invalid path", "");
        return -1;
    }

    if (rec == NULL && depth != -1) {
        display_error("ERROR: Invalid path", "");
        return -1;
    }

    if(rec == NULL) {
        if (path != NULL) {
        if (path[0] == '/')
            strcpy(temp, path);
        else
            strcat(temp, path);
    }
        return h1_p(temp, sub, 1, 0);
    }else{
        if (rec[0] == '/'){strcpy(temp, rec);}
        else{strcat(temp, rec);}
        return h1_p(temp, sub, depth, 1);
    }return 0;
}


ssize_t bn_kill(char **tokens){
    int i, j;
    if (tokens[1] == NULL){
        display_error("ERROR: Invalid signal specified", "");
        return -1;
    }
    i = atoi(tokens[1]);
    if(tokens[2] == NULL){
        if(kill(i, 15) == -1){
            display_error("ERROR: The process does not exist", "");
            return -1;
        }
    }else{
        j = atoi(tokens[2]);
        if(kill(i, j) == -1){
            display_error("ERROR: Invalid signal specified", "");
            return -1;
        }
    }
    return 0;
}

ssize_t bn_ps(char **tokens){
    cur_ps();
    return 0;
}
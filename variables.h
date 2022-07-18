#ifndef __VARIABLES_H__
#define __VARIABLES_H__

#include <string.h>
#include <stdlib.h>

typedef struct variable{
 char name[512];
 char value[521];
 struct variable *next;
} Variable;


typedef struct background {
    int idx;
    pid_t pid;
    char cmd[50];
    struct background *next;
} Background;



void create_variable(char *name, char *value);
char *get_value(char *name);

int new_idx();
void check_bg(pid_t pid);
void set_bg();
void cur_ps();


#endif
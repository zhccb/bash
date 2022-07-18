#include <stdio.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "variables.h"
#include "builtins.h"
#include "io_helpers.h"

Variable *head = NULL;
Background *head_bg = NULL;
char buf[64];




void create_variable(char *name, char *value){
    Variable *new_v = malloc(sizeof(Variable));
    strcpy(new_v->name, name);
    strcpy(new_v->value, value);
    new_v->next = head;
    head = new_v;
}

char *get_value(char *name){
    Variable *curr = head;
    while(curr != NULL){
        if (strcmp(curr->name, &name[1]) == 0){
            return curr->value;
        }
        curr = curr->next;
    }
    return name;
}


int new_idx(){
    int i = 0;
    Background *curr = head_bg;
    if (curr == NULL) { i = 1;}
    while(curr != NULL){
        if(curr->idx > i){
            i = curr->idx + 1;
        }
        curr = curr->next;
    }
    return i;
}

void set_bg(pid_t p, char *cmd){
    int i = new_idx();
    sprintf(buf, "[%d] %d\n", i, p);
    display_message(buf);
    Background *new = malloc(sizeof(Background));
    new->idx = i;
    new->pid = p;
    strcpy(new->cmd, cmd);
    new->next = head_bg;
    head_bg = new;

}


void check_bg(pid_t pid){
    Background *curr = head_bg;
    Background *last = NULL;
    while(curr != NULL){
        if (curr->pid == pid) {
            sprintf(buf, "[%d]+  Done %s\n", curr->idx, curr->cmd);
            display_message(buf);


            if (last == NULL) {
                head_bg = curr->next;
            } else {
                last->next = curr->next;
            }
            free(curr);
            break;
        } 
        last = curr;
        curr = curr->next;
    }
}


void cur_ps(){
    Background *curr = head_bg;
    while(curr != NULL){
        sprintf(buf, "%s %d\n", curr->cmd, curr->pid);
        display_message(buf);
        curr = curr->next;
    }
}
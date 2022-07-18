#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "builtins.h"
#include "io_helpers.h"
#include "variables.h"
#include "commands.h"


int main(int argc, char* argv[]) {
    char *prompt = "mysh$ "; // TODO Step 1, Uncomment this.

    char input_buf[MAX_STR_LEN + 1];
    input_buf[MAX_STR_LEN] = '\0';
    char *token_arr[MAX_STR_LEN] = {NULL};
    char *cmd_arr[MAX_STR_LEN] = {NULL};

    unkill();



    while (1) 
    {
        // Prompt and input tokenization

        // TODO Step 2:
        // Display the prompt via the display_message function.
        display_message(prompt);

        int ret = get_input(input_buf);
        

        // Clean exit
        if (ret == 0 || (ret != 0 && (strncmp("exit", input_buf, 4) == 0))) {
            check_close("close-server");
            break;
        }

        int pid;
        while ((pid = waitpid(-1, NULL, WNOHANG)) > 0) {
            check_bg(pid);
        }


        if(strstr(input_buf, "start-server") != NULL){
            check_connect(input_buf);
            continue;
        }
        if(strstr(input_buf, "start-client") != NULL){
            check_client(input_buf);
            continue;
        }
        if(strstr(input_buf, "send") != NULL){
            check_send(input_buf);
            continue;
        }
        if(strstr(input_buf, "close-server") != NULL){
            check_close(input_buf);
            continue;
        }


        int rbg = 0;
        if(strlen(input_buf) > 1 && input_buf[strlen(input_buf) - 2] == '&'){rbg = 1;}
        pid_t p = -1;
        if(rbg){
            input_buf[strlen(input_buf) - 2] = '\0';
            if ((p = fork()) == -1) display_error("failed to pipe (fork)", "");
            if(p > 0){
                set_bg(p, input_buf);
                continue;
            }
        }



        int num_cmd = split_cmd(input_buf, cmd_arr);

        int fd[num_cmd-1][2];
        if(num_cmd > 1){
            for (int i = 0; i < num_cmd-1; i++) 
                pipe(fd[i]);
        }

        for (int i = 0; i < num_cmd; i++) {
            pid_t p;
            if (num_cmd > 1 && (p = fork()) == -1) {
                display_error("failed to pipe (fork).", "");
            }
            else if (num_cmd > 1 && p == 0) {
                
                if (i < num_cmd - 1) {dup2(fd[i][1], STDOUT_FILENO);}
                 
                if (i > 0) {dup2(fd[i-1][0], STDIN_FILENO);}

                for (int j = 0; j < num_cmd - 1; j++){        
                    close(fd[j][0]);
                    close(fd[j][1]);
                }

                size_t token_count = tokenize_input(cmd_arr[i], token_arr);

                exe(token_count, token_arr);
                return 0;
            }else if(num_cmd == 1){
                size_t token_count = tokenize_input(cmd_arr[0], token_arr);
                exe(token_count, token_arr);
            }
        }
        int i = 0;
        while (i < num_cmd && num_cmd > 1){
            if((i + 1) < num_cmd){
                close(fd[i][0]);
                close(fd[i][1]);
            }
            wait(NULL);
            i++;
        }

        if (rbg) {return 0;}

    }

    return 0;
}
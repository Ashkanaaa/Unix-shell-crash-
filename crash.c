#include <stdio.h>
#include <errno.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>



#define MAXJOBS 32
#define MAXLINE 1024

char **environ;


int count = 0; //order of the job
char msg[256]; //char pointer for the job
int place =0; //for managing the indext of the array in which the jobs are stored



// TODO: you will need some data structure(s) to keep track of jobs

typedef struct J_tracker{
    int order;
    pid_t PID;
    char *cmd;
    bool done;
    bool fg;
    
}J_tracker;

J_tracker jobs_array[32];

struct J_tracker my_tracker; //real jobs
struct J_tracker dummy_tracker;//dummy jobs for filling the array
struct J_tracker my_fg; 


void handle_sigchld(int sig) {
    int index=0;
    int stat;
    bool Index_mod = false;

    while (1) {
    int pid = waitpid(-1, &stat, WNOHANG);
    if (pid <= 0) {
        break;
    }

    if(my_fg.fg && my_fg.PID ==pid){
    //printf("recieved");
       my_fg.fg = false;
       
    }

    if(WTERMSIG(stat) == SIGQUIT){
        for(int x=0;x<MAXJOBS;x++){
            if(jobs_array[x].PID == pid && jobs_array[x].order >= 0){
                index = x;
            }
        }
            
        int len = snprintf(msg, sizeof(msg), "[%d] (%d)  killed (core dumped)  %s\n", jobs_array[index].order, jobs_array[index].PID, jobs_array[index].cmd);
        write(STDOUT_FILENO, msg, len);
        jobs_array[index].done = true;    


    }

    else if(WTERMSIG(stat) == SIGINT){
        for(int x=0;x<MAXJOBS;x++){
            if(jobs_array[x].PID == pid && jobs_array[x].order >= 0){
                index = x;
            }
        }
            
        int len = snprintf(msg, sizeof(msg), "[%d] (%d)  killed  %s\n", jobs_array[index].order, jobs_array[index].PID, jobs_array[index].cmd);
        write(STDOUT_FILENO, msg, len);
        jobs_array[index].done = true;    

    }

    else if(WTERMSIG(stat) == SIGKILL){
        
        for(int x=0;x<MAXJOBS;x++){
            if(jobs_array[x].PID == pid && jobs_array[x].order >= 0){
                index = x;
            }
        }
            
        int len = snprintf(msg, sizeof(msg), "[%d] (%d)  killed  %s\n", jobs_array[index].order, jobs_array[index].PID, jobs_array[index].cmd);
        write(STDOUT_FILENO, msg, len);
        jobs_array[index].done = true;    
        

    }else if(WIFEXITED(stat)){
        
        for(int x=0;x<MAXJOBS;x++){
            if(jobs_array[x].PID == pid && jobs_array[x].order >= 0 &&jobs_array[x].done ==false){
                Index_mod =true;
                
                index = x;
            }
        }

        if(Index_mod){
            int len = snprintf(msg, sizeof(msg), "[%d] (%d)  finished  %s\n", jobs_array[index].order, jobs_array[index].PID, jobs_array[index].cmd);
            write(STDOUT_FILENO, msg, len);
            jobs_array[index].done = true;
        }
        
    }

    


    }
    // TODO
    // while (waitpid(-1, NULL, WNOHANG)!= -1) {
    // // write(STDOUT_FILENO,"parent: child died\n",14);


    //     int len = snprintf(msg, sizeof(msg), "[%d] (%d) finished %s\n", count, jobs_array[count-1].PID, jobs_array[count-1].cmd);
    //     write(STDOUT_FILENO, msg, len);

    // }

}

void handle_sigtstp(int sig) {
    // TODO
}

void handle_sigint(int sig) {
    // TODO
    // if(my_fg.fg){
    //     int len = snprintf(msg, sizeof(msg), "[%d] (%d)  killed  %s\n", my_fg.order, my_fg.PID, my_fg.cmd);
    //     write(STDOUT_FILENO, msg, len);
        
    // }
}

void handle_sigquit(int sig) {
    // TODO
    if(my_fg.fg == false){
        exit(1);
    }
}

void install_signal_handlers() {
    // TODO

    struct sigaction act;
    act.sa_handler = handle_sigchld;
    act.sa_flags = SA_RESTART;
    sigemptyset(&act.sa_mask);
    sigaction(SIGCHLD, &act, NULL);

    struct sigaction act2;
    act2.sa_handler = handle_sigint;
    act2.sa_flags = SA_RESTART;
    sigemptyset(&act2.sa_mask);
    sigaction(SIGINT, &act2, NULL);

    struct sigaction act3;
    act3.sa_handler = handle_sigquit;
    act3.sa_flags = SA_RESTART;
    sigemptyset(&act3.sa_mask);
    sigaction(SIGQUIT, &act3, NULL);

}

void spawn(const char **toks, bool bg) { // bg is true iff command ended with &
    // TODO
    
    
    count++; 
    if(bg){
        sigset_t mask;
        sigemptyset(&mask);
        sigaddset(&mask, SIGCHLD);
        sigprocmask(SIG_BLOCK, &mask, NULL);
        
        pid_t p1 = fork(); 
        if(p1 == -1){printf("fork fails\n");}  //check if fork has been done correctley 
                       
        if(p1!=0){
            printf("[%d] (%d)  running  %s\n", count, p1, toks[0]);
        }

        //child
        if(p1 == 0){
            
            
            sigprocmask(SIG_UNBLOCK, &mask, NULL);

            int a = execvp(toks[0], toks); 
            if(a==-1){printf("ERROR: cannot run %s\n", toks[0]);} 

            exit(1);
            
        }else{ 
            
            my_tracker.order = count;
            my_tracker.PID = p1;
            my_tracker.done = false;
            my_tracker.cmd = malloc(sizeof(toks[0]));
            strcpy(my_tracker.cmd, toks[0]);

            if(place < MAXJOBS){
                jobs_array[place] = my_tracker;
                place++;
            }else{

                place = Find_Place();
                if(place !=-1){
                    jobs_array[place] = my_tracker;
                }

                place++;
            }

            sigprocmask(SIG_UNBLOCK, &mask, NULL);
             
        }

    }
///////////////////////////////////////////////////////////////////
    else{
        
        sigset_t mask;
        sigemptyset(&mask);
        sigaddset(&mask, SIGCHLD);
        sigprocmask(SIG_BLOCK, &mask, NULL);

        pid_t p1 = fork(); 
        if(p1 == -1){printf("fork fails");}                    //check if fork has been done correctley
        
        //child
        if (p1 == 0) { 
            sigprocmask(SIG_UNBLOCK, &mask, NULL);
            int b = execvp(toks[0], toks); 
            if(b==-1){printf("ERROR: cannot run %s\n", toks[0]);}
                    
            exit(1);    
             
        } else {
            my_fg.fg = true;
            my_fg.order = count;
            my_fg.PID = p1;
            my_fg.done = true;
            my_fg.cmd = malloc(sizeof(toks[0]));
            strcpy(my_fg.cmd, toks[0]);

            if(place < MAXJOBS){
                jobs_array[place] = my_fg;
                place++;
            }else{

                place = Find_Place();
                if(place !=-1){
                    jobs_array[place] = my_fg;
                }

                place++;
            }
            
            sigprocmask(SIG_UNBLOCK, &mask, NULL);

            while(my_fg.fg){
                sleep(0.001);
            }
                
            
        }   
    }
}

void cmd_jobs(const char **toks) {
    // TODO
    if (toks[1] != NULL) {
        fprintf(stderr, "ERROR: jobs takes no arguments\n");
    }else{
        for(int x = 0;x<MAXJOBS;x++){
        if(jobs_array[x].done == false && jobs_array[x].order >= 0){
        printf("[%d] (%d)  running  %s\n", jobs_array[x].order,jobs_array[x].PID, jobs_array[x].cmd);
        }
    }
    }
    

}

void cmd_fg(const char **toks) {
    // TODO
    bool found =false;
    if(toks[1] ==NULL){
        printf("ERROR: fg takes exactly one argument\n");
    }else{

        if(toks[1][0] == '%'){
            // printf("AAAAAA");
            int num;
            char *job_id_str = strchr(toks[1], '%');
            num = (int) strtol(job_id_str + 1, NULL, 10);
            

            for(int i =0;i<MAXJOBS;i++){
                if(jobs_array[i].order>=0 && jobs_array[i].done == false && jobs_array[i].order == num){
                    jobs_array[i].done = true;//elinminate the bg job

                    my_fg.order = jobs_array[i].order;
                    my_fg.PID = jobs_array[i].PID;
                    my_fg.cmd = jobs_array[i].cmd;
                    my_fg.done = true;//////
                    my_fg.fg = true;

                    
                    found =true;
                }
            }
                if(found == false){
                    printf("ERROR: no job %%%d\n", num);
                }   
            Bring_FG();
        }else{
            int num;
            
            //char *job_id_str = strchr(toks[1], '%');
            num = (int) strtol(toks[1], NULL, 10);
            

            for(int i =0;i<MAXJOBS;i++){
                if(jobs_array[i].order>=0 && jobs_array[i].done == false && jobs_array[i].PID == num){
                    jobs_array[i].done = true;//elinminate the bg job

                    my_fg.order = jobs_array[i].order;
                    my_fg.PID = jobs_array[i].PID;
                    my_fg.cmd = jobs_array[i].cmd;
                    my_fg.done = true; ////
                    my_fg.fg = true;

                    
                    found =true;
                }
            }
                if(found == false){
                    printf("ERROR: no PID %d\n", num);
                }   
            Bring_FG();
        }

    }
    

}

void cmd_bg(const char **toks) {
    // TODO
    
}

void cmd_nuke(const char **toks) {
    // TODO
    
    int signal = SIGKILL;
    int result =0;
    bool found =false;

    if(toks[1] ==NULL){
        for(int x=0;x<MAXJOBS;x++){
            if(jobs_array[x].order>=0 && jobs_array[x].done == false){
                
                result = kill(jobs_array[x].PID, signal);
                if(result != 0){
                printf("Failed to kill process %d.\n", jobs_array[x].PID);
                }
            }
            
        }
    }else{
        
        
        if(toks[1][0] == '%'){
            // printf("AAAAAA");
            int num;
            char *job_id_str = strchr(toks[1], '%');
            num = (int) strtol(job_id_str + 1, NULL, 10);
            

            for(int i =0;i<MAXJOBS;i++){
                if(jobs_array[i].order>=0 && jobs_array[i].done == false && jobs_array[i].order == num){

                    result = kill(jobs_array[i].PID, signal);
                    if(result != 0){
                    printf("Failed to kill process %d.\n", jobs_array[i].PID);
                    }
                    
                    found =true;
                }
            }
            if(found == false){
                    printf("ERROR: no job %%%d\n", num);
                }   
        }else{
            // printf("BBBBB");
            int num;
            num = (int) strtol(toks[1], NULL, 10);
            
            for(int i =0;i<MAXJOBS;i++){
                if(jobs_array[i].order>=0 && jobs_array[i].done == false && jobs_array[i].PID == num){

                    result = kill(jobs_array[i].PID, signal);
                    if(result != 0){
                    printf("Failed to kill process %d.\n", jobs_array[i].PID);
                    }
                    
                    found =true;
                }
                
            }
                if(found == false){
                    printf("ERROR: no PID %d\n", num);
                }   
        }

    }

    
    
}

void cmd_quit(const char **toks) {
    if (toks[1] != NULL) {
        fprintf(stderr, "ERROR: quit takes no arguments\n");
    } else {
        exit(0);
    }
}

void eval(const char **toks, bool bg) { // bg is true iff command ended with &
    assert(toks);
    if (*toks == NULL) return;
    if (strcmp(toks[0], "quit") == 0) {
        cmd_quit(toks);
    // TODO: other commands
    } else if (strcmp(toks[0], "jobs") == 0) {
        cmd_jobs(toks);
    // TODO: other commands
    } else if(strcmp(toks[0], "nuke")==0){
           cmd_nuke(toks); 
    }else if(strcmp(toks[0], "fg")==0){
            cmd_fg(toks);
    }
    else {
        spawn(toks, bg);
    }
}

// you don't need to touch this unless you want to add debugging
void parse_and_eval(char *s) {
    assert(s);
    const char *toks[MAXLINE + 1];
    
    while (*s != '\0') {
        bool end = false;
        bool bg = false;
        int t = 0;

        while (*s != '\0' && !end) {
            while (*s == '\n' || *s == '\t' || *s == ' ') ++s;
            if (*s != ';' && *s != '&' && *s != '\0') toks[t++] = s;
            while (strchr("&;\n\t ", *s) == NULL) ++s;
            switch (*s) {
            case '&':
                bg = true;
                end = true;
                break;
            case ';':
                end = true;
                break;
            }
            if (*s) *s++ = '\0';
        }
        toks[t] = NULL;
        eval(toks, bg);
    }
}

// you don't need to touch this unless you want to add debugging
void prompt() {
    printf("crash> ");
    fflush(stdout);
}

// you don't need to touch this unless you want to add debugging
int repl() {
    char *buf = NULL;
    size_t len = 0;
    while (prompt(), getline(&buf, &len, stdin) != -1) {
        parse_and_eval(buf);
    }

    if (buf != NULL) free(buf);
    if (ferror(stdin)) {
        perror("ERROR");
        return 1;
    }
    return 0;
}

int Find_Place(){
    for(int i=0;i<MAXJOBS;i++){
        if(jobs_array[i].done == true){
            return i;
        }
    }
    return -1;
}

void Init_Array(){
    
    dummy_tracker.done = true;
    dummy_tracker.order = -11;
    dummy_tracker.PID = -10;
    dummy_tracker.cmd = "dummy Job";

    
    for(int i =0;i <MAXJOBS;i++){
            jobs_array[i] = dummy_tracker;
    }
   
}

int Bring_FG(){
    while(my_fg.fg){
                sleep(0.001);
    }
}

// you don't need to touch this unless you want to add debugging options
int main(int argc, char **argv) {
    Init_Array();
    install_signal_handlers();
    return repl();
}




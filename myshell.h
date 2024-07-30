// myshell.h

#ifndef MYSHELL_H
#define MYSHELL_H

#include <stdio.h> // Include stdio.h for getline
#include <signal.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <assert.h>
#include <termios.h>

// Function prototypes
int num_builtins(void);
char *myshell_readline(void);
char **myshell_splitline(char *line);
void free_args(char **args); // Corrected the prototype

// Define constants
#define RL_BUFSIZE 1024
#define ARG_BUFSIZE 64
#define ARG_DELIM " \t\r\n\a"
#define symbols "<|>"

//Struct for making the Shell
struct general_cmd {
    int command_type;          
};

struct execcmd {
    int command_type;
    char *argv[ARG_BUFSIZE]; 
};

struct redirected_command {
    int command_type;          // < or >
    char *file;        // the input/output file
    int mode;          // the mode to open the file with
    int fd;            // the file descriptor number to use for the file  
    struct general_cmd *general_cmd;   // the command to be run (e.g., an execcmd)
};

struct pipecmd {
    int command_type;          // |
    struct general_cmd *first_part;  // first_part is left side of pipe
    struct general_cmd *second_part; // second_part is right side of pipe
};

struct general_cmd *parsecmd(char*);


struct general_cmd *parseline(char**, char*);
struct general_cmd *parsepipe(char**, char*);
struct general_cmd *parseexec(char**, char*);


// Define builtin commands
int myshell_cd(char **args);
int myshell_clr(char **args);
int myshell_dir(char **args);
int myshell_environ(char **args);
int myshell_echo(char **args);
int myshell_help(char **args);
int myshell_pause(char **args);
int myshell_quit(char **args);


void sig_handler(int num);
void run_command(struct general_cmd *general_cmd); //run the internal command like "ls cat etcs"

void execute_command(char *buf); //run the external command liek "dir, echo, help etc" if the given 
//command is not a built in command it will pass that string to the runCommand for the internal command

int getcmd(char *buf, int nbuf);
void exit_shell();


int gettoken(char **ps, char *es, char **q, char **eq);
int peek(char **ps, char *es, char *toks);
char *mkcopy(char *s, char *es);
int splitArraySize(char **splitArray);
void printPrompt();
struct general_cmd* parsecmd(char *s);
struct general_cmd* parseline(char **ps, char *es);
struct general_cmd* parsepipe(char **ps, char *es);
struct general_cmd* parseredirs(struct general_cmd *general_cmd, char **ps, char *es);
struct general_cmd* parseexec(char **ps, char *es);

int main_(int argc, char **argv);
#endif
// utility.c
#include "myshell.h"

int forgrounds[RL_BUFSIZE];
int background[RL_BUFSIZE];
int my_pid;
int bind=0;
int ind=0;

char **myshell_splitline(char *line) {
    int bufsize = ARG_BUFSIZE, position = 0;
    char **tokens = malloc(bufsize * sizeof(char *));
    if (!tokens) {
        perror("Allocation error");
        exit(EXIT_FAILURE);
    }
    char *token = strtok(line, ARG_DELIM);
    while (token != NULL) {
        tokens[position++] = strdup(token); 
        if (position >= bufsize) {
            bufsize += ARG_BUFSIZE;
            tokens = realloc(tokens, bufsize * sizeof(char *));
            if (!tokens) {
                perror("Allocation error");
                exit(EXIT_FAILURE);
            }
        }
        token = strtok(NULL, ARG_DELIM);
    }
    tokens[position] = NULL;
    return tokens;
}

void free_args(char **args) {
    for (int i = 0; args[i] != NULL; i++) {
        free(args[i]);
    }
    free(args);
}

struct general_cmd* execcmd(void) {
    struct execcmd *general_cmd;

    general_cmd = malloc(sizeof(*general_cmd));
    memset(general_cmd, 0, sizeof(*general_cmd));
    general_cmd->command_type = ' ';
    return (struct general_cmd*)general_cmd;
}

struct general_cmd* redirected_command(struct general_cmd *subcmd, char *file, int command_type) {
    struct redirected_command *general_cmd;

    general_cmd = malloc(sizeof(*general_cmd));
    memset(general_cmd, 0, sizeof(*general_cmd));
    general_cmd->command_type = command_type;
    general_cmd->general_cmd = subcmd;
    general_cmd->file = file;
    general_cmd->mode = (command_type == '<') ?  O_RDONLY : O_WRONLY|O_CREAT|O_TRUNC;
    general_cmd->fd = (command_type == '<') ? 0 : 1;
    return (struct general_cmd*)general_cmd;
}

struct general_cmd* pipecmd(struct general_cmd *first_part, struct general_cmd *second_part) {
    struct pipecmd *general_cmd;

    general_cmd = malloc(sizeof(*general_cmd));
    memset(general_cmd, 0, sizeof(*general_cmd));
    general_cmd->command_type = '|';
    general_cmd->first_part = first_part;
    general_cmd->second_part = second_part;
    return (struct general_cmd*)general_cmd;
}


void sig_handler(int num){
    write(STDOUT_FILENO,"\nCtrl+c is handled\n",20);
    ind = 0;
    int i = 0;
    for(; i < ind; i++) kill(forgrounds[i],SIGKILL);
}


// Execute general_cmd.  Never returns.
void run_command(struct general_cmd *general_cmd) {
    struct execcmd *ecmd;
    struct pipecmd *pcmd;
    struct redirected_command *rcmd;

    if(general_cmd == 0)
        exit(0);
    switch(general_cmd->command_type){
    default:
        fprintf(stderr, "unknown run_command\n");
        exit(-1);

    case ' ':
        ecmd = (struct execcmd*)general_cmd;
        if(ecmd->argv[0] == 0)
            exit(0);
        execvp(ecmd->argv[0],ecmd->argv);
        break;

    case '>':
    case '<':
        rcmd = (struct redirected_command*)general_cmd;
        if(rcmd->command_type == '<'){
            int file = open(rcmd->file,O_RDONLY, 0777);
            dup2(file,0);
        }
        else {
            // write output to file instead of stdout
            int file = open(rcmd->file,O_WRONLY | O_TRUNC | O_CREAT, S_IRUSR | S_IRGRP | S_IWGRP | S_IWUSR);
            if(file < 0) {
                fprintf(stderr, "failed to open file\n");
                exit(1);
            }
            dup2(file,1);
        }
        run_command(rcmd->general_cmd);
        break;

    case '|':
        pcmd = (struct pipecmd*)general_cmd;
        
        int filedes[2];
        pipe(filedes);
        int rv;
        rv = fork();
        if (rv == 0) {
            // child proces will write to pipe and parent will read it
            close(filedes[0]); // close unnecessary pipe end for reading in child process
            dup2(filedes[1],1); // redirect output to pipe instead of stdout
            run_command(pcmd->first_part);
            close(filedes[0]);
        }
        else{
            close(filedes[1]); // close unnecessary pipe end for writing in child process
            dup2(filedes[0],0);
            run_command(pcmd->second_part);
            close(filedes[0]);
        }
        break;
    }
    exit(0);
}

void printPrompt(){
    if (isatty(fileno(stdin))){
        char hostname[50];
        gethostname(hostname,50);
        char current_dir[200];
        getcwd(current_dir,200);
        fprintf(stdout,"%s@%s:%s$ ",getenv("USER"),hostname,current_dir);
    }
}

int getcmd(char *buf, int nbuf) {

    printPrompt();
    memset(buf, 0, nbuf);
    fgets(buf, nbuf, stdin);
    if(buf[0] == 0) // EOF
        return -1;

    if(buf[strlen(buf) - 1] == '\n' ){
        buf[strlen(buf) - 1] = '\0';
    }
    return 0;
}


int gettoken(char **ps, char *es, char **q, char **eq) {
    char *s;
    int ret;
  
    s = *ps;
    while(s < es && strchr(ARG_DELIM, *s))
        s++;
    if(q)
        *q = s;
    ret = *s;
    switch(*s){
    case 0:
        break;
    case '|':
    case '<':
        s++;
        break;
    case '>':
        s++;
        break;
    default:
        ret = 'a';
        while(s < es && !strchr(ARG_DELIM, *s) && !strchr(symbols, *s))
            s++;
        break;
    }
    if(eq)
        *eq = s;
  
    while(s < es && strchr(ARG_DELIM, *s))
        s++;
    *ps = s;
    return ret;
}

int peek(char **ps, char *es, char *toks) {
    char *s;
  
    s = *ps;
    while(s < es && strchr(ARG_DELIM, *s))
        s++;
    *ps = s;
    return *s && strchr(toks, *s);
}

char *mkcopy(char *s, char *es) {
    int n = es - s;
    char *c = malloc(n+1);
    assert(c);
    strncpy(c, s, n);
    c[n] = 0;
    return c;
}

struct general_cmd* parsecmd(char *cmdLine) {
    char *endOfString;
    struct general_cmd *cmd;

    endOfString =  cmdLine + strlen(cmdLine);
    cmd = parseline(&cmdLine, endOfString);
    peek(&cmdLine, endOfString, "");
    if(cmdLine != endOfString){
        fprintf(stderr, "leftovers: %s\n", cmdLine);
        exit(-1);
    }

    return cmd;
}


struct general_cmd* parseline(char **ps, char *es) {
    struct general_cmd *general_cmd;
    general_cmd = parsepipe(ps, es);
    return general_cmd;
}

struct general_cmd* parsepipe(char **ps, char *es) {
    struct general_cmd *general_cmd;

    general_cmd = parseexec(ps, es);
    if(peek(ps, es, "|")){
        gettoken(ps, es, 0, 0);
        general_cmd = pipecmd(general_cmd, parsepipe(ps, es));
    }
    return general_cmd;
}

struct general_cmd* parseredirs(struct general_cmd *general_cmd, char **ps, char *es) {
    int tok;
    char *q, *eq;

    while(peek(ps, es, "<>")){
        tok = gettoken(ps, es, 0, 0);
        if(gettoken(ps, es, &q, &eq) != 'a') {
            fprintf(stderr, "missing file for redirection\n");
            exit(-1);
        }
        switch(tok){
        case '<':
            general_cmd = redirected_command(general_cmd, mkcopy(q, eq), '<');
            break;
        case '>':
            general_cmd = redirected_command(general_cmd, mkcopy(q, eq), '>');
            break;
        }
    }
    return general_cmd;
}

struct general_cmd* parseexec(char **ps, char *es) {
    char *q, *eq;
    int tok, argc;
    struct execcmd *general_cmd;
    struct general_cmd *ret;
  
    ret = execcmd();
    general_cmd = (struct execcmd*)ret;

    argc = 0;
    ret = parseredirs(ret, ps, es);
    while(!peek(ps, es, "|")){
        if((tok=gettoken(ps, es, &q, &eq)) == 0)
            break;
        if(tok != 'a') {
            fprintf(stderr, "syntax error\n");
            exit(-1);
        }
        general_cmd->argv[argc] = mkcopy(q, eq);
        argc++;
        if(argc >= ARG_BUFSIZE) {
            fprintf(stderr, "too many args %d\n", argc);
            exit(-1);
        }
        ret = parseredirs(ret, ps, es);
    }
    general_cmd->argv[argc] = 0;
    return ret;
}


int splitArraySize(char **splitArray){
    int size = 0;
    if(splitArray == NULL){
        return size;
    }
    for(int i = 0; splitArray[i] != NULL; i++){
        size ++;
    }
    return size;
}

void run_built_in_command(char * buf){

    if(buf[strlen(buf) - 1] == '&'){
        char *tok = strtok(buf,"&");
        if(fork() == 0){
            setpgid(0,0);
            printf("\n+[%d] done\n",getpid());
            background[bind++] = getpid();
            run_command(parsecmd(tok));
            signal(SIGINT,sig_handler);
        }
    }
    else{
        int forkId = fork();
        if(forkId == 0){
            forgrounds[ind++] = getpid();
            run_command(parsecmd(buf));
        }
        else{
            wait(&forkId);
        }
    }
}

int myshell_cd(char **args){
    if (args[1] == NULL) {
        char cwd[1024];
        if (getcwd(cwd, sizeof(cwd)) != NULL) {
            printf("Current directory: %s\n", cwd);
        } else {
            fprintf(stderr, "Error: Unable to get current directory\n");
            return 1;
        }
    } else {
        if (chdir(args[1]) != 0) {
            fprintf(stderr, "Error: Unable to change directory\n");
            return 1;
        }
    }
    return 0;
}
int myshell_clr(char **args){
    run_built_in_command("clear");
    return 0;
}
int myshell_dir(char **args){
    int splitSize = splitArraySize(args);
    char fullcmd[RL_BUFSIZE] =  "ls -al";

    if(splitSize != 1){
        //if user enter dir as well as something other forward by dir we need to append that after "ls -al"
        for (int i = 1; args[i] != NULL; i++){
            strcat(fullcmd, " ");
            strcat(fullcmd, args[i]);
        }
    }
    run_built_in_command( fullcmd);

    return 0;
}
int myshell_environ(char **args){
    //I'm using echo built in commnad for displaying the environ varaibles
    char fullcmd[RL_BUFSIZE*2] = "echo";
    extern char **environ;
    int splitSize = splitArraySize(args);

    for (char **env = environ; *env != NULL; env++) {
        strcat(fullcmd, " ");
        strcat(fullcmd, *env);
    }

    if(splitSize != 1){
        //if user enter environ as well as something other forward by environ
        for (int i = 1; args[i] != NULL; i++){
            strcat(fullcmd, " ");
            strcat(fullcmd, args[i]);
        }
    }
    // printf("[%s]", fullcmd);
    run_built_in_command( fullcmd);

    return 0;
}
int myshell_echo(char **args){
    char fullcmd[RL_BUFSIZE] = "echo";
    for (int i = 1; args[i] != NULL; i++){

        if (args[i][0] == '$') {
            char *expanded = getenv(args[i] + 1);
            if (expanded != NULL) {
                strcat(fullcmd, " ");
                strcat(fullcmd, expanded);
            }
            else {
                strcat(fullcmd, " ");
                strcat(fullcmd, args[i]);
            }
        } else {
            strcat(fullcmd, " ");
            strcat(fullcmd, args[i]);
        }
    }

    run_built_in_command( fullcmd);

    return 0;
}
int myshell_help(char **args){
    int splitSize = splitArraySize(args);
    char fullcmd[RL_BUFSIZE] =  "more manual.txt";

    if(splitSize != 1){
        //if user enter dir as well as something other forward by dir we need to append that after "ls -al"
        for (int i = 1; args[i] != NULL; i++){
            strcat(fullcmd, " ");
            strcat(fullcmd, args[i]);
        } 
    }
    run_built_in_command( fullcmd);

    return 0;
}
int myshell_pause(char **args) {
    //We are going to hold the whole terminal so that
    // other no one (background) processes could use the shell
    struct termios oldAttr, newAttr;
    tcgetattr(STDIN_FILENO, &oldAttr);
    newAttr = oldAttr;
    newAttr.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newAttr);
    printf("Press Enter to continue...\n");
    fflush(stdout);
    int ch;
    while ((ch = getchar()) != '\n' && ch != EOF);
    
    tcsetattr(STDIN_FILENO, TCSANOW, &oldAttr);
    return 0;
}
int myshell_quit(char **args){
    if(my_pid != 0)
        killpg(getpid(),SIGKILL);
    exit(0);
    return 0;
}

void execute_command(char *buf) {
    char bufCopy[RL_BUFSIZE];
    strcpy(bufCopy, buf);

    char ** splitBuf = myshell_splitline(buf);

    if(splitBuf != NULL){
        
        if( strcmp(buf,"cd") == 0 ){
            myshell_cd (splitBuf);
        }
        else if( strcmp(buf,"clr") == 0 ){
            myshell_clr (splitBuf);
        }
        else if( strcmp(buf,"dir") == 0 ){
            myshell_dir (splitBuf);
        }
        else if( strcmp(buf,"environ") == 0 ){
            myshell_environ (splitBuf);
        }
        else if( strcmp(buf,"echo") == 0){
            myshell_echo (splitBuf);
        }
        else if( strcmp(buf,"help") == 0 ){
            myshell_help (splitBuf);
        }
        else if( strcmp(buf,"pause") == 0 ){
            myshell_pause (splitBuf);
        }
        else if( strcmp(buf,"quit") == 0 ){
            myshell_quit (splitBuf);
        }
        else{
            run_built_in_command(bufCopy);
        }
    }    
}

int main_(int argc, char **argv) {
    static char buf[RL_BUFSIZE];
    int r;
    if(argc == 1){
        while(getcmd(buf, sizeof(buf)) >= 0){
            execute_command(buf);       
            sleep(1); 
        }
    }
    else{
        FILE *fp = fopen(argv[1], "r");
        while(fgets(buf, RL_BUFSIZE, fp)){
            execute_command(buf);
        }
        fclose(fp);
    }
    myshell_quit(NULL);
    return EXIT_SUCCESS;
}

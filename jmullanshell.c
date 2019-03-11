#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <dirent.h>
#include <stdbool.h>
#include <fcntl.h>

void flags();
void commandToRun(char**);
void appendPath(char*, char*);
char** parseInput(char input[]);
void parseFile(char*);
void cd(char*);
void dir(char*);
void echo(char**);
void environ();
void pauseMe();
void help();
void clr();
void extCommand(char**);

void outdir(char**);
void outenv(char**);
void outecho(char**);
void outhelp(char**);
void outextCommand(char**);

void inextCommand(char**);

void appendoutdir(char**);
void appendoutenv(char**);
void appendoutecho(char**);
void appendouthelp(char**);
void appendoutext(char**);

void extBackground(char**);

void parseFunc(char**);
void pipeExe(char**, char**, int, int);

char** fullLine;
int buffer = 500;
int numCommands = 0;
int lineSize = 0;
bool pipeMe = false;
bool in = false;
bool out = false;
bool appendOut = false;
bool background;
int wc;
char* currentPath = "/home";
DIR *currentDir; //used to keep track of currentDirectory
DIR *readDir; //Used for comparison statements
FILE *fp;
struct dirent *sd;
int main(int argc, char* argv[])
{
    currentDir = opendir(currentPath);
    //currentDir = chdir(currentPath);
    setenv("PWD", currentPath, 1);
    char input[buffer];
    if(argc == 1){
        while (1 == 1){
            printf("%s%s", getenv("PWD"), "$shell> ");
            fgets(input, buffer, stdin); //gets standard input, allocates size buffer, names it "input"
            fullLine = parseInput(input);
            if(fullLine[0][0] == 'q' && fullLine[0][1] == 'u' && fullLine[0][2] == 'i'
            && fullLine[0][3] == 't' && strlen(fullLine[0]) == 5){ //this gets called before fork because
                exit(0);             //this command exits the whole shell, not just the child
            }

            flags();
            if(pipeMe){
                parsePipe(fullLine);
            }
            else{
                commandToRun(fullLine);
            }
        }
    }
    else if (argc == 2){
        parseFile(argv[1]);
    }
    else{
        printf("This program cannot accept more than one batch file\n");
        exit(0);
    }
    free(fullLine);
}
/*-----------------------------------------------------------------------------*/
void flags(){
    for(size_t i = 0 ; i < wc; i++){
        if(fullLine[i][0] == '|' && strlen(fullLine[i]) == 1){
            pipeMe = true;
        }
        if(fullLine[i][0] == '<' && strlen(fullLine[i]) == 1){
            in = true;
        }
        if(fullLine[i][0] == '>' && strlen(fullLine[i]) == 1){
            out = true;
        }
        if(fullLine[i][0] == '>' && fullLine[i][1] == '>' && strlen(fullLine[i]) == 2){
            appendOut = true;
        }
        if(fullLine[i][0] == '&' && (strlen(fullLine[i]) == 1) || (strlen(fullLine[i]) == 2)){
            background = true;
        }
    }
}

void commandToRun(char** command){
    if(command[0][0] == 'c' && command[0][1] == 'd' && (strlen(command[0]) == 2 || strlen(command[0]) == 3)){
        command[1][strlen(command[1]) - 1] = '\0';
        if(wc == 1){
            cd(currentPath);
        }
        else{
            cd(command[1]);
        }
    }
    else if(command[0][0] == 'd' && command[0][1] == 'i' && command[0][2] == 'r' && (strlen(command[0]) == 3 || strlen(command[0]) == 4)){
        if(wc == 1){
            dir(currentPath);
        }
        else if(out){
            outdir(command);
        }
        else if(appendOut){
            appendoutdir(command);
        }
        else{
            command[1][strlen(command[1]) - 1] = '\0';
            dir(command[1]);
        }
        printf("\n");
    }
    else if(command[0][0] == 'e' && command[0][1] == 'n' && command[0][2] == 'v' && command[0][3] == 'i'
    && command[0][4] == 'r' && command[0][5] == 'o' && command[0][6] == 'n' &&
    (strlen(command[0]) == 7 || strlen(command[0]) == 8)){
        if(out){
            outenv(command);
        }
        else if(appendOut){
            appendoutenv(command);
        }
        else{
            environ();
        }
    }
    else if(strcmp(command[0], "echo") == 0){
        if(out){
            outecho(command);
        }
        else if(appendOut){
            appendoutecho(command);
        }
        else{
            echo(command);
        }
    }
    else if(command[0][0] == 'h' && command[0][1] == 'e' && command[0][2] == 'l' && command[0][3] == 'p'
    && (strlen(command[0]) == 4) || (strlen(command[0]) == 5)){
        if(out){
            outhelp(command);
        }
        else if(appendOut){
            appendouthelp(command);
        }
        else{
            help();
        }
    }
    else if(command[0][0] == 'c' && command[0][1] == 'l' && command[0][2] == 'r' && strlen(command[0]) == 4){
        clr();
    }
    else if(command[0][0] == 'p' && command[0][1] == 'a' && command[0][2] == 'u' && command[0][3] == 's'
    && fullLine[0][4] == 'e' && strlen(fullLine[0]) == 6){
        pauseMe();
    }
    else{
        if(out && !in){
            outextCommand(command);
        }
        else if(in && !out && !appendOut){
            inextCommand(command);
        }
        else if(appendOut){
            appendoutextCommand(command);
        }
        else if(in && appendOut){
            inAndAppendOutExtCommand(command);
        }
        else if(in && out){
            inAndOutExtCommand(command);
        }
        else if(background){
            extBackground(command);
        }
        else{
            extCommand(command);
        }
    }
}

void parseFile(char* filename){
    wc = 1;
    FILE *measure = fopen(filename, "r");
    FILE *read = fopen(filename, "r");
    if(measure == NULL || read == NULL){
        printf("Unable to open specified file\n");
        exit(0);
    }
    char i = fgetc(measure);
    while(i != EOF){
        if(i == '\n'){
            numCommands++;
        }
        i = fgetc(measure);
    }

    rewind(measure);//Setting the file pointer back to its original position
    i = fgetc(measure);

    for(int n = 0; n < numCommands; n++){
        while(i != '\n'){
            if(i == ' '){
                wc++;
            }
            lineSize++;
            i = fgetc(measure);
        }

        char c = fgetc(read);
        int j = 0;
        char* input = malloc(sizeof(char) * lineSize + 1);
        while(c != '\n'){
            input[j] = c;
            j++;
            c = fgetc(read);
        }
        input[lineSize + 1] = '\0';

        fullLine = parseInput(input);

        flags();
        if(fullLine[0][0] == 'q' && fullLine[0][1] == 'u' && fullLine[0][2] == 'i'
            && fullLine[0][3] == 't' && strlen(fullLine[0]) == 5){ //this gets called before fork because
                exit(0);             //this command exits the whole shell, not just the child
            }
        flags();
        commandToRun(fullLine);
        i = fgetc(measure);
        lineSize = 0;
    }
}

char** parseInput(char input[]){
    int index = 0;
    char i = input[0];

    if(input == ""){
        wc = 0;
    }
    else{
        wc = 1;
        while(i != '\0'){
            i = input[index++];
            if(i == ' '){
                wc++;
            }
        }
    }

    index = 0;
    char *c = strtok(input, " ");
    char **returnMe = (char**)malloc(sizeof(char*)*wc);
    for(int j = 0; j < wc; j++){
        returnMe[j] = (char*)malloc((sizeof(char) * 100));
    }
    while(c != NULL){
        returnMe[index++] = c;
        c = strtok(NULL, " ");
    }

    return returnMe;
}

void appendPath(char* base, char* ext){
    int baseLen = strlen(base);
    int extLen = strlen(ext);
    char* returnMe = (char*)malloc((sizeof(char) * baseLen) + (sizeof(char) * extLen) + (sizeof(char) * 2));
    for(int i = 0; i < baseLen; i++){
        returnMe[i] = base[i];
    }
    returnMe[baseLen] = '/';
    for(int i = 0; i < extLen; i++){
        returnMe[i + baseLen + 1] =  ext[i];
    }
    returnMe[baseLen + extLen + 1] = '\0';
    currentPath = returnMe;
}
/*-----------------------Basic Commands----------------------------*/
void cd(char* path){
    currentDir = opendir(path);
    if(currentDir == NULL){
        appendPath(currentPath, path);
        if(chdir(currentPath) == 0){
            currentDir = chdir(currentPath);
            setenv("PWD", currentPath, 1);
        }
        else{
            printf("Error, unable to open directory\n");
        }
    }
    else{
        currentDir = opendir(path);
        if(chdir(path) != 0){
            printf("Error, unable to open directory\n");
        }
        else{
            setenv("PWD", path, 1);
            currentPath = path;
        }
    }
}

void dir(char* direct){
    readDir = opendir(direct);
    if(readDir == NULL){
        printf("Error, unable to find directory\n");
    }
    else{
        while((sd = readdir(readDir)) != NULL){
            puts(sd->d_name);
        }
    }
    closedir(readDir);
}

void echo(char** command){
    for(int i = 1; i < wc; i++){
        puts(command[i]);
    }
}

void environ(){
    printf("%s%s\n", "XDG_VTNR: ", getenv("XDG_VTNR"));
    printf("%s%s\n", "CLUTTER_IM_MODULE: ", getenv("CLUTTER_IM_MODULE"));
    printf("%s%s\n", "SESSION: ", getenv("SESSION"));
    printf("%s%s\n", "SHELL: ", getenv("SHELL"));
    printf("%s%s\n", "TERM: ", getenv("TERM"));
    printf("%s%s\n", "WINDOWID: ", getenv("WINDOWID"));
    printf("%s%s\n", "USER: ", getenv("USER"));
    printf("%s%s\n", "PWD: ", getenv("PWD"));
    printf("%s%s\n", "DESKTOP_SESSION: ", getenv("DESKTOP_SESSION"));
    printf("%s%s\n", "LANG: ", getenv("LANG"));
    printf("%s%s\n", "JOB: ", getenv("JOB"));
}

void clr(){
    printf("\e[1;1H\e[2J"); //This is an ANSI escape code, "\e[1;1H moves the cursor to position (1,1)
                            //and "\e[2J clears the screen
                            //https://www.reddit.com/r/learnprogramming/comments/1d566l/c_clear_screen_command
                            //https://en.wikipedia.org/wiki/ANSI_escape_code
}

void help(){
    pid_t child;
    char* args[] = {"more", "readme", NULL};
    if((child = fork()) == 0){//child process
    execvp(args[0], args);
    //If child process reaches this point, then exec failed
    fprintf(stderr, "Failed to process command\n");
    }
    else{
        wait(NULL);
    }
}

void pauseMe(){
    while(1 == 1){
        char i = fgetc(stdin);
        if(i == '\n'){
            break;
        }
    }
}

void extCommand(char** command){
    pid_t child;
    char* args[wc + 1];
    if(wc > 1){
        for(int i = 1; i < wc - 1; i++){ //Terminating the strings properly before passing them into exec
            command[i][strlen(command[i])] = '\0';
        }
    }
    command[wc - 1][strlen(command[wc - 1]) - 1] = '\0'; //we have to seperate the arguements because the last string contains '\n'

    for(int i = 0; i < wc; i++){ //setting the values to be passed into execvp
        args[i] = command[i];
    }
    args[wc] = NULL; //The last elementn passed to execvp has to be null

    if((child = fork()) == 0){//child process
        execvp(args[0], args);
        //If child process reaches this point, then exec failed
        fprintf(stderr, "Failed to process command\n");
    }
    else{
        wait(NULL);
    }
}
/*-------------------Redirect out commands----------------------*/
void outdir(char** command){
    out = false;
    command[wc - 1][strlen(command[wc - 1]) - 1] = '\0'; //correctly parsing the last word (filename)
    int fd = open(command[wc - 1], O_CREAT | O_RDWR, 0600);
    int ret;
    int saved_stdout;

    if(fd < 0){
        perror("open");
    }

    saved_stdout = dup(1);
    ret = dup2(fd, 1);

    if(ret < 0){
        perror("dup");
    }

    readDir = opendir(command[1]);
    if(readDir == NULL){
        printf("Error, unable to find directory\n");
    }
    else{
        while((sd = readdir(readDir)) != NULL){
            puts(sd->d_name);
        }
    }
    closedir(readDir);

    fflush(stdout);
    close(stdout);

    dup2(saved_stdout, 1);

    close(fd);
}

void outenv(char** command){
    out = false;
    command[wc - 1][strlen(command[wc - 1]) - 1] = '\0'; //correctly parsing the last word (filename)
    int fd = open(command[wc - 1], O_CREAT | O_RDWR, 0600);
    int ret;
    int saved_stdout;
    int pid = getpid();

    if(fd < 0){
        perror("open");
    }

    saved_stdout = dup(1);
    ret = dup2(fd, 1);

    if(ret < 0){
        perror("dup");
    }

    printf("%s%s\n", "XDG_VTNR: ", getenv("XDG_VTNR"));
    printf("%s%s\n", "CLUTTER_IM_MODULE: ", getenv("CLUTTER_IM_MODULE"));
    printf("%s%s\n", "SESSION: ", getenv("SESSION"));
    printf("%s%s\n", "SHELL: ", getenv("SHELL"));
    printf("%s%s\n", "TERM: ", getenv("TERM"));
    printf("%s%s\n", "WINDOWID: ", getenv("WINDOWID"));
    printf("%s%s\n", "USER: ", getenv("USER"));
    printf("%s%s\n", "PWD: ", getenv("PWD"));
    printf("%s%s\n", "DESKTOP_SESSION: ", getenv("DESKTOP_SESSION"));
    printf("%s%s\n", "LANG: ", getenv("LANG"));
    printf("%s%s\n", "JOB: ", getenv("JOB"));


    fflush(stdout);
    close(stdout);

    dup2(saved_stdout, 1);

    close(fd);
}

void outecho(char** command){
    out = false;
    int ret;
    int saved_stdout;
    command[wc - 1][strlen(command[wc - 1]) - 1] = '\0'; //correctly parsing the last word (filename)
    int fd = open(command[wc - 1], O_CREAT | O_RDWR, 0600);

    if(fd < 0){
        perror("open");
    }

    saved_stdout = dup(1);
    ret = dup2(fd, 1);

    if(ret < 0){
        perror("dup");
    }

    for(int i = 1; i < wc - 2; i++){
        printf(command[i]);
        printf(" ");
    }

    fflush(stdout);
    close(stdout);

    dup2(saved_stdout, 1);

    close(fd);
}

void outhelp(char** command){
    out = false;
    command[wc - 1][strlen(command[wc - 1]) - 1] = '\0'; //correctly parsing the last word (filename)
    int fd = open(command[wc - 1], O_CREAT | O_RDWR, 0600);
    int ret;
    int saved_stdout;
    int pid = getpid();
    pid_t child;

    if(fd < 0){
        perror("open");
    }

    saved_stdout = dup(1);
    ret = dup2(fd, 1);

    if(ret < 0){
        perror("dup");
    }

    char* args[] = {"more", "readme", NULL};
    if((child = fork()) == 0){//child process
    execvp(args[0], args);
    //If child process reaches this point, then exec failed
    fprintf(stderr, "Failed to process command\n");
    }
    else{
        wait(NULL);
    }

    fflush(stdout);
    close(stdout);

    dup2(saved_stdout, 1);

    close(fd);
}

void outextCommand(char** command){
    out = false;
    pid_t child;
    command[wc - 1][strlen(command[wc - 1]) - 1] = '\0'; //we have to seperate the arguements because the last string contains '\n'
    char* args[wc - 1];
    int fd = open(command[wc - 1], O_CREAT | O_RDWR, 0600);
    int ret;
    int saved_stdout;

    if(fd < 0){
        perror("open");
    }

    saved_stdout = dup(1);
    ret = dup2(fd, 1);

    if(ret < 0){
        perror("dup");
    }

    for(int i = 0; i < wc - 2; i++){ //setting the values to be passed into execvp
        args[i] = command[i];
    }
    args[wc - 2] = NULL; //The last elementn passed to execvp has to be null

    if((child = fork()) == 0){//child process
        execvp(args[0], args);
        //If child process reaches this point, then exec failed
        fprintf(stderr, "Failed to process command\n");
    }
    else{
        wait(NULL);
    }

    fflush(stdout);
    close(stdout);

    dup2(saved_stdout, 1);

    close(fd);

}

/*-------------------Redirect in commands-----------------*/
void inextCommand(char** command){
    in = false;
    pid_t child;
    command[wc - 1][strlen(command[wc - 1]) - 1] = '\0'; //we have to seperate the arguements because the last string contains '\n'
    char* args[wc - 1];
    int fd = open(command[wc - 1], O_RDONLY, 0600);
    int ret;
    int saved_stdin;

    if(fd < 0){
        perror("open");
    }

    saved_stdin = dup(0);
    ret = dup2(fd, 0);

    if(ret < 0){
        perror("dup");
    }

    for(int i = 0; i < wc - 2; i++){ //setting the values to be passed into execvp
        args[i] = command[i];
    }
    args[wc - 2] = NULL; //The last elementn passed to execvp has to be null

    if((child = fork()) == 0){//child process
        execvp(args[0], args);
        //If child process reaches this point, then exec failed
        fprintf(stderr, "Failed to process command\n");
    }
    else{
        wait(NULL);
    }

    fflush(stdin);
    close(stdin);

    dup2(saved_stdin, 0);

    close(fd);

    printf("\n");
}

/*------------------Redirect append out commands----------------------------*/
void appendoutdir(char** command){
    appendOut = false;
    command[wc - 1][strlen(command[wc - 1]) - 1] = '\0'; //correctly parsing the last word (filename)
    int fd = open(command[wc - 1], O_CREAT | O_RDWR | O_APPEND, 0600);
    int ret;
    int saved_stdout;

    if(fd < 0){
        perror("open");
    }

    saved_stdout = dup(1);
    ret = dup2(fd, 1);

    if(ret < 0){
        perror("dup");
    }

    readDir = opendir(command[1]);
    if(readDir == NULL){
        printf("Error, unable to find directory\n");
    }
    else{
        while((sd = readdir(readDir)) != NULL){
            puts(sd->d_name);
        }
    }
    closedir(readDir);

    fflush(stdout);
    close(stdout);

    dup2(saved_stdout, 1);

    close(fd);
}

void appendoutenv(char** command){ //THIS NEEDS WORKKSKFSF!!!!ONE11!!
    appendOut = false;
    command[wc - 1][strlen(command[wc - 1]) - 1] = '\0'; //correctly parsing the last word (filename)
    int fd = open(command[wc - 1], O_CREAT | O_RDWR | O_APPEND, 0600);
    int ret;
    int saved_stdout;
    int pid = getpid();

    if(fd < 0){
        perror("open");
    }

    saved_stdout = dup(1);
    ret = dup2(fd, 1);

    if(ret < 0){
        perror("dup");
    }

    printf("%s%s\n", "XDG_VTNR: ", getenv("XDG_VTNR"));
    printf("%s%s\n", "CLUTTER_IM_MODULE: ", getenv("CLUTTER_IM_MODULE"));
    printf("%s%s\n", "SESSION: ", getenv("SESSION"));
    printf("%s%s\n", "SHELL: ", getenv("SHELL"));
    printf("%s%s\n", "TERM: ", getenv("TERM"));
    printf("%s%s\n", "WINDOWID: ", getenv("WINDOWID"));
    printf("%s%s\n", "USER: ", getenv("USER"));
    printf("%s%s\n", "PWD: ", getenv("PWD"));
    printf("%s%s\n", "DESKTOP_SESSION: ", getenv("DESKTOP_SESSION"));
    printf("%s%s\n", "LANG: ", getenv("LANG"));
    printf("%s%s\n", "JOB: ", getenv("JOB"));


    fflush(stdout);
    close(stdout);

    dup2(saved_stdout, 1);

    close(fd);
}

void appendoutecho(char** command){
    appendOut = false;
    int ret;
    int saved_stdout;
    command[wc - 1][strlen(command[wc - 1]) - 1] = '\0'; //correctly parsing the last word (filename)
    int fd = open(command[wc - 1], O_CREAT | O_RDWR | O_APPEND, 0600);

    if(fd < 0){
        perror("open");
    }

    saved_stdout = dup(1);
    ret = dup2(fd, 1);

    if(ret < 0){
        perror("dup");
    }

    for(int i = 1; i < wc - 2; i++){
        printf(command[i]);
        printf(" ");
    }

    fflush(stdout);
    close(stdout);

    dup2(saved_stdout, 1);

    close(fd);
}

void appendouthelp(char** command){
    appendOut = false;
    command[wc - 1][strlen(command[wc - 1]) - 1] = '\0'; //correctly parsing the last word (filename)
    int fd = open(command[wc - 1], O_CREAT | O_RDWR | O_APPEND, 0600);
    int ret;
    int saved_stdout;
    int pid = getpid();
    pid_t child;

    if(fd < 0){
        perror("open");
    }

    saved_stdout = dup(1);
    ret = dup2(fd, 1);

    if(ret < 0){
        perror("dup");
    }

    char* args[] = {"more", "readme", NULL};
    if((child = fork()) == 0){//child process
    execvp(args[0], args);
    //If child process reaches this point, then exec failed
    fprintf(stderr, "Failed to process command\n");
    }
    else{
        wait(NULL);
    }


    fflush(stdout);
    close(stdout);

    dup2(saved_stdout, 1);

    close(fd);
}

void appendoutextCommand(char** command){
    appendOut = false;
    pid_t child;
    command[wc - 1][strlen(command[wc - 1]) - 1] = '\0'; //we have to seperate the arguements because the last string contains '\n'
    char* args[wc - 1];
    int fd = open(command[wc - 1], O_CREAT | O_RDWR | O_APPEND, 0600);
    int ret;
    int saved_stdout;

    if(fd < 0){
        perror("open");
    }

    saved_stdout = dup(1);
    ret = dup2(fd, 1);

    if(ret < 0){
        perror("dup");
    }

    for(int i = 0; i < wc - 2; i++){ //setting the values to be passed into execvp
        args[i] = command[i];
    }
    args[wc - 2] = NULL; //The last elementn passed to execvp has to be null

    if((child = fork()) == 0){//child process
        execvp(args[0], args);
        //If child process reaches this point, then exec failed
        fprintf(stderr, "Failed to process command\n");
    }
    else{
        wait(NULL);
    }

    fflush(stdout);
    close(stdout);

    dup2(saved_stdout, 1);

    close(fd);

}

void inAndOutExtCommand(char** command){
    in = false;
    out = false;
    pid_t child;
    command[wc - 1][strlen(command[wc - 1]) - 1] = '\0'; //we have to seperate the arguements because the last string contains '\n'
    command[wc - 3][strlen(command[wc - 3])] = '\0';
    char* args[wc - 3];
    int fdWrite = open(command[wc - 1], O_CREAT | O_RDWR, 0600);
    int fdRead = open(command[wc - 3], O_CREAT | O_RDWR, 0600);
    int retWrite;
    int retRead;
    int saved_stdout;
    int saved_stdin;

    if(fdWrite < 0){
        perror("write open");
    }
    if(fdRead < 0){
        perror("read open");
    }

    saved_stdout = dup(1);
    retWrite = dup2(fdWrite, 1);

    saved_stdin = dup(0);
    retRead = dup2(fdRead, 0);

    if(retWrite < 0){
        perror("write dup");
    }

    if(retRead < 0){
        perror("read dup");
    }

    for(int i = 0; i < wc - 4; i++){ //setting the values to be passed into execvp
        args[i] = command[i];
    }
    args[wc - 4] = NULL; //The last element passed to execvp has to be null

    if((child = fork()) == 0){//child process
        execvp(args[0], args);
        //If child process reaches this point, then exec failed
        fprintf(stderr, "Failed to process command\n");
    }
    else{
        wait(NULL);
    }

    fflush(stdout);
    close(stdout);
    dup2(saved_stdout, 1);
    close(fdWrite);

    fflush(stdin);
    close(stdin);
    dup2(saved_stdin, 0);
    close(fdRead);
}

void inAndAppendOutExtCommand(char** command){
    in = false;
    appendOut = false;
    pid_t child;
    command[wc - 1][strlen(command[wc - 1]) - 1] = '\0'; //we have to seperate the arguements because the last string contains '\n'
    command[wc - 3][strlen(command[wc - 3])] = '\0';
    char* args[wc - 3];
    int fdWrite = open(command[wc - 1], O_CREAT | O_RDWR | O_APPEND, 0600);
    int fdRead = open(command[wc - 3], O_CREAT | O_RDWR | O_APPEND, 0600);
    int retWrite;
    int retRead;
    int saved_stdout;
    int saved_stdin;

    if(fdWrite < 0){
        perror("write open");
    }
    if(fdRead < 0){
        perror("read open");
    }

    saved_stdout = dup(1);
    retWrite = dup2(fdWrite, 1);

    saved_stdin = dup(0);
    retRead = dup2(fdRead, 0);

    if(retWrite < 0){
        perror("write dup");
    }

    if(retRead < 0){
        perror("read dup");
    }

    for(int i = 0; i < wc - 4; i++){ //setting the values to be passed into execvp
        args[i] = command[i];
        printf(args[i]);
    }
    args[wc - 4] = NULL; //The last elementn passed to execvp has to be null

    if((child = fork()) == 0){//child process
        execvp(args[0], args);
        //If child process reaches this point, then exec failed
        fprintf(stderr, "Failed to process command\n");
    }
    else{
        wait(NULL);
    }

    fflush(stdout);
    close(stdout);
    dup2(saved_stdout, 1);
    close(fdWrite);

    fflush(stdin);
    close(stdin);
    dup2(saved_stdin, 0);
    close(fdRead);
}

void extBackground(char** command){
    background = false;
    pid_t child;
    char* args[wc];
    if(wc > 1){
        for(int i = 1; i < wc - 1; i++){ //Terminating the strings properly before passing them into exec
            command[i][strlen(command[i])] = '\0';
        }
    }
    command[wc - 1][strlen(command[wc - 1]) - 1] = '\0'; //we have to seperate the arguements because the last string contains '\n'

    for(int i = 0; i < wc - 1; i++){ //setting the values to be passed into execvp
        args[i] = command[i];
    }
    args[wc - 1] = NULL; //The last elementn passed to execvp has to be null

    if((child = fork()) == 0){//child process
        execvp(args[0], args);
        //If child process reaches this point, then exec failed
        fprintf(stderr, "Failed to process command\n");
    }
}


/*-----------------Piping---------------------------*/
void parsePipe(char** command){
    int wc1;
    int wc2;
    int index = 0;
    char c = command[0][0];
    while(c != '|'){ //this loop determines the size of each argument
        index++;
        c = command[index][0];
    }
    wc1 = index;
    wc2 = wc - index - 1;

    char **argv1 = (char**)malloc(sizeof(char*)*wc1); //Allocating space for argv1
    for(int j = 0; j < wc1; j++){
        argv1[j] = (char*)malloc((sizeof(char) * 100));
    }

    char **argv2 = (char**)malloc(sizeof(char*)*wc2); //Allocating space for argv2
    for(int j = 0; j < wc2; j++){
        argv2[j] = (char*)malloc((sizeof(char) * 100));
    }

    for(int i = 0; i < wc1; i++){ //Setting the values of argv1
        argv1[i] = command[i];
    }

    index = 0;
    for(int i = wc1 + 1; i < wc; i++){ //Setting the values of argv2
        argv2[index] = command[i];
        index++;
    }

    for(int i = 0; i < wc1; i++){
        argv1[i][strlen(argv1[i])] = '\0';
    }

    for(int i = 0; i < wc2; i++){
        argv2[i][strlen(argv2[i])] = '\0';
    }
    argv2[wc2 - 1][strlen(argv2[wc2 - 1]) - 1] = '\0';

    pipeExe(argv1, argv2, wc1, wc2);
}

void pipeExe(char** first, char** second, int wc1, int wc2){
    pipeMe = false;
    pid_t pid1, pid2;
    int mypipefd[2];
    char* argv1[wc1 + 1];
    char* argv2[wc2 + 1];

    //These are the two commands that we will execute
    for(int i = 0; i < wc1; i++){
        argv1[i] = first[i];
    }
    for(int i = 0; i < wc2; i++){
        argv2[i] = second[i];
    }

    argv1[wc1] = NULL;
    argv2[wc2] = NULL;

    //create the pipe
    pipe(mypipefd);

    //Create our first process
    pid1 = fork();
    if(pid1 == 0){ //If we are in the child process
        //Hook stdout to the write end of the pipe
        dup2(mypipefd[1], STDOUT_FILENO);
        //Close the read end of the pipe because it is not needed in this process
        close(mypipefd[0]);
        //Execing the commands
        execvp(argv1[0], argv1);
        //If the code makes it this far, exec failed
        perror("exec");
        close(mypipefd[1]);
        return;
    }
    pid2 = fork();
    if(pid2 == 0){
        //Hook stdin to the read end of the pipe
        dup2(mypipefd[0], STDIN_FILENO);
        //Close the write end of the pipe because it is not needed in this process
        close(mypipefd[1]);
        //Similarly exec "wc -l"
        execvp(argv2[0], argv2);
        perror("exec");
        close(mypipefd[0]);
        return;
    }

    //Close both ends of the pipe
    //The respective read write ends of the pipe persist in the two processes creeated above
    close(mypipefd[0]);
    close(mypipefd[1]);

    //Wait for everything to finish then exit
    wait(NULL);
    wait(NULL);
}


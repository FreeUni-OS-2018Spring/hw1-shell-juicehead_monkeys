#include <ctype.h>
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <signal.h>
#include <sys/wait.h>
#include <termios.h>
#include <unistd.h>
#include <dirent.h>

#include "tokenizer.h"

/* Convenience macro to silence compiler warnings about unused function parameters. */
#define unused __attribute__((unused))

/* Whether the shell is connected to an actual terminal or not. */
bool shell_is_interactive;

/* File descriptor for the shell input */
int shell_terminal;

/* Terminal mode settings for the shell */
struct termios shell_tmodes;

/* Process group id for the shell */
pid_t shell_pgid;

int cmd_exit(struct tokens *tokens);
int cmd_help(struct tokens *tokens);
int cmd_pwd(struct tokens * tokens);
int cmd_cd(struct tokens *tokens);


/* Built-in command functions take token array (see parse.h) and return int */
typedef int cmd_fun_t(struct tokens *tokens);

/* Built-in command struct and lookup table */
typedef struct fun_desc {
  cmd_fun_t *fun;
  char *cmd;
  char *doc;
} fun_desc_t;

fun_desc_t cmd_table[] = {
  {cmd_help, "?", "show this help menu"},
  {cmd_exit, "exit", "exit the command shell"},
  {cmd_pwd,"pwd","prints working directory"},
  {cmd_cd,"cd","change directory"}

};


int cmd_cd(unused struct tokens * tokens){
    char *path = tokens_get_token(tokens,(size_t)1); 
   
    char buffer [512]; //buffer for current directory
    char * currentDirectory = getcwd(buffer,512);
  
    if(strcmp(path,"..") == 0 && currentDirectory != NULL)
    {
     
        char * last = strrchr(currentDirectory,'/'); //get address of last occurence of '/'
        char previousDirectory [512];
        memset(previousDirectory,0,512);
        strncpy(previousDirectory,buffer,last-currentDirectory); //last-currentDirectory is size of previous Dir
        int status = chdir(previousDirectory);
        if(status == 0){
           return 0; 

        }else{
             printf("folowwing error happned : %s\n",strerror(errno));
             return -1;

        }
    }

    int status = chdir(path); //change dir if possible for absolute path
    if(status == 0){
      return 0;


    }else{
      strcat(buffer,path);
      int statusRelativePath = chdir(buffer); //change dir if possible for relative current dir + relative path
      if(statusRelativePath == 0){

        return 0;
      }else{
           printf("folowwing error happned : %s\n",strerror(errno));
           return -1;

      }


    }
}

/*prints working directory */
int cmd_pwd(unused struct tokens *tokens){
    char * buffer = malloc(512);
    char * result =  getcwd(buffer,512);
   if(result == NULL){
     printf("folowwing error happned : %s\n",strerror(errno));
    
     free(buffer);
     return -1;

   } else {
  
     printf("%s\n",buffer);
     free(buffer);
     return 0;

   }
}



/* executes given program,if absolutePath variable is empty that means we already have absolute paht in tokens[0],
if it's not empty then absolute path will be in absolutePath variable

 */
int progrExe(struct tokens *tokens,char * absolutePath) {

  pid_t pid;

  pid = fork();

  if (pid < 0) {
    fprintf(stderr, "Fork Failed");
    return 1;

  } else if (pid == 0) {

    size_t nArgs = tokens_get_length(tokens);
    char *arr[nArgs+1];

    if(absolutePath == NULL){
      arr[0] = tokens_get_token(tokens,0);
    }else{
      arr[0] = absolutePath;
    }
    for (size_t i = 1; i < nArgs; ++i) {
     
       arr[i] = tokens_get_token(tokens, i);
      
    }

    arr[nArgs] = NULL;

    execv(arr[0], arr);

    exit(EXIT_FAILURE); //it comes to this line if only execv failed.In this case terminate child process with failure

   

  } else {
    int status = 0;
    wait(&status);
   
    return WEXITSTATUS(status); //on success returns 0,on error return 1
    
    

  }

  return 0;
}

  if (pid < 0) {
    fprintf(stderr, "Fork Failed");
    return 1;

  } else if (pid == 0) {

    size_t nArgs = tokens_get_length(tokens);
    char *arr[nArgs+1];

    for (size_t i = 0; i < nArgs; ++i) {
     arr[i] = tokens_get_token(tokens, i);
    }

    arr[nArgs] = NULL;

    execv(arr[0], arr);

  } else {
    wait(NULL);
  }

  return 0;
}

/* Prints a helpful description for the given command */
int cmd_help(unused struct tokens *tokens) {
  for (unsigned int i = 0; i < sizeof(cmd_table) / sizeof(fun_desc_t); i++)
    printf("%s - %s\n", cmd_table[i].cmd, cmd_table[i].doc);
  return 1;
}

/* Exits this shell */
int cmd_exit(unused struct tokens *tokens) {

  exit(0);
}

/* Looks up the built-in command, if it exists. */
int lookup(char cmd[]) {
  for (unsigned int i = 0; i < sizeof(cmd_table) / sizeof(fun_desc_t); i++)
    if (cmd && (strcmp(cmd_table[i].cmd, cmd) == 0))
      return i;
  return -1;
}

/* Intialization procedures for this shell */
void init_shell() {
  /* Our shell is connected to standard input. */
  shell_terminal = STDIN_FILENO;

  /* Check if we are running interactively */
  shell_is_interactive = isatty(shell_terminal);

  if (shell_is_interactive) {
    /* If the shell is not currently in the foreground, we must pause the shell until it becomes a
     * foreground process. We use SIGTTIN to pause the shell. When the shell gets moved to the
     * foreground, we'll receive a SIGCONT. */
    while (tcgetpgrp(shell_terminal) != (shell_pgid = getpgrp()))
      kill(-shell_pgid, SIGTTIN);

    /* Saves the shell's process id */
    shell_pgid = getpid();

    /* Take control of the terminal */
    tcsetpgrp(shell_terminal, shell_pgid);

    /* Save the current termios to a variable, so it can be restored later. */
    tcgetattr(shell_terminal, &shell_tmodes);
  }
}

char * searchInPath(char * program){
  char * path = getenv("PATH");
 
  for (char *token = strtok(path,":"); token != NULL; token = strtok(NULL, ":"))
  {
    DIR *d;
    struct dirent *dir;
 
    d = opendir(token);
    if (d) {
     while ((dir = readdir(d)) != NULL) {
       if(strcmp(dir->d_name,program) == 0){
         char * copy = malloc(strlen(token) + strlen(program) +2); //keep absolute path to program
         strcpy(copy,token);
         strcat(copy,"/");
         strcat(copy,program);

     
         return copy;
       }
     //  printf("%s\n", dir->d_name);
     }
    closedir(d);
  }


  }

  return NULL;
}


//calls progrExe based on given absolute path / only command 
int runMyProgram(struct  tokens * tokens){
  int status = progrExe(tokens,NULL); //if user gave us absolute path

  if(status == 0){
    return 0;
  }else{
   
    char * commandPath = searchInPath(tokens_get_token(tokens,0)); //user gave us only command.Get the absolutePath
    
   
    int statusForCommand = progrExe(tokens,commandPath);
    free(commandPath); //deallocation of path
    if(statusForCommand ==0 ){ //success
      return 0;
    }else{
      printf("error with execution program");
      
      return -1;
    }

  }

}

int main(unused int argc, unused char *argv[]) {
  init_shell();

  static char line[4096];
  int line_num = 0;

  /* Please only print shell prompts when standard input is not a tty */
  if (shell_is_interactive)
    fprintf(stdout, "%d: ", line_num);

  while (fgets(line, 4096, stdin)) {
    /* Split our line into words. */
    struct tokens *tokens = tokenize(line);

    /* Find which built-in function to run. */
    int fundex = lookup(tokens_get_token(tokens, 0));

    if (fundex >= 0) {
      cmd_table[fundex].fun(tokens);
    } else {
    
      /* REPLACE this to run commands as programs. */
       if(tokens_get_length(tokens) != 0)
          runMyProgram(tokens);

    }

    if (shell_is_interactive)
      /* Please only print shell prompts when standard input is not a tty */
      fprintf(stdout, "%d: ", ++line_num);

    /* Clean up memory */
    tokens_destroy(tokens);
  }

  return 0;
}

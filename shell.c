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
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>

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





int isIOCommand(struct tokens *tokens) {
  char *strings[] = {">", "<", "<<", ">>"};
  int size = tokens_get_length(tokens);
  for (int i = 0; i < size; ++i) {
    char *tok = tokens_get_token(tokens, i);
    int arrLen = sizeof(strings)/sizeof(char *);
    for (int j = 0; j < arrLen; j++) {
      if (strcmp(tok, strings[j]) == 0){
        return i;
      } 
    }
  }
  return 0;  
}


void handleIoCommand(struct tokens *tokens) {

}



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

int isBg(struct tokens *tokens) {
  if (strcmp(tokens_get_token(tokens, tokens_get_length(tokens)-1), "&") == 0) return 1;
  return 0;
}

/* executes given program,if absolutePath variable is empty that means we already have absolute paht in tokens[0],
if it's not empty then absolute path will be in absolutePath variable

 */
int progrExe(struct tokens *tokens,char * absolutePath) {

  int isBgProcess = isBg(tokens);
  int isIO = isIOCommand(tokens);
  pid_t pid;

  pid = fork();

  if (pid < 0) {
    fprintf(stderr, "Fork Failed");
    return 1;

  } else if (pid == 0) {

    if (setpgid(0, 0) == -1) {
      perror(NULL);
    }

    size_t nArgs = tokens_get_length(tokens);
    if (isIO) {
      nArgs = isIO; // program arguments should be before io sign
    } else if (isBgProcess) {
      nArgs = nArgs-1; // this means the last token is '&' symbol and is not a program argument
    }

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

    char *file = tokens_get_token(tokens, isIO+1);

    int outputFile;


    int flags;
    int newfd;

    char *tok = tokens_get_token(tokens, isIO);
    if (strcmp(tok, ">") == 0) {
      newfd = 1;
      flags = O_CREAT | O_WRONLY | O_TRUNC;

    } else if (strcmp(tok, "<") == 0) {
      newfd = 0;
      flags = O_RDONLY;

    } else if (strcmp(tok, ">>") == 0) {
      newfd = 1;
      flags = O_CREAT | O_WRONLY | O_APPEND;
    }

    if (isIO) {
      outputFile = open(file, flags, S_IRUSR | S_IWUSR);
      dup2(outputFile, newfd);
    }

    //tcsetpgrp(newfd, getpid());
    execv(arr[0], arr);

    exit(EXIT_FAILURE); //it comes to this line if only execv failed.In this case terminate child process with failure

   

  } else {
    signal(SIGTTOU, SIG_IGN); // ignore 
    if (setpgid(pid, pid) == -1 && errno != EACCES) {
      perror(NULL);
    }
    if (!isBgProcess) {
      tcsetpgrp(0, pid);
    }

    int status = 0;
    wait(&status);
    if (!isBgProcess) tcsetpgrp(0, getpid());
    return WEXITSTATUS(status); //on success returns 0,on error return 1
    
    

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
  char * pathVariable = getenv("PATH");
  char * copyPath = malloc(strlen(pathVariable)+1);
  memset(copyPath,0,strlen(pathVariable)+1);
  strncpy(copyPath,pathVariable,strlen(pathVariable));

  

 
  for (char *token = strtok(copyPath,":"); token != NULL; token = strtok(NULL, ":"))
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
         

         free(copyPath); //free copy of env variable 
         return copy;
       }

     }
    closedir(d);
  }


  }
  free(copyPath); //free copy of env variable 
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
    if(statusForCommand == 0 ){ //success
      return 0;
    }else{
      printf("error with execution program");
      
      return -1;
    }

  }

}




char ** getExecvArgument(struct  tokens * tokens,int quantityOfPipes , int * pipeTokenLocations ,int childIndex,int numChildren){
     int start;
     int end;
     
        if(childIndex == 0){
          start = 0;
          end = pipeTokenLocations[0];
        }else {
          start = pipeTokenLocations[childIndex-1]+1;  
          if(childIndex == quantityOfPipes ){
            end = tokens_get_length(tokens);
          }else {
            end = pipeTokenLocations[childIndex];
          }
        }
        int argSize = end - start + 1;
        int argsPos = 1;
        char ** args = malloc(argSize * sizeof(char*));
        args[0] = searchInPath(tokens_get_token(tokens,start));

       
        for(int index =start+1; index < end;index++ ){
          args[argsPos] = tokens_get_token(tokens,index);

          argsPos++;
          
        }

         args[argSize-1] = NULL;


    return args;     

}


int makePipes(struct tokens * tokens,int * pipeTokenLocations,int quantityOfPipes){
  int pfd[quantityOfPipes][2];
  int numChildren = quantityOfPipes +1;

   for (int i=0; i<quantityOfPipes; i++)
    {
        if (pipe(pfd[i]) == -1)
        {
             printf("folowwing error happned : %s\n",strerror(errno));
             return -1;
        }
    }

  for(int i=0;i<numChildren;i++){
    pid_t pid = fork();

    if(pid < 0 ){
       printf("folowwing error happned : %s\n",strerror(errno));
      exit(EXIT_FAILURE);
    }else if(pid == 0){


       
          //bind my stdin to previous pipe
         if(i>0 && i<numChildren-1 ){
           if(pfd[i-1][0] !=STDIN_FILENO){
              if(dup2(pfd[i-1][0],STDIN_FILENO) == -1){
                //errExit("dup2 ");
                    printf("error with dup : %s\n",strerror(errno));
                    exit(EXIT_FAILURE);
              }
              if(close(pfd[i-1][0]) == -1){
                //errExit("close desc"); 
                  printf("error with close: %s\n",strerror(errno));
                  exit(EXIT_FAILURE);

              }



           }
           //close my copy of previous pipe's writing
            close(pfd[i-1][1]);

            //bind my output to next pipe
             if(pfd[i][1] !=STDOUT_FILENO){
                if(dup2(pfd[i][1],STDOUT_FILENO) == -1){
                  //errExit("dup2 1");
                      exit(EXIT_FAILURE);
                }
                if(close(pfd[i][1]) == -1){
                  //errExit("close 2"); 
                      exit(EXIT_FAILURE);

                }

            }
            //close my copy of next pipe's reading
            close(pfd[i][0]);
                 //close my copy of other descriptors
          for (int j = 0; j < numChildren-1; j++) {
            if (j != i && j != i - 1)  {
              close(pfd[j][0]);
              close(pfd[j][1]);
              }
         }
          char ** args = getExecvArgument(tokens,quantityOfPipes,pipeTokenLocations,i,numChildren); 
          execv(args[0],args);

        }else   if(i == 0){
          
               //close my copy of reading 
            if(close(pfd[i][0]) == -1){
              exit(EXIT_FAILURE);
            }
             //bind my output to next pipe
             if(pfd[i][1] !=STDOUT_FILENO){
                if(dup2(pfd[i][1],STDOUT_FILENO) == -1){
                  //errExit("dup2 1");
                      exit(EXIT_FAILURE);
                }
                if(close(pfd[i][1]) == -1){
                  //errExit("close 2"); 
                      exit(EXIT_FAILURE);

                }

            }
           
             for (int j = 0; j < numChildren-1; j++) {
              if (j != i)  {
                close(pfd[j][0]);
                close(pfd[j][1]);
                }
             }

            char ** args = getExecvArgument(tokens,quantityOfPipes,pipeTokenLocations,i,numChildren); 
            execv(args[0],args);

        }else {

            //close my copy of writing

             if(close(pfd[i-1][1]) == -1){
               exit(EXIT_FAILURE);
             }
             if(pfd[i-1][0] !=STDIN_FILENO){
              if(dup2(pfd[i-1][0],STDIN_FILENO) == -1){
                //errExit("dup2 ");
                    printf("error with dup : %s\n",strerror(errno));
                    exit(EXIT_FAILURE);
              }
              if(close(pfd[i-1][0]) == -1){
                //errExit("close desc"); 
                  printf("error with close: %s\n",strerror(errno));
                  exit(EXIT_FAILURE);

              }
           
            for (int j = 0; j < numChildren-1; j++) {
              if (j != i-1  )  {
                close(pfd[j][0]);
                close(pfd[j][1]);
                }
             }

               char ** args = getExecvArgument(tokens,quantityOfPipes,pipeTokenLocations,i,numChildren); 
               execv(args[0],args);
        }

        

       
 
      //errExit("exec problem");
      printf("folowwing error happned : %s\n",strerror(errno));
      exit(EXIT_FAILURE);


      }
    }


  }



  //parent closes it own descriptors
  for(int i=0;i<numChildren-1;i++){
    close(pfd[i][0]);
    close(pfd[i][1]);
  }

  for(int i=0;i<numChildren;i++){
    wait( NULL);
  }

  return 0;


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

      int quantityOfPipes = 0;
      int pipeTokenLocations[tokens_get_length(tokens)];
      for(int i=0;i<tokens_get_length(tokens);i++){
    
        if(strcmp(tokens_get_token(tokens,i),"|") == 0){
          
          pipeTokenLocations[quantityOfPipes] = i;
          quantityOfPipes++;
        }

      }
    
    
      if(quantityOfPipes > 0){
        makePipes(tokens,pipeTokenLocations,quantityOfPipes);
       

      }else{
       if(tokens_get_length(tokens) != 0)
          runMyProgram(tokens);

      }
    }

    if (shell_is_interactive)
      /* Please only print shell prompts when standard input is not a tty */
      fprintf(stdout, "%d: ", ++line_num);

    /* Clean up memory */
    tokens_destroy(tokens);
  }

  return 0;
}

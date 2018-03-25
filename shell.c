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
#include <sys/time.h>
#include <sys/resource.h>
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
int cmd_ulimit(struct tokens * tokens);
int cmd_nice(struct tokens * tokens);
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
  {cmd_cd,"cd","change directory"},
  {cmd_ulimit,"ulimit","prints or changes current limit"},
  {cmd_nice,"nice","prints or changes niceness"}
};
int getlimit(unused struct tokens * tokens) {
	int status = -1;
	struct rlimit  * t = malloc(sizeof(struct rlimit)); 
	if(tokens_get_length(tokens)  == 1) {
		status = getrlimit(RLIMIT_FSIZE,t);
		if(t->rlim_cur == RLIM_INFINITY) {
			printf("file size  :   %s\n","unlimited");
		} else {
			printf("file size  : %lu \n",t->rlim_cur);
		}
		return status;
	}
	char * lastpart = tokens_get_token(tokens,(size_t)1); 
	if(strcmp(lastpart,"-a")== 0 || strcmp(lastpart,"-Sa")== 0 || strcmp(lastpart,"-Ha")== 0) {
		if(strcmp(lastpart,"-Ha")== 0) {
			status = getrlimit(RLIMIT_CORE,t);
			if(t->rlim_max == RLIM_INFINITY) {
				printf("core file size		(blocks, -c) %s\n","unlimited");
			} else{
				printf("core file size		(blocks, -c) %lu\n",t->rlim_max);
			} 
			status = getrlimit(RLIMIT_DATA,t);
			if(t->rlim_max == RLIM_INFINITY) {
				printf("data seg size		(kbytes, -d) %s\n","unlimited");
			} else{
				printf("data seg size		(kbytes, -d) %lu\n",t->rlim_max/1024);
			}
			status = getrlimit(RLIMIT_FSIZE,t);
			if(t->rlim_max == RLIM_INFINITY) {
				printf("file size		(blocks, -f) %s\n","unlimited");
			} else{
				printf("file size		(blocks, -f) %lu\n",t->rlim_max);
			}
			status = getrlimit(RLIMIT_SIGPENDING,t);
			if(t->rlim_max == RLIM_INFINITY) {
				printf("pending signals			(-i) %s\n","unlimited");
			} else{
				printf("pending signals			(-i) %lu\n",t->rlim_max);
			}
			status = getrlimit(RLIMIT_MEMLOCK,t);
			if(t->rlim_max == RLIM_INFINITY) {
				printf("max locked memory	(kbytes, -l) %s\n","unlimited");
			} else{
				printf("max locked memory	(kbytes, -l) %lu\n",t->rlim_max/1024);
			}
			status = getrlimit(RLIMIT_NOFILE,t);
			if(t->rlim_max == RLIM_INFINITY) {
				printf("open files			(-n) %s\n","unlimited");
			} else{
				printf("open files			(-n) %lu\n",t->rlim_max);
			}
			status = getrlimit(RLIMIT_MSGQUEUE,t);
			if(t->rlim_max == RLIM_INFINITY) {
				printf("POSIX message queues	 (bytes, -q) %s\n","unlimited");
			} else{
				printf("POSIX message queues	 (bytes, -q) %lu\n",t->rlim_max);
			}
			status = getrlimit(RLIMIT_RTPRIO,t);
			if(t->rlim_max == RLIM_INFINITY) {
				printf("real-time priority		(-r) %s\n","unlimited");
			} else{
				printf("real-time priority		(-r) %lu\n",t->rlim_max);
			}
			status = getrlimit(RLIMIT_STACK,t);
			if(t->rlim_max == RLIM_INFINITY) {
				printf("stack size		 (kbytes,-s) %s\n","unlimited");
			} else{
				printf("stack size		 (kbytes,-s) %lu\n",t->rlim_max/1024);
			}
			status = getrlimit(RLIMIT_CPU,t);
			if(t->rlim_max == RLIM_INFINITY) {
				printf("spu time		(seconds,-t) %s\n","unlimited");
			} else{
				printf("cpu time		(seconds,-t) %lu\n",t->rlim_max);
			}
			status = getrlimit(RLIMIT_NPROC,t);
			if(t->rlim_max == RLIM_INFINITY) {
				printf("max user processes		(-u) %s\n","unlimited");
			} else{
				printf("max user processes		(-u) %lu\n",t->rlim_max);
			}
			status = getrlimit(RLIMIT_AS,t);
			if(t->rlim_max == RLIM_INFINITY) {
				printf("virtual memory		 (kbytes,-v) %s\n","unlimited");
			} else{
				printf("virtual memory		 (kbytes,-v) %lu\n",t->rlim_max/1024);
			}
			status = getrlimit(RLIMIT_LOCKS,t);
			if(t->rlim_max == RLIM_INFINITY) {
				printf("file locks			(-x) %s\n","unlimited");
			} else{
				printf("file locks			(-x) %lu\n",t->rlim_max);
			}
			return status;

		}
			status = getrlimit(RLIMIT_CORE,t);
			if(t->rlim_cur == RLIM_INFINITY) {
				printf("core file size		(blocks, -c) %s\n","unlimited");
			} else{
				printf("core file size		(blocks, -c) %lu\n",t->rlim_cur);
			} 
			status = getrlimit(RLIMIT_DATA,t);
			if(t->rlim_cur == RLIM_INFINITY) {
				printf("data seg size		(kbytes, -d) %s\n","unlimited");
			} else{
				printf("data seg size		(kbytes, -d) %lu\n",t->rlim_cur/1024);
			}
			status = getrlimit(RLIMIT_FSIZE,t);
			if(t->rlim_cur == RLIM_INFINITY) {
				printf("file size		(blocks, -f) %s\n","unlimited");
			} else{
				printf("file size		(blocks, -f) %lu\n",t->rlim_cur);
			}
			status = getrlimit(RLIMIT_SIGPENDING,t);
			if(t->rlim_cur == RLIM_INFINITY) {
				printf("pending signals			(-i) %s\n","unlimited");
			} else{
				printf("pending signals			(-i) %lu\n",t->rlim_cur);
			}
			status = getrlimit(RLIMIT_MEMLOCK,t);
			if(t->rlim_cur == RLIM_INFINITY) {
				printf("max locked memory	(kbytes, -l) %s\n","unlimited");
			} else{
				printf("max locked memory	(kbytes, -l) %lu\n",t->rlim_cur/1024);
			}
			status = getrlimit(RLIMIT_NOFILE,t);
			if(t->rlim_cur == RLIM_INFINITY) {
				printf("open files			(-n) %s\n","unlimited");
			} else{
				printf("open files			(-n) %lu\n",t->rlim_cur);
			}
			status = getrlimit(RLIMIT_MSGQUEUE,t);
			if(t->rlim_cur == RLIM_INFINITY) {
				printf("POSIX message queues	 (bytes, -q) %s\n","unlimited");
			} else{
				printf("POSIX message queues	 (bytes, -q) %lu\n",t->rlim_cur);
			}
			status = getrlimit(RLIMIT_RTPRIO,t);
			if(t->rlim_cur == RLIM_INFINITY) {
				printf("real-time priority		(-r) %s\n","unlimited");
			} else{
				printf("real-time priority		(-r) %lu\n",t->rlim_cur);
			}
			status = getrlimit(RLIMIT_STACK,t);
			if(t->rlim_cur == RLIM_INFINITY) {
				printf("stack size		 (kbytes,-s) %s\n","unlimited");
			} else{
				printf("stack size		 (kbytes,-s) %lu\n",t->rlim_cur/1024);
			}
			status = getrlimit(RLIMIT_CPU,t);
			if(t->rlim_cur == RLIM_INFINITY) {
				printf("spu time		(seconds,-t) %s\n","unlimited");
			} else{
				printf("cpu time		(seconds,-t) %lu\n",t->rlim_cur);
			}
			status = getrlimit(RLIMIT_NPROC,t);
			if(t->rlim_cur == RLIM_INFINITY) {
				printf("max user processes		(-u) %s\n","unlimited");
			} else{
				printf("max user processes		(-u) %lu\n",t->rlim_cur);
			}
			status = getrlimit(RLIMIT_AS,t);
			if(t->rlim_cur == RLIM_INFINITY) {
				printf("virtual memory		 (kbytes,-v) %s\n","unlimited");
			} else{
				printf("virtual memory		 (kbytes,-v) %lu\n",t->rlim_cur/1024);
			}
			status = getrlimit(RLIMIT_LOCKS,t);
			if(t->rlim_cur == RLIM_INFINITY) {
				printf("file locks			(-x) %s\n","unlimited");
			} else{
				printf("file locks			(-x) %lu\n",t->rlim_cur);
			}
			return status;
			
	}
	if(strcmp(lastpart,"-f")== 0 || strcmp(lastpart,"-Sf")== 0 || strcmp(lastpart,"-Hf")== 0){
		status = getrlimit(RLIMIT_FSIZE,t);
		if(strcmp(lastpart,"-Hf")== 0) {
			if(t->rlim_max == RLIM_INFINITY) {
				printf("%s\n","unlimited");
			} else {
				printf("%lu\n",t->rlim_max);
			}
			return status;
		}
		if(t->rlim_cur == RLIM_INFINITY) {
			printf("%s\n","unlimited");
		} else {
			printf("%lu\n",t->rlim_cur);
		}
		return status;
	} else if(strcmp(lastpart,"-c")== 0 || strcmp(lastpart,"-Sc")== 0 || strcmp(lastpart,"-Hc")== 0) {
		status = getrlimit(RLIMIT_CORE,t);
		if(strcmp(lastpart,"-Hc")== 0) {
			if(t->rlim_max == RLIM_INFINITY) {
				printf("%s\n","unlimited");
			} else {
				printf("%lu\n",t->rlim_max);
			}
			return status;
		}
		if(t->rlim_cur == RLIM_INFINITY) {
			printf("%s\n","unlimited");
		} else {
			printf("%lu\n",t->rlim_cur);
		}
		return status;
	} else if(strcmp(lastpart,"-t")== 0 || strcmp(lastpart,"-St")== 0 || strcmp(lastpart,"-Ht")== 0) {
		status = getrlimit(RLIMIT_CPU,t);
		if(strcmp(lastpart,"-Ht")== 0) {
			if(t->rlim_max == RLIM_INFINITY) {
				printf("%s\n","unlimited");
			} else {
				printf("%lu\n",t->rlim_max);
			}
			return status;
		}
		if(t->rlim_cur == RLIM_INFINITY) {
			printf("%s\n","unlimited");
		} else {
			printf("%lu\n",t->rlim_cur);
		}
		return status;
	} else if(strcmp(lastpart,"-v")== 0 || strcmp(lastpart,"-Sv")== 0 || strcmp(lastpart,"-Hv")== 0) {
		status = getrlimit(RLIMIT_AS,t);
		if(strcmp(lastpart,"-Hv")== 0) {
			if(t->rlim_max == RLIM_INFINITY) {
				printf("%s\n","unlimited");
			} else {
				printf("%lu\n",t->rlim_max/1024);
			}
			return status;
		}
		if(t->rlim_cur == RLIM_INFINITY) {
			printf("%s\n","unlimited");
		} else {
			printf("%lu\n",t->rlim_cur/1024);
		}
		return status;
	} else if(strcmp(lastpart,"-d")== 0 || strcmp(lastpart,"-Sd")== 0 || strcmp(lastpart,"-Hd")== 0) {
		status = getrlimit(RLIMIT_DATA,t);
		if(strcmp(lastpart,"-Hd")== 0) {
			if(t->rlim_max == RLIM_INFINITY) {
				printf("%s\n","unlimited");
			} else {
				printf("%lu\n",t->rlim_max/1024);
			}
			return status;
		}
		if(t->rlim_cur == RLIM_INFINITY) {
			printf("%s\n","unlimited");
		} else {
			printf("%lu\n",t->rlim_cur/1024);
		}
		return status;
	} else if(strcmp(lastpart,"-x")== 0 || strcmp(lastpart,"-Sx")== 0 || strcmp(lastpart,"-Hx")== 0) {
		status = getrlimit(RLIMIT_LOCKS,t);
		if(strcmp(lastpart,"-Hx")== 0) {
			if(t->rlim_max == RLIM_INFINITY) {
				printf("%s\n","unlimited");
			} else {
				printf("%lu\n",t->rlim_max);
			}
			return status;
		}
		if(t->rlim_cur == RLIM_INFINITY) {
			printf("%s\n","unlimited");
		} else {
			printf("%lu\n",t->rlim_cur);
		}
		return status;
	} else if(strcmp(lastpart,"-l")== 0 || strcmp(lastpart,"-Sl")== 0 || strcmp(lastpart,"-Hl")== 0) {
		status = getrlimit(RLIMIT_MEMLOCK,t);
		if(strcmp(lastpart,"-Hl")== 0) {
			if(t->rlim_max == RLIM_INFINITY) {
				printf("%s\n","unlimited");
			} else {
				printf("%lu\n",t->rlim_max/1024);
			}
			return status;
		}
		if(t->rlim_cur == RLIM_INFINITY) {
			printf("%s\n","unlimited");
		} else {
			printf("%lu\n",t->rlim_cur/1024);
		}
		return status;
	}
	else if(strcmp(lastpart,"-q")== 0 || strcmp(lastpart,"-Sq")== 0 || strcmp(lastpart,"-Hq")== 0) {
		status = getrlimit(RLIMIT_MSGQUEUE,t);
		if(strcmp(lastpart,"-Hq")== 0) {
			if(t->rlim_max == RLIM_INFINITY) {
				printf("%s\n","unlimited");
			} else {
				printf("%lu\n",t->rlim_max);
			}
			return status;
		}
		if(t->rlim_cur == RLIM_INFINITY) {
			printf("%s\n","unlimited");
		} else {
			printf("%lu\n",t->rlim_cur);
		}
		return status;
	} else if(strcmp(lastpart,"-n")== 0 || strcmp(lastpart,"-Sn")== 0 || strcmp(lastpart,"-Hn")== 0) {
		status = getrlimit(RLIMIT_NOFILE,t);
		if(strcmp(lastpart,"-Hn")== 0) {
			if(t->rlim_max == RLIM_INFINITY) {
				printf("%s\n","unlimited");
			} else {
				printf("%lu\n",t->rlim_max);
			}
			return status;
		}
		if(t->rlim_cur == RLIM_INFINITY) {
			printf("%s\n","unlimited");
		} else {
			printf("%lu\n",t->rlim_cur);
		}
		return status;
	}else if(strcmp(lastpart,"-r")== 0 || strcmp(lastpart,"-Sr")== 0 || strcmp(lastpart,"-Hr")== 0) {
		status = getrlimit(RLIMIT_RTPRIO,t);
		if(strcmp(lastpart,"-Hr")== 0) {
			if(t->rlim_max == RLIM_INFINITY) {
				printf("%s\n","unlimited");
			} else {
				printf("%lu\n",t->rlim_max);
			}
			return status;
		}
		if(t->rlim_cur == RLIM_INFINITY) {
			printf("%s\n","unlimited");
		} else {
			printf("%lu\n",t->rlim_cur);
		}
		return status;
	}	else if(strcmp(lastpart,"-i")== 0 || strcmp(lastpart,"-Si")== 0 || strcmp(lastpart,"-Hi")== 0) {
		status = getrlimit(RLIMIT_SIGPENDING,t);
		if(strcmp(lastpart,"-Hi")== 0) {
			if(t->rlim_max == RLIM_INFINITY) {
				printf("%s\n","unlimited");
			} else {
				printf("%lu\n",t->rlim_max);
			}
			return status;
		}
		if(t->rlim_cur == RLIM_INFINITY) {
			printf("%s\n","unlimited");
		} else {
			printf("%lu\n",t->rlim_cur);
		}
		return status;
	}	else if(strcmp(lastpart,"-s")== 0 || strcmp(lastpart,"-Ss")== 0 || strcmp(lastpart,"-Hs")== 0) {
			status = getrlimit(RLIMIT_STACK,t);
			if(strcmp(lastpart,"-Hs")== 0) {
				if(t->rlim_max == RLIM_INFINITY) {
					printf("%s\n","unlimited");
				} else {
					printf("%lu\n",t->rlim_max/1024);
				}
			return status;
		}
		if(t->rlim_cur == RLIM_INFINITY) {
			printf("%s\n","unlimited");
		} else {
			printf("%lu\n",t->rlim_cur/1024);
		}
		return status;
	}	else if(strcmp(lastpart,"-u")== 0 || strcmp(lastpart,"-Su")== 0 || strcmp(lastpart,"-Hu")== 0) {
		status = getrlimit(RLIMIT_NPROC,t);
		if(strcmp(lastpart,"-Hu")== 0) {
			if(t->rlim_max == RLIM_INFINITY) {
				printf("%s\n","unlimited");
			} else {
				printf("%lu\n",t->rlim_max);
			}
			return status;
		}
		if(t->rlim_cur == RLIM_INFINITY) {
			printf("%s\n","unlimited");
		} else {
			printf("%lu\n",t->rlim_cur);
		}
		return status;
	}
	return status;
}
int setlimit(unused struct tokens * tokens) {
	int status = -1;
	int setstatus = -1;
	struct rlimit  * t = malloc(sizeof(struct rlimit)); 
	struct rlimit *newlimit = malloc(sizeof(struct rlimit));
	char * lastpart = tokens_get_token(tokens,(size_t)1); 
	if(strcmp(lastpart,"-f")== 0 || strcmp(lastpart,"-Sf")== 0 || strcmp(lastpart,"-Hf")== 0){
		status = getrlimit(RLIMIT_FSIZE,t);
		if(strcmp(lastpart,"-Hf")== 0) {
			newlimit->rlim_cur = t->rlim_cur;
			if(strcmp(tokens_get_token(tokens,(size_t)2),"unlimited") == 0) {
				newlimit->rlim_max = RLIM_INFINITY;
			} else {
				newlimit->rlim_max = atoi(tokens_get_token(tokens,(size_t)2));
			}
			setstatus = setrlimit(RLIMIT_FSIZE,newlimit);
			return setstatus;
		}
			newlimit->rlim_max = t->rlim_max;
			if(strcmp(tokens_get_token(tokens,(size_t)2),"unlimited") == 0) {
				newlimit->rlim_cur = RLIM_INFINITY;
			} else {
				newlimit->rlim_cur = atoi(tokens_get_token(tokens,(size_t)2));
			}
			setstatus = setrlimit(RLIMIT_FSIZE,newlimit);
			return setstatus;
		
	} else if(strcmp(lastpart,"-c")== 0 || strcmp(lastpart,"-Sc")== 0 || strcmp(lastpart,"-Hc")== 0) {
		status = getrlimit(RLIMIT_CORE,t);
		if(strcmp(lastpart,"-Hc")== 0) {
			newlimit->rlim_cur = t->rlim_cur;
			if(strcmp(tokens_get_token(tokens,(size_t)2),"unlimited") == 0) {
				newlimit->rlim_max = RLIM_INFINITY;
			} else {
				newlimit->rlim_max = atoi(tokens_get_token(tokens,(size_t)2));
			}
			setstatus = setrlimit(RLIMIT_CORE,newlimit);
			return setstatus;
		}
			newlimit->rlim_max = t->rlim_max;
			if(strcmp(tokens_get_token(tokens,(size_t)2),"unlimited") == 0) {
				newlimit->rlim_cur = RLIM_INFINITY;
			} else {
				newlimit->rlim_cur = atoi(tokens_get_token(tokens,(size_t)2));
			}
			setstatus = setrlimit(RLIMIT_CORE,newlimit);
			return setstatus;
	} else if(strcmp(lastpart,"-t")== 0 || strcmp(lastpart,"-St")== 0 || strcmp(lastpart,"-Ht")== 0) {
		status = getrlimit(RLIMIT_CPU,t);
		if(strcmp(lastpart,"-Ht")== 0) {
			newlimit->rlim_cur = t->rlim_cur;
			if(strcmp(tokens_get_token(tokens,(size_t)2),"unlimited") == 0) {
				newlimit->rlim_max = RLIM_INFINITY;
			} else {
				newlimit->rlim_max = atoi(tokens_get_token(tokens,(size_t)2));
			}
			setstatus = setrlimit(RLIMIT_CPU,newlimit);
			return setstatus;
		}
			newlimit->rlim_max = t->rlim_max;
			if(strcmp(tokens_get_token(tokens,(size_t)2),"unlimited") == 0) {
				newlimit->rlim_cur = RLIM_INFINITY;
			} else {
				newlimit->rlim_cur = atoi(tokens_get_token(tokens,(size_t)2));
			}
			setstatus = setrlimit(RLIMIT_CPU,newlimit);
			return setstatus;
	} else if(strcmp(lastpart,"-v")== 0 || strcmp(lastpart,"-Sv")== 0 || strcmp(lastpart,"-Hv")== 0) {
		status = getrlimit(RLIMIT_AS,t);
		if(strcmp(lastpart,"-Hv")== 0) {
			newlimit->rlim_cur = t->rlim_cur;
			if(strcmp(tokens_get_token(tokens,(size_t)2),"unlimited") == 0) {
				newlimit->rlim_max = RLIM_INFINITY;
			} else {
				newlimit->rlim_max = atoi(tokens_get_token(tokens,(size_t)2))*1024;
			}
			setstatus = setrlimit(RLIMIT_AS,newlimit);
			return setstatus;
		}
			newlimit->rlim_max = t->rlim_max;
			if(strcmp(tokens_get_token(tokens,(size_t)2),"unlimited") == 0) {
				newlimit->rlim_cur = RLIM_INFINITY;
			} else {
				newlimit->rlim_cur = atoi(tokens_get_token(tokens,(size_t)2))*1024;
			}
			setstatus = setrlimit(RLIMIT_AS,newlimit);
			return setstatus;
	} else if(strcmp(lastpart,"-d")== 0 || strcmp(lastpart,"-Sd")== 0 || strcmp(lastpart,"-Hd")== 0) {
		status = getrlimit(RLIMIT_DATA,t);
		if(strcmp(lastpart,"-Hd")== 0) {
			newlimit->rlim_cur = t->rlim_cur;
			if(strcmp(tokens_get_token(tokens,(size_t)2),"unlimited") == 0) {
				newlimit->rlim_max = RLIM_INFINITY;
			} else {
				newlimit->rlim_max = atoi(tokens_get_token(tokens,(size_t)2))*1024;
			}
			setstatus = setrlimit(RLIMIT_DATA,newlimit);
			return setstatus;
		}
			newlimit->rlim_max = t->rlim_max;
			if(strcmp(tokens_get_token(tokens,(size_t)2),"unlimited") == 0) {
				newlimit->rlim_cur = RLIM_INFINITY;
			} else {
				newlimit->rlim_cur = atoi(tokens_get_token(tokens,(size_t)2))*1024;
			}
			setstatus = setrlimit(RLIMIT_DATA,newlimit);
			return setstatus;
	} else if(strcmp(lastpart,"-x")== 0 || strcmp(lastpart,"-Sx")== 0 || strcmp(lastpart,"-Hx")== 0) {
		status = getrlimit(RLIMIT_LOCKS,t);
		if(strcmp(lastpart,"-Hx")== 0) {
			newlimit->rlim_cur = t->rlim_cur;
			if(strcmp(tokens_get_token(tokens,(size_t)2),"unlimited") == 0) {
				newlimit->rlim_max = RLIM_INFINITY;
			} else {
				newlimit->rlim_max = atoi(tokens_get_token(tokens,(size_t)2));
			}
			setstatus = setrlimit(RLIMIT_LOCKS,newlimit);
			return setstatus;
		}
			newlimit->rlim_max = t->rlim_max;
			if(strcmp(tokens_get_token(tokens,(size_t)2),"unlimited") == 0) {
				newlimit->rlim_cur = RLIM_INFINITY;
			} else {
				newlimit->rlim_cur = atoi(tokens_get_token(tokens,(size_t)2));
			}
			setstatus = setrlimit(RLIMIT_LOCKS,newlimit);
			return setstatus;
	} else if(strcmp(lastpart,"-l")== 0 || strcmp(lastpart,"-Sl")== 0 || strcmp(lastpart,"-Hl")== 0) {
		status = getrlimit(RLIMIT_MEMLOCK,t);
		if(strcmp(lastpart,"-Hl")== 0) {
			newlimit->rlim_cur = t->rlim_cur;
			if(strcmp(tokens_get_token(tokens,(size_t)2),"unlimited") == 0) {
				newlimit->rlim_max = RLIM_INFINITY;
			} else {
				newlimit->rlim_max = atoi(tokens_get_token(tokens,(size_t)2))*1024;
			}
			setstatus = setrlimit(RLIMIT_MEMLOCK,newlimit);
			return setstatus;
		}
			newlimit->rlim_max = t->rlim_max;
			if(strcmp(tokens_get_token(tokens,(size_t)2),"unlimited") == 0) {
				newlimit->rlim_cur = RLIM_INFINITY;
			} else {
				newlimit->rlim_cur = atoi(tokens_get_token(tokens,(size_t)2))*1024;
			}
			setstatus = setrlimit(RLIMIT_MEMLOCK,newlimit);
			return setstatus;

	}	else if(strcmp(lastpart,"-q")== 0 || strcmp(lastpart,"-Sq")== 0 || strcmp(lastpart,"-Hq")== 0) {
		status = getrlimit(RLIMIT_MSGQUEUE,t);
		if(strcmp(lastpart,"-Hq")== 0) {
			newlimit->rlim_cur = t->rlim_cur;
			if(strcmp(tokens_get_token(tokens,(size_t)2),"unlimited") == 0) {
				newlimit->rlim_max = RLIM_INFINITY;
			} else {
				newlimit->rlim_max = atoi(tokens_get_token(tokens,(size_t)2));
			}
			setstatus = setrlimit(RLIMIT_MSGQUEUE,newlimit);
			return setstatus;
		}
			newlimit->rlim_max = t->rlim_max;
			if(strcmp(tokens_get_token(tokens,(size_t)2),"unlimited") == 0) {
				newlimit->rlim_cur = RLIM_INFINITY;
			} else {
				newlimit->rlim_cur = atoi(tokens_get_token(tokens,(size_t)2));
			}
			setstatus = setrlimit(RLIMIT_MSGQUEUE,newlimit);
			return setstatus;
	} else if(strcmp(lastpart,"-n")== 0 || strcmp(lastpart,"-Sn")== 0 || strcmp(lastpart,"-Hn")== 0) {
		status = getrlimit(RLIMIT_NOFILE,t);
		if(strcmp(lastpart,"-Hn")== 0) {
			newlimit->rlim_cur = t->rlim_cur;
			if(strcmp(tokens_get_token(tokens,(size_t)2),"unlimited") == 0) {
				newlimit->rlim_max = RLIM_INFINITY;
			} else {
				newlimit->rlim_max = atoi(tokens_get_token(tokens,(size_t)2));
				printf("es unda gaxdes limiti %lu\n",newlimit->rlim_max);
			}
			setstatus = setrlimit(RLIMIT_NOFILE,newlimit);
			printf("%d/n",setstatus);
			return setstatus;
		}
			newlimit->rlim_max = t->rlim_max;
			if(strcmp(tokens_get_token(tokens,(size_t)2),"unlimited") == 0) {
				newlimit->rlim_cur = RLIM_INFINITY;
			} else {
				newlimit->rlim_cur = atoi(tokens_get_token(tokens,(size_t)2));
			}
			setstatus = setrlimit(RLIMIT_NOFILE,newlimit);
			return setstatus;
	}else if(strcmp(lastpart,"-r")== 0 || strcmp(lastpart,"-Sr")== 0 || strcmp(lastpart,"-Hr")== 0) {
		status = getrlimit(RLIMIT_RTPRIO,t);
		if(strcmp(lastpart,"-Hr")== 0) {
			newlimit->rlim_cur = t->rlim_cur;
			if(strcmp(tokens_get_token(tokens,(size_t)2),"unlimited") == 0) {
				newlimit->rlim_max = RLIM_INFINITY;
			} else {
				newlimit->rlim_max = atoi(tokens_get_token(tokens,(size_t)2));
			}
			setstatus = setrlimit(RLIMIT_RTPRIO,newlimit);
			return setstatus;
		}
			newlimit->rlim_max = t->rlim_max;
			if(strcmp(tokens_get_token(tokens,(size_t)2),"unlimited") == 0) {
				newlimit->rlim_cur = RLIM_INFINITY;
			} else {
				newlimit->rlim_cur = atoi(tokens_get_token(tokens,(size_t)2));
			}
			setstatus = setrlimit(RLIMIT_RTPRIO,newlimit);
			return setstatus;
	}	else if(strcmp(lastpart,"-i")== 0 || strcmp(lastpart,"-Si")== 0 || strcmp(lastpart,"-Hi")== 0) {
		status = getrlimit(RLIMIT_SIGPENDING,t);
		if(strcmp(lastpart,"-Hi")== 0) {
			newlimit->rlim_cur = t->rlim_cur;
			if(strcmp(tokens_get_token(tokens,(size_t)2),"unlimited") == 0) {
				newlimit->rlim_max = RLIM_INFINITY;
			} else {
				newlimit->rlim_max = atoi(tokens_get_token(tokens,(size_t)2));
			}
			setstatus = setrlimit(RLIMIT_SIGPENDING,newlimit);
			return setstatus;
		}
			newlimit->rlim_max = t->rlim_max;
			if(strcmp(tokens_get_token(tokens,(size_t)2),"unlimited") == 0) {
				newlimit->rlim_cur = RLIM_INFINITY;
			} else {
				newlimit->rlim_cur = atoi(tokens_get_token(tokens,(size_t)2));
			}
			setstatus = setrlimit(RLIMIT_SIGPENDING,newlimit);
			return setstatus;
	}	else if(strcmp(lastpart,"-s")== 0 || strcmp(lastpart,"-Ss")== 0 || strcmp(lastpart,"-Hs")== 0) {
		status = getrlimit(RLIMIT_STACK,t);
		if(strcmp(lastpart,"-Hs")== 0) {
			newlimit->rlim_cur = t->rlim_cur;
			if(strcmp(tokens_get_token(tokens,(size_t)2),"unlimited") == 0) {
				newlimit->rlim_max = RLIM_INFINITY;
			} else {
				newlimit->rlim_max = atoi(tokens_get_token(tokens,(size_t)2))*1024;
			}
			setstatus = setrlimit(RLIMIT_STACK,newlimit);
			return setstatus;
		}
			newlimit->rlim_max = t->rlim_max;
			if(strcmp(tokens_get_token(tokens,(size_t)2),"unlimited") == 0) {
				newlimit->rlim_cur = RLIM_INFINITY;
			} else {
				newlimit->rlim_cur = atoi(tokens_get_token(tokens,(size_t)2))*1024;
			}
			setstatus = setrlimit(RLIMIT_STACK,newlimit);
			return setstatus;
	}	else if(strcmp(lastpart,"-u")== 0 || strcmp(lastpart,"-Su")== 0 || strcmp(lastpart,"-Hu")== 0) {
		status = getrlimit(RLIMIT_NPROC,t);
		if(strcmp(lastpart,"-Hu")== 0) {
			newlimit->rlim_cur = t->rlim_cur;
			if(strcmp(tokens_get_token(tokens,(size_t)2),"unlimited") == 0) {
				newlimit->rlim_max = RLIM_INFINITY;
			} else {
				newlimit->rlim_max = atoi(tokens_get_token(tokens,(size_t)2));
			}
			setstatus = setrlimit(RLIMIT_NPROC,newlimit);
			return setstatus;
		}
			newlimit->rlim_max = t->rlim_max;
			if(strcmp(tokens_get_token(tokens,(size_t)2),"unlimited") == 0) {
				newlimit->rlim_cur = RLIM_INFINITY;
			} else {
				newlimit->rlim_cur = atoi(tokens_get_token(tokens,(size_t)2));
			}
			setstatus = setrlimit(RLIMIT_NPROC,newlimit);
			return setstatus;
	}
	return (setstatus +status);
}
int cmd_ulimit(unused struct tokens * tokens) {
	int status = -1;
	if(tokens_get_length(tokens)  == 2  || tokens_get_length(tokens)  == 1) {
		 status = getlimit(tokens);
		 return status;
	}
	if(tokens_get_length(tokens)  == 3) {
		status = setlimit(tokens);
		return status;
	}
	return status;
}
int cmd_nice(unused struct tokens * tokens) {
	if(tokens_get_length(tokens) == 1) {
		errno = 0;
		int prio = getpriority(PRIO_PROCESS,0);
		if(prio == -1 && errno != 0) {
			 printf("folowwing error happned : %s\n",strerror(errno));
			 return -1;
		}
		printf("%d\n",prio);
		return 0;
	} else if(tokens_get_length(tokens) == 2) {
		int newniceval = atoi(tokens_get_token(tokens,(size_t)1));
		int status = setpriority(PRIO_PROCESS,0,newniceval);
		if(status == -1) {
			printf("folowwing error happned : %s\n",strerror(errno));
			return -1;
		}
	}
	return 0;
}



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
      printf("error with execution program\n");
      
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

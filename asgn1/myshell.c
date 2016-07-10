/*
Jinyan Li
ID:jli134
CMPS111 asgn1

references:
file descriptor explanation: http://web.cs.ucla.edu/classes/fall08/cs111/scribe/4/index.html
man pages I looked at:
https://www.freebsd.org/cgi/man.cgi?fork(2)
https://www.freebsd.org/cgi/man.cgi?query=wait&sektion=2
https://www.freebsd.org/cgi/man.cgi?query=dup&sektion=2&manpath=FreeBSD+7.1-RELEASE

*/

#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <sys/file.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/stat.h>

void exe(char **  allargs, int* fd, int is_pipe_line, int std_err_redirection);
extern char ** get_args();

//In the main(), I used the code provided by argshell.c
int main()
{
    int            i;
    char **     args;

    while (1) {
		printf ("Command ('exit' to quit): ");
		args = get_args();
		for (i = 0; args[i] != NULL; i++) {
			//printf ("Argument %d: %s\n", i, args[i]);
		}
        if (args[0] == NULL) {
			printf ("No arguments on line!\n");
		} else if ( !strcmp (args[0], "exit")) {
			printf ("Exiting...\n");
			break;
		}else{
			exe(args, NULL, 0, 0);
		}
    }
	return 0;
}

void exe(char **  allargs, int* fd, int is_pipe_line, int std_err_redirection){
          //printf("is_pipe_line: %d\n",is_pipe_line);
          //if(fd==NULL){ 
			 //printf ("fd is NULL\n");
		  //}
		 int  i, argslength=0, specialchars_index=0, specialchars=0, seqlength=0, semicolindex, chainflag=0;

		 //get the first subsequence of  the command delimited by ";"
		 for(i=0; allargs[i]!=NULL;i++){
			  semicolindex=i;
			  if(strcmp(allargs[i],";")==0){
				  chainflag=1;
				  break;
			  }
			  seqlength++;
		 }
         if(seqlength!=0){
			 
				 char * args[seqlength+1];

				 for(i=0; i<seqlength; i++){
					 args[i]=allargs[i];
					//printf("subsequence_args[%d]:%s\n", i, args[i]);
				 }

				 args[seqlength]=NULL;

				 for (i = 0; args[i] != NULL; i++) {
					argslength++;
				 }

				 if ( strcmp (args[0], "exit")==0) {
					printf ("Exiting...\n");
					exit(0);
				 }

				 for (i = 0; args[i] != NULL; i++) {
					if(strcmp(args[i],">>")==0){
						specialchars=1;
						specialchars_index=i;
		//				printf ("special chars:%s\n", args[i]);
		//				printf("filename: %s\n", args[i+1]);
						break;
					}else if(strcmp(args[i],">>&")==0){
						specialchars=1;
						std_err_redirection=1;
						specialchars_index=i;
		//				printf ("special chars:%s\n", args[i]);
		//				printf("filename: %s\n", args[i+1]);
						break;
					}
					else if(strcmp(args[i],">")==0){
						specialchars=2;
						specialchars_index=i;
		//				printf ("special chars:%s\n", args[i]);
		//				printf("filename: %s\n", args[i+1]);
						break;
					}else if(strcmp(args[i],">&")==0){
						specialchars=2;
						std_err_redirection=1;
						specialchars_index=i;
		//				printf ("special chars:%s\n", args[i]);
		//				printf("filename: %s\n", args[i+1]);
						break;
					}
					else if(strcmp(args[i],"<")==0){
						specialchars=3;
						specialchars_index=i;
		//				printf ("special chars:%s\n", args[i]);
		//				printf("filename: %s\n", args[i+1]);
						break;
					}else if(strcmp(args[i],"|")==0){
						specialchars=4;
						if(strcmp(args[i],"|")==0){
							 specialchars_index=i;	
						}
						//is_pipe_line=1;
						break;
					}else if(strcmp(args[i],"|&")==0){
						specialchars=4;
						std_err_redirection=1;
						specialchars_index=i;
						is_pipe_line=1;
						break;
					}

				}

				//if the command is "cd" don't fork()
				if(strcmp(args[0],"cd")==0){
					char buf[100];
					char * currentpath=getcwd(buf, 100);
					if(currentpath==NULL){
						perror("getcwd()");
					}
					int error;
					if(argslength==1){
						error=chdir(currentpath);
					}else{
						error=chdir(args[1]);
					}

					if(error==-1){
						perror(args[0]);
					}
					if(chainflag==1){
						//recursion call
						exe(&allargs[semicolindex+1], NULL, 0, std_err_redirection);
					}
				}else{
						int pid, e=0, status=0;
						/* I looked at the code of this site
  						    http://www.csl.mtu.edu/cs4411.ck/www/NOTES/process/fork/exec.html
							and the lecture slide as reference when I wrote this part.
						*/
						if((pid=fork())!=0){
							//parent code
								   if(pid<0){
									  fprintf(stderr,"Error: Can't fork process PID %d\n", pid);
									  perror(args[0]);
								   }else{
												if (wait(&status)<0) {
													fprintf(stderr,"Error occurred when calling process PID: %d\n", pid);
													perror(args[0]);
												}

										if(chainflag==1){
											//recursion call
											exe(&allargs[semicolindex+1], NULL, 0, std_err_redirection);
										}
									}
						}else{
							//child code
							//printf("-----------------------child code---------------------\n");
							if(specialchars>0 & specialchars<4){//command with special characters.
								//printf("----------------command with special characters.----------------------------\n");
								int j;
								//printf ("specialchars_index: %d\n", specialchars_index);
								char *     newargs[specialchars_index+1];
								for(j=0;j<specialchars_index;j++){
									newargs[j]=args[j];
									//printf("newargs:%s\n", newargs[j]);
								}

								newargs[specialchars_index]=NULL;

								int fd, error=0;

								if(specialchars==1){
									fd=open(args[specialchars_index+1], O_CREAT | O_APPEND | O_WRONLY, S_IRWXU | S_IRWXG | S_IRWXO);
									if(fd==-1) perror(args[specialchars_index+1]);

									//close(1);
									error=dup2(fd,1);
									if(std_err_redirection==1){
										//close(2);
										error=dup2(fd,2);
									}
								}else if (specialchars==2){
									fd=open(args[specialchars_index+1], O_CREAT | O_EXCL |O_WRONLY, S_IRWXU | S_IRWXG | S_IRWXO);
									if(fd==-1) perror(args[specialchars_index+1]);
									//close(1);
									error=dup2(fd,1);
									if(std_err_redirection==1){
										//close(2);
										error=dup2(fd, 2);
									}
								}else{
									fd=open(args[specialchars_index+1], O_RDONLY );
									if(fd==-1) perror(args[specialchars_index+1]);
									//close_err=close(0);
									error=dup2(fd,0);
								}
								 
								 if (error == -1){
									perror("dup2()");
									exit(1);
								 }
								 
								e=execvp(newargs[0], newargs);

							}else if(specialchars==4 | is_pipe_line==1){//code for implementing pipe "|"
									int  j;
									//printf("--------------pipeline case 1--------------------\n");
									//printf ("pipe:specialchars_index: %d\n", specialchars_index);
									//printf ("pipe:specialchars: %d\n", specialchars);
									//printf ("pipe:argslength: %d\n", argslength);
									//printf("seqlength: %d\n", seqlength);
									//printf("specialchars_index: %d\n", specialchars_index);
									if(fd==NULL & specialchars_index!=0 ){
										int commlen=0;
										
										if(std_err_redirection!=1){
											for(j=seqlength; strcmp(args[j-1], "|")!=0; j--){
												commlen++;
											}
										}else{
											for(j=seqlength; strcmp(args[j-1], "|&")!=0; j--){
												commlen++;
											}				
										}

										for(j=seqlength-commlen; args[j] != NULL; j++){
											//printf("currentargs[%d]:%s \n", j, args[j]);
										}
										char* prevcommand[seqlength-commlen-1];
										
										for(j=0; j<seqlength-commlen-1 ; j++){
											prevcommand[j]=args[j];
											//printf("prevcommand[%d]:%s \n", j, prevcommand[j]);
										}
										prevcommand[seqlength-commlen-1]=NULL;
										
										//for(j=0; prevcommand[j]!=NULL ; j++){
											//printf("prevcommand[%d]:%s \n", j, prevcommand[j]);
										//}

										int  fd[2];
																		
										if (pipe(fd)== -1){
											perror("pipe(fd)");
											exit(1);
										}
										//printf("parent fd[0]:%d, fd[1]:%d\n", fd[0], fd[1]);

										if (dup2(fd[0],0)== -1){
											perror("dup2(fd[0],0)");
											exit(1);
										}
										
										exe(prevcommand, fd, 1, std_err_redirection);

										if (close(fd[1])== -1){
											perror("close()");
											exit(1);
										}
										
										e=execvp(args[seqlength-commlen], &args[seqlength-commlen]);

									}else if(fd!=NULL & specialchars_index!=0){
										//printf("--------------pipeline case 2--------------------\n");						
										int commlen=0;
										
										if(std_err_redirection!=1){
											for(j=seqlength; strcmp(args[j-1], "|")!=0; j--){
												commlen++;
											}
										}else{
											for(j=seqlength; strcmp(args[j-1], "|&")!=0; j--){
												commlen++;
											}				
										}

										//for(j=seqlength-commlen; args[j] != NULL; j++){
											//printf("currentargs[%d]:%s \n", j, args[j]);
										//}
										char* prevcommand[seqlength-commlen-1];
										
										for(j=0; j<seqlength-commlen-1 ; j++){
											prevcommand[j]=args[j];
										}
										prevcommand[seqlength-commlen-1]=NULL;
										
										//printf("parent fd[0]:%d, fd[1]:%d\n", fd[0], fd[1]);	
										int  newfd[2];	
										
										pipe(newfd);
										
										if (pipe(newfd)== -1){
											perror("pipe(newfd)");
											exit(1);
										}
										//printf("child newfd[0]:%d, newfd[1]:%d\n", newfd[0], newfd[1]);
					

										if (dup2(newfd[0],0)== -1){
											perror("dup2(newfd[0],0)");
											exit(1);
										}

										if (dup2(fd[1],1)== -1){
											perror("dup2(fd[1],1)");
											exit(1);
										}
										
										exe(prevcommand, newfd, 1, std_err_redirection);
										
										if (close(newfd[1])== -1){
											perror("close(newfd[1])");
											exit(1);
										}
										
										if (close(fd[0])== -1){
											perror("close(fd[0])");
											exit(1);
										}
										
										e=execvp(args[seqlength-commlen], &args[seqlength-commlen]);

									}else{
										//fprintf(stderr, "--------------------------------pipe case 3-----------------------------\n");
										//printf("child fd[0]:%d, fd[1]:%d\n", fd[0],fd[1]);
										for(j=0; args[j] != NULL; j++){
											//printf("current args[%d]:%s \n", j, args[j]);
										}
											if(dup2(fd[1],1)==-1){
												perror("dup2(fd[1],1)");
											}							
											
											if(std_err_redirection==1){
											   if(dup2(fd[1],2)==-1){
												   perror("dup2(fd[1],2)");
											  }
											}
											if(close(fd[0])==-1){
												perror("close(fd[0])");										
											}
																			
										   if (execvp(args[0], args)== -1){
												perror(args[0]);
												exit(1);
											}						   
										}
									
									exit(0);
							}else{
									//command with zero or more arguments.
									e=execvp(args[0], args);
							}
							
							if (e == -1){
								  perror(args[0]);
								  exit(1);
							}
							exit(0);
						 
					}
				}
		}
}

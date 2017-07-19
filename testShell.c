#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <string.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <sys/stat.h>

//////////////////////////////////////////////////////////
//  Name: Osbaldo Esquivel                              //
//  Date: 19FEB2016                                     //
//  Title: smallsh                                      //
//  Purpose: This program mimics a shell where you are  //
//           able to run commands, redirect input/output//
//           and run processes in the background using  //
//           the & character.                           //
//////////////////////////////////////////////////////////

int main() {
	struct sigaction sa;
	char *tempArgs[513], *outFile = NULL, *inFile = NULL, *toke, *row = NULL;
	char *STAT = "status";
	char *EXIT = "exit";
	char *CHGDIR = "cd";
	int count, front, backgd, stat = 0, procID, ex = 0, i;

	// signal handler that ignores SIGINT
	sa.sa_handler = SIG_IGN;
	sigaction(SIGINT, &sa, NULL);

	while (!ex) {
		count = 0; // to track number of arguments
		ssize_t num;
		backgd = 0; // background process boolean

		// command line prompt
	   	printf(": ");
        	fflush(stdout);

		// get the command line arguments
	    	if (!(getline(&row, &num, stdin))) {
			return 0;
  	    	}

		// tokenize row
        	toke = strtok(row, "  \n");

        	while (toke != NULL) {
			// check for input redirection
			if (strcmp(toke, "<") == 0) {

				toke = strtok(NULL, " \n");
				inFile = strdup(toke);

				toke = strtok(NULL, " \n");
			}
			// check if background process and
			// set boolean
			else if (strcmp(toke, "&") == 0) {
				backgd = 1;
				break;
			}
			// check for output redirection
			else if (strcmp(toke, ">") == 0) {
				toke = strtok(NULL, " \n");
				outFile =  strdup(toke);
				toke = strtok(NULL, " \n");
			}
			// otherwise store arguments
			else {
				tempArgs[count] = strdup(toke);
				toke = strtok(NULL, " \n");
				count++;
			}
        	}

		// set last element of array to NULL
        	tempArgs[count] = NULL;
		// if argument is status, return status of last process
	        if (strcmp(tempArgs[0], STAT) == 0) {
			if (WIFEXITED(stat)) {
				printf("exit value %d\n", WEXITSTATUS(stat));
			}
			else {
				printf("The signal that terminated is %d\n", stat);
			}
		}
		// if argument is exit, exit program
	    	else if (strcmp(tempArgs[0], EXIT) == 0) {
			exit(0);
			ex = 1;
        	}
		// if it is a blank line or a comment, do nothing
        	else if (!(strncmp(tempArgs[0], "#", 1)) || tempArgs[0] == NULL) {
			;
        	}
		// if agrument is change directory, change the directory to home
		// otherwise change it to provided directory
	    	else if (strcmp(tempArgs[0], CHGDIR) == 0) {
            		if (tempArgs[1] == NULL) {
               			chdir(getenv("HOME"));
           		}
		    	else {
                		chdir(tempArgs[1]);
            		}
       		}
		// if argument is any other command, fork the process
	    	else {
            		procID = fork();

			// if the child process
            		if (procID == 0) {
				// if not background process, set default signal
				if(!backgd) {
					sa.sa_handler = SIG_DFL;
					sa.sa_flags = 0;
					sigaction(SIGINT, &sa, NULL);
				}

				if (outFile != NULL) {
					front = open(outFile, O_WRONLY | O_CREAT | O_TRUNC, 0644);
					// output if error opening file
					if (front == -1) {
						fprintf(stderr, "Could not open file %s\n", outFile);
						fflush(stdout);
						exit(1);
					}
					// output if dup error
					if (dup2(front, 1) == -1) {
						fprintf(stderr, "There was a dup2 error.\n");
						exit(1);
					}

					close(front);
				}
				else if (inFile != NULL) {
					front = open(inFile, O_RDONLY);
					// output if error opening file
					if (front == -1) {
						fprintf(stderr, "Could not open file %s\n", inFile);
						fflush(stdout);
						exit(1);
					}
					// ouput if dup error
					else if (dup2(front, 0) == -1) {
						fprintf(stderr, "There was a dup2 error.\n");
						exit(1);
					}

					close(front);
				}
				else if (backgd) {
					front = open("/dev/null", O_RDONLY);
					// output if error opening file
					if (front == -1) {
						fprintf(stderr, "Error opening file.\n");
						exit(1);
					}
					// output if dup error
					else if (dup2(front, 0) == -1) {
						fprintf(stderr, "There was a dup2 error.\n");
						exit(1);
					}

					close(front);
				}


				// if execvp returns, then it failed. outputs error message
				// and exits program with status 1. Otherwise, it will execute
				// the users command
				if (execvp(tempArgs[0], tempArgs)) {
					fprintf(stderr, "You cannot run command %s\n",tempArgs[0]);
					fflush(stdout);
					exit(1);
				}
            		}
			// if the pid is anything less than zero, there has been
			// a fork error
			else if (procID < 0) {
				printf("There was a fork error.\n");
				fflush(stdout);
				stat = 1;
				break;
			}
			// wait for process to finish
			else {
				if (!backgd) {
					do {
						waitpid(procID, &stat, 0);
				   } while (!WIFEXITED(stat) && !WIFSIGNALED(stat));
				}
				else {
					printf("background pid is %d\n", procID);
					fflush(stdout);
				}
			}
        }
	// reset file pointers
	inFile = NULL;
	outFile = NULL;

	// reset the array for next line
        while(i < count) {
            	tempArgs[i] = NULL;
		i++;
        }

        // wait for any child process and retun immediately if no child
        // has exited
	procID = waitpid(-1, &stat, WNOHANG);

	// if background processes have finished, output that they have
	// finished, the status if applicable, or the signal that terminated
	// the process
        while (procID > 0) {
            	printf("background pid %d is done:", procID);

 	    	if (WIFEXITED(stat)) {
	    		printf("exit value %d\n", WEXITSTATUS(stat));
			fflush(stdout);
	    	}
	    	else {
			printf("The signal that terminated is %d\n", stat);
			fflush(stdout);
	    	}

	    	procID = waitpid(-1, &stat, WNOHANG);
        }
    }

    return 0;
}

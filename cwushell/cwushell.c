#include <stdio.h> 
#include <sys/types.h> 
#include <unistd.h>  
#include <stdlib.h> 
#include <errno.h>   
#include <sys/wait.h>
#include <string.h>

#define BUFFER_SIZE 100
#define DELIMITERS " \t\r\n\a"

typedef struct Cwushell {
	char* prompt;
	char* command;
	char** args;
	int argsSize;

} Cwushell;

Cwushell* createShell() {
	Cwushell* shell = malloc(sizeof(Cwushell));
	shell->prompt = "cwushell>";
	shell->command = NULL;
	shell->args = NULL;
	shell->argsSize = 0;
	return shell;
}

void deleteArgs(Cwushell* shell)
{
	for (int i = 0; i < shell->argsSize - 1; i++) {//size - 1 because it is a null terminated list
		free(shell->args[i]);
	}
	free(shell->args);
}

void deleteShell(Cwushell* shell) {
	free(shell->prompt);
	free(shell->command);
	deleteArgs(shell);
}


int exitShell(char* exitCode) {
	if (!exitCode) {
		exit(0);
	}
	else if (strcmp(exitCode, "-h") == 0) {
		printf("exit [n]:\nThis command will terminate the shell providing the number given in 'n' as the exit code.\nInput argument must be an integer that is not 0.\nShould you wish to exit the program with a successful error code (0) do not include an argument.\n");
		return 1;
	}
	int code = atoi(exitCode);
	if (code == 0) {
		printf("Invalid argument provided. Expected a non-zero integer and you gave %s\n", exitCode);
		return 1;
	}
	else {
		exit(code);
	}
}

int prompt(Cwushell* shell, char* newPrompt) {
	if (!newPrompt) {
		shell->prompt = "cwushell>";
	}
	else if (strcmp(newPrompt, "-h") == 0) {
		printf("prompt [newprompt]:\nThis command will change the prompt which is displayed before each command you enter.\nThis takes 1 input argument and if no argument is given the default value will be used \"cwushell>\".\n");
		return 1;
	}
	else {
		shell->prompt = newPrompt;
	}
	return 1;
}

int cpuinfo(char* swtch) {
	if (!swtch) {
		printf("No switch provided. Please provide either -c, -t, -n, or -h if you need help.\n");
	}
	else if (strcmp(swtch,"-h") == 0) {
		printf("cpuinfo -[c|t|n] - switch - values c or t or n\nThis function will provide information about the CPU you are currently using.\nThe switch is required for the command to work.\nWith a value of 'c' it will print the cpu clock.\nWith a value of 't' it will print the cpu type.\nWith a value of 'n' it will print the number of cores.\n");
	}
	else {
		FILE *cpuinfo = fopen("/proc/cpuinfo", "r");
		if (cpuinfo == NULL) {
			printf("There was a problem finding your cpuinfo file.\n");// handle error;
			return 1;
		}
		char line[256];
		if (strcmp(swtch, "-t") == 0) {
			while (line[0] != EOF && fgets(line, sizeof(line), cpuinfo))
			{
				char cpuType[256];
				if (sscanf(line, "model name : %s", cpuType) == 1)
				{
					fclose(cpuinfo);
					printf("%s\n", cpuType);
					return 1;
				}
			}
		}
		else if (strcmp(swtch, "-c") == 0) {
			while (line[0] != EOF && fgets(line, sizeof(line), cpuinfo))
			{
				float clock = 0.0;
				if (sscanf(line, "cpu MHz : %f", &clock) == 1)
				{
					fclose(cpuinfo);
					printf("%f MHz\n", clock);
					return 1;
				}
			}
		}
		else if (strcmp(swtch, "-n") == 0) {
			int cores = 0;
			while (line[0] != EOF && fgets(line, sizeof(line), cpuinfo)) {
				if (sscanf(line, "cpu cores : %d", &cores) == 1)
				{
					fclose(cpuinfo);
					printf("%d Cores\n", cores);
					return 1;
				}
			}
		}
		else {
			fclose(cpuinfo);
			printf("You provided an invalid switch. Please provide either -c, -t, -n, or -h if you need help.\n");
			return 1;
		}
		fclose(cpuinfo); //if we have made it this far then we did not find the right text in the file
		printf("The requested information was not found in your system.\n");
		return 1;
	}
}

int meminfo(char* swtch) {
	if (!swtch) {
		printf("No switch provided. Please provide either -t, -u, -c, or -h if you need help.\n");
		return 1;
	}
	else if (strcmp(swtch, "-h") == 0) {
		printf("meminfo -[t|u|c] - switch - values t or u or \nThis function will provide you information about the system memory being used.\nThe switch is required for the command to work.\nWith a value of 't' it will print the total RAM memory in in bytes.\nWith a value of 'u' it will print the used RAM memory.\nWith a value of 'c' it will print the size of the L2 cache / core in bytes.\n");
		return 1;
	}
	else {
		FILE *meminfo = fopen("/proc/meminfo", "r");
		if (!meminfo) {
			printf("There was a problem finding your meminfo file.\n");// handle error;
			return 1;
		}
		char line[256];
		if (strcmp(swtch, "-t") == 0) {
			while (line[0] != EOF && fgets(line, sizeof(line), meminfo))
			{
				int ram;
				if (sscanf(line, "MemTotal: %d kB", &ram) == 1)
				{
					fclose(meminfo);
					printf("%d Bytes Total\n", ram * 1024); //meminfo file is in kiloBytes so have to convert from kB to B
					return 1;
				}
			}
		}
		else if (strcmp(swtch, "-u") == 0) {
			int ram = 0, free = 0, buffers = 0, cached = 0;
			while (line[0] != EOF && fgets(line, sizeof(line), meminfo))
			{
				//printf("%s", line);
				if (ram == 0) {
					sscanf(line, "MemTotal: %d kB", &ram);
					//printf("%d ram\n", ram);
				}
				if (free == 0) {
					sscanf(line, "MemFree: %d kB", &free);
					//printf("%d free\n", free);
				}
				if (buffers == 0) {
					sscanf(line, "Buffers: %d kB", &buffers);
					//printf("%d buffers\n", buffers);
				}
				if (cached == 0) {
					sscanf(line, "Cached: %d kB", &cached);
					//printf("%d cached\n", cached);
				}
				if(ram != 0 && free != 0 && buffers!= 0 && cached != 0){
					fclose(meminfo);
					printf("%d Bytes Used\n", (ram - (free + buffers + cached)) * 1024); //meminfo file is in kiloBytes so have to convert from kB to B
					return 1;
				}
			}
		}
		else if (strcmp(swtch, "-c") == 0) {
			fclose(meminfo);
			FILE *cacheinfo= fopen("/sys/devices/system/cpu/cpu0/cache/index2/size", "r");
			if (!cacheinfo) {
				printf("There was a problem finding your size file.\n");// handle error;
				return 1;
			}
			while (line[0] != EOF && fgets(line, sizeof(line), cacheinfo)) {
				int cache = 0;
				sscanf(line, "%dK", &cache);
				printf("%d Bytes\n", cache * 1024); //this file is in kilobytes as well
				return 1;
			}
			fclose(cacheinfo);//special case and file so close it before we go past and return same as below
			printf("The requested information was not found in your system.\n");
			return 1;
		}
		else {
			fclose(meminfo);
			printf("You provided an invalid switch. Please provide either -t, -u, -c, or -h if you need help.\n");
			return 1;
		}
		fclose(meminfo); //if we have made it this far then we did not find the right text in the file
		printf("The requested information was not found in your system.\n");
		return 1;
	}
}

int execute(char** args) {
	int status = 1;
	int pid = fork();
	if (pid < 0) {//error
		printf("There was an error while trying to fork.\n");
		exit(EXIT_FAILURE);
	}
	else if (pid == 0) {//child
		printf("This is the child executing your command, pid = %u \n", getpid());
		if (execvp(args[0], args) == -1) {
			perror("There was an issue executing your comand");
		}
		exit(EXIT_FAILURE);
	}
	else {//parent
		printf("This is the parent process waiting for the child, pid = %u \n", getppid());
		while (!WIFEXITED(status) && !WEXITSTATUS(status)) {
			waitpid(pid, &status, 0);
		}
	}
	return 1;
}

void splitCmd(Cwushell* shell) {
	int bufSize = BUFFER_SIZE, position = 0;
	char* arg;
	shell->args = malloc(bufSize * sizeof(char*));
	

	if (!shell->args) {
		printf("Allocation Error!\n");
		exit(EXIT_FAILURE);
	}

	arg = strtok(shell->command, DELIMITERS);
	while (arg) {
		shell->args[position] = arg;
		position++;

		if (position >= bufSize) {
			bufSize += BUFFER_SIZE;
			shell->args = realloc(shell->args, bufSize * sizeof(char*));
			if (!shell->args) {
				printf("Allocation Error!\n");
				exit(EXIT_FAILURE);
			}
		}

		arg = strtok(NULL, DELIMITERS);
	}
	shell->args[position] = NULL;
	shell->argsSize = position;
}

int analyzeCmd(Cwushell* shell) {
	if (!shell->args) {
		printf("No command was given. Pleasse give a command to run. Type 'manual' for an explanation of commands.");
		return 1;
	}
	else if (strcmp(shell->args[0], "exit") == 0) {
		if (shell->argsSize > 2) {
			printf("Too many arguments were provided. Expected 1 or 2 arguments: exit [exitCode] \nYou gave %d arguments\n", shell->argsSize);
			return 1;
		}
		return exitShell(shell->args[1]);
	}
	else if (strcmp(shell->args[0], "manual") == 0) {
		if (shell->argsSize > 1) {
			printf("Too many arguments were provided. Expected 1 argument: manual \nYou gave %d arguments\n", shell->argsSize);
			return 1;
		}
		char* arguments[] = { "cat","./manual.txt", NULL };
		return execute(arguments);
	}
	else if (strcmp(shell->args[0], "prompt") == 0) {
		if (shell->argsSize > 2) {
			printf("Too many arguments were provided. Expected 1 or 2 arguments: prompt [newPrompt] \nYou gave %d arguments\n", shell->argsSize);
			return 1;
		}
		return prompt(shell, shell->args[1]);
	}
	else if (strcmp(shell->args[0], "cpuinfo") == 0) {
		if (shell->argsSize > 2) {
			printf("Too many arguments were provided. Expected 2 arguments: cpuinfo -[c|t|n] \nYou gave %d arguments\n", shell->argsSize);
			return 1;
		}
		return cpuinfo(shell->args[1]);
	}
	else if (strcmp(shell->args[0], "meminfo") == 0) {
		if (shell->argsSize > 2) {
			printf("Too many arguments were provided. Expected 2 arguments: meminfo -[t|u|c] \nYou gave %d arguments\n", shell->argsSize);
			return 1;
		}
		return meminfo(shell->args[1]);
	}
	else {
		return execute(shell->args);
	}
}

int main() {
	int pid = 0, status = 1;
	Cwushell* shell = createShell();
	while (status) {
		printf("%s", shell->prompt);
		size_t bufSize = 0;
		getline(&shell->command, &bufSize, stdin);
		char *tmpCheck;
		memcpy(tmpCheck, shell->command, strlen(shell->command) + 1);
		if (!strtok(tmpCheck, DELIMITERS)) {
			printf("Please enter a command. For help type 'manual'\n");
			continue;
		}
		splitCmd(shell);
		status = analyzeCmd(shell);
	}
	deleteShell(shell);
	return EXIT_SUCCESS;
}
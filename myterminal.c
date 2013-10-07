#include<stdio.h>
#include<stdlib.h>
#include<sys/types.h>
#include<sys/ptrace.h>
#include<sys/stat.h>
#include<fcntl.h>
#include<string.h>
#include<wait.h>
#include<unistd.h>
#include<signal.h>
#include<sys/wait.h>
FILE *file_pointer;
char *check, *check2, *check3, *check4;
int pid_ = 0, sig_id_ = 0, pid_fg = 0;
int status,count,curr,pipefd1[2],pipefd2[2];
void signal_jobs(int,int);
FILE *proc_info;
void pinfo();
int pinfo_var = 9999;
void list_jobs();
int check_command_child();
void pid_now();
void my_overkill();
void child_handler(int);
void parse_proc_file();
void my_fg(int);
void check_redir();
char *home;
char parent_name[100];
char *user_name, *sys_name;
char *path;
char curr_path[100];
char line[100];
void ctrlz_handler();
void check_command_parent();
void check_bg();
void create_child();
void kill_zombi();
void start_shell();
char *input;
int pinfo_counter = 0;
typedef struct process{
	char *buffer[10];
	char *word[10][10];
	int count[10];
	int multicount;
	int index;
	int job_id;
	int id;
	int state;
	int back;
	int **pipeline;
}process;
process command[1000];

int main(int argc, char *argv[])
{
	// Signal Handling
	signal(SIGINT,SIG_IGN);
	signal(SIGTSTP,ctrlz_handler);
	signal(SIGQUIT,SIG_IGN);
	signal(SIGCHLD,child_handler);

	//Set env variables
	user_name = getenv ("USER");
	sys_name = getenv ("HOSTNAME");

	home = getcwd(home,100);
	curr = 0;
	// Assign parent name to a.out
	strcpy(parent_name,argv[0]);
	start_shell();
	return 0;
}

void start_shell()
{

	//Clearing input
	input = (char *)malloc(sizeof(char)*100);
	input[0]=0;

	//Relative path
	path = getcwd(path,100);
	strcpy(curr_path,"~");
	path = path + strlen(home);
	strcat (curr_path,path);
	path = path - strlen(home);

	//Print prompt shell
	printf("<%s@%s:%s>",user_name,sys_name,curr_path);

	//Scan the input
	scanf("%[^\n]", input );
	getchar();
	if(!strncmp(input, "pinfo", 5))
	{
		if(input[6] == 0)
		{
			pinfo_var = 9999;
		}
		else {
			check = strtok(input, " ");
			while(check !=NULL)
			{
				pinfo_counter++;
				//printf("check %s\n", check);
				check = strtok(NULL, " ");
				if(pinfo_counter == 1)
				{	

					//printf("check is %s",check);
					pinfo_var = atoi(check);
					pinfo_counter = 0;
					break;
				}
			}
		}
	}

	if(!strncmp(input , "kjob", 4))
	{
		int counter = 0;
		check2 = strtok(input, " ");
		while(check2 != NULL)
		{
			check2 = strtok(NULL, " ");
			if(counter == 0){
				if(check2)
					pid_ = atoi(check2);
				counter ++ ;
			}
			else if(counter == 1){
				if(check2)
					sig_id_ = atoi(check2);
				counter = 0;
			}		

		}
	}
	if(!strncmp(input, "fg", 2))
	{	
		int counter_fg = 0;
		check3 = strtok(input, " ");
		while(check3 != NULL)
		{
			check3 = strtok(NULL," ");
			if(counter_fg == 0)
			{
				if(check3)
					pid_fg = atoi(check3);
				counter_fg ++;
			}
		}
		counter_fg = 0;
	}

	if(!strcmp(input,"quit"))
	{
		kill_zombi();
		exit(0);
	}

	//Parsing the Input

	count=0;
	// count checks the length of the command
	command[curr].buffer[count] = strtok (input, " \t");
	while(command[curr].buffer[count++] != NULL)
		command[curr].buffer[count] = strtok (NULL, " \t");
	count = count - 1;
	if(count==0)
		start_shell();
	command[curr].multicount=0;
	// Check if the current (curr) process is background
	check_bg();
	// Check for redirection ops
	check_redir();
	// Execute process by creating child
	create_child();
	
	curr++;
	start_shell();
}
void ctrlz_handler()
{
	int i;
	for(i=0;i<curr;i++)
	{
		if(command[i].state == 1 && command[i].back == 0)
		{
			//printf("In handler\n");
			kill(command[i].id,SIGSTOP);
			command[i].back = 1;
			signal(SIGCHLD, child_handler);
			start_shell();
		}
	}
}
void child_handler(int signum)
{
	int i, id = waitpid(WAIT_ANY, &status, WNOHANG);
	for(i = 0 ; i <= curr ; i++)
	{
		if(command[i].id == id)
			break;
	}
	command[i].state = 1;
	if(command[i].back == 0 || status != 0)
	{
		if(status != 0)
			start_shell();
		fflush(stdout);
		signal(SIGCHLD, child_handler);
		return;
	}
	printf("\n");
	printf("%d exited normally\n",command[i].id);
	start_shell();
	fflush(stdout);
	signal(SIGCHLD, child_handler);
	return;
}

int check_command_child()
{
	if (!strcmp (command[curr].word[command[curr].index][0],"cd") || !strcmp (command[curr].word[command[curr].index][0],"pid") || !strncmp (command[curr].word[command[curr].index][0],"hist",4) ||!strncmp (command[curr].word[command[curr].index][0],"fg", 2) ||!strncmp (command[curr].word[command[curr].index][0],"kjob", 4) || !strcmp (command[curr].word[command[curr].index][0],"overkill") || !strcmp (command[curr].word[command[curr].index][0],"jobs") || !strcmp (command[curr].word[command[curr].index][0],"pinfo"))
		return 1;
	else
		return 0;

}

void pid_now()
{
	int i;
	for(i = 0 ; i < curr ; i++)
	{
		if(command[i].state == 0)
		{
			printf("command name: ");
			int j=0;
			while(command[i].buffer[j] != NULL)
			{
				printf("%s ",command[i].buffer[j]);
				j++;
			}
			printf("process id: %d\n",command[i].id);
		}
	}
}
int *process_arr;
int proc_count = 0;
void check_command_parent()
{
	char ch;
	if(!strcmp(command[curr].word[command[curr].index][0],"cd"))
	{
		path = getcwd (path,100);
		strcat (path,"/");
		strcat (path,command[curr].word[command[curr].index][1]);
		if (chdir(path) != 0)
			perror ("chdir()");	
	}
	else if(!strcmp (command[curr].word[command[curr].index][0],"pid"))
	{
		if(count == 1)
			printf("command name: %s process id: %d\n",parent_name,getpid());
		else if(!strcmp (command[curr].word[command[curr].index][1],"all"))
		{
			
			printf("command name: %s process id: %d\n",parent_name,getpid());
			pid_now();
		}
	}
	else if(!strcmp (command[curr].word[command[curr].index][0],"pinfo"))
	{
		pinfo();
	}
	else if(!strcmp (command[curr].word[command[curr].index][0],"overkill"))
	{
		my_overkill();
		fflush(stdout);
	}
	else if(!strcmp (command[curr].word[command[curr].index][0],"jobs"))
	{
		list_jobs();
	}
	else if(!strncmp (command[curr].word[command[curr].index][0],"kjob", 4))
	{
		signal_jobs(pid_,sig_id_);
	}
	else if(!strncmp (command[curr].word[command[curr].index][0],"fg", 2))
	{
		my_fg(pid_fg);

	}	
}
int proc_arr[100];
char path_file[100];
char state;
char memory;
char buff[64];
int tmp = 0;
int i,j;

void parse_proc_file(char* check)
{
	strcat(path_file,check);
	strcat(path_file,"/status");
	
	int fd = open(path_file, O_RDONLY);
	int lcount  = 0;
	char ch, f1[1];  
	
	while((ch = read(fd, &f1, 1)) != 0 )
	{  
		if(f1[0]=='\n')
			lcount++;
		if((lcount==1 || lcount==2 || lcount==12))
			i = write(1, f1, 1);
	}  
	
	f1[0]='\n';
	i = write(1, f1, 1);
	printf("Executable Path -- ~/a.out\n");
}

void pinfo()
{
	//Read into file proc_info
	char buff[1000];
	if(pinfo_var != 9999)
	{
		memset(&path_file,0, sizeof(path_file));
		strcpy(path_file,"/proc/");
		int i,j;
		for(i=0;i<curr;i++)
		{
			//printf("command id is %d\n",command[i].id);
			if(command[i].id == pinfo_var) 
			{
				parse_proc_file(check);
				//char temp[10];
				//sprintf(temp, "%d" ,command[i].id);

			}
		}
	}
	else 
	{
		int p,k;
		for(p = 0 ; p < curr ; p++)
		{

			//path_file = (char *)malloc(sizeof(char));
			//path_file[0]=0;
			memset(&path_file,0, sizeof(path_file));
			strcpy(path_file,"/proc/");
			check4 = (char *)malloc(sizeof(char));
			check4[0]=0;
			sprintf(check4, "%d", command[p].id);
			parse_proc_file(check4);
		}
	}
}

void signal_jobs(int pid, int sig_id)
{
	int i;
	for(i = 0; i< curr ;i++)
	{
		if(command[i].job_id == pid)
		{
			printf("pid is %d\n",command[i].id);
			kill(command[i].id, sig_id);
			command[i].state = 1;
		}
	}
	kill_zombi();
}

void my_fg(int pid)
{
	int i, id;
	for(i =0;i<curr;i++)
	{
		if(command[i].id == pid && command[i].back == 1)
		{
			command[i].back = 0;
			waitpid(command[i].id, NULL ,0);
		}
	}
}

void my_overkill()
{
	for(i=0;i<curr;i++)
	{
		if(command[i].back == 1)
		{
			kill(command[i].id,9);
			fflush(stdout);
			signal(SIGCHLD,child_handler);
			//kill(command[i].id,SIGCONT);
		}
	}
}
	
void list_jobs()
{
	int i, k = 1;
	for(i=0;i<curr;i++)
	{
		if(command[i].back == 1 && command[i].state == 0)
		{
			printf("[%d] %s [%d]\n",(k),command[i].word[command[i].index][0],command[i].id);	
			command[i].job_id = k;
			printf("Job state is %d\n",command[i].state);
			k++;
		}
	}
}

void check_bg()
{
	command[curr].state=0;
	if (!strcmp(command[curr].buffer[count-1],"&"))
	{
		command[curr].buffer[count-1] = NULL;
		count--;
		command[curr].back = 1;
	}
	else
		command[curr].back = 0;
}

void exec_child()
{
	//printf("%s\n",command[curr].word[command[curr].index][0]);
	int ret = execvp(*command[curr].word[command[curr].index],command[curr].word[command[curr].index]);
	//Check if execvp returns an error
	if(ret == -1)
	{
		if(check_command_child() == 0)
		{
			if(command[curr].back == 1)
			{
				printf("\n");
				perror ("Unknown command");
			}
			else
				perror ("Unknown command");

		}
		_exit(1);
	}
	_exit(1);
}

void create_child()
{
	pid_t pid;
	int i,j,k,l,n;
	//printf("multicount for process is %d\n", command[curr].multicount);
	for(i = 0; i < command[curr].multicount ; i++)
	{
		pipe(pipefd1);
		pipe(pipefd2);
		command[curr].index=i;
		j = -1; // Counter for redirection of standard input 
		k = -1; //Counter for redirection of standard output
		int status;
		if(i == 0)
		{
			j = command[curr].count[i]-1;
			//printf("%d is j\n",j);
			//printf(" %s\n",command[curr].word[i][j]);
			//check the argument for the command , by pass the redirection
			for( j = command[curr].count[i]-1 ; j >=0 ; j--)
			{
				if(strcmp(command[curr].word[i][j],"<") == 0)
					break;
			}
		}
		if(i == (command[curr].multicount-1))
		{
			k = command[curr].count[i]-1;
			while(k >= 0)
			{
				if(strcmp(command[curr].word[i][k],">") == 0)
					break;
				k--;
			}

		}
		pid = fork();
		// Success pid == 0
		if(pid == 0)	
		{
			if(i != 0)
			{
				close(command[curr].pipeline[i-1][1]);
				dup2(command[curr].pipeline[i-1][0], STDIN_FILENO);
				close(command[curr].pipeline[i][0]);
			}
			else
			{
				//Making input redirection in pipefd
				if(j >= 0)
				{
					file_pointer = fopen(command[curr].word[i][j+1],"r");
					while(fgets(line,100,file_pointer) != NULL)
					{
						n = strlen(line);
						if(write(pipefd1[1],line,n)!=n)
							perror("write error to pipe");
					}
					close(pipefd1[1]);
					fclose(file_pointer);
					dup2(pipefd1[0],STDIN_FILENO);
				}
				else
					close(command[curr].pipeline[i][0]);

			}
			// If the process is the first process 
			if(i != (command[curr].multicount-1))
				dup2(command[curr].pipeline[i][1],STDOUT_FILENO);
			else
			{
				if(k >= 0)
				{
					close(pipefd2[0]);
					dup2(pipefd2[1],STDOUT_FILENO);
				}
			}
			l=0;
			while(l < command[curr].count[i])
			{
				if((strcmp(command[curr].word[i][l],"<") == 0) || (strcmp(command[curr].word[i][l],">") == 0))
					command[curr].word[i][l] = NULL;

				l++;
			}
			exec_child();
		}
		else
		{
			if(j >= 0)
			{
				close(pipefd1[0]);
				close(pipefd1[1]);
			}
			command[curr].id = pid;
			if(command[curr].back == 0)
			{
				wait(&status);
				if(k >= 0)
				{
					close(pipefd2[1]);
					file_pointer=fopen(command[curr].word[i][k+1],"w");
					while( read(pipefd2[0],line,1)==1 )
						fputs(line,file_pointer);
					fclose(file_pointer);
				}
				close(command[curr].pipeline[i][1]);
				if(i !=0)
				{
					close(command[curr].pipeline[i-1][0]);
				}
				command[curr].state=1;
			}
			check_command_parent();
		}
	}
}


void kill_zombi()
{
	int i;
	for( i = 0 ; i < curr ; i++)
	{
		if(command[i].state == 0)
		{
			command[i].back=0;
			kill(command[i].id,SIGTERM);
		}
	}
}


void check_redir()
{
	int i = 0,j = 0;
	command[curr].multicount = 0;
	//Check if count is not 0 ; count was the length of the command
	for(i = 0; i<count ;i++)
	{
		//Check the last char of scanned command
		if(strcmp(command[curr].buffer[i],"|") == 0)
		{
			command[curr].word[command[curr].multicount][j]=(char*)malloc(sizeof(char)*100);
			command[curr].word[command[curr].multicount][j]=NULL;
			command[curr].count[command[curr].multicount]=j;
			j=0;
			command[curr].multicount++;
		}
		else
		{
			command[curr].word[command[curr].multicount][j]=(char *)malloc(sizeof(char) * 100 );
			strcpy(command[curr].word[command[curr].multicount][j],command[curr].buffer[i]);
			j++;
		}
	}
	command[curr].word[command[curr].multicount][j]=NULL;
	command[curr].count[command[curr].multicount]=j;
	command[curr].multicount++;

	//Give a pipe to each command 
	command[curr].pipeline = (int **)malloc(sizeof(int *)*(command[curr].multicount));
	for(i = 0 ; i < command[curr].multicount ; i++)
	{
		command[curr].pipeline[i]=(int*)malloc(sizeof(int)*2);
		if(pipe(command[curr].pipeline[i]) < 0 )
			perror("Pipe Error");
	}
}





#include<fcntl.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include<signal.h>
#include<sys/wait.h>
#define MAX_CMD_ARG 10
#define BUFSIZ 256

const char *prompt = "myshell> ";
char* cmdvector[MAX_CMD_ARG];
char  cmdline[BUFSIZ];
int backgroundCheck=0;

void fatal(char *str){
	perror(str);
	exit(1);
}

int makelist(char *s, const char *delimiters, char** list, int MAX_LIST){	
  int i = 0;
  int numtokens = 0;
  char *snew = NULL;

  if( (s==NULL) || (delimiters==NULL) ) return -1;

  snew = s + strspn(s, delimiters);	/* Skip delimiters */
  if( (list[numtokens]=strtok(snew, delimiters)) == NULL )
    return numtokens;
	
  numtokens = 1;
  
  while(1){
     if( (list[numtokens]=strtok(NULL, delimiters)) == NULL)
	break;
     if(numtokens == (MAX_LIST-1)) return -1;
     numtokens++;
  }
 
  return numtokens;
}


void cmd_cd(int argc, char **argv){
    char *path;
    
    if(argc==1){
         //cd만 입력되고 디렉토리는 입력 안되었을 때 
          path=(char*)getenv("HOME");
    }
    else{
        //path값에 디렉토리 이름을 대입한다.
        path=argv[1];
    }

   if(chdir(path)){
       //chdir 함수가 해당 path가 존재하지 않으면 0을 리턴함.
       printf("No such directory!\n");
   }

}


int cmd_redir_out(char **cmdvector){
    
    int fd;
    int pos;
    for(pos=0;cmdvector[pos]!=NULL;pos++){
        if(!strcmp(cmdvector[pos],">")){
            //">"기호를 찾으면 해당 위치에서 멈추고, 위치를 저장해둔다.
            break;
        }
    }

    if(cmdvector[pos]){
        if(!cmdvector[pos+1]){
            //파일 이름 입력하지 않았을 시 오류 메세지 출력
            perror("Enter file name!");
            return -1;
        }
        else{
            if((fd=open(cmdvector[pos+1],O_RDWR|O_CREAT|O_TRUNC,0644))==-1){
                //파일 오픈해주고, 오류 발생시 오류 메세지 출력
                perror("open error!");
                return -1;
            }
        
        //표준 출력 닫아주고, 출력 방향을 파일로 바꿔준 다음에 파일 디스크립터도 닫아준다.
        close(STDOUT_FILENO);
        dup2(fd,STDOUT_FILENO);
        close(fd);
    
    //필요없는 커맨드는 제거한다.
      for (pos=pos;cmdvector[pos]!=NULL;pos++){
           cmdvector[pos]=cmdvector[pos+2];
        }
        cmdvector[pos]=NULL;
        }
    }
    return 0;
  
}

int cmd_redir_in(char ** cmdvector){
    int fd;
    int pos;
    for(pos=0;cmdvector[pos]!=NULL;pos++){
        //">"기호를 찾으면 해당 위치에서 멈추고, 위치를 저장해둔다.
        if(!strcmp(cmdvector[pos],"<")){
            break;
        }
    }

    if(cmdvector[pos]){
        if(!cmdvector[pos+1]){
             //파일 이름 입력하지 않았을 시 오류 메세지 출력
            perror("Enter file name!");
            return -1;
        }
        else{
            if((fd=open(cmdvector[pos+1],O_RDONLY,0644))==-1){
                 //파일 오픈해주고, 오류 발생시 오류 메세지 출력
                perror("open error!");
                return -1;}

          //표준 입력을 닫아주고, 입력 방향을 파일로 바꿔준 다음에 파일 디스크립터도 닫아준다.   
        close(STDIN_FILENO);
        dup2(fd,STDIN_FILENO);
        close(fd);
    
    //필요 없는 커맨드는 제거한다.
      for (pos=pos;cmdvector[pos]!=NULL;pos++){
           cmdvector[pos]=cmdvector[pos+2];
        }
        cmdvector[pos]=NULL;
        }}
    return 0;
  
}


 char* pipevec1[10];
 char* pipevec2[10];
 char* pipevec3[10];

int cmd_pipe_check(){
    int i;
    int pipenum=0;
    int j=0;
     int c1,c2,c3=0;

    for(i=0;cmdvector[i]!=NULL;i++){
       
      if(!strcmp(cmdvector[i],"|")){
          //|를 기준으로 파이프 커맨드를 잘라줍니다.
         j=0; 
         pipenum++;  
      }
      // |를 기준으로 각 명령어들을 각각의 벡터에 따로 담아줍니다.
      else if(pipenum==0){
          pipevec1[j]=cmdvector[i];
          j++;
          c1++;
      }
      else if(pipenum==1){
          pipevec2[j]=cmdvector[i];
          j++;
          c2++;
      }
      else if(pipenum==2){
          pipevec3[j]=cmdvector[i];
          j++;
          c3++;
      }
    }
    //execvp함수에서 EOF를 전달하기 위해 맨 끝부분을 NULL로 지정합니다.
    pipevec1[c1]=NULL;
    pipevec2[c2]=NULL;
    pipevec3[c3]=NULL;
    return pipenum;
}

int cmd_pipe_proc2(char* vec1[],char* vec2[]){
     int fd[2];
    if(pipe(fd)==-1){
        //하나의 파이프가 pipe함수를 통해 생성. 
        // 각 파일 디스크립터가  파이프의 읽기 끝/쓰기 끝 부분에 할당된다.
        perror("pipe failed!");
        exit(1);
    }
     
    switch(fork()){
    case -1 : perror("fork error"); break;
    case 0 :
     //|을 기준으로 자식 프로세스에선 앞쪽 명령 프로세스 생성
     cmd_redir_in(vec1);
     cmd_redir_out(vec1);
     close(STDOUT_FILENO);
     dup2(fd[1],STDOUT_FILENO); //표준출력을 파이프의 입구 부분과 연결
    if(close(fd[1])==-1 || close(fd[0])==-1) perror("close error!");//파일 디스크립터
     execvp(vec1[0], vec1);
     perror("exec error!");
     exit(0);
   default:
      //리다이렉션이 있는지를 확인.
     cmd_redir_in(vec2);
     cmd_redir_out(vec2);
     close(STDIN_FILENO);
     dup2(fd[0],STDIN_FILENO);//표준입력을 파이프의 출구 끝과 연결
     if(close(fd[1])==-1||close(fd[0])==-1) perror("close error!");
     execvp(vec2[0], vec2);
     perror("exec error!");
     exit(0);
   }
    return 0; 

}

int cmd_pipe_proc3(char* vec1[],char* vec2[],char* vec3[]){
    int fd[2],fd2[2];
    if(pipe(fd)==-1){
        perror("pipe failed!");
        exit(1);
    }
    //파이프를 하나 더 생성해준다.
   
    switch(fork()){
        case 0:
         if(pipe(fd2)==-1){
         perror("pipe failed!");
         exit(1);
         }
          switch (fork()){
            case 0:
            //리다이렉션이 있는지를 확인.
            cmd_redir_in(vec1);
            cmd_redir_out(vec1);
            //표준 출력을 2번 파이프의 입구로 연결한다.
            dup2(fd2[1],STDOUT_FILENO);
            //파이프 두개가 복제되었으므로 모두 닫아준다.
            if( close(fd[0])==-1 || close(fd[1])==-1 || close(fd2[0])==-1 || close(fd2[1])==-1) 
               perror("close error!");
            execvp(vec1[0],vec1);
            exit(0);
    
            default:
            cmd_redir_in(vec2);
            cmd_redir_out(vec2);
            close(STDOUT_FILENO);
            //표준 입력을 2번 파이프의 출구에 연결한다. 
            dup2(fd2[0],STDIN_FILENO);
            //표준 출력을 1번 파이프의 입구에 연결한다.
            dup2(fd[1],STDOUT_FILENO);
            if( close(fd[0])==-1 || close(fd[1])==-1 || close(fd2[0])==-1 || close(fd2[1])==-1) 
               perror("close error!");
            execvp(vec2[0],vec2);
            wait(NULL);
        }
        exit(0);

        default:
        //표준 입력을 1번 파이프의 입구에 연결한다.
          cmd_redir_in(vec3);
          cmd_redir_out(vec3);
          close(STDIN_FILENO);
          dup2(fd[0],STDIN_FILENO);
          if( close(fd[0])==-1 || close(fd[1])==-1) perror("close error!");
          execvp(vec3[0],vec3);
          wait(NULL);
    }
}




void child_terminate(int sig){
    pid_t p_id;
    int status;
    p_id=waitpid(-1,&status,WNOHANG);
}

int sig=0;

void sigint_handler(){
   signal(SIGINT,SIG_IGN);
   sig=1;
   return;
}

void sigquit_handler(){
    signal(SIGQUIT,SIG_IGN);
    sig=1;
    return;
}


// SIGCHLD signal handler function
static void child_handler(int sig){ 
	int status;
	pid_t pid;
	while((pid = waitpid(-1, &status, WNOHANG)) > 0){}
}

// Function that prevents termination
void handler_func(int sig){
	printf("\n");
	return;
}


int main(int argc, char**argv){
  pid_t pid;  
  int i=0;
  
  struct sigaction act;
  
  act.sa_handler=child_terminate;
  sigemptyset(&act.sa_mask);
  act.sa_flags=0;
  sigaction(SIGCHLD,&act,0);
  
  while (1) {
    signal(SIGINT,(void*)sigint_handler);
    signal(SIGQUIT,(void*)sigquit_handler);
    
      //프롬프트 출력
  	fputs(prompt, stdout);
      //cmdline으로부터 문자열 가져옴
	fgets(cmdline, BUFSIZ, stdin);

    if(sig==1){
        sig=0;
        continue;
    }

	cmdline[strlen(cmdline) -1] = '\0';
     //delim 공백을 기준으로 단어를 잘라낸다.
	
    int cmdNum= makelist(cmdline, " \t", cmdvector, MAX_CMD_ARG);


     if(strcmp(cmdvector[cmdNum-1],"&")==0){
            //vector의 마지막 문자열이 &라면, 백그라운드 확인 변수를 1로 세팅한다.
            backgroundCheck=1;
            cmdvector[cmdNum-1]=NULL;
        }

     int pipeNum=cmd_pipe_check(); 

    if(strcmp(cmdvector[0],"cd")==0){
         cmd_cd(cmdNum,cmdvector);
        }
    else if (strcmp(cmdvector[0],"exit")==0){
           exit(1);
        } 
     else{
	   switch(pid=fork()){
	   case 0:
           //프롬프트로 입력받은 첫번째 단어를 실행 
        if(backgroundCheck!=1){    
          signal(SIGQUIT,SIG_DFL);
          signal(SIGINT,SIG_DFL);}
        else{
            signal(SIGINT,SIG_IGN);
            signal(SIGQUIT,SIG_IGN);}

          if(pipeNum==1){
           cmd_pipe_proc2(pipevec1,pipevec2);
           continue; }
          if(pipeNum==2){  
          cmd_pipe_proc3(pipevec1,pipevec2,pipevec3);
          continue; }

          cmd_redir_in(cmdvector);
          cmd_redir_out(cmdvector); 
          execvp(cmdvector[0], cmdvector);
          fatal("main()");
          return 0;
	    case -1:
  		  fatal("main()");
       default:
       //backgroundCheck 변수가 1로 세팅되어있다면 부모 프로세스는
        //자식 프로세스가 끝나는 것을 기다리지 않고 바로 다음 프롬프트 입력받음
          if(backgroundCheck==1){
              backgroundCheck=0;
              continue;
          }
          else{
              //background 변수가 세팅되지 않았다면 자식 프로세스가 끝나길 기다린다.
		    wait(NULL);
          }
     }
    } 
  }
  return 0;

}
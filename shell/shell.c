#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <syslog.h>
#include <sys/resource.h>
#include <sys/types.h> 
#include <dirent.h>
#include <sys/stat.h>


#define FILE_SIZE 1024
#define BUF_SIZE 256
#define STR_ARR 50

#define STDIN 0
#define STDOUT 1
#define STDERR 2

// 2
void daemonize(char* cmd)
{
   int i, fd0, fd1, fd2;
   pid_t pid;
   struct rlimit rl;
   struct sigaction sa;
   umask(0); /* clear out file creation mask */
   if (getrlimit(RLIMIT_NOFILE, &rl) < 0)
      perror("getrlimit");
   if ((pid = fork()) < 0)
      exit(1);
   else if (pid != 0)
      exit(0); /* The parent process exits */

   setsid(); /* become session leader */
   sa.sa_handler = SIG_IGN;
   sigemptyset(&sa.sa_mask);
   sa.sa_flags = 0;
   if (sigaction(SIGHUP, &sa, NULL) < 0)
      perror("sigaction: can't ignore SIGHUP");
   if ((pid = fork()) < 0)
      exit(1);
   else if (pid != 0)
      exit(0); /* The parent process exits */

   chdir("/"); /* change the working dir */
   if (rl.rlim_max == RLIM_INFINITY)
      rl.rlim_max = 1024;
   for (i = 0; i < rl.rlim_max; i++)
      close(i);
   fd0 = open("/dev/null", O_RDWR);
   fd1 = dup(0); fd2 = dup(0);
   openlog(cmd, LOG_CONS, LOG_DAEMON);
   if (fd0 != 0 || fd1 != 1 || fd2 != 2) {
      syslog(LOG_ERR, "unexpected file descriptors %d %d %d", fd0, fd1, fd2);
   }
}

// 3
void signal_handler(int singno){ 
   switch (singno){
      case SIGINT:
         printf("Received SIGINIT (Ctrl-C)\n");
         exit(0);
      case SIGTSTP:
         printf("Received SIGTSTP (Ctrl-Z)\n");
         
         kill(getpid(), SIGTSTP);

         printf("Process moved to background.\n");
      }
   }



// 4
void parse_redirect(char* argv[]) {
    int cmdlen = strlen(argv[0]);
    int fd;

    if(strcmp("<", argv[1]) == 0) {
        if((fd = open(argv[2], O_RDONLY | O_CREAT, 0644)) < 0) {
            printf("file failure.\n");
            return;
        }
        dup2(fd, STDIN_FILENO);
        close(fd);
        argv[1] = NULL;
    } else if(strcmp(">", argv[1]) == 0) {
        if((fd = open(argv[2], O_WRONLY | O_CREAT | O_TRUNC, 0644)) < 0) {
            printf("file failure.\n");
            return;
        }
        dup2(fd, STDOUT_FILENO);
        close(fd);
        argv[1] = NULL;
    }
}


// 5.ls
void ls() {
   DIR* pdir;
   struct dirent* pde;
   char buf[256];
   int i = 0;
   if (getcwd(buf, 256) == NULL) {
      printf("Directory not found");
      exit(1);
   }

   pdir = opendir(buf);

   while ((pde = readdir(pdir)) != NULL) {

      if (strcmp(pde->d_name, ".") == 0 || strcmp(pde->d_name, "..") == 0) {
         continue;
      }

      printf("%10s ", pde->d_name);
      if (++i % 5 == 0)
         printf("\n");

   }
   printf("\n");
   closedir(pdir);
}

// 5.mkdir
void new_dir(char* argv[]) {
    char str[512];
    char buf[256];
    
    if (getcwd(buf, 256) == NULL) {
        printf("Directory not found");
        exit(1);
    }
    
    sprintf(str, "%s/%s", buf, argv[1]);
    printf("%s", str);
    
    int success = mkdir(str, 0777);
    
    if (success == -1)
        printf(" [Directory creation failed]\n");
    else if (success == 0) {
        printf(" [Directory creation successful]\n");
        ls();
    }
}

// 5.rmdir
void remove_dir(char* argv[]) {
    int success;
    
    success = rmdir(argv[1]);
    
    if (success == 0) {
        printf("%s delete \n", argv[1]);
    }
    else
        printf("error\n");

   ls();
}

// 5.pwd
void print_current_directory() {
    char buf[256];
    
    if (getcwd(buf, sizeof(buf)) == NULL) {
        perror("error\n");
    } else {
        printf("현재 작업 디렉터리: %s\n", buf);
    }
}

// 5.rm
void remove_file(char* path) {
    int success = remove(path);

    if (success == 0) {
        printf("%s가 삭제되었습니다.\n", path);
    } else {
        perror("error\n");
    }
}

// 5.mv
void mv(int argc, char* argv[]){
   struct stat buf; 
   char *target; 
   char *src_file_name_only; 
   if (argc <3) { 
      fprintf(stderr, "Usage: file_rename src target\n"); 
      exit(1); 
   } 
   // Check argv[1] (src) whether it has directory info or not. 
   if (access(argv[1], 0) <0) { 
      fprintf(stderr, "%s not exists\n", argv[1]); 
      exit(1); 
   } 
   else { 
      char *slash = strrchr(argv[1], '/'); 
      src_file_name_only = argv[1]; 
      if (slash !=NULL) { // argv[1] has directory info. 
         src_file_name_only = slash +1; 
      } 
   }
    
   // Make target into a file name if it is a directory 
   target = (char *)calloc(strlen(argv[2])+1, sizeof(char)); 
   strcpy(target, argv[2]); 
   if (access(argv[2], 0) ==0) { 
      if (lstat(argv[2], &buf) <0) { 
         perror("lstat"); 
         exit(1); 
      } 
      else { 
         if (S_ISDIR(buf.st_mode)) { 
            free(target); 
            target = (char *) calloc(strlen(argv[1]) + strlen(argv[2]) +2, sizeof(char)); 
            strcpy(target, argv[2]); 
            strcat(target, "/"); 
            strcat(target, src_file_name_only); 
         } 
      }
   }
   printf("source = %s\n", argv[1]); 
   printf("target = %s\n", target); 
   if (rename(argv[1], target) <0) { 
      perror("rename"); 
      exit(1); 
   } 
   free(target);
}

// 5.cd
void cd(int argc, char* argv[]) {
   int success;
   char buf[256];

   if (argc == 1) {
      success = chdir(getenv("HOME"));
      if (success != 0)
         printf("change failed\n");

      else if (success == 0) {
         getcwd(buf, 256);
         printf("변경된 디렉토리 : %s\n", buf);
      }
   }

   else if (argc == 2) {
      success = chdir(argv[1]);

      if (success != 0)
         printf("change failed\n");

      else if (success == 0) {
         getcwd(buf, 256);
         printf("변경된 디렉토리 : %s\n", buf);
      }
   }

   else
      printf("cd : %s", argv[1]);
}

// 5.ln
void ln(int argc, char* argv[]){
   char cmd; 
   char *src; 
   char *target; 

   if (argc < 3) { 
      fprintf(stderr, "Usage: file_link [l,u,s] ...\n"); 
      fprintf(stderr, " file_link l[ink] src target\n"); 
      fprintf(stderr, " file_link u[nlink] filename\n"); 
      fprintf(stderr, " file_link s[ymlink] src target\n"); 
      exit(1); 
   } 

   cmd = (char) *argv[1]; 

   if ( cmd == 'l' ) {
    
      if (argc < 4) { 
         fprintf(stderr, "file_link l src target [link]\n"); 
         exit(1); 
      }
       
      src = argv[2]; 
      target = argv[3]; 

      if (link(src, target) < 0) { 
         perror("link"); 
         exit(1); 
      } 
   } 

   else if (cmd == 's') { 

      if (argc < 4) { 
         fprintf(stderr, "file_link l src target [link]\n"); 
         exit(1); 
      } 

      src = argv[2]; 
      target = argv[3];

      if (symlink(src, target) < 0) { 
         perror("symlink"); 
         exit(1); 
      } 
   } 

   else if (cmd == 'u') { 
      src = argv[2]; 

      if (unlink(src) < 0) { 
         perror("unlink"); 
         exit(1); 
      } 
   } 

   else { 
      fprintf(stderr, "Unknown command...\n"); 
   }
}

// 5.cp
void cp(int argc, char* argv[]){
   FILE *src; /* source file */
   FILE *dst; /* destination file */

   char ch;
   int count = 0;

   if (argc < 3) {
      printf("Usage: file_copy source_filedestination_file\n");
      exit(1);
   }   

   if ( (src = fopen(argv[1], "r")) == NULL ) {
      perror("fopen: src");
      exit(1);
   }

   if ( (dst = fopen(argv[2], "w")) == NULL ) {
      perror("fopen: dst");
      exit(1);
   }

   while ( !feof(src) ) {
      ch = (char) fgetc(src);

      if ( ch != EOF )
         fputc((int)ch, dst);
      count++;
   }

   fclose(src);
   fclose(dst);
}

// 5.cat
void cat(int argc, char* argv[]) {
   int fd;
   long size;
   unsigned char buf[BUF_SIZE];

   if (argc < 2 || argc >2) {
      printf(" Doesn't follow the rules \n");
      exit(1);
   }

   if (argc == 2) {

      if ((fd = open(argv[1], O_RDONLY)) < 0) {
         printf(" File does not exist \n");
         exit(1);
      }

      while (size = read(fd, buf, BUF_SIZE)) {
         write(1, buf, size);
      }
   }
}

int main()
{
   char buf[256];
   char* argv[STR_ARR] = { NULL, };
   char str[STR_ARR];
   pid_t pid;
   int argc;

   while (1) {

      argc = 0;

      printf("\n[ My Shell Program ]\n\n");
      printf("Shell> ");
      fgets(str, sizeof(str), stdin);
      str[strlen(str) - 1] = '\0';

      char* temp = strtok(str, " ");

      while (temp != NULL)
      {
         argv[argc] = temp;
         temp = strtok(NULL, " ");
         argc++;
      }

	
      // 1
      if (strcmp("exit", argv[0]) == 0)
      {
         printf("\nprogram exit\n\n");
         exit(1);
      }
      
      // 5.ls
      if (strcmp("ls", argv[0]) == 0) {
         ls();
      }
      
      // 5.mkdir
      if(strcmp("mkdir", argv[0]) == 0){
         new_dir(argv);
      }
      
      // 5.rmdir
      if(strcmp("rmdir", argv[0]) == 0){
         remove_dir(argv);
      }
      
      
      // 4
        parse_redirect(argv);

        pid = fork();
        if (pid < 0) {
            perror("fork failed");
            //exit(1);
        } else if (pid == 0) {
            execvp(argv[0], argv);
            perror("execvp failed");
            //exit(1);
        }
      
      // 5.pwd 그냥 pwd 입력
            if(strcmp("pwd", argv[0]) == 0) {
                 print_current_directory();
            }
            
            // 5.rm //rm (지울 파일 이름)
            if(strcmp("rm", argv[0]) == 0) {
              if (argc < 2) {
                     printf("Usage: rm Mfile_or directory>\n");
                  } 
                  else {
                   remove_file(argv[1]);
                  }
            }
            
      // 5.mv(argc,argv);
      if(strcmp("mv", argv[0]) == 0){
         mv(argc,argv);
      }
      
      // 5.cd(argc, argv);
      if(strcmp("cd", argv[0]) == 0){
         cd(argc, argv);
      }
      
      // 5.ln(argc,argv);
      if(strcmp("ln", argv[0]) == 0){
         ln(argc, argv);
      }
      
      // 5.cp(argc,argv);
      if(strcmp("cp", argv[0]) == 0){
         cp(argc, argv);
      }
      
      // 5.cat(argc,argv);
      if(strcmp("cat", argv[0]) == 0){
         cat(argc, argv);
      }

   }
}

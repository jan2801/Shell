#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <signal.h>

#define STRSIZE 64
#define N 2000
#define READSTR_BUFSIZE 1024

int flag;
int flag_dev;
int bufsize=16;
int redir_out=0;
int redir_in=0;
int fd[2][2];
int red1, red2;
int maxu;
char *redirect_file=NULL;
int fp;


char *copystr(char *tmp);
char *readstr(void);
void redirection(int u);

void waiter(void)
{
    int i;
    for (i=0; i<N; i++)
        waitpid(-1, 0, WNOHANG);
}

void handler(int s)
{
    wait(NULL);
    exit(0);
}

void one_step(char ***s_m, int u)
{
    int l;
    pid_t pid;

    if ((pid=fork())==-1)
    {
        perror("impossible to create child process\n");
        exit(1);
    }
    if (pid==0)
    {
        redirection(u);                         
        int i=0;
        while (s_m[u][i]!=NULL)
        {
            printf("%s\n", s_m[u][i]);
            i++;
        }  
        execvp(s_m[u][0], s_m[u]);
        perror("exec error\n");   
        exit(1);
    }
    else
    {
        if (u)
        {
            close(fd[0][0]);
            close(fd[0][1]);
        }
        fd[0][0]=fd[1][0];
        fd[0][1]=fd[1][1];
    } 
}


int output_red(int redir_flag, int i, char *str)                                 //synchronize with canal work
{
    char *redirect_file=NULL;
    char *s;
    int j=0;
    if (redir_flag)
    {
        while ((str[i]!='&')&&(str[i]!='\0')&&(str[i]!=' '))
        {
            if (j==0)
            {
                s=calloc(bufsize, sizeof(char));
            }
            if (j>=bufsize)
            {
                bufsize+=4;
                s=realloc(s, bufsize);
            }
            s[j]=str[i];
            i++;
            j++;
        }
        redirect_file=copystr(s);
        if (s)
        {
            free(s);
        } 
    }
    if (redir_flag==1)
    {
        red2=open(redirect_file, O_CREAT|O_TRUNC|O_WRONLY, 0660);
        if (red2==-1)
        {
            perror("can't open redirect file\n");
            exit(1);
        }

    }
    if (redir_flag==2)
    {

        red2=open(redirect_file, O_CREAT|O_WRONLY|O_APPEND, 0660);
        if (red2==-1)
        {
            perror("can't open redirect file\n");
            exit(1);
        }
    }
    return i;
}

int input_red(int input_flag, int i, char *str)                  //synchronize with canal work
{
    int j=0;
    char *s;
    if (input_flag)
    {

        while ((str[i]!='&')&&((str[i])!='\0')&&(str[i]!=' ')&&(str[i]!='>'))
        {
            if (j==0)
            {
                s=calloc(bufsize, sizeof(char));
            }
            if (j>=bufsize)
            {
                bufsize+=4;
                s=realloc(s, bufsize);
            }
            s[j]=str[i];
            i++;
            j++;
        }
        redirect_file=copystr(s);
        if (s)
        {
            free(s);
        } 
        red1=open(redirect_file, O_RDONLY);
        if (red1==-1)
        {
            perror("can't open redirect file\n");
            exit(1);
        }

    }
    return i;
}


void redirection(int u)
{
    if (u!=maxu)
    {
        dup2(fd[1][1], 1);
        close(fd[1][1]);
        close(fd[1][0]);
    }
    else
    {
        if (redir_out)
        {
            dup2(red2, 1);
            close(red2);
        }
    }

    if (u!=0)
    {
        dup2(fd[0][0], 0);
        close(fd[0][0]);
        close(fd[0][1]);
    }
    if (u==0)
    {
        if (flag)
            dup2(fp, 0);
    }
    else
    {
        if (redir_in)
        {
            dup2(red1, 0);
            close(red1);
        }
        
    }
}




char ***parsing(char *str)
{   
    int flag=0;
    int i=0;
    int j=0;
    int k=0;
    int u=0;
    char *s;
    char ***s_m = calloc(15, sizeof(char**));
    i=0;
    s=calloc(15, sizeof(char));
    s_m[u]=calloc(15, sizeof(char *));
    
    u=0;
    k=0;
    while (str[i])
    {
        if ((str[i]=='|')||(str[i]=='>')||(str[i]=='<')||(str[i]=='&')||(str[i]==' '))
        {
            if (s && *s)
            {
                s_m[u][k]=copystr(s);
                k++;
                if (s)
                {
                    free(s);
                    j=0;
                }
                s=NULL;
                if (str[i]=='<')
                {
                    flag=1;
                    maxu=u;
                    redir_in++;
                    i++;
                    if (redir_in)
                    {
                        while (str[i]==' ')
                        {
                            i++;
                        }
                    }  
                    i=input_red(redir_in, i, str);
                }
                if (str[i]=='>')
                {
                    flag=1;
                    maxu=u;
                    printf("this is problem\n");
                    if (str[i+1]=='>')
                    {
                        redir_out++;
                        i++;
                    }
                    redir_out++;
                    i++;
                    if (redir_out)
                    {
                        while (str[i]==' ')
                        {
                            i++;
                        }
                    } 
                    i=output_red(redir_out, i, str);

                }
                if (str[i]=='&')
                {
                    flag_dev=1;
                    /*&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&& mode*/
                }
            }
            else
            {
                if (str[i]=='<')
                {
                    flag_dev=1;
                    maxu=u;
                    redir_in++;
                    i++;
                    if (redir_in)
                    {
                        while (str[i]==' ')
                        {
                            i++;
                        }
                    }  
                    i=input_red(redir_in, i, str);
                }
                if (str[i]=='>')
                {
                    flag=1;
                    maxu=u;
                    printf("this is problem\n");
                    if (str[i+1]=='>')
                    {
                        redir_out++;
                        i++;
                    }
                    redir_out++;
                    i++;
                    if (redir_out)
                    {
                        while (str[i]==' ')
                        {
                            i++;
                        }
                    } 
                    i=output_red(redir_out, i, str);

                }
                if (str[i]=='&')
                {
                    flag_dev=1;
                    /*&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&& mode*/
                }
            }
        
            if (str[i]!=' ')
            {

                u++;
                s_m[u]=calloc(15, sizeof(char *));
                k=0;
            }
        }
        else
        {
            if (!s)
            {
                s=calloc(STRSIZE, sizeof(char));
            }
            s[j]=str[i];
            j++; 
        }
        if (str[i]!='\0')
        {
            i++;        
        }
    }
    if (s) 
    {
        s_m[u][k]=copystr(s);    
    }
    if (!flag)
        maxu=u;
    return s_m; 
}  


char *copystr(char *tmp)
{
    int i=0;
    int j=4;
    char *str=calloc(j, sizeof(char));
    while (*(tmp + i) != '\0')
        {
            if (i + 1 == j)
            {
                j *= 2;
                str = realloc(str, j*sizeof(char));
            }
            *(str + i) = *(tmp + i);
            i++;
        }
        *(str+i)='\0';
    return str;
}



char *readstr(void)
{
    int bufsize=READSTR_BUFSIZE;
    int position=0;
    char *buffer=calloc(sizeof(char), bufsize);
    int c;

    if (!buffer) 
    {
        fprintf(stderr, "Memory allocation error\n");
        exit(1);
    }
    while (1) 
    {
        c=getchar();
        if (c==EOF||c=='\n'||c=='\0') 
        {
            buffer[position]='\0';
            return buffer;
        } 
        else 
        {
            buffer[position]=c;
        }
        position++;
        if (position>=bufsize) 
        {
            bufsize+=READSTR_BUFSIZE;
            buffer=realloc(buffer, bufsize);
            if (!buffer) 
            {
                fprintf(stderr, "Memory error\n");

                exit(1);
            }
        }
    }
    return buffer;
}



int main(int argc, char **argv)
{

    fp=open("/dev/null", O_RDONLY, 0660);
    char ch='y';
    int m;
    int u;
    char ***str_mass;
    char *str=NULL;
    while (ch=='y')
    {
        printf("Enter your commands\n");
        waiter();
        str=readstr();
        waiter();
        str_mass=parsing(str);
        for (u=0; u<=maxu; u++)       
        {
            if (u<maxu)
            {
                if (pipe(fd[1])==-1)
                {
                    perror("impossible to create new canal\n");
                    exit(1);
                }
            }
            one_step(str_mass, u);
        }
        printf("%d", flag_dev);
        for (u=0; u<=maxu; u++)
        {
            if (!flag_dev)
            {
                signal(SIGINT, SIG_DFL);
                wait(NULL);

            }
            else
            {
                signal(SIGINT, SIG_IGN);
            }
            
        }
        printf("Press 'y', if you want to continue entering data, 'n' if you don't\n");
        ch=getchar();
        getchar();
        free(str);
        u=0;
        m=0;
        while (str_mass[u][m])
        {
            free(str_mass[u][m]);
            m++;
        }
        while (str_mass[u])
        {
            free(str_mass[u]);
            u++;
        }
        free(str_mass);
        if (redir_in)
            close(red1), redir_in = 0;
        if (redir_out)
            close(red2), redir_out = 0;
    }
    return 0;
}
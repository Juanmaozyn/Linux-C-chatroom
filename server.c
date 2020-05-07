#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <pthread.h>

#include <sqlite3.h>

#define PORT 33333

struct message
{
    int cmd;
    char name[50];
    char passward[50];
    char toname[20];
    char msg[1024];
};

struct online
{
    int cfd;
    char name[50];
    char passward[50];

    struct online *next;
};

struct online *head = NULL;

//创建数据库
int create_table(sqlite3 *db)
{
	char *sql;
	char *errmsg;
	int rec;

	sql = "create table if not exists mytable (id txt primary key, passward text);";

    rec = sqlite3_exec(db,sql,NULL,NULL,&errmsg);

	if(rec != SQLITE_OK)
	{
		printf("create table error!\n");
		exit(-1);
	}

	return 0;
}

int insert_record(sqlite3 *db, struct message *msg)
{

	char sql[100];
	char *errmsg;
	int rec;
	char name[50];
    char passward[50];

    strcpy(name, msg->name);
    strcpy(passward, msg->passward);

    sprintf(sql,"insert into mytable (id,passward) values('%s','%s');",name, passward);

    rec = sqlite3_exec(db,sql,NULL,NULL,&errmsg);

	if(rec != SQLITE_OK)
	{
		printf("insert table error:%s!\n",errmsg);
		exit(-1);
	}

	return 0;
}

int displaycb(void * para,int ncolumn,char **columnvalue,char **columnname)
{
    int i;
	printf("total column is:%d\n",ncolumn);

	for(i = 0; i < ncolumn;i++)
	{
		printf("columnname:%s---->columnvalue:%s\n",columnname[i],columnvalue[i]);
	}
	printf("***************************\n");

	return 0;
}

int inquire_usecb(sqlite3 *db)
{
    
	char *sql;
	char *errmsg;
	int rec;

	sql = "select * from mytable;";

    rec = sqlite3_exec(db,sql,displaycb,NULL,&errmsg);

	if(rec != SQLITE_OK)
	{
		printf("select error:%s!\n",errmsg);
		exit(-1);
	}

	return 0;
}


int find_usecb(sqlite3 *db, struct message *msg)
{
    char sql[100];
    char sql_two[100];
	char *errmsg;
	int rec, rec_two;
    char name[50];
    char passward[50];

    char **pResult, **pResult_two;
    int rowCount = 0, columnCount = 0;
    int rowCount_two = 0, columnCount_two = 0;

    
    strcpy(name, msg->name);
    strcpy(passward, msg->passward);

	sprintf(sql,"select * from mytable where id = '%s' ", name);

    //rec = sqlite3_exec(db,sql,NULL,NULL,&errmsg);
    sqlite3_get_table(db, sql, &pResult, &rowCount, &columnCount, &errmsg);

	if(rowCount == 1)
	{
        //printf("用户名重复\n");
		//printf("delete error:%s!\n",errmsg);
        sprintf(sql_two,"select * from mytable where id = '%s' and passward = '%s' ", name, passward);
        //rec_two = sqlite3_exec(db,sql_two,NULL,NULL,&errmsg);
        sqlite3_get_table(db, sql_two, &pResult_two, &rowCount_two, &columnCount_two, &errmsg);

        if(rowCount_two == 1)
        {
            printf("登录成功\n");
            return 3;
        }

        printf("密码错误\n");
        return 2;
	}
    
    printf("注册\n");
    return 1;
}


void insert_user(struct online *new)
{
   if(head == NULL)
   {
       new->next = NULL;
	   head = new;
   }
   else
   {
       new->next = head->next;
	   head->next = new;
   }
}

int find_cfd(char *toname)
{
    if(head == NULL)
    {
        return -1;
    }

    struct online *temp = head;

    while(temp != NULL)
    {
        if(strcmp(temp->name,toname) == 0)
        {
            return temp->cfd;
	    }

	    temp = temp->next;
    }

    return -1;

}

void * recv_message(void *arg)
{
    int ret;
    int to_cfd;
    int cfd = *((int *)arg);
    int rec_one;
    
    int rec;
    sqlite3 *db;
    rec = sqlite3_open("mydatabase.db",&db);
	if(rec != SQLITE_OK)
	{
		printf("open error!\n");
		exit(-1);
	}

    //char buffer[1024];
    struct online *new;
    struct message *msg = (struct message *)malloc(sizeof(struct message));

    while(1)
    {
        memset(msg,0,sizeof(struct message));

	    if((ret = recv(cfd,msg,sizeof(struct message),0)) < 0)
	    {
            printf("ret = %d\n",ret);
	        perror("recv error!");
	        exit(1);
	    }

	    if(ret == 0)
	    {
	        printf("%d is close!\n",cfd);
	        pthread_exit(NULL);
	    }
         
	    switch(msg->cmd)
	    {
            
	        case 1:
	        {
	            new = (struct online *)malloc(sizeof(struct online));
		        new->cfd = cfd;
		        strcpy(new->name,msg->name);
                strcpy(new->passward,msg->passward);

                rec_one = find_usecb(db, msg);

                //printf("rec = %d\n", rec_one);
                if (rec_one == 1)
                {
                    msg->cmd = 1;
                    insert_record(db, msg);
                    insert_user(new);
                }
                if (rec_one == 3)
                {
                    msg->cmd = 1;
                    insert_user(new);
                }
                if (rec_one == 2)
                {
                    msg->cmd = -1;
                }


                //insert_record(db, msg);
                inquire_usecb(db);
		        //insert_user(new);

		        //msg->cmd = 1;
		        send(cfd,msg,sizeof(struct message),0);
		        break;
	        }

            
	        case 2:
	        {
	            to_cfd = find_cfd(msg->toname);
                  
		        msg->cmd = 2;
		        send(to_cfd,msg,sizeof(struct message),0);
                  
		        break;
	        }


	        case 3:
	        {
	            struct online *temp = head;

		        while(temp != NULL)
		        {
		            to_cfd = temp->cfd;
		            msg->cmd = 3;
		            send(to_cfd,msg,sizeof(struct message),0);
		            temp = temp->next;
	            }
                break;
            }
	    }

	    usleep(3);
    }

     pthread_exit(NULL);
}




int main()
{
    int cfd;
    int sockfd;

    int c_len;

    char buffer[1024];

    pthread_t id;


    //数据库
    sqlite3 *db;
	int rec;
    rec = sqlite3_open("mydatabase.db",&db);

	if(rec != SQLITE_OK)
	{
		printf("open error!\n");
		exit(-1);
	}

	create_table(db);


    struct sockaddr_in s_addr;
    struct sockaddr_in c_addr;

    if((sockfd = socket(AF_INET,SOCK_STREAM,0)) < 0)
    {
        perror("socket error!");
        exit(1);
    }

    printf("client socket success!\n");

    bzero(&s_addr,sizeof(struct sockaddr_in));
    s_addr.sin_family = AF_INET;
    s_addr.sin_port = htons(PORT);
    s_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(sockfd,(struct sockaddr *)(&s_addr),sizeof(struct sockaddr_in)) < 0)
    {
        perror("bind error!");
        exit(1);
    }

    printf("bind success!\n");

    if (listen(sockfd,3) < 0)
    {
        perror("listen error!");
        exit(1);
    }

    printf("listen success!\n");

    while(1)
    {    
        memset(buffer,0,sizeof(buffer));

        bzero(&c_addr,sizeof(struct sockaddr_in));
        c_len = sizeof(struct sockaddr_in);

        printf("accepting........!\n");
    
        if((cfd = accept(sockfd,(struct sockaddr *)(&c_addr),&c_len)) < 0)
        {
            perror("accept error!");
	        exit(1);
        }

        printf("port = %d ip = %s\n",ntohs(c_addr.sin_port),inet_ntoa(c_addr.sin_addr));

	    if(pthread_create(&id, NULL, recv_message, (void *)(&cfd)) != 0)
	    {
	        perror("pthread create error!");
	        exit(1);
	    }

        usleep(3);

    }

    return 0;
}
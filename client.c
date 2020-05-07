#include <gtk/gtk.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <pthread.h>

#define PORT 33333

GtkWidget *window;
GtkWidget *home;
GtkTextBuffer *bufferuser;

char login_name[50];

struct message
{
    int cmd;
    char name[50];
    char passward[50];
    char toname[20];
    char msg[1024];
};

GtkWidget *entry;
GtkWidget *entry_two;
GtkWidget *entry_name;
GtkWidget *entry_txt;

int sockfd;
struct sockaddr_in s_addr;

void * recv_message(void *arg)
{
    int i;
    int ret;
    int cfd = *((int *)arg);
    char buf[1024];

    struct message *msg = (struct message *)malloc(sizeof(struct message));
   
    while(1)
    {
        memset(msg,0,sizeof(struct message));
        memset(buf, 0, 1024);

	    if((ret = recv(cfd,msg,sizeof(struct message),0)) < 0)
	    {
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
	            printf("reg succcess!\n");
		        break;
	        }
	        case 2:
	        {
	            printf("%s : %s\n",msg->name,msg->msg);
                //strcpy(buf,msg->msg);
                sprintf(buf,"recv : %s\n",msg->msg);
                GtkTextIter start,end;
                gtk_text_buffer_get_bounds(GTK_TEXT_BUFFER(bufferuser),&start,&end);
                gtk_text_buffer_insert(GTK_TEXT_BUFFER(bufferuser),&start,buf,strlen(buf));
		        break;
	        }
	        case 3:
	        {
	            printf("all recv:%s\n",msg->msg);sprintf(buf,"%s : %s\n",msg->name,msg->msg);
                sprintf(buf,"ALL : %s\n",msg->msg);
                GtkTextIter start,end;
                gtk_text_buffer_get_bounds(GTK_TEXT_BUFFER(bufferuser),&start,&end);
                gtk_text_buffer_insert(GTK_TEXT_BUFFER(bufferuser),&start,buf,strlen(buf));
		        break;
	        }



            case -1:
            {
                printf("密码错误\n");
                exit(0);
                break;
            }

	    }
	    
        usleep(3);
    }

     pthread_exit(NULL);
}




void deal_pressed(GtkWidget *button,gpointer entry_two)
{
    const char *name;
    const char *passward;

    char buffer[1024];

    pthread_t id;

    if((sockfd = socket(AF_INET,SOCK_STREAM,0)) < 0)
    {
        perror("socket error!");
	    exit(1);
    }

    bzero(&s_addr,sizeof(struct sockaddr_in));
    s_addr.sin_family = AF_INET;
    s_addr.sin_port = htons(PORT);
    s_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    if(connect(sockfd,(struct sockaddr *)(&s_addr),sizeof(struct sockaddr_in)) < 0)
    {
        perror("connect error!");
	    exit(1);
    }

    printf("connect success!\n");

    
    if(pthread_create(&id,NULL,recv_message,(void *)(&sockfd)) != 0)
    {
        perror("pthread create error!");
	    exit(1);
    }


    name = gtk_entry_get_text(GTK_ENTRY(entry));
    strcpy(login_name,name);
    passward = gtk_entry_get_text(GTK_ENTRY(entry_two));

    printf("name = %s\n",name);
    printf("passward = %s\n",passward);

    struct message *msg = (struct message *)malloc(sizeof(struct message));
    strcpy(msg->name,name);
    strcpy(msg->passward,passward);
    msg->cmd = 1;

    if(send(sockfd,msg,sizeof(struct message),0) < 0)
	{
	    perror("send error reg!\n");
		exit(1);
    }

    gtk_widget_destroy(window);
}

void sendtouser(GtkButton * button, gpointer user_data)
{
    char *buf;
	buf = (char *)malloc(1024);
	memset(buf, 0, 1024);
	int sendbytes;	//用来记录发送的字节数。

    struct message *msg = (struct message *)malloc(sizeof(struct message));


	const gchar  *text = gtk_entry_get_text(GTK_ENTRY(entry_txt));	//获得行编辑entry的内容并静态建立text指针进行指定。
	const char *but = gtk_button_get_label(button);	//获得获取按钮button文本内容并静态建立but指针进行指定。
	if(strlen(text)==0)
    {
		printf("不能为空\n");
		return;
	}
    else
    {
        if(strcmp(but,"发送>>")==0)
        {
			const char *toname = gtk_entry_get_text(GTK_ENTRY(entry_name));	//获得行编辑entryname的内容并静态建立name指针进行指定。
			if(strlen(toname)==0)
            {	//如果name的长度为0
			    printf("name为空。\n");	//打印内容。
			    return;
		    }
            const char *msg_txt = gtk_entry_get_text(GTK_ENTRY(entry_txt));
            msg->cmd = 2;
            strcpy(msg->toname,toname);
            strcpy(msg->msg,msg_txt);
            if ((sendbytes = send(sockfd,msg,sizeof(struct message),0)) == -1)
            {
                perror("fail to send");
            }

		    /*sprintf(buf,"User:%d:%s%s\n",strlen(name),name,text); 	//将内容写入buf。
		    if ((sendbytes = send(clientfd, buf, strlen(buf), 0)) == -1)	//将buf由指定的socket端口clientfd传给对方主机并将发送的字节数存进sendbytes；如果发送失败。
		    {
			    perror("fail to send");    //把"fail to send"输出到标准错误stderr。       
		    }*/
		    return;
        }
        if(strcmp(but,"群发>>")==0)
        {
            const char *msg_txt = gtk_entry_get_text(GTK_ENTRY(entry_txt));
            msg->cmd = 3;
            strcpy(msg->msg,msg_txt);
            if ((sendbytes = send(sockfd,msg,sizeof(struct message),0)) == -1)
            {
                perror("fail to send");
            }
        }
        /*else
        {
			sprintf(buf,"%s%s\n","All::",text);	//将内容写入buf。
			if ((sendbytes = send(clientfd, buf, strlen(buf), 0)) == -1)	//将buf由指定的socket端口clientfd传给对方主机并将发送的字节数存进sendbytes；如果发送失败。
			{
				perror("fail to send");	//把"fail to send"输出到标准错误 stderr。
			}
			return;
		}*/
    }
    
}

void login(int argc,char *argv[])
{
    gtk_init(&argc, &argv);/*gtk初始化*/
    window = gtk_window_new(GTK_WINDOW_TOPLEVEL);/*创建窗口*/
    gtk_window_set_title((GtkWindow *)window,"登录");/*设置标题*/
    gtk_widget_set_size_request(window,600,450);/*设置窗口大小*/
    gtk_window_set_position((GtkWindow *)window,GTK_WIN_POS_CENTER_ALWAYS);/*窗口居中*/

    g_signal_connect(window,"destroy",G_CALLBACK(gtk_main_quit),NULL);/*关闭时退出程序*/

    GtkWidget *fixed = gtk_fixed_new();/*创建固定容器*/
    gtk_container_add(GTK_CONTAINER(window),fixed);/*将容器放窗口中*/

    GtkWidget *label_one = gtk_label_new("用户名");/*创建标签*/
    gtk_fixed_put(GTK_FIXED(fixed),label_one,200,280);/*将标签放入布局*/

    GtkWidget *label_two = gtk_label_new("密码");/*创建标签*/
    gtk_fixed_put(GTK_FIXED(fixed),label_two,210,340);/*将标签放入布局*/

    entry = gtk_entry_new();/*创建行编辑*/
    gtk_entry_set_max_length(GTK_ENTRY(entry),50);/*设置最大长度*/
    gtk_editable_set_editable(GTK_EDITABLE(entry),TRUE);
    gtk_fixed_put(GTK_FIXED(fixed),entry,270,275);

    entry_two = gtk_entry_new();/*创建行编辑*/
    gtk_entry_set_max_length(GTK_ENTRY(entry_two),50);/*设置最大长度*/
    gtk_entry_set_visibility(GTK_ENTRY(entry_two),FALSE);/*密码模式*/
    gtk_editable_set_editable(GTK_EDITABLE(entry_two),TRUE);
    gtk_fixed_put(GTK_FIXED(fixed),entry_two,270,335);

    GtkWidget *button = gtk_button_new_with_label("   登录   ");/*创建按钮*/
    gtk_fixed_put(GTK_FIXED(fixed),button,280,395);
    g_signal_connect(button,"pressed",G_CALLBACK(deal_pressed),entry_two);

    GdkPixbuf * pic = gdk_pixbuf_new_from_file("./image/1.jpg",NULL);/*加入图片*/
    GdkPixbuf * pic_dest = gdk_pixbuf_scale_simple(pic,160,160,GDK_INTERP_BILINEAR);/*设置大小*/
    g_object_unref((GtkObject *)pic);/*释放第一个图片资源*/

    GtkWidget *image = gtk_image_new_from_pixbuf(pic_dest);
    gtk_fixed_put(GTK_FIXED(fixed),image,250,50);

    gtk_widget_show_all(window);/*显示窗口控件*/

    gtk_main();/*主事件循环*/

} 

void homepage(int argc, char *argv[])
{
    char *buf;
    buf = (char *)malloc(1024);

    memset(buf,0,1024);

    gtk_init(&argc, &argv);

    home = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(home),"欢迎");
    gtk_window_set_position(GTK_WINDOW(home),GTK_WIN_POS_CENTER);
    gtk_widget_set_size_request(home,300,600);
    
    g_signal_connect(home,"destroy",G_CALLBACK(gtk_main_quit), NULL);

    GtkWidget *fixed = gtk_fixed_new();
    gtk_container_add(GTK_CONTAINER(home), fixed);

    GtkWidget *label_one;
    GtkWidget *label_two;
    GtkWidget *label_three;
    GtkWidget *label_four;

    label_one = gtk_label_new("联系人");
	gtk_fixed_put(GTK_FIXED(fixed), label_one,20,300);

    label_two = gtk_label_new("内容");
	gtk_fixed_put(GTK_FIXED(fixed), label_two,20,350);

    printf("name = %s\n",login_name);
    label_three = gtk_label_new(login_name);                 //显示登录名
	gtk_fixed_put(GTK_FIXED(fixed), label_three,60,560);

    label_four = gtk_label_new("你好 , ");
	gtk_fixed_put(GTK_FIXED(fixed), label_four,20,560);


    entry_name = gtk_entry_new();
	gtk_entry_set_max_length(GTK_ENTRY(entry_name),500);
	gtk_editable_set_editable(GTK_EDITABLE(entry_name), TRUE);
	gtk_fixed_put(GTK_FIXED(fixed), entry_name,80,300); 
 
	entry_txt = gtk_entry_new();		
	gtk_entry_set_max_length(GTK_ENTRY(entry_txt),500);
	gtk_editable_set_editable(GTK_EDITABLE(entry_txt), TRUE);
	gtk_fixed_put(GTK_FIXED(fixed), entry_txt,80,350);
    
    GtkWidget *bsend = gtk_button_new_with_label("发送>>");
	gtk_fixed_put(GTK_FIXED(fixed), bsend,100,450);
 
	GtkWidget *send_all = gtk_button_new_with_label("群发>>");
	gtk_fixed_put(GTK_FIXED(fixed), send_all,100,500);

    GtkWidget *view = gtk_text_view_new(); 
	gtk_widget_set_size_request (view, 250, 250);
	gtk_text_view_set_cursor_visible(GTK_TEXT_VIEW(view), FALSE);
	gtk_fixed_put(GTK_FIXED(fixed), view, 20, 30);
	bufferuser=gtk_text_view_get_buffer(GTK_TEXT_VIEW(view));

    g_signal_connect(bsend, "pressed", G_CALLBACK(sendtouser),entry);
	g_signal_connect(send_all, "pressed", G_CALLBACK(sendtouser),entry);

     
    gtk_widget_show_all(home);
    gtk_main();
}


int main(int argc, char *argv[])
{
    login(argc,argv);
    homepage(argc,argv);
    return 0;
}
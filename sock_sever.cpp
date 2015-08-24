#include <iostream>
#include <winsock.h>
#include <winsock2.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <windows.h>
#include <direct.h>
using namespace std;

WSADATA wsaData;
int port = 4000;
#define MaxWord 4096
SOCKET 	client_socket, sever_socket;
struct 	sockaddr_in     servaddr;
char    cmd_buffer[MaxWord],data_buffer[MaxWord];
char  	path[MaxWord];
//char 	tempFile[MaxWord]="e:\\temp.txt";
int     n,err;
char cmd[256]="";
char fname[256]="";

void severInit(){
	//WSAStartup and socket
	WSAStartup( MAKEWORD( 1, 1 ), &wsaData );
	sever_socket=socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);

	//bind
	servaddr.sin_family=AF_INET;
	servaddr.sin_port=htons(port);
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);

	err=bind(sever_socket,(SOCKADDR*)&servaddr,sizeof(servaddr));
	if(err==-1){
		cout<<"bind error\n";
		exit(0);
	}
	memset(path,0,sizeof(path));
	getcwd(path,MaxWord);

}

void severListen(){
	//listen
	err=listen(sever_socket,1);
	if(err==-1){
		cout<<"listen error\n";
		exit(0);
	}
}

void sendFile(){
	char buf[2048];
	char fullname[256];
	sprintf(fullname,"%s\\%s",path,fname);
	FILE* file=fopen(fullname,"rb");
	if(file==NULL){
		   cerr<<"open error!"<<endl;
	   memset(buf,0,sizeof(buf));
	   buf[0]=-1;
	   send(client_socket,buf,1,0);
	   return;
	}
	else{
	   fseek(file,0,2);
	   long long flen=ftell(file);//得到文件最大长度；
	   int begin,nlen;
	   fseek(file,0,0);//回到文件头部
	   /*
	    对文件进行分块传送
	   */
	   for(int i=0;;)
	   {
	    memset(buf,0,sizeof(buf));
	    memcpy(buf,&flen,sizeof(long long));
	    memcpy(buf+sizeof(long long),&i,4);
	    begin=ftell(file);
	    fread(buf+sizeof(long long)+8,1,1024,file);
	    nlen=ftell(file)-begin;/*文件读取的内容长度*/
	    if(nlen==0)/*文件内容读取完全*/
	    {
	     break;
	    }
	    memcpy(buf+sizeof(long long)+4,&nlen,4);
	    send(client_socket,buf,sizeof(buf),0);
	    /*
	     先等待，让客户端接收完毕
	    */
	    recv(client_socket,buf,64,0);
	   }
   fclose(file);
   memset(buf,0,sizeof(buf));
   send(client_socket,buf,1,0);
	}
}
void getFile(){
	//send(client,buf,sizeof(buf),0);
	char buf[2048];
	recv(client_socket,buf,sizeof(buf),0);
	if(buf[0]==-2)
	   return ;
	if(buf[0]==-1)
	{
	   cerr<<"ftp server do not have the file:"<<fname<<endl;
	   return;
	}
	else
	{
	   long long flen;
	   int i=0,nlen;
	   memcpy(&flen,buf,sizeof(long long));
	   memcpy(&i,buf+sizeof(long long),4);
	   memcpy(&nlen,buf+sizeof(int)+sizeof(long long),4);
	   FILE *file=fopen(fname,"wb");
	   if(file==NULL)
	   {
	    cerr<<fname<<" create error!"<<endl;
	    return;
	   }
	   for(;i*1024+nlen!=flen;)
	   {
	    /*
	     将讲到的内容写到文件中;
	    */
	    fwrite(buf+8+sizeof(long long),1,nlen,file);
	    memset(buf,0,sizeof(buf));
	    /*
	     接收成功标识，以便继续传送
	    */
	    send(client_socket,buf,64,0);
	    /*
	     再次接收
	    */
	    int flag=recv(client_socket,buf,sizeof(buf),0);
	    memcpy(&i,buf+sizeof(long long),4);
	    memcpy(&nlen,buf+4+sizeof(long long),4);
	    if(nlen==0)
	    {
	     break;
	    }
	   }
	   fwrite(buf+sizeof(int)*2+sizeof(long long),1,nlen,file);
	   fclose(file);
	}
}
void severCd(){
	chdir(fname);
	getcwd(path,MaxWord);
	send(client_socket,path,strlen(path),0);
}
void severPwd(){
	memset(path,0,sizeof(path));
	getcwd(path,MaxWord);
	send(client_socket,path,strlen(path),0);
}
void severDir(){
	char cmd[256];
	char fname[256]="temp123.txt";
	sprintf(cmd,"DIR >temp123.txt");
	system(cmd);
	FILE* fp=fopen(fname,"rb");
	memset(data_buffer,0,sizeof(data_buffer));
	fread(data_buffer,1,MaxWord,fp);
	send(client_socket,data_buffer,strlen(data_buffer),0);
	fclose(fp);
	sprintf(cmd,"del temp123.txt");
	system(cmd);
}
void cmdProcess(){
	memset(fname,0,256);
	memset(cmd,0,16);
	sscanf(cmd_buffer,"%s%s",cmd,fname);
	cout<<cmd<<"  "<<fname<<endl;
	if(strcmp(cmd,"dir")==0)	{severDir();return;}
	if(strcmp(cmd,"pwd")==0)	{severPwd();return;}
	if(strcmp(cmd,"get")==0)	{sendFile();return;}
	if(strcmp(cmd,"put")==0)	{getFile();return;}
	if(strcmp(cmd,"cd")==0)	{severCd();return;}
	send(client_socket,cmd_buffer,strlen(cmd_buffer),0);
}

int main()
{
	start:
	severInit();
	l_listen:
	severListen();

	client_socket=accept(sever_socket, (struct sockaddr*)NULL, NULL);
	if(client_socket==-1){
	cout<<"accept error\n";
	goto l_listen;
	}
	//循环接受命令
	while(true){
		memset(cmd_buffer,0,sizeof(cmd_buffer));
		n = recv(client_socket, cmd_buffer, MaxWord, 0);
		if(n<0){
			cout<<"recv error\n";
			goto l_listen;
		}
		cmd_buffer[n] = '\0';
		cmdProcess();
	}
    return 0;
}

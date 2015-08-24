#include <iostream>
#include <winsock.h>
#include <winsock2.h>
#include <stdio.h>
#include <string>
#include <stdlib.h>
#include <windows.h>
using namespace std;

WSADATA wsaData;
int port = 4000;
#define MaxWord 4096
char IP_str[256];
SOCKET 	client_socket;
struct 	sockaddr_in     servaddr;
char    cmd_buffer[MaxWord],data_buffer[MaxWord];
int     n,err;
char cmd[16]="";
char fname[256]="";

void clientInit(){
	WSAStartup(MAKEWORD(1,1),&wsaData);
	client_socket =socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);
	servaddr.sin_family=AF_INET;
	servaddr.sin_port=htons(port);
}
void clientConnect(){
	l_connect:
	cout<<"Input IP:";
	cin>>IP_str;
	servaddr.sin_addr.s_addr = inet_addr(IP_str);
	err=connect(client_socket,(struct sockaddr*)&servaddr,sizeof(servaddr));
	if(err<0){
		cout<<"connect error\n";
		goto l_connect;
	}

}
void getFile(){
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
void sendFile(){
	char buf[2048];
	// char fullname[256];
	// strcpy(fullname,root);
	// strcat(fullname,fname);
	FILE* file=fopen(fname,"rb");
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
void clientPwd(){
	memset(data_buffer,0,sizeof(data_buffer));
	recv(client_socket, data_buffer, MaxWord, 0);
	cout<<data_buffer<<endl;
}
void clientDir(){
	memset(data_buffer,0,sizeof(data_buffer));
	recv(client_socket, data_buffer, MaxWord, 0);
	cout<<data_buffer<<endl;
}
void clientCd(){
	memset(data_buffer,0,sizeof(data_buffer));
	recv(client_socket, data_buffer, MaxWord, 0);
	cout<<data_buffer<<endl;
}
void cmdProcess(){
	send(client_socket, cmd_buffer, strlen(cmd_buffer), 0);
	memset(fname,0,256);
	memset(cmd,0,16);
	sscanf(cmd_buffer,"%s%s",cmd,fname);
	if(strcmp(cmd,"dir")==0)	{clientDir();return;}
	if(strcmp(cmd,"pwd")==0)	{clientPwd();return;}
	if(strcmp(cmd,"get")==0)	{getFile();return;}
	if(strcmp(cmd,"put")==0)	{sendFile();return;}
	if(strcmp(cmd,"cd")==0)	{clientCd();return;}
}
int main()
{
	clientInit();
	clientConnect();

	//send(client_socket, IP_str, strlen(IP_str), 0);
	cout<<"connect success"<<endl;
	while(true){
		memset(cmd_buffer,0,sizeof(cmd_buffer));
		string str;
		getline(cin,str);
		strcpy(cmd_buffer,str.c_str());
		cmdProcess();
	}
    return 0;
}

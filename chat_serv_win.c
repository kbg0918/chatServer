#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include <process.h> 

#define BUF_SIZE 256
#define MAX_CLNT 256
#define MAX_ROOM 256
#define ROOM_ID_DEFAULT -1

typedef int BOOL;
#define TRUE 1
#define FALSE 0



unsigned WINAPI HandleClnt(void * arg);
void SendMsg(char * msg, int len);
void ErrorHandling(char * msg);
int getIndexSpace(char *msg);

int clntCnt=0;
SOCKET clntSocks[MAX_CLNT];
HANDLE hMutex;

struct Client{
 int roomId;
 int socket; 

};
typedef struct Client Client;

int sizeClient=0;
Client arrClient[MAX_CLNT];	

struct Room{
 int id;	
 char name[BUF_SIZE];	

};
typedef struct Room Room;	


int sizeRoom=0;	
Room arrRoom[MAX_ROOM];	

int issuedId = 0;


Client * addClient(int socket){
 	WaitForSingleObject(hMutex, INFINITE);
    Client *client = &( arrClient[sizeClient++] );	
    client->roomId=ROOM_ID_DEFAULT;
    client->socket=socket;		
    ReleaseMutex(hMutex);
 return client;			

}
void removeClient(int socket){
	WaitForSingleObject(hMutex, INFINITE);
	int i=0;
	
	for(i=0; i<sizeClient; i++){
		if(socket == arrClient[i].socket){
			while(i++<sizeClient-1)
				arrClient[i] = arrClient[i+1];
			break;
		}
	}
	
	sizeClient--;
	ReleaseMutex(hMutex);
	
}

int main(int argc, char *argv[])
{
	WSADATA wsaData;
	SOCKET hServSock, hClntSock;
	SOCKADDR_IN servAdr, clntAdr;
	int clntAdrSz;
	HANDLE  hThread;
	if(argc!=2) {
		printf("Usage : %s <port>\n", argv[0]);
		exit(1);
	}
	if(WSAStartup(MAKEWORD(2, 2), &wsaData)!=0)
		ErrorHandling("WSAStartup() error!"); 
  
	hMutex=CreateMutex(NULL, FALSE, NULL); //Mutex를 사용하기 위해 
	hServSock=socket(PF_INET, SOCK_STREAM, 0); //TCP용 소켓을 생성 

	memset(&servAdr, 0, sizeof(servAdr));
	servAdr.sin_family=AF_INET; //인터넷 통신 
	servAdr.sin_addr.s_addr=htonl(INADDR_ANY);
	servAdr.sin_port=htons(atoi(argv[1])); //사용자 지정 포트 
	
	if(bind(hServSock, (SOCKADDR*) &servAdr, sizeof(servAdr))==SOCKET_ERROR)
		ErrorHandling("bind() error");
	if(listen(hServSock, 5)==SOCKET_ERROR)
		ErrorHandling("listen() error");
	
	while(1)
	{
		clntAdrSz=sizeof(clntAdr);
		hClntSock=accept(hServSock, (SOCKADDR*)&clntAdr,&clntAdrSz);
		Client *client = addClient(hClntSock); //client 정보 추가 
		
		WaitForSingleObject(hMutex, INFINITE);
		clntSocks[clntCnt++]=hClntSock;
		ReleaseMutex(hMutex);
	
		hThread=
			(HANDLE)_beginthreadex(NULL, 0, HandleClnt, (void*)&client, 0, NULL); //handleClnt에서 사용할 client의 정보 
		printf("Connected client IP: %s \n", inet_ntoa(clntAdr.sin_addr));
	}
	closesocket(hServSock);
	WSACleanup();
	return 0;
}

//특정 사람에게 메시지 보내기 
void sendMessageUser(char * msg, int socket){
    int length = send(socket, msg, strlen(msg), 0);
 	printf("(info) send user(%d): %s, result length=%d\n",socket,msg,length);

}

// 방 안의 사람들에게 보내기 
void sendMessageRoom(char * msg, int roomId){
	int i; 
    WaitForSingleObject(hMutex, INFINITE);

    for(i=0; i<sizeClient; i++){
    	if(arrClient[i].roomId == roomId)
    		sendMessageUser(msg, arrClient[i].socket);
	}
	
	ReleaseMutex(hMutex);
}



char *nameClear(char *msg){ 
	char str[BUF_SIZE];
	for(; *msg != '\0'; msg++){
		if(*msg == ']'){
			strcpy(str,msg+1);
		}
	}
	
	char *result = strtok(str," ");
	
	return result;
	
}

//사용자가 방 안에 들어가 있는지  
BOOL userInRoom(int socket){

	int i=0;
	for(i=0; i<sizeClient; i++){
		if(arrClient[i].socket == socket && arrClient[i].roomId!=ROOM_ID_DEFAULT) return TRUE;	
	}
	
	return FALSE;	// 아니면 방에 들어가 있지 않다

}

void getRoomMenuNum(char * menu, char *msg){
    if(msg==NULL) return;
    int indexSpace = getIndexSpace(msg);

    if(indexSpace<0) return;	

    char * change = &msg[indexSpace+1];

 	strcpy(menu,change);

 	menu[4] = 0;

}



//방 생성 
Room * addRoom(char * name)	{
	
    WaitForSingleObject(hMutex, INFINITE);
    Room *room = &( arrRoom[sizeRoom++] );

    room->id=issuedId++;	

    strcpy(room->name, name);	

    ReleaseMutex(hMutex);

    return room;

}

// 방 삭제 
void removeRoom(int roomId){
 	int i=0;
    WaitForSingleObject(hMutex, INFINITE);
    
    for(i=0; i<sizeRoom; i++){
    	if(arrRoom[i].id==roomId){
    		while(i++<sizeRoom-1)
    			arrRoom[i] = arrRoom[i+1];
    		break;
		}
	} 

    sizeRoom--;
    
    ReleaseMutex(hMutex);

}


//방 안에 사람 있는지 확인 
BOOL isEmptyRoom(int roomId){
	int i=0;
	for(i=0; i<sizeClient; i++){
	
	if(arrClient[i].roomId == roomId)	
		return FALSE;
	
	}
	
	return TRUE;

}

//방 있는지 확인 
BOOL isExistRoom(int roomId){
	

	int i=0;
	
	for( i=0; i<sizeRoom; i++){
		if(arrRoom[i].id==roomId)
			return TRUE;
	
	}
	
	return FALSE;

}

// 방 입장 
void enterRoom(Client * client, int roomId){
	char buf[BUF_SIZE]="";
	if(!isExistRoom(roomId)){ //위의 방 있는지 확인하는 코드 
		sprintf(buf,"[server] 오류! 방 입장 불가.\n");	// 못들어간다. 에러 메시지 작성
	}
	
	else{ // 방이 있다면 roomId 저장 후 입장. 
	    client->roomId = roomId;
	    sprintf(buf,"[server] 입장 %d번 방\n",roomId);	
	
	}
	sendMessageUser(buf, client->socket); //client에 buf에 담긴 입장 ~몇 번 방이라고 알림. 

}

//공백 제거 함수 
int getIndexSpace(char * msg){
	
	int indexSpace= 0;
	int length = strlen(msg);
	int i=0;
	
	for(i=0; i<length; i++){
		if(msg[i]==' '){
			indexSpace = i;
			break;
		}
	}
	
	if(indexSpace+1>=length){
		printf("메시지가 너무짧아요!\n");	
		return -1;
	}

 return indexSpace;

}


//문자열 자르기 
int getNum(char *msg){
	
	
	if(msg==NULL) return -1;
	int indexSpace = getIndexSpace(msg);
	
	if(indexSpace<0) return -1;
	
	char change = msg[indexSpace+1]; 

 
 	return change-'0';	
 
 
}
// 방 만들기 함수
void createRoom(Client * client){
	char buf[BUF_SIZE] = "";
	char title[BUF_SIZE];
	sprintf(buf,"[server] 방 제목을 입력해주세요! :");
	sendMessageUser(buf, client->socket); 
	
	if(recv(client->socket,title,sizeof(title),0)>0){ //client로 부터 제목을 받는다. 
		Room * room = addRoom(nameClear(title)); //addRoom 함수에 제목을 보낸다 .
		enterRoom(client, room->id); // 방 만듦과 동시에 입장한다. 
	} 
	
}

// 방 목록 표시 
void listRoom(Client * client){
	
	char buf[BUF_SIZE] = "";
	int i=0;
	
	sprintf(buf,"===Room List===\n");
	sendMessageUser(buf,client->socket);
	
	for(i=0; i<sizeRoom; i++){ //총 방의 개수 만큼 
		Room * room = &(arrRoom[i]);	
		sprintf(buf," 방 Id:%d 방 제목:%s\n",room->id, room->name); //방의 개수 만큼 돌면서 roomId와 room Name을 client에게 알린다. 
		sendMessageUser(buf,client->socket);
	}

	sprintf(buf,"[server] 채팅 방 수%d\n", sizeRoom);   
 	sendMessageUser(buf,client->socket);

}

// 채팅방에 접속중인 유저의 수  
void roomUser(Client * client, int roomId){
	
	char buf[BUF_SIZE]="";	
	 
	int i=0;			
	int sizeMember=0;		

	sprintf(buf,"[server] 접속중인 유저 수 %d\n",roomId);                 
	
	sendMessageUser(buf, client->socket);	
	
	for(i=0; i<sizeClient; i++){
	
	if(arrClient[i].roomId == roomId){
		sprintf(buf," 번호%d: 사용자의 소켓=%d\n",i,arrClient[i].socket);	
		sendMessageUser(buf,client->socket);
	
	sizeMember++;
	}

	sprintf(buf,"[server] 유저의 수 %d\n", sizeMember);  
	
	sendMessageUser(buf,client->socket);
 }
 
}


int getRoomId(int socket){

	int roomId = -1;
	char buf[BUF_SIZE] = "";
	
	sprintf(buf,"(i) 방 아이디를 입력하세요 : ");
	
	sendMessageUser(buf,socket);
	if(recv(socket,buf,sizeof(buf),0)>0){
		
		char name[BUF_SIZE]=""; 
		sscanf(buf,"%s %d",name, &roomId);	
	
	}
	
	return roomId;

}


// 대기실 메뉴판 
void lobby(Client * client){

	char buf[BUF_SIZE]="";
	
	sprintf(buf,"===Lobby===\n");
	sendMessageUser(buf,client->socket);
	
	sprintf(buf,"1) 채팅방 개설\n");
	sendMessageUser(buf, client->socket);
	
	sprintf(buf,"2) 채팅방 목록\n");
	sendMessageUser(buf, client->socket);
	
	sprintf(buf,"3) 채팅방 입장\n");	
	sendMessageUser(buf, client->socket);
	
	sprintf(buf,"q) 종료\n");
	sendMessageUser(buf, client->socket);

}


// 채팅방 내의 메뉴 
void chatRoomLobby(Client * client){

	char buf[BUF_SIZE]="";
	
	sprintf(buf,"===chatRoom===\n");
	sendMessageUser(buf,client->socket);
	
	sprintf(buf,"exit) 방 나가기\n");
	sendMessageUser(buf, client->socket);
	
	
	sprintf(buf,"list) 개설된 채팅방\n");
	sendMessageUser(buf, client->socket);
	
	
	
	sprintf(buf,"help) 도움말\n");
	sendMessageUser(buf, client->socket);

}

//접속중인 채팅방 나가기 
void exitRoom(Client * client){

	int roomId = client->roomId;
	client->roomId = ROOM_ID_DEFAULT;
	
	
	char buf[BUF_SIZE]="";	
	
	sprintf(buf,"[server] 안녕히가세요! %d번 방\n", roomId);
	sendMessageUser(buf, client->socket);
	
	if(isEmptyRoom(roomId))	{
	    removeRoom(roomId);
	
	}

}
//방에서 메뉴 선택  
void RoomMenu(char * menu, Client * client, char * msg){

	printf("serve menu:%s\n",menu);
	if(strcmp(menu,"exit")==0)	
		exitRoom(client);
	
	else if(strcmp(menu,"list")==0)
		roomUser(client, client->roomId);
	
	else if(strcmp(menu,"help")==0)	
	    chatRoomLobby(client);
	
	else
		sendMessageRoom(msg, client->roomId);

}


//로비 메뉴 선택하기 
void menuSelect(int num, Client * client){
	
	int roomId = ROOM_ID_DEFAULT;

	switch(num){
		case 1:
			createRoom(client);
			break;
		case 2:
			listRoom(client);
			break;
		case 3:
			roomId = getRoomId(client->socket);
			enterRoom(client, roomId);
			break;
		case 4:
			roomId = getRoomId(client->socket);
			roomUser(client, roomId);
			break;
		default:
			lobby(client);
			break;
			
	}
}


unsigned WINAPI HandleClnt(void * arg)
{
	//위의 스레드에서 받아온다. 
	SOCKET hClntSock=*((SOCKET*)arg);
	int strLen=0, i;
	char msg[BUF_SIZE];
	//client 구조체의 정보를 받아온다. 
	Client * client;
	client = hClntSock;
	
	//소켓의 값과 roomId를 저장 
	int clnt_sock = client->socket;
	int roomId = client->roomId;
	
	//로비 화면을 출력한다. 
	lobby(client);
	 
	while((strLen=recv(clnt_sock, msg, sizeof(msg), 0))!=0){
		msg[strLen] = '\0'; 
		if(userInRoom(clnt_sock)){//user가 채팅 방 안에 있다면 
			char menu[BUF_SIZE]="";
			getRoomMenuNum(menu, msg); //menu의 값을 받아온다. 
			RoomMenu(menu, client, msg); //menu의 기능 동작 함수 
			
		}else{
			int num = getNum(msg); //msg의 값을 숫자로 변환 
			menuSelect(num, client); // menu의 기능 동작 함수 
		}
	}
	
	//채팅이 끝난 후 client 제거 
	removeClient(clnt_sock);
	// 소켓 연결 끊기 
	closesocket(clnt_sock);
	return 0;
}

void SendMsg(char * msg, int len)   // send to all	
{
	int i;
	WaitForSingleObject(hMutex, INFINITE);
	for(i=0; i<clntCnt; i++)
		send(clntSocks[i], msg, len, 0);

	ReleaseMutex(hMutex);
}

void ErrorHandling(char * msg)
{
	fputs(msg, stderr);
	fputc('\n', stderr);
	exit(1);
}
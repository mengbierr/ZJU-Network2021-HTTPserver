#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#include<cstdio>
#include<winsock2.h>
#include<thread>
#include<mutex>
#include<vector>
#include<string>
#include<iostream>
#define DEFAULT_PORT 5224
#define SERVER_NAME "ARC-POTATO"
#define MAXBUFFER 65536
#define MAXTHREAD 100
std::thread* t[MAXTHREAD];
bool used[MAXTHREAD];
struct message
{
	char* data;
	SOCKET sClient;
	int id;
	message(char *data,SOCKET sClient,int i):data(data),sClient(sClient),id(id){}
};
struct Client
{
	char* ip;
	int port;
	int id;
	std::thread::id thread_id;
	SOCKET socket;
};
std::mutex lock;
std::vector<struct Client*> client_list;
int count = 0;
void Send404(message s)
{
	char protocol[] = "HTTP/1.1 404 Not Found \r\n";
	char servName[] = "Server:ARC-POTATO\r\n";
	char cntLen[] = "Content-length:108\r\n";
	char cntType[] = "Content-type:text\html\r\n\r\n";
	char content[] = "<html><head><title>Failed to access ARC-POTATO</title></head><font size=+5><br>404 Not Found</font></html>";

	send(s.sClient, protocol, strlen(protocol), 0);
	send(s.sClient, servName, strlen(servName), 0);
	send(s.sClient, cntLen, strlen(cntLen), 0);
	send(s.sClient, cntType, strlen(cntType), 0);
	send(s.sClient, content, strlen(content), 0);
	//closesocket(s.sClient);
}
void sendFile(std::string file, message s)
{
	FILE* fp;
	if ((fp = fopen(file.c_str(), "rb")) == NULL)
	{
		Send404(s);
	}
	else
	{
		std::string tmp;
		tmp += "HTTP/1.1 200 OK\n";
		tmp += "Content-Type: text/html;charset=ISO-8859-1\nContent-Length: ";
		char* content;int len;
		fseek(fp, 0, SEEK_END);
		len = ftell(fp);
		rewind(fp);
		content = (char*)malloc(len * sizeof(char));
		tmp += std::to_string(len);
		tmp += "\n\n";
		if (send(s.sClient, tmp.c_str(), tmp.length(), 0)==SOCKET_ERROR)
		{
			printf("send error!");
			//closesocket(s.sClient);
			return;
		}
		fread(content, 1, len, fp);
		if (send(s.sClient, content, len, 0) == SOCKET_ERROR)
		{
			printf("send error!");
			//closesocket(s.sClient);
			return;
		}
		else
		{
			printf("send successfully!");
			//closesocket(s.sClient);
			return;
		}
	}

}
void handleMessage(message s)
{
	if (strlen(s.data) < 1)
	{
		printf("empty  message");

	}
	else if (s.data[0] == 'P')
	{
		std::cout << "Get a post" << std::endl;
		std::string name, password,data=s.data;
		data = s.data + 5;
		int pos1 = data.find("login"),pos2=data.find("pass");
		name = data.substr(pos1 + 6, pos2 - pos1 - 7);
		password = data.substr(pos2 + 5);
		std::string tmp;
		//std::cout<<data<<s
		std::cout << name << " " << password << std::endl;
		if (name == "3190105224" && password == "5224")
		{

			std::cout << "login success";
			tmp += "<html><body>Hello " + name + "!</body></html>\n";
		}	
		else
		{
			tmp += "<html><body>Login failed!</body></html>\n";
		}
		int len = tmp.length();
		std::string res;
		res += "HTTP/1.1 200 OK\n";
		res += "Content-Type: text/html;charset=gb2312\nContent-Length: ";
		res += std::to_string(len);
		res += "\n\n";
		res += tmp;
		std::cout << res << std::endl;
		if (send(s.sClient, res.c_str(), res.length(), 0) == SOCKET_ERROR)
		{
			printf("send failed!");
		}
		else
		{
			printf("send successfully!");
		}
	}
	else if (s.data[0] == 'G')
	{
		std::cout << "ok";
		std::string name, password, data = s.data;
		data = s.data + 4;
		data = data.substr(0, data.find(" "));
		if (data.substr(0, 6) == "/file/")
		{
			if (data.substr(6, 13) == "../img/")
			{
				int pos = data.find('.');
				std::string type = data.substr(pos + 1);
				if (type == "jpg")
				{
					std::string filepath = "file/img/" + data.substr(13);
					std::cout << filepath << std::endl;
					sendFile(filepath, s);
				}
			}
			int pos = data.find('.');
			std::string type = data.substr(pos + 1);
			if (type == "html")
			{
				std::string filepath = "file/html/" + data.substr(6);
				std::cout << filepath << std::endl;
				sendFile(filepath, s);
			}
			else if (type == "jpg")
			{
				std::string filepath = "file/img/" + data.substr(6);
				std::cout << filepath << std::endl;
				sendFile(filepath, s);
			}
			else if (type == "txt")
			{
				std::string filepath = "file/txt/" + data.substr(6);
				std::cout << filepath << std::endl;
				sendFile(filepath, s);
			}
			else Send404(s);
		}
		else if (data.substr(0, 5) == "/img/")
		{
			int pos = data.find('.');
			std::string type = data.substr(pos + 1);
			if (type == "jpg")
			{
				std::string filepath = "file/img/" + data.substr(5);
				std::cout << filepath << std::endl;
				sendFile(filepath, s);
			}
		}
	}
}
void solve(SOCKET sClient)
{
	SOCKADDR_IN client_info = { 0 };
	int addrsize = sizeof(client_info);
	std::thread::id thread_id = std::this_thread::get_id();
	getpeername(sClient, (struct sockaddr*)&client_info, &addrsize);
	char* ip = inet_ntoa(client_info.sin_addr);
	int port = client_info.sin_port;
	Client now;
	now.socket = sClient;now.ip = ip;now.id = count++;
	now.port = port;now.thread_id = thread_id;
	Client* pnow = &now;
	lock.lock();
	client_list.push_back(pnow);
	lock.unlock();
	bool finish = 0;
	char buffer[MAXBUFFER] = "\0";
	while (!finish)
	{
		ZeroMemory(buffer, MAXBUFFER);
		int state = recv(sClient, buffer, MAXBUFFER, 0);
		std::cout << buffer << std::endl;
		if (state > 0)
		{
			message msg(buffer, sClient, now.id);
			handleMessage(msg);
		}
		else if (state == 0)
		{
			printf("Connection closed.\n");
			finish = 1;
		}
		else
		{
			printf("rev() error!\n");
			closesocket(sClient);
			finish = 1;
		}
	}
	closesocket(sClient);
	std::vector<Client*>::iterator it;
	for (it = client_list.begin();it != client_list.end();it++)
	{
		if ((*it)->id == pnow->id && (*it)->ip == pnow->ip) break;
	}
	lock.lock();
	client_list.erase(it);
	lock.unlock();
	return;
}
int main()
{
	WSADATA wsaData;
	SOCKET sListen, sClient;
	SOCKADDR_IN servAddr, clntAddr;
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
	{
		printf("WSAStartup() error!\n");
		return 0;
	}
	sListen = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (sListen == INVALID_SOCKET)
	{
		printf("socket() error");
		return 0;
	}
	servAddr.sin_family = AF_INET;
	servAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servAddr.sin_port = htons(DEFAULT_PORT);
	if (bind(sListen, (SOCKADDR*)&servAddr, sizeof(servAddr)) == SOCKET_ERROR)
	{
		printf("bind() error! code:%d\n", WSAGetLastError());
		closesocket(sListen);
		return 0;
	}

	if (listen(sListen, 5) == SOCKET_ERROR)
	{
		printf("listen() error! code:%d\n", WSAGetLastError());
		closesocket(sListen);
		return 0;
	}


	while (1)
	{
		int szClntAddr = sizeof(clntAddr);
		sClient = accept(sListen, (SOCKADDR*)&clntAddr, &szClntAddr);

		if (sClient == INVALID_SOCKET)
		{
			printf("accept() error! code:%d\n", WSAGetLastError());
			closesocket(sClient);
			return 0;
		}
		else
		{
			std::thread(solve, std::ref(sClient)).detach();
		}
	}

	closesocket(sListen);
	WSACleanup();
	return 0;
}
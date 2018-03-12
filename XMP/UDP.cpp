#include "UDP.h"
#include<iostream>
#include <process.h> 

UDP::UDP(void)
{
}

UDP::~UDP(void)
{
}

void UDP::Createconnect()
{
	WSADATA wsaData;
	WORD sockVersion = MAKEWORD(2, 2);
	SOCKET serSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	sockaddr_in serAddr;
	serAddr.sin_family = AF_INET;
	serAddr.sin_port = htons(8888);
	serAddr.sin_addr.S_un.S_addr = INADDR_ANY;
	if (bind(serSocket, (sockaddr *)&serAddr, sizeof(serAddr)) == SOCKET_ERROR)
	{
		closesocket(serSocket);
	}
	sockaddr_in remoteAddr;
	int nAddrLen = sizeof(remoteAddr);
	char recvData[255];
	int ret = recvfrom(serSocket, recvData, 255, 0, (sockaddr*)&remoteAddr, &nAddrLen);
	if (ret > 0)
	{
		recvData[ret] = 0x00;
		char sendBuf[20] = { '\0' };
	}
	char * sendData = "一个来自服务端的UDP数据包\n";
	sendto(serSocket, sendData, strlen(sendData), 0, (sockaddr *)&remoteAddr, nAddrLen);
	closesocket(serSocket);
	//WSACleanup();
}


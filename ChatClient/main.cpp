
#define WIN32_LEAN_AND_MEAN

#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdio.h>
#include <string>
#include <iostream>
#include "window.h"

// link with Ws2_32.lib
#pragma comment(lib, "Ws2_32.lib")

#define DEFAULT_PORT 10000 //Use any port you want. Has to be in port forwarding settings.
#define DEFAULT_BUFLEN 512
#define SERVER_IP "" //Public IP used for clients outside of servers network. (public IPv4 of server)
//#define SERVER_IP "" //LAN Address used for clients inside of servers network, because they cant connect to the public IP due to the NAT. (LAN IPv4 of Server)

//Global variables used by all threads.
bool running = true;
Window* window = new Window;

//Creates a window and handles its messages.
DWORD WINAPI WindowHandling(LPVOID lpParameter)
{
	//Create the window.
	window->Instantiate(450, 550, 0, "ChatRoom");

	//While the window is opened
	while (window->GetState() == Window::WindowState::Started)
	{
		//Spawn new messages when a message is recieved.
		if (window->GetRecievedState())
		{
			window->GetRecievedState() = false;
			window->RecievedMessage(window->GetRecievedMessage());
			UpdateWindow(window->GetHandle());
		}

		//Check for incoming window messages and handle them.
		MSG msg = { };
		if (PeekMessageA(&msg, window->GetHandle(), 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	//End application.
	running = false;
	return 0;
}	

//Handles recieving data from the server with the server socket as a parameter.
DWORD WINAPI RecieveData(LPVOID lpParameter)
{
	//Parameter conversion
	SOCKET ServerSocket = (SOCKET)lpParameter;

	int rResult;
	int recvbuflen = DEFAULT_BUFLEN;
	char recvbuf[DEFAULT_BUFLEN];

	//Receive data until the server closes the connection
	do
	{
		//Recieve data
		rResult = recv(ServerSocket, recvbuf, recvbuflen, 0);

		//If there is data recieved
		if (rResult > 0)
		{
			//Convert buffer to string message and resize it.
			std::string message = recvbuf;
			message.resize(rResult);

			//Trigger event so the window spawns a new text with the message.
			window->SetRecievedMessage(message);
			window->GetRecievedState() = true;
			printf("Recieved %d bytes from server \n", rResult);
			printf("Message recieved is: %s \n", message.c_str());
		}
		//If connection is closing:
		else if (rResult == 0)
			printf("Connection closed\n");
		//If failed:
		else
		{
			printf("recv failed: %d\n", WSAGetLastError());
			running = false;
		}
	} while (running);

	return 0;
}

int main()
{
	//Input your name and save it.
	std::string name;
	std::cout << "Input your name please: ";
	std::cin >> name;
	std::cout << std::endl;

	//Create window thread.
	DWORD dwThreadIdWindow;
	CreateThread(NULL, 0, WindowHandling, NULL, 0, &dwThreadIdWindow);

	// Initialize Winsock
	WSADATA wsaData;
	int fResult = 0;
	fResult = WSAStartup(MAKEWORD(2, 2), &wsaData);

	//Error handling.
	if (fResult != 0)
	{
		printf("WSAStartup failed: %d\n", fResult);
		return 1;
	}

	// Setup our socket address structure
	SOCKADDR_IN SockAddr;
	SockAddr.sin_port = htons(DEFAULT_PORT); //Set port
	SockAddr.sin_family = AF_INET; //IPv4 Address
	inet_pton(AF_INET, SERVER_IP, &SockAddr.sin_addr.s_addr); //Convert the server IP to the sockaddr_in format and set it.

	//Create ServerSocket for clients to connect to.
	SOCKET ServerSocket = INVALID_SOCKET;

	//Create Socket object. 
	ServerSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	//Error handling.
	if (ServerSocket == INVALID_SOCKET)
	{
		printf("Failed to execute socket(). Error Code: %ld\n", WSAGetLastError());
		WSACleanup();
		return 1;
	}

	// Connect to server.
	fResult = connect(ServerSocket, reinterpret_cast<SOCKADDR*>(&SockAddr), sizeof(SOCKADDR_IN));

	//Error handling.
	if (fResult == SOCKET_ERROR) 
	{
		printf("Unable to connect to server! Error Code: %ld\n", WSAGetLastError());
		closesocket(ServerSocket);
		ServerSocket = INVALID_SOCKET;
	}

	if (ServerSocket == INVALID_SOCKET) 
	{
		printf("Unable to connect to server!");
		WSACleanup();
		return 1;
	}

	//Create thread for handling data thats recieved by the server. Pass the server socket as argument.
	DWORD dwThreadId;
	CreateThread(NULL, 0, RecieveData, (LPVOID)ServerSocket, 0, &dwThreadId);

	int sResult;

	//Spawn and send connecting message.
	std::string start = name;
	start += " connected to the Chat Room!";

	//Trigger event to spawn message.
	window->SetRecievedMessage(start);
	window->GetRecievedState() = true;

	//Send message to server.
	sResult = send(ServerSocket, start.c_str(), start.size(), 0);

	//Error handling.
	if (sResult == SOCKET_ERROR)
	{
		printf("send failed: %d\n", WSAGetLastError());
		closesocket(ServerSocket);
		WSACleanup();
		return 1;
	}

	//While the window is opened.
	while (running)
	{
		//When the send button is pressed and a message is in the textbox.
		if (window->GetCurrentMessage() != "")
		{
			//Add name to the message.
			std::string message = name + ": ";
			message += window->GetCurrentMessage();

			//Send that message to the server.
			sResult = send(ServerSocket, message.c_str(), message.size(), 0);
			printf("Sending %d bytes... \n", message.size());

			//Error handling.
			if (sResult == SOCKET_ERROR)
			{
				printf("send failed: %d\n", WSAGetLastError());
				closesocket(ServerSocket);
				WSACleanup();
				return 1;
			}

			//Spawn new message.
			window->SetRecievedMessage(message);
			window->GetRecievedState() = true;

			//Reset the inputted message.
			window->SetCurrentMessage("");
		}
	}

	//Send disconnected message
	std::string end = name;
	end += " disconnected from the Chat Room! \n";

	//Send
	sResult = send(ServerSocket, end.c_str(), end.size(), 0);

	//Error handling.
	if (sResult == SOCKET_ERROR)
	{
		printf("send failed: %d\n", WSAGetLastError());
		closesocket(ServerSocket);
		WSACleanup();
		return 1;
	}

	//Shutdown send connection to Server.
	int rResult = shutdown(ServerSocket, SD_SEND);

	//Error handling.
	if (rResult == SOCKET_ERROR)
	{
		printf("shutdown failed: %d\n", WSAGetLastError());
		closesocket(ServerSocket);
		WSACleanup();
		return 1;
	}

	//Close connection to Server, cleanup WinSock and delete the window.
	closesocket(ServerSocket);
	WSACleanup();
	delete window;

	return 0;
}
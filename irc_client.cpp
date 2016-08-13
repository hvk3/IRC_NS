#include <iostream>
#include <vector>
#include <string>
#include <algorithm>
#include <stdio.h>
#include <stdlib.h>

#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>

using namespace std;

#define REGISTRATION_PORT 5555
#define LOGIN_PORT 5556
#define MAX 500

char server_host[MAX];

int initializeSocket(int port_number)
{
	int socket_descriptor = socket(AF_INET, SOCK_STREAM, 0);
	struct sockaddr_in server_address;
	
	memset(&server_address, '0', sizeof(server_address));
	server_address.sin_family = AF_INET;
	server_address.sin_port = htons(port_number);

	if (socket_descriptor < 0)
	{
		printf("Socket creation failed.\n");
		return -1;
	}

	if (inet_pton(AF_INET, server_host, &server_address.sin_addr) <= 0)
	{
		printf("Invalid or unsupported address.\n");
		return -1;
	}

	if (connect(socket_descriptor, (sockaddr*)&server_address, sizeof(server_address)) < 0)
	{
		printf("Connection failed. Please try again later.\n");
		return -1;
	}
	return socket_descriptor;
}

void sendData(int socket_descriptor, string data)	// Returns 1 if an error occurs while writing.
{
	char* dataToBeSent = new char[data.length() + 1];
	for (int i = 0; i < data.length(); i++)
		dataToBeSent[i] = data[i];
	dataToBeSent[data.length()] = '\0';
	write(socket_descriptor, dataToBeSent, strlen(dataToBeSent) + 1);
}

bool receiveData(int socket_descriptor)
{
	char buffer[MAX];
	read(socket_descriptor, buffer, MAX);
	cout << buffer << '\n';
	return buffer[0] == 'I';
}

int enterIRC(int socket_descriptor)
{
	string username, password;
	
	cout << "Enter username: ";
	cin >> username;
	
	cout << "Enter password: ";
	cin >> password;

	sendData(socket_descriptor, username);
	sendData(socket_descriptor, password);
	
	if (receiveData(socket_descriptor) == 1)
		enterIRC(socket_descriptor);
	return 0;
}

int main(int argc, char *argv[])
{
	ios::sync_with_stdio(0);

	if (argc != 2)
	{
		cout << "Usage: " << argv[0] << " <Server IP address>\n";
		return -1;
	}
	strcpy(server_host, argv[1]);

	int registration_socket = initializeSocket(REGISTRATION_PORT), login_socket = initializeSocket(LOGIN_PORT);
	string username, password;
	char buffer[MAX];
	if (registration_socket == -1 || login_socket == -1)
		exit(0);

	cout << "IRC Channel\n1) Register\n2) Login\n3) Exit\n";
	while (1)
	{
		cout << "Enter the corresponding number: ";

		string menu_choice;
		cin >> menu_choice;

		if (menu_choice.length() > 1 || menu_choice[0] > '3')
		{
			cout << "Invalid option. Please try again.\n";
			continue;
		}
		else
		{
			if (menu_choice == "1")
				enterIRC(registration_socket);
			else if (menu_choice == "2")
				enterIRC(login_socket);
			else if (menu_choice == "3")
			{
				cout << "Bye!\n";
				exit(0);
			}
		}
	}
	return 0;
}
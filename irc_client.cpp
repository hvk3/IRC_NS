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

int sendData(string data, int socket_descriptor)
{
	const char* dataToBeSent = data.c_str();
	return write(socket_descriptor, dataToBeSent, strlen(dataToBeSent) + 1) == -1;
}

int receiveData(int socket_descriptor)
{
	char buffer[MAX];
	if (read(socket_descriptor, buffer, sizeof(buffer)) <= 0)
		return -1;
	cout << buffer << '\n';
	return buffer[0] == 'T' ? -1 : 0;
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

	int registration_socket = initializeSocket(REGISTRATION_PORT);
	if (registration_socket == -1)
		exit(0);
	int IRC_socket = initializeSocket(LOGIN_PORT);
	bool logged_in = false;
	string str;
	char buffer[MAX];
	if (registration_socket == -1 || IRC_socket == -1)
		exit(0);

	cout << "IRC Channel\n";
	cout << "Here\'s a list of commands to get you started:\n";
	cout << "/who: List all users online.\n";
	cout << "/register <username> <password>: Create a new account.\n";
	cout << "/login <username> <password>: Log in to IRC.\n";
	cout << "/msg <username> <message>: Send <message> to the specified <username>.\n";
	cout << "/join_group <group>: Join <group>.\n";
	cout << "/create_group <group>: Create a new <group>.\n";
	cout << "/msg_group <group> <message>: Broadcast <message> to <group>.\n";
	cout << "/send <username> <filename>: Send <filename> to <username>.\n";
	cout << "/recv: Receive the file that is being sent by someone.\n";
	cout << "/logout: Log out from IRC.\n";
	cout << "/exit_portal: Exit portal.\nEnter your command after \">>\".\n";
	
	while (1)
	{
		cout << ">> ";
		getline(cin, str);

		if (str.substr(0, 9) == "/register")	//works
		{
			sendData(str, registration_socket);
			receiveData(registration_socket);
		}
		else if (str == "/exit_portal")
		{
			if (logged_in)
			{
				sendData(str, IRC_socket);
				receiveData(IRC_socket);
			}
			else
				cout << "Goodbye!\n";
			exit(0);
		}
		else if (str == "/logout")
		{
			if (logged_in)
			{
				sendData(str, IRC_socket);
				receiveData(IRC_socket);
				logged_in = false;
			}
			else
				cout << "Please log in first.\n";
		}
		else if (str.substr(0, 6) == "/login")
		{
			sendData(str, IRC_socket);
			if (receiveData(IRC_socket) != -1)
				logged_in = true;
		}
		// else if 
	}
	return 0;
}
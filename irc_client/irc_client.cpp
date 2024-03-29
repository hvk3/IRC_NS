#include <iostream>
#include <vector>
#include <string>
#include <algorithm>
#include <stdio.h>
#include <stdlib.h>
#include <set>
#include <fstream>
#include <sstream>

#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>

using namespace std;

#define REGISTRATION_PORT 5555
#define IRC_PORT 5556
#define MAX 1024
#define FILE_SIZE 5000000

char server_host[MAX];
bool isLoggedIn = false, confirmation = false;
set <string> joinedGroups;
int counter = 0;

int initializeSocket(int port_number)
{
	int socket_descriptor = socket(AF_INET, SOCK_STREAM, 0);
	int enable = 1;
	if (setsockopt(socket_descriptor, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0)
    	return -1;
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

void* receive(void* sock)
{
	int socket_descriptor = *(int*)sock;
	char buffer[MAX], *lock;
	while (1)
	{
		if (!read(socket_descriptor, buffer, sizeof(buffer)))
			break;
		string message(buffer);

		if (message == "")
			message = "You have no unread messages.";
		cout << message << endl;

		if (message == "You have been successfully logged in.")
			isLoggedIn = true;

		else if (message == "You have been successfully logged out.")
			isLoggedIn = false;

		else if (message.substr(0, 13) == "You were sent")
		{
			int num_files = 0;
			for (int i = 14; message[i] >= '0' && message[i] <= '9'; i++)
				num_files = num_files * 10 + message[i] - '0';
			for (int i = 0; i < num_files; i++)
			{
				char input[FILE_SIZE];
				read(socket_descriptor, input, sizeof(input));
				string in(input), filename = "client_" + to_string(socket_descriptor) + "_" + to_string(counter);
				filename = in.substr(0, in.find("\n"));
				in.erase(0, in.find("\n"));
				counter++;
				fstream file;
				file.open(filename.c_str(), fstream::out);
				file.write(in.c_str(), in.length());
				file.close();
			}
		}
		else
			continue;
	}
}

int sendData(string message, int sock)
{
	const char* info = message.c_str();
	if (write(sock, info, strlen(info) + 1) == -1)
		return -1;
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

	int IRC_socket = initializeSocket(IRC_PORT);
	if (IRC_socket == -1)
		exit(0);
	int registration_socket = initializeSocket(REGISTRATION_PORT);
	if (registration_socket == -1)
		exit(0);
	string in;

	cout << "Hey there! Welcome to sampleIRC!\n";
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
	cout << "/recv_msg: Receive all messages that were sent while you were logged out.\n";
	cout << "/all_users: List all registered users.\n";
	cout << "/exit: Exit portal.\nYou can start now.\n";

	pthread_t IRC_receive, reg_receive;
	pthread_create(&reg_receive, NULL, receive, (void*)&registration_socket);
	pthread_create(&IRC_receive, NULL, receive, (void*)&IRC_socket);

	while (1)
	{
		getline(cin, in);

		if (in.substr(0, 9) == "/register")
		{
			if (sendData(in, registration_socket) == -1)
			{
				cout << "Connection with server is down. Exiting.\n";
				close(registration_socket);
				close(IRC_socket);
				exit(0);
			}
		}
		else if (in == "/exit")
		{
			if (isLoggedIn)
				cout << "Log out before exiting." << endl;
			else
			{
				cout << "Goodbye!" << endl;
				exit(0);
			}
		}
		else if (in == "/logout" || in.substr(0, 4) == "/msg" || in == "/recv_msg" || in.substr(0, 10) == "/msg_group" || in == "/who" || in == "/recv" || in == "/all_users")
		{
			if (isLoggedIn)
			{
				if (sendData(in, IRC_socket) == -1)
				{
					cout << "Connection with server is down. Exiting.\n";
					close(registration_socket);
					close(IRC_socket);
					exit(0);
				}
				if (in == "/who")
					cout << "List of online users:" << endl;
				if (in == "/all_users")
					cout << "All users:" << endl;
			}
			else
				cout << "Please log in first.\n";
		}
		else if (in.substr(0, 6) == "/login")
		{
			if (!isLoggedIn)
			{
				if (sendData(in, IRC_socket) == -1)
				{
					cout << "Connection with server is down. Exiting.\n";
					close(registration_socket);
					close(IRC_socket);
					exit(0);
				}
			}
			else
				cout << "Please log out first.\n";
		}
		else if (in.substr(0, 13) == "/create_group" || in.substr(0, 11) == "/join_group")
		{
			if (isLoggedIn)
			{
				if (sendData(in, IRC_socket) == -1)
				{
					cout << "Connection with server is down. Exiting.\n";
					close(registration_socket);
					close(IRC_socket);
					exit(0);
				}
				char temp[MAX], *lock;
				strcpy(temp, in.c_str());
				char *temp1 = strtok_r(temp, " ", &lock);
				string g(temp1);
				joinedGroups.insert(g);
			}
			else
				cout << "Please log in first.\n";
		}
		else if (in.substr(0, 5) == "/send")
		{
			if (isLoggedIn)
			{
				const char* temp = in.c_str();
				char x[strlen(temp) + 1], *lock;
				for (int i = 0; i < strlen(temp); i++)
					x[i] = temp[i];
				x[strlen(temp)] = '\0';

				char* temp1 = strtok_r(x, " ", &lock);
				temp1 = strtok_r(NULL, " ", &lock);
				if (!temp1)
				{
					cout << "No receiver specified." << endl;
					continue;
				}
				string receiver(temp1);
				temp1 = strtok_r(NULL, " ", &lock);
				if (!temp1)
				{
					cout << "No file specified." << endl;
					continue;
				}

				string fname(temp1);
				ifstream file;
				temp = fname.c_str();
				file.open(temp);
				if (!file)
				{
					cout << "Could not open the file. Check if the file name and path is correct." << endl;
					continue;
				}
				if (!file.is_open())
				{
					cout << "Incorrect filename provided. Please try again." << endl;
					continue;
				}

				if (sendData(in, IRC_socket) == -1)
				{
					cout << "Connection with server is down. Exiting.\n";
					close(registration_socket);
					close(IRC_socket);
					exit(0);
				}

				stringstream buf;
				buf << file.rdbuf() << '\0';
				file.close();
				string fdata(buf.str());
				if (buf.str().length() > 1000000)
				{
					cout << "The file is too large to be sent over this socket." << endl;
					continue;
				}
				string res = to_string(buf.str().length());
				if (sendData(res, IRC_socket) == -1)
				{
					cout << "Connection with server is down. Exiting.\n";
					close(registration_socket);
					close(IRC_socket);
					exit(0);
				}
				if (sendData(fdata, IRC_socket) == -1)
				{
					cout << "Connection with server is down. Exiting.\n";
					close(registration_socket);
					close(IRC_socket);
					exit(0);
				}
			}
			else
				cout << "Please log in first.\n";
		}
		else if (in.length() == 0)
			continue;
		else
			cout << "This command is not a part of the commands supported. Try again." << endl;
	}
	close(registration_socket);
	close(IRC_socket);
	return 0;
}
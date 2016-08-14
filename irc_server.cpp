#include <iostream>
#include <stdio.h>
#include <vector>
#include <string>
#include <map>
#include <set>
#include <algorithm>
#include <fstream>

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
#define MAX 500

map <string, pair <string, bool> > users;
map <string, pair <string, bool> >::iterator iter;
set <string> groups;

pthread_mutex_t lock;

int initializeSocket(int port_number)
{
	struct sockaddr_in addr;
	int socket_descriptor = socket(AF_INET, SOCK_STREAM, 0);

	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = htonl(INADDR_ANY);
	addr.sin_port = htons(port_number);

	bind(socket_descriptor, (sockaddr*)&addr, sizeof(addr));
	listen(socket_descriptor, 100);
	printf("Listening on port %d.\n", port_number);

	return socket_descriptor;
}

int loadUsers()
{
	string str;
	vector <string> records;
	ifstream file("records.txt");
	if (!file.good())
		return -1;
	while (file >> str)
		records.push_back(str);
	for (int i = 0; i < records.size(); i += 2)
		users[records[i]] = make_pair(records[i + 1], false);
	return 0;
}

void saveUsers()
{
	fstream file;
	file.open("records.txt", fstream::out);
	for (iter = users.begin(); iter != users.end(); iter++)
	{
		string temp = iter -> first + ' ' + iter -> second.first;
		file << temp << endl;
	}
	file.close();
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
	return 0;
}

void* registerUser(void* socket_descriptor)
{
	int sock = *(int*)socket_descriptor;
	char buffer[MAX];
	
	read(sock, buffer, sizeof(buffer));
	char *temp = strtok(buffer, " ");
	
	temp = strtok(NULL, " ");
	string username(temp);
	
	temp = strtok(NULL, " ");
	string password(temp);

	if (users.count(username) > 0)
	{
		string error = "This username is already in use. Try a different one.";
		sendData(error, sock);
	}
	else
	{
		pthread_mutex_lock(&lock);
		users[username] = make_pair(password, false);
		saveUsers();
		pthread_mutex_unlock(&lock);
		string success = "You have been successfully registered.";
		sendData(success, sock);
	}
}

vector <string> loggedInUsers()
{
	vector <string> loggedIn;
	for (iter = users.begin(); iter != users. end(); iter++)
		if (iter -> second.second)
			loggedIn.push_back(iter -> first);
	return loggedIn;
}

int main()
{
	ios::sync_with_stdio(0);

	int registration_socket = initializeSocket(REGISTRATION_PORT), IRC_socket = initializeSocket(IRC_PORT);
	int registration_fd = 0, IRC_fd = 0;

	loadUsers();

	while (1)
	{
		registration_fd = accept(registration_socket, (sockaddr*)NULL, NULL);
		IRC_fd = accept(IRC_socket, (sockaddr*)NULL, NULL);

		pthread_t IRC, registration, commands;
		pthread_create(&registration, NULL, registerUser, &registration_fd);
		pthread_create(&IRC, NULL, IRCUser, &IRC_fd);
		// pthread_create(&commands, NULL, functionHandler, &IRC_fd);
	}

	return 0;
}
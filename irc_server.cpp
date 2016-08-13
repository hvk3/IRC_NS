#include <iostream>
#include <stdio.h>
#include <vector>
#include <string>
#include <map>
#include <algorithm>

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
#define LOGIN_PORT 5556
#define MAX 500

map <string, pair <string, bool> > users;
map <string, pair <string, bool> >::iterator iter;
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

pair <string, string> parseInput(void* socket_descriptor)
{
	int socket_d = *(int*)socket_descriptor, pos = 0, pos1 = 0;
	string username = "", password = "";
	char buffer[MAX], input[500];
	memset(buffer, '0', sizeof(buffer));
	
	read(socket_d, buffer, sizeof(buffer));
	
	for (int i = 0; i < MAX; i++)
	{
		if (buffer[i] == '\0')
		{
			pos = i;
			break;
		}
		else
			username += buffer[i];
	}
	for (int i = pos + 1; i < MAX; i++)
	{
		if (buffer[i] == '\0')
		{
			pos1 = i;
			break;
		}
		else
			password += buffer[i];
	}
	if (pos1 == 0)
	{
		read(socket_d, buffer, sizeof(buffer));
		string temp(buffer);
		password = temp;
	}
	return make_pair(username, password);
}

void* registerUser(void* socket_descriptor)
{
	printf("New registration request received.\n");
	int socket_d = *(int*)socket_descriptor;
	char input[500];
	pair <string, string> p = parseInput(socket_descriptor);
	string username = p.first, password = p.second;
	
	printf("Username received: %s\nPassword received: %s\n", username.c_str(), password.c_str());

	if (users.count(username) > 0)
	{
		strcpy(input, "This username already exists. Try a different one.");
		write(socket_d, input, strlen(input) + 1);
		registerUser(socket_descriptor);
	}
	pthread_mutex_lock(&lock);
	users[username] = make_pair(password, false);
	strcpy(input, "You have been successfully registered.");
	write(socket_d, input, strlen(input) + 1);
	pthread_mutex_unlock(&lock);
	return 0;
}

void* loginUser(void* socket_descriptor)
{
	printf("\nNew login request received.\n");
	int socket_d = *(int*)socket_descriptor;
	char input[500];
	pair <string, string> p = parseInput(socket_descriptor);
	string username = p.first, password = p.second;

	printf("Username received: %s\nPassword received: %s\n", username.c_str(), password.c_str());

	if (users.count(username) == 0 || users[username].first != password)
	{
		strcpy(input, "Invalid credentials. Please try again.");
		write(socket_d, input, strlen(input) + 1);
		loginUser(socket_descriptor);
	}
	users[username].second = true;
	strcpy(input, "You have successfully logged in.");
	write(socket_d, input, strlen(input) + 1);
	return 0;
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

	int registration_socket = initializeSocket(REGISTRATION_PORT), login_socket = initializeSocket(LOGIN_PORT);
	int registration_fd = 0, login_fd = 0;

	while (1)
	{
		registration_fd = accept(registration_socket, (sockaddr*)NULL, NULL);
		login_fd = accept(login_socket, (sockaddr*)NULL, NULL);
		
		pthread_t login, registration;
		pthread_create(&registration, NULL,  registerUser, &registration_fd);
		pthread_create(&login, NULL,  loginUser, &login_fd);
	}

	return 0;
}
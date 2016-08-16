#include <iostream>
#include <stdio.h>
#include <vector>
#include <string>
#include <map>
#include <set>
#include <queue>
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
map <string, pair <string, bool> >::iterator users_iter;

map <string, int> user_to_conn;
map <int, string> conn_to_user;

map <string, vector <string> > groups;
map <string, queue <string> > personal_chat, group_chat;

int initializeSocket(int port_number)
{
	struct sockaddr_in addr;
	int socket_descriptor = socket(AF_INET, SOCK_STREAM, 0);

	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = htonl(INADDR_ANY);
	addr.sin_port = htons(port_number);

	bind(socket_descriptor, (sockaddr*)&addr, sizeof(addr));
	listen(socket_descriptor, 100);

	cout << "Listening on port " << port_number << endl;
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
	for (users_iter = users.begin(); users_iter != users.end(); users_iter++)
	{
		string temp = users_iter -> first + ' ' + users_iter -> second.first;
		file << temp << endl;
	}
	file.close();
}

int receiveData(int socket_descriptor)
{
	char buffer[MAX];
	if (read(socket_descriptor, buffer, sizeof(buffer)) <= 0)
		return -1;
	cout << buffer << endl;
	return 0;
}

void* registerUser(void* socket_descriptor)
{
	int sock = *(int*)socket_descriptor;
	char buffer[MAX];
	
	while (1)
	{
		if (read(sock, buffer, sizeof(buffer)) <= 0)
			return 0;
		char *temp = strtok(buffer, " ");

		temp = strtok(NULL, " ");
		string username(temp);
	
		temp = strtok(NULL, " ");
		string password(temp), res;

		if (users.count(username) > 0)
			res = "This username is already in use. Try a different one.";
		else
		{
			users[username] = make_pair(password, false);
			saveUsers();
			res = "You have been successfully registered. Login to continue.";
		}
		const char* info = res.c_str();
		if (write(sock, info, strlen(info) + 1) == -1)
			return 0;
	}
}

int loginUser(char *buffer, int sock)
{
	char *temp = strtok(buffer, " ");
	if (temp == NULL)
		return -1;

	temp = strtok(NULL, " ");
	if (temp == NULL)
		return -1;
	string username(temp);
	
	temp = strtok(NULL, " ");
	if (temp == NULL)
		return -1;
	string password(temp), res;

	int flag = 0;

	if (users.count(username) == 0 || users[username].first != password)
	{
		res = "These credentials are invalid. Try again.";
		flag = -1;
	}
	else
	{
		users[username].second = true;
		conn_to_user[sock] = username;
		user_to_conn[username] = sock;
		res = "You have been successfully logged in.";
	}
	const char* info = res.c_str();
	if (write(sock, info, strlen(info) + 1) == -1)
		return -1;
	return flag;
}

void logoutUser(int sock)
{
	string username = conn_to_user[sock];
	users[username].second = false;
	user_to_conn.erase(username);
	conn_to_user.erase(sock);
	string success = "You have been successfully logged out.";
	const char* info = success.c_str();
	if (write(sock, info, strlen(info) + 1) == -1)
		return;
}

void loggedInUsers(int sock)
{
	vector <string> loggedIn;
	for (users_iter = users.begin(); users_iter != users.end(); users_iter++)
		if (users_iter -> second.second)
			loggedIn.push_back(users_iter -> first);
	string users = "";
	for (int i = 0; i < loggedIn.size(); i++)
	{
		users += loggedIn[i];
		if (i != loggedIn.size() - 1)
			users += "\n";
	}
	const char* info = users.c_str();
	if (write(sock, info, strlen(info) + 1) == -1)
		return;
}

void createGroup(char *buffer, int sock)
{
	char *temp = strtok(buffer, " ");
	temp = strtok(NULL, " ");
	string group(temp), res;

	if (groups.count(group) > 0)
		res = "This group already exists. Try joining it.";
	else
	{
		vector <string> members;
		members.push_back(conn_to_user[sock]); 
		groups[group] = members;
		res = "This group has been made, and you have been added to it.";
	}

	const char* info = res.c_str();
	if (write(sock, info, strlen(info) + 1) == -1)
		return;
}

void joinGroup(char *buffer, int sock)
{
	char *temp = strtok(buffer, " ");
	temp = strtok(NULL, " ");
	string group(temp), res;

	if (groups.count(group) == 0)
		res = "This group does not exist. Try creating it.";
	else
	{
		string username = conn_to_user[sock];
		bool flag = false;
		for (int i = 0; i < groups[group].size(); i++)
			if (groups[group][i] == username)
			{
				flag = 1;
				break;
			}
		if (!flag)
		{
			groups[group].push_back(username);
			res = "You have been added to this group.";
		}
		else
			res = "You are already in this group.";
	}
	const char* info = res.c_str();
	if (write(sock, info, strlen(info) + 1) == -1)
		return;
}

void messageUser(char *buffer, int sock)
{
	char *temp = strtok(buffer, " ");
	if (temp == NULL)
		return;
	temp = strtok(NULL, " ");
	if (temp == NULL)
		return;
	string receiver(temp);
	temp = strtok(NULL, " ");
	if (temp == NULL)
		return;
	string message(temp);
	while (temp = strtok(NULL, " "))
	{
		string x(temp);
		message += " " + x;
	}
	message = conn_to_user[sock] + ": " + message;

	if (users.count(receiver) == 0)
	{
		message = "This user doesn\'t exist. Select a different user to message.";
		const char* info = message.c_str();
		if (write(sock, info, strlen(info) + 1) == -1)
			return;
	}

	if (users[receiver].second)
	{
		while (users[receiver].second && !personal_chat[receiver].empty())
		{
			string temp = personal_chat[receiver].front();
			personal_chat[receiver].pop();
			const char* info = temp.c_str();
			if (write(user_to_conn[receiver], info, strlen(info) + 1) == -1)
				return;
		}
		const char* info = message.c_str();
		if (write(user_to_conn[receiver], info, strlen(info) + 1) == -1)
			return;
	}
	else
		personal_chat[receiver].push(message);
}

void messageGroup(char *buffer, int sock)
{
	char *temp = strtok(buffer, " ");
	if (temp == NULL)
		return;
	temp = strtok(NULL, " ");
	if (temp == NULL)
		return;
	string groupname(temp);
	temp = strtok(NULL, " ");
	if (temp == NULL)
		return;
	string message(temp);
	while (temp = strtok(NULL, " "))
	{
		string x(temp);
		message += " " + x;
	}
	message = conn_to_user[sock] + ": " + message + " (sent to group " + groupname + ")";

	if (groups.count(groupname) == 0)
	{
		message = "This group doesn\'t exist. Select a different group to message, or create a new group.";
		const char* info = message.c_str();
		if (write(sock, info, strlen(info) + 1) == -1)
			return;
	}

	else
	{
		vector <string> members = groups[groupname];
		for (int i = 0; i < members.size(); i++)
		{
			if (users[members[i]].second)
			{
				const char* info = message.c_str();
				if (write(user_to_conn[members[i]], info, strlen(info) + 1) == -1)
					return;
			}
			else
				personal_chat[members[i]].push(message);
		}
	}
}


void clearPersonalMessageQueue(int sock)
{
	string username = conn_to_user[sock], temp = "";
	queue <string> messages = personal_chat[username];
	while (!messages.empty())
	{
		temp += messages.front();
		messages.pop();
		if (messages.size())
			temp += '\n';
	}
	const char* info = temp.c_str();
	if (write(sock, info, strlen(info) + 1) == -1)
		return;
}

void *functionHandler(void* socket_descriptor)
{
	int sock = *(int*)socket_descriptor;
	char buffer[MAX], input[MAX], temp_buffer[MAX];
	
	while (1)
	{
		if (!read(sock, buffer, sizeof(buffer)))
			return 0;
	
		strcpy(temp_buffer, buffer);
		char *temp = strtok(temp_buffer, " ");
		string command(temp);
		cout << "Command received: " << command << endl;
		if (command == "/login")
			loginUser(buffer, sock);
		else if (command == "/logout")
			logoutUser(sock);
		else if (command == "/who")
			loggedInUsers(sock);
		else if (command == "/create_group")
			createGroup(buffer, sock);
		else if (command == "/join_group")
			joinGroup(buffer, sock);
		else if (command == "/msg")
			messageUser(buffer, sock);
		else if (command == "/msg_group")
			messageGroup(buffer, sock);
		else if (command == "/recv_msg")
			clearPersonalMessageQueue(sock);
	}
	close(sock);
}

int main()
{
	ios::sync_with_stdio(0);

	int registration_socket = initializeSocket(REGISTRATION_PORT);
	if (registration_socket == -1)
		exit(0);
	int IRC_socket = initializeSocket(IRC_PORT);
	int registration_fd = 0, IRC_fd = 0, flag = 1;

	loadUsers();

	cout << endl;

	pthread_t IRC, registration;

	while (1)
	{
		IRC_fd = accept(IRC_socket, (sockaddr*)NULL, NULL);
		registration_fd = accept(registration_socket, (sockaddr*)NULL, NULL);
		
		pthread_create(&registration, NULL, registerUser, &registration_fd);
		pthread_create(&IRC, NULL, functionHandler, &IRC_fd);
	}

	close(registration_socket);
	close(IRC_socket);
	return 0;
}
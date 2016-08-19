#include <iostream>
#include <stdio.h>
#include <vector>
#include <string>
#include <map>
#include <set>
#include <queue>
#include <algorithm>
#include <fstream>
#include <sstream>

#include <sys/socket.h>
#include <sys/types.h>
#include <sys/sendfile.h>
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

map <string, pair <string, bool> > users;
map <string, pair <string, bool> >::iterator users_iter;

map <string, int> user_to_conn;
map <int, string> conn_to_user;

map <string, vector <string> > groups;
map <string, queue <string> > personal_chat, group_chat;

map <string, vector <pair <string, string> > > files;
map <string, vector <pair <string, string> > >::iterator file_iter;

int counter = 0;

pthread_mutex_t save_users, logout_user, login_user, create_group, personal_message, group_message, clear_queue, recv_file, send_file, logged_in_users, join_group;

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
	char buffer[MAX], *lock;
	
	while (1)
	{
		pthread_mutex_lock(&save_users);
		if (read(sock, buffer, sizeof(buffer)) <= 0)
		{
			pthread_mutex_unlock(&save_users);
			return 0;
		}
		char *temp = strtok_r(buffer, " ", &lock);
		if (!temp)
			return 0;

		temp = strtok_r(NULL, " ", &lock);
		if (!temp)
		{
			char message[] = "No username provided for registration.";
			write(sock, message, strlen(message) + 1);
			pthread_mutex_unlock(&save_users);
			return 0;
		}
		string username(temp);
	
		temp = strtok_r(NULL, " ", &lock);
		if (!temp)
		{
			char message[] = "No password provided for registration.";
			write(sock, message, strlen(message) + 1);
			pthread_mutex_unlock(&save_users);
			return 0;
		}
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
		write(sock, info, strlen(info) + 1);
		pthread_mutex_unlock(&save_users);
	}
}

int loginUser(char *buffer, int sock)
{
	char *lock;
	pthread_mutex_lock(&login_user);
	char *temp = strtok_r(buffer, " ", &lock);

	temp = strtok_r(NULL, " ", &lock);
	if (!temp)
	{
		char message[] = "No username provided for logging in.";
		write(sock, message, strlen(message) + 1);
		pthread_mutex_unlock(&login_user);
		return -1;
	}
	string username(temp);
	
	temp = strtok_r(NULL, " ", &lock);
	if (!temp)
	{
		char message[] = "No password provided for logging in.";
		write(sock, message, strlen(message) + 1);
		pthread_mutex_unlock(&login_user);
		return -1;
	}
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
	write(sock, info, strlen(info) + 1);
	pthread_mutex_unlock(&login_user);
	return flag;
}

void logoutUser(int sock)
{
	pthread_mutex_lock(&logout_user);
	string username = conn_to_user[sock];
	users[username].second = false;
	user_to_conn.erase(username);
	conn_to_user.erase(sock);
	string success = "You have been successfully logged out.";
	const char* info = success.c_str();
	write(sock, info, strlen(info) + 1);
	pthread_mutex_unlock(&logout_user);
}

void loggedInUsers(int sock)
{
	pthread_mutex_lock(&logged_in_users);
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
	write(sock, info, strlen(info) + 1);
	pthread_mutex_unlock(&logged_in_users);
}

void createGroup(char *buffer, int sock)
{
	pthread_mutex_lock(&create_group);
	char *lock;
	char *temp = strtok_r(buffer, " ", &lock);
	temp = strtok_r(NULL, " ", &lock);
	if (!temp)
	{
		char message[] = "No name provided for creating the group.";
		write(sock, message, strlen(message) + 1);
		pthread_mutex_unlock(&create_group);
		return;
	}
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
	write(sock, info, strlen(info) + 1);
	pthread_mutex_unlock(&create_group);
}

void joinGroup(char *buffer, int sock)
{
	pthread_mutex_lock(&join_group);
	char *lock;
	char *temp = strtok_r(buffer, " ", &lock);
	temp = strtok_r(NULL, " ", &lock);
	if (!temp)
	{
		char message[] = "No group name provided to join into.";
		write(sock, message, strlen(message) + 1);
		pthread_mutex_unlock(&join_group);
		return;
	}
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
	write(sock, info, strlen(info) + 1);
	pthread_mutex_unlock(&join_group);
}

void messageUser(char *buffer, int sock)
{
	pthread_mutex_lock(&personal_message);
	char *lock;
	char *temp = strtok_r(buffer, " ", &lock);

	temp = strtok_r(NULL, " ", &lock);
	if (!temp)
	{
		char message[] = "No receiver provided.";
		write(sock, message, strlen(message) + 1);
		pthread_mutex_unlock(&personal_message);
		return;
	}
	string receiver(temp);
	temp = strtok_r(NULL, " ", &lock);
	if (!temp)
	{
		char message[] = "No message to send.";
		write(sock, message, strlen(message) + 1);
		pthread_mutex_unlock(&personal_message);
		return;
	}
	string message(temp);
	while (temp = strtok_r(NULL, " ", &lock))
	{
		string x(temp);
		message += " " + x;
	}
	message = conn_to_user[sock] + ": " + message;

	if (users.count(receiver) == 0)
	{
		message = "This user doesn\'t exist. Select a different user to message.";
		const char* info = message.c_str();
		write(sock, info, strlen(info) + 1);
		pthread_mutex_unlock(&personal_message);
		return;
	}

	if (users[receiver].second)
	{
		while (users[receiver].second && !personal_chat[receiver].empty())
		{
			string temp = personal_chat[receiver].front();
			personal_chat[receiver].pop();
			const char* info = temp.c_str();
			write(user_to_conn[receiver], info, strlen(info) + 1);
			pthread_mutex_unlock(&personal_message);
			return;
		}
		const char* info = message.c_str();
		write(user_to_conn[receiver], info, strlen(info) + 1);
		pthread_mutex_unlock(&personal_message);
		char temp[] = "Your messages have been successfully delivered.";
		write(sock, temp, strlen(temp) + 1);
		return;
	}
	else
	{
		personal_chat[receiver].push(message);
		char message[] = "The user you messaged is currently offline. He can view your message after logging in.";
		write(sock, message, strlen(message) + 1);
	}
	pthread_mutex_unlock(&personal_message);
}

void messageGroup(char *buffer, int sock)
{
	pthread_mutex_lock(&group_message);
	char *lock;
	char *temp = strtok_r(buffer, " ", &lock);
	temp = strtok_r(NULL, " ", &lock);
	if (!temp)
	{
		char message[] = "No group name provided for messaging.";
		write(sock, message, strlen(message) + 1);
		pthread_mutex_unlock(&group_message);
		return;
	}
	string groupname(temp);
	temp = strtok_r(NULL, " ", &lock);
	if (!temp)
	{
		char message[] = "No message to send.";
		write(sock, message, strlen(message) + 1);
		pthread_mutex_unlock(&group_message);
		return;
	}
	string message(temp);
	while (temp = strtok_r(NULL, " ", &lock))
	{
		string x(temp);
		message += " " + x;
	}
	message = conn_to_user[sock] + ": " + message + " (sent to group " + groupname + ")";
	if (groups.count(groupname) == 0)
	{
		message = "This group doesn\'t exist. Select a different group to message, or create a new group.";
		const char* info = message.c_str();
		write(sock, info, strlen(info) + 1);
		pthread_mutex_unlock(&group_message);
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
				{
					pthread_mutex_unlock(&group_message);
					return;
				}
			}
			else
				personal_chat[members[i]].push(message);
		}
	}
	pthread_mutex_unlock(&group_message);
}


void clearPersonalMessageQueue(int sock)
{
	pthread_mutex_lock(&clear_queue);
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
	write(sock, info, strlen(info) + 1);
	pthread_mutex_unlock(&clear_queue);
	return;
}

void receiveFile(char* buffer, int sock)
{
	pthread_mutex_lock(&recv_file);

	char recd_file[MAX], *lock;
	char *temp = strtok_r(buffer, " ", &lock);

	temp = strtok_r(NULL, " ", &lock);
	string receiver(temp);
	temp = strtok_r(NULL, " ", &lock);
	string filename(temp), servername = "server_file_" + to_string(sock) + to_string(counter), extension = "", tag_along = "";
	read(sock, recd_file, sizeof(recd_file));
	
	int filesize = 0, i;
	for (i = 0; recd_file[i] >= '0' && recd_file[i] <= '9'; i++)
		filesize = filesize * 10 + recd_file[i] - '0';
	for (int j = i + 1; j < MAX; j++)
		tag_along += recd_file[j];
	char fc[filesize + 1];
	read(sock, fc, sizeof(fc));
	string contents(fc);
	contents = tag_along + contents + fc[strlen(fc) - 1];

	for (int i = 0; i < filename.length(); i++)
		if (filename[i] == '.')
			for (int j = i; j < filename.length(); j++)
				extension += filename[j];
	servername += extension;
	if (files.count(receiver) == 0)
	{
		vector <pair <string, string> > temp;
		temp.push_back(make_pair(servername, filename));
		files[receiver] = temp;
	}
	else
		files[receiver].push_back(make_pair(servername, filename));
	counter++;
	fstream file;
	file.open(servername.c_str(), fstream::out);
	file.write(contents.c_str(), strlen(contents.c_str() + 1));
	file.close();
	string message = "File received by server.";
	const char* info = message.c_str();
	if (write(sock, info, strlen(info) + 1) == -1)
	{
		pthread_mutex_unlock(&recv_file);
		return;
	}
	pthread_mutex_unlock(&recv_file);
}

void sendFile(int sock)
{
	pthread_mutex_lock(&send_file);
	string receiver = conn_to_user[sock], msg, servername, filename;
	bool flag = 0;
	if (files.count(receiver) == 0)
	{
		msg = "You have no files to receive.";
		flag = 1;
	}
	else
	{
		msg = "You were sent " + to_string(files[receiver].size());
		msg += (files[receiver].size() == 1 ? " file." : " files.");
	}
	const char* info = msg.c_str();
	if (write(sock, info, strlen(info) + 1) == -1 || flag)
	{
		pthread_mutex_unlock(&send_file);
		return;
	}
	ifstream file;
	vector <pair <string, string> > t = files[receiver]; 
	for (int i = 0; i < t.size(); i++)
	{
		servername = t[i].first, filename = t[i].second;
		file.open(servername);
		string x = to_string(sock) + filename;
		stringstream buf;
		buf << file.rdbuf();
		file.close();
		string fdata(buf.str());
		fdata = x + "\n" + fdata;
		info = fdata.c_str();
		if (write(sock, info, strlen(info) + 1) == -1 || flag)
		{
			pthread_mutex_unlock(&send_file);
			return;
		}
		remove(servername.c_str());
	}
	files.erase(receiver);
	pthread_mutex_unlock(&send_file);
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
		char *lock;
		char *temp = strtok_r(temp_buffer, " ", &lock);
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
		else if (command == "/send")
			receiveFile(buffer, sock);
		else if (command == "/recv")
			sendFile(sock);
	}
	cout << "Closing connection." << endl;
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
# IRC_NS
A simple IRC chat portal made as an assignment for the Network Security (CSE 550) Course.

The following commands are supported by the client:
/who: List all users online.
/register <username> <password>: Create a new account with the given credentials.
/login <username> <password>: Log in to IRC.
/msg <username> <message>: Send <message> to the specified <username>.
/join_group <group>: Join <group>.
/create_group <group>: Create a new <group>.
/msg_group <group> <message>: Broadcast <message> to <group>.
/send <username> <filename>: Send <filename> to <username>.
/recv: Receive the file that is being sent by someone.
/logout: Log out from IRC.
/recv_msg: Receive all messages that were sent while you were logged out.
/exit: Exit portal.

Assumptions:
Files to be shared are specifically text-based, as all other formats are converted to binaries and are not handled at the moment.
Files are to be sent and received within users of the IRC, and not random users.

Working:
Two threads working on the server for each new connection received; one thread handles all incoming registrations and responses, while the other thread handles all other commands.
Similarly, there are two threads on the client; one for sending registration requests, and one for handling user commands.

For running the programs:
Server: make && ./server
Client: make && ./client <IP of server>

Error handling:
Using threads and locks for handling multiple connections.
Resolving errors coming up while reading from and writing to socket buffers. This was due to the standard behaviour of TCP merging various write commands into one until the buffer capacity was exceeded.
File handling was a major problem, since the size of the file to be sent, and its extension, were both to be provided. A solution was found by clubbing these with the data being sent, and resolving them appropriately at the receiving end.

Example:
Hey there! Welcome to sampleIRC!
Here's a list of commands to get you started:
/who: List all users online.
/register <username> <password>: Create a new account.
/login <username> <password>: Log in to IRC.
/msg <username> <message>: Send <message> to the specified <username>.
/join_group <group>: Join <group>.
/create_group <group>: Create a new <group>.
/msg_group <group> <message>: Broadcast <message> to <group>.
/send <username> <filename>: Send <filename> to <username>.
/recv: Receive the file that is being sent by someone.
/logout: Log out from IRC.
/recv_msg: Receive all messages that were sent while you were logged out.
/exit: Exit portal.
You can start now.
/register hvk 1243 
You have been successfully registered. Login to continue.
/login hvk 1243
You have been successfully logged in.
/msg hvk hey there
hvk: hey there
/logout
You have been successfully logged out.
/register dummyuser1 1234
You have been successfully registered. Login to continue.
/login dummyuser 1234
These credentials are invalid. Try again.
/login dummyuser1 1234
You have been successfully logged in.
/msg hvk hola
/send hvk Makefile  
/logout
File received by server.
/logout
You have been successfully logged out.
/login hvk 1243
You have been successfully logged in.
/recv_msg
dummyuser1: hola
/recv
You were sent 1 file.
/logout
You have been successfully logged out.
/exit
Goodbye!

Lingering bugs:
Occasionally, the server might not send a response back immediately, and these get delayed by one command.

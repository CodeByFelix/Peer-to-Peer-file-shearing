#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include <iostream>
#include <string>
#include <sstream>
#include <fstream>
#include <WinSock2.h>

#pragma comment (lib, "ws2_32.lib")


class SocketServer
{
private:
	int portNumber;
	WSADATA wsaData;
	SOCKET server, client;
	sockaddr_in serverAddr, clientAddr;
	static const int bufferSize = 102400;
	

public:
	explicit SocketServer(const int& _portNumber) : portNumber(_portNumber) {}

	~SocketServer()
	{
		std::cout << "Socket Closing and Cleanup.\n";
		closesocket(server);
		closesocket(client);
		WSACleanup();
	}
	//Creating Socket
	void creatSocket()
	{
		if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
		{
			throw std::runtime_error("Winsock did not startup\n");
		}

		server = socket(AF_INET, SOCK_STREAM, 0);
		if (server == INVALID_SOCKET)
		{
			WSACleanup();
			throw std::runtime_error("Socket Creation Failed\n");
		}

		serverAddr.sin_family = AF_INET;
		serverAddr.sin_addr.s_addr = INADDR_ANY;
		serverAddr.sin_port = htons(portNumber);
		if (bind(server, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR)
		{
			closesocket(server);
			WSACleanup();
			throw std::runtime_error("Socket Binding Failed\n");
		}

		std::cout << "Socket Created Successfult\n";
	}

	void acceptConnection()
	{
		listen(server, SOMAXCONN);
		int clientAddrLen = sizeof(clientAddr);
		client = accept(server, (sockaddr*)&clientAddr, &clientAddrLen);
		if (client == INVALID_SOCKET)
		{
			closesocket(server);
			WSACleanup();
			throw std::runtime_error("Connection to client, Failed\n");
		}
		std::cout << "Connection to Client: " << client << " Successful.\n";
	}

	//Open Files
	void sendFile(const std::string& fileName)
	{
		std::string buffer(bufferSize, '\0');
		std::ifstream file(fileName, std::ios::binary);
		if (!file)
			throw std::runtime_error("File Fail to open\n");
        
        file.seekg(0, std::ios::end);
        std::streampos fileSize = file.tellg();
        file.seekg(0, std::ios::beg);
        std::cout << "File Size is " << fileSize << "bytes.\n"; 
        std::ostringstream oss;
        oss << fileName << "-" << fileSize;
        std::string header = oss.str();
		if (send(client, header.c_str(), header.length() +1, 0) == SOCKET_ERROR)
			throw std::runtime_error("Error sending file Name.\n");

        unsigned long byteSent = 0;
        int percent;
		while (!file.eof())
		{
			file.read(&buffer[0], bufferSize);
			int bytesRead = file.gcount();
            byteSent += bytesRead;
            percent = (static_cast<float> (byteSent)/fileSize) * 100;
			std::cout << "Sending " << percent << "%" << std::endl;
			if (send(client, buffer.c_str(), bytesRead, 0) == SOCKET_ERROR)
				throw std::runtime_error("Error Sending File.\n");
            int bytesReceived = recv(client, &buffer[0], bufferSize, 0);
            if(!(bytesReceived > 0))
                throw std::runtime_error("Receiving Acknoledgement failed.\n client disconnected");
		}
		
        buffer = "file has endded";
		if (send(client, buffer.c_str(), 15, 0) == SOCKET_ERROR)
			throw std::runtime_error("Error Sending File.\n");
        int bytesReceived = recv(client, &buffer[0], bufferSize, 0);
        if(!(bytesReceived > 0))
            throw std::runtime_error("Receiving Acknoledgement failed.\n client disconnected");

		std::cout << "File sent Successfully.\n";
	}

	//Receive from client
	[[nodiscard]] const std::string receive()
	{
		std::string buffer(bufferSize, '\0');
		
		int bytesRead = recv(client, &buffer[0], bufferSize, 0);
		if (bytesRead > 0)
		{
			std::cout << "Message received is: " << buffer << std::endl;
			return buffer;
		}
		else
			throw std::runtime_error("Client Disconnected\n");

	}
};


class SocketClient
{
private:
	int portNumber;
	std::string ipAddress;
	WSADATA wsaData;
	SOCKET client;
	sockaddr_in serverAddr;
	static const int bufferSize = 102400;
	

public:
	explicit SocketClient(const int& _portNumber, const std::string& _ipAddress) :
		portNumber(_portNumber),
		ipAddress(_ipAddress)
	{}
	~SocketClient()
	{
		std::cout << "Socket Closing and Cleanup.\n";
		closesocket(client);
		WSACleanup();
	}

	//Creating Socket
	void creatSocket()
	{
		if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
		{
			throw std::runtime_error("Winsock did not startup\n");
		}

		client = socket(AF_INET, SOCK_STREAM, 0);
		if (client == INVALID_SOCKET)
		{
			WSACleanup();
			throw std::runtime_error("Socket Creation Failed\n");
		}

		serverAddr.sin_family = AF_INET;
		serverAddr.sin_addr.s_addr = inet_addr(ipAddress.c_str());
		serverAddr.sin_port = htons(portNumber);

		if (connect(client, (sockaddr*)&serverAddr, sizeof(serverAddr)) < 0)
		{
			closesocket(client);
			WSACleanup();
			throw std::runtime_error("Faile to connect to server\n");
		}
		std::cout << "Client Connected to server.\n";
	}

	void receiveFile()
	{
		std::string buffer(bufferSize, '\0');
		int bytesReceived;
		std::string fileName;
		bytesReceived = recv(client, &buffer[0], bufferSize, 0);
		if (bytesReceived <= 0)
			throw std::runtime_error("Server Disconnected!!!\n");

        int pos = buffer.find('-', 0);
		fileName = buffer.substr(0, pos);
        std::string fsize = buffer.substr(pos+1, bytesReceived);
        unsigned long fileSize = std::stoul(fsize);
		std::cout << "Trying to create file: " << fileName << " of size " 
                    << fileSize << "bytes." << std::endl;
		std::ofstream file(fileName, std::ios::binary);
		if (!file)
			throw std::runtime_error("File could not be created\n");

        bool endOfFile = false;
        unsigned long t_bytesReceived = 0;
        int percent;
        while(!endOfFile)
        {
            bytesReceived = recv(client, &buffer[0], bufferSize, 0);
            t_bytesReceived += bytesReceived;
            percent = (static_cast<float> (t_bytesReceived)/fileSize) * 100;
			std::cout << "Receiving File... " << percent << "%.\n";
            if(buffer.contains("file has endded"))
                endOfFile = true;
			if (bytesReceived > 0 && !endOfFile)
				file.write(buffer.c_str(), bytesReceived);
            std::string message = "ACK";
            if (send(client, message.c_str(), message.length()+1, 0) == SOCKET_ERROR)
				throw std::runtime_error("Error Sending Acknoledgment.\n");  
        }

		std::cout << "File received successfuly.\n";
		file.close();
	}

	void sendOut(const std::string& message)
	{
		if (send(client, message.c_str(), message.length(), 0) == SOCKET_ERROR)
		{
			throw std::runtime_error("Fail to send Data\n");
		}
		std::cout << "Message sent\n";
	}

};


int prompt()
{
	int input;
	std::cout << "\n\t\tSimple peer-to-peer file transfer App.\n"
		<< "\tEnter Action.\n"
		<< "\t1 -> Create\n"
		<< "\t2 -> Connect\n"
		<< "\t3 -> Exit.\n\n"
		<< "Input: ";
	std::cin >> input;

	return input;
}

int main()
{
	int action;
	int portNumber;
	std::string ipAddress;
	do
	{
		action = prompt();
		switch (action)
		{
		case 1:
		{
			std::cout << "\nEnter Prot to creat server: ";
			std::cin >> portNumber;

			SocketServer server(portNumber);
			try
			{
				server.creatSocket();
				try
				{
					std::string fileName;
					std::cout << "Waiting for connection\n";
					server.acceptConnection();
					bool connection = true;

					while (connection)
					{
						std::cout << "\nEnter File Name to send: ";
						//std::cin >> fileName;
                        std::getline (std::cin, fileName);
						try
						{
							server.sendFile(fileName);
						}
						catch (std::runtime_error& e) { std::cout << e.what(); }
					}
					

				}
				catch (const std::runtime_error& e) { std::cout << e.what(); }
			}
			catch (const std::runtime_error& e) { std::cout << e.what(); }
			break;
		}
		case 2:
		{
			std::cout << "\nEnter Server port Number: ";
			std::cin >> portNumber;
			std::cout << "\nEnter Server IP Address: ";
			std::cin >> ipAddress;
			SocketClient client(portNumber, ipAddress);
			try
			{
				client.creatSocket();
				
				bool connection = true;
				while (connection)
				{
					try
					{
						std::cout << "Waiting to receive file.";
						client.receiveFile();
					}
					catch (std::runtime_error& e) 
					{ 
						std::cout << e.what(); 
						//if (e.what() == "Server Disconnected!!!\n")
						connection = false;
					}
				}
			}
			catch (std::runtime_error& e) { std::cout << e.what(); }
			break;
		}
		case 3:
		{
			std::cout << "Exiting code !!!";
			break;
		}
		default:
		{
			std::cout << "Wrong input!!! Try again\n";
			break;
		}
		}

	} while (action != 3);
	

	return 0;
}

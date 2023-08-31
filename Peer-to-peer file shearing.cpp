#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include <iostream>
#include <string>
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
	static const int bufferSize = 1024;
	char buffer[bufferSize];

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
		std::ifstream file(fileName, std::ios::binary);
		if (!file)
			throw std::runtime_error("File Fail to open\n");

		if (send(client, fileName.c_str(), fileName.length() +1, 0) == SOCKET_ERROR)
			throw std::runtime_error("Error sending file Name.\n");

		while (!file.eof())
		{
			file.read(buffer, bufferSize);
			int bytesRead = file.gcount();
			std::cout << "Sending " << bytesRead << "Bytes." << std::endl;
			if (send(client, buffer, bytesRead, 0) == SOCKET_ERROR)
				throw std::runtime_error("Error Sending File.\n");
		}
		std::cout << "File sent Successfully.\n";
	}

	//Receive from client
	[[nodiscard]] const auto& receive()
	{
		ZeroMemory(buffer, bufferSize);
		int bytesRead = recv(client, buffer, bufferSize, 0);
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
	//char ipAddress [20];
	std::string ipAddress;
	WSADATA wsaData;
	SOCKET client;
	sockaddr_in serverAddr;
	static const int bufferSize = 1024;
	char buffer[bufferSize];

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
		int bytesReceived;
		std::string fileName;
		bytesReceived = recv(client, buffer, bufferSize, 0);
		if (bytesReceived <= 0)
			throw std::runtime_error("Server Disconnected!!!\n");

		fileName = buffer;
		std::cout << "Trying to create file: " << fileName << std::endl;
		std::ofstream file(fileName, std::ios::binary);
		if (!file)
			throw std::runtime_error("File could not be created\n");

		
		do
		{
			bytesReceived = recv(client, buffer, bufferSize, 0);
			std::cout << "Receiving File. bytes received is: " << bytesReceived << "bytes.\n";
			if (bytesReceived > 0)
				file.write(buffer, bytesReceived);

		} while (bytesReceived == 1024);

		std::cout << "File received successfult.\n";
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


inline int prompt()
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
						std::cin >> fileName;
						try
						{
							server.sendFile(fileName);
							try
							{
								const auto& message = server.receive();
								std::cout << "Response from connected device: " << message << std::endl;
							}
							catch (std::runtime_error& e) 
							{ 
								std::cout << e.what();
								connection = false;
							}
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
						try
						{
							client.sendOut("File received Successfully!");
						}
						catch (std::runtime_error& e) { std::cout << e.what(); }

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

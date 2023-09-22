#include <iostream>
#include <string>
#include <WS2tcpip.h>
#include <sstream>

#pragma comment (lib, "ws2_32.lib")

using namespace std;

void clientBroadcast(fd_set master, SOCKET listening, SOCKET socket, string message);

void main() {
	// Ініціалізація winsock
	WSADATA wsData;
	WORD ver = MAKEWORD(2, 2);

	int wsOk = WSAStartup(ver, &wsData);
	if (wsOk != 0) {
		cerr << "Error during initializing winsock" << endl;
		return;
	}

	// Створюємо сокет
	SOCKET listening = socket(AF_INET, SOCK_STREAM, 0);
	if (listening == INVALID_SOCKET) {
		cerr << "Error during opening listening socket" << endl;
		return;
	}

	// Привязуємо ip адресу і порт до сокету
	sockaddr_in hint;
	hint.sin_family = AF_INET;
	hint.sin_port = htons(54000);
	hint.sin_addr.S_un.S_addr = INADDR_ANY;

	bind(listening, (sockaddr*)&hint, sizeof(hint));

	// Заставляємо сокет слухати цю адресу і порт
	listen(listening, SOMAXCONN);

	fd_set master;
	FD_ZERO(&master);

	FD_SET(listening, &master);

	while (true) {
		// копіюємо наш master тому що select змінює дані в ньому, а ми не хочемо втрачати наш listening socket
		fd_set copy = master;
		int socketCount = select(0, &copy, nullptr, nullptr, nullptr);

		for (int i = 0; i < socketCount; i++) {
			SOCKET socket = copy.fd_array[i];
			if (socket == listening) {
				// Приймаємо конекшн від клієнта
				SOCKET client = accept(listening, nullptr, nullptr);
				// Записуємо його в наш fd_set
				FD_SET(client, &master);

				cout << "Client " << client << " connected\r\n";
				clientBroadcast(master, listening, client, "connected");

				// Відсилаємо привітання з підключенням
				string welcomeMessage = "Welcome!\r\n";
				send(client, welcomeMessage.c_str(), welcomeMessage.size() + 1, 0);
			}
			else {
				char buffer[4096];
				ZeroMemory(buffer, 4096);

				// Отримуємо повідомлення від клієнта, якщо 0, то значить що клієнт відключився, очищаємо його
				int receivedBytes = recv(socket, buffer, 4096, 0);
				if (receivedBytes <= 0) {
					cout << "Client " << socket << " disconnected\r\n";
					clientBroadcast(master, listening, socket, "disconnected");

					closesocket(socket);
					FD_CLR(socket, &master);
				}
				else {
					// Відправляємо всім клієнтам повідомлення яке надіслав вхідний клієнт
					clientBroadcast(master, listening, socket, buffer);
				}
			}
		}
	}

	// Виключення winsock
	WSACleanup();
}

void clientBroadcast(fd_set master, SOCKET listening, SOCKET socket, string message) {
	for (int i = 0; i < master.fd_count; i++) {
		SOCKET outSocket = master.fd_array[i];
		if (outSocket != listening && outSocket != socket) {
			ostringstream ss;
			ss << "SOCKET #" << socket << " " << message << "\r\n";
			string strOut = ss.str();

			send(outSocket, strOut.c_str(), strOut.size() + 1, 0);
		}
	}
}
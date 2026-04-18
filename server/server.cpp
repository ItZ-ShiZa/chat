#include <iostream>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>
#include <thread>
#include <vector>
#include <mutex>
#include <map>
#include <algorithm>
#include <sstream>
#include <fstream>

#define bufferSize 1024

std::mutex clientMutex;
std::map<std::string, sockaddr_in> clients;
std::vector<std::string> wrongConnect = {"Сеанс уже существует", "Неверный пароль", "Нет пользователя", "Пользователь существует"};
std::vector<std::string> correctConnect = {"Успешный вход", "Успешная регистрация"};
std::map<std::string, std::string> users;

int bindServer() {
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = 0;
    bind(sockfd, (sockaddr*)&serverAddr, sizeof(serverAddr));
    socklen_t len = sizeof(serverAddr);
    getsockname(sockfd, (sockaddr*)&serverAddr, &len);
    std::cout << "Сервер запущен на порту: " << ntohs(serverAddr.sin_port) << std::endl;
    std::ofstream file("settingsServer.txt");
    file << "127.0.0.1" << std::endl;
    file << ntohs(serverAddr.sin_port) << std::endl;
    std::cout << "Ожидание сообщений... (Ctrl+C для выхода)" << std::endl;
    return sockfd;
}
void allUsers() {
    std::ifstream file("users.txt");
    std::string user, login, password;
    std::getline(file, user);
    std::istringstream iss(user);
    iss >> login >> password;
    users[login] = password;
}
void dispatchMessages(int sockfd, std::string message, int length, const sockaddr_in& sender_addr, std::string senderInfo) {
    std::string resultMessage;

    // size_t pos = message.find(' ');
    // std::string tempMessage = (pos != std::string::npos) ? message.substr(pos + 1) : "";
    // if (tempMessage != "/exit"){
    //     resultMessage = senderInfo + ": " + tempMessage;
    // } else if (tempMessage == "/exit") {
    //     resultMessage = senderInfo + " покидает чат.";
    //     clients.erase(senderInfo);
    // }

    if (message != "/exit"){
        resultMessage = senderInfo + ": " + message;
    } else if (message == "/exit") {
        resultMessage = senderInfo + " покидает чат.";
        {
            std::lock_guard<std::mutex> lock(clientMutex);
            clients.erase(senderInfo);
        }
    }

    if (senderInfo == "server") {
        std::istringstream iss(message);
        std::string name, firstWord, tempWord;
        iss >> firstWord;
        if (firstWord == "Пользователь") {
            iss >> name;
        } else {
            iss >> tempWord >> name;
        }
        senderInfo = name;
    }

    {
        std::lock_guard<std::mutex> lock(clientMutex);
        for (const auto& client : clients) {
            if (client.first != senderInfo) {
                sendto(sockfd, resultMessage.c_str(), resultMessage.size(), 0, (sockaddr*)&client.second, sizeof(client.second));
            }
        }
    }
}

void processingMessages(int sockfd, sockaddr_in clientAddr, std::string buffer, int n) {
    std::string msg(buffer);
    std::string fullMessage;
    std::istringstream iss(msg);
    std::string firstKey, clientKey, passwordKey;
    iss >> firstKey;
    buffer[n] = '\0';

    if (firstKey != "login" && firstKey != "singin") {
        std::getline(iss, fullMessage);
        if (!fullMessage.empty() && fullMessage[0] == ' ') {
            fullMessage = fullMessage.substr(1);
        }
        clientKey = firstKey;
        std::cout << "Получено: " << fullMessage << std::endl;
        dispatchMessages(sockfd, std::string(fullMessage), n, clientAddr, clientKey);   
    } else {
        iss >> clientKey;
        iss >> passwordKey;

        if (firstKey == "login") {
            if (users.find(clientKey) != users.end() && users[clientKey] == passwordKey) {
                if (clients.find(clientKey) == clients.end()) {
                    {
                        std::lock_guard<std::mutex> lock(clientMutex);
                        clients[clientKey] = clientAddr;
                    }
                    std::string loginMessage = "Пользователь " + clientKey + " присоединился"; 
                    std::cout << loginMessage << std::endl;
                    sendto(sockfd, correctConnect[0].c_str(), correctConnect[0].size(), 0, (sockaddr*)&clientAddr, sizeof(clientAddr));
                    dispatchMessages(sockfd, std::string(loginMessage), n, clientAddr, std::string("server"));
                } else {
                    sendto(sockfd, wrongConnect[0].c_str(), wrongConnect[0].size(), 0, (sockaddr*)&clientAddr, sizeof(clientAddr));
                }
            } else if (users.find(clientKey) != users.end() && users[clientKey] != passwordKey) {
                sendto(sockfd, wrongConnect[1].c_str(), wrongConnect[1].size(), 0, (sockaddr*)&clientAddr, sizeof(clientAddr));
            } else {
                sendto(sockfd, wrongConnect[2].c_str(), wrongConnect[2].size(), 0, (sockaddr*)&clientAddr, sizeof(clientAddr));
            }
        } else {
            if (users.find(clientKey) != users.end()) {
                sendto(sockfd, wrongConnect[3].c_str(), wrongConnect[3].size(), 0, (sockaddr*)&clientAddr, sizeof(clientAddr));
            } else {
                {
                    std::lock_guard<std::mutex> lock(clientMutex);
                    clients[clientKey] = clientAddr;
                    users[clientKey] = passwordKey;
                    std::ofstream file("users.txt", std::ios::app);
                    file << clientKey << " " << passwordKey << "\n";
                }
                std::string singinMessage = "Новый пользователь " + clientKey; 
                std::cout << singinMessage << std::endl;
                sendto(sockfd, correctConnect[1].c_str(), correctConnect[1].size(), 0, (sockaddr*)&clientAddr, sizeof(clientAddr));
                dispatchMessages(sockfd, std::string(singinMessage), n, clientAddr, std::string("server"));
            }
        }
    }
}

void controlClients(int sockfd) {
    char buffer[bufferSize];
    sockaddr_in clientAddr{};
    socklen_t addrLen = sizeof(clientAddr);
    
    while (true) {
        memset(buffer, 0, bufferSize);
        int n = recvfrom(sockfd, buffer, bufferSize - 1, 0, (sockaddr*)&clientAddr, &addrLen);
        if (n > 0) {
            buffer[n] = '\0';
            std::thread(processingMessages, sockfd, clientAddr, std::string(buffer), n).detach();
        }
    }
}

void printClients() {
    while (true) {
        std::this_thread::sleep_for(std::chrono::seconds(10));
        std::lock_guard<std::mutex> lock(clientMutex);
        
        std::cout << "\n--- Активные клиенты (" << clients.size() << ") ---" << std::endl;
        for (const auto& client : clients) {
            std::cout << "  " << client.first << std::endl;
        }
        std::cout << "-----------------------------\n" << std::endl;
    }
}

int main() {
    allUsers();
    int sockfd = bindServer();
    std::thread print_thread(printClients);

    print_thread.detach();
    
    controlClients(sockfd);
    close(sockfd);
    return 0;
}
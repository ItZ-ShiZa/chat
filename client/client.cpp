#include <iostream>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>
#include <string>
#include <thread>
#include <atomic>
#include <csignal>
#include <fstream>
#include <mutex>
#include <termios.h>
#include <algorithm>
#include <vector>
#include <sstream>

#define bufferSize 1024
std::mutex clientMutex;
std::atomic<bool> running{true};
std::string msg;
std::vector<std::string> wrongConnect = {"Сеанс уже существует", "Неверный пароль", "Нет пользователя", "Пользователь существует"};

void popChar(std::string& str) {
    if (str.empty()) return;
    while (!str.empty() && (str.back() & 0xC0) == 0x80) {
        str.pop_back();
    }
    if (!str.empty()) {
        str.pop_back();
    }
}

void receiveMessages(int sockfd) {
    char buffer[bufferSize];
    while (running) {
        int n = recvfrom(sockfd, buffer, bufferSize - 1, 0, nullptr, nullptr);
        if (n > 0) {
            {
                std::lock_guard<std::mutex> lock(clientMutex);
                buffer[n] = '\0';
                std::cout << "\r\033[K" << buffer << std::endl;
                std::cout << "> " << msg << std::flush;
            }
        } else if (n < 0 && errno != EINTR) {
            if (running) {
                std::cerr << "\nОшибка приема данных" << std::endl;
            }
            break;
        }
    }
}

void sendMessages(int sockfd, sockaddr_in& serverAddr, std::string name) {
    termios oldTerminal, newTerminal;
    tcgetattr(STDIN_FILENO, &oldTerminal);
    newTerminal = oldTerminal;
    newTerminal.c_lflag &= ~(ECHO | ICANON);
    tcsetattr(STDIN_FILENO, TCSANOW, &newTerminal);

    char c;
    std::cout << "> " << std::flush;

    while (running) {
        if (read(STDIN_FILENO, &c, 1) > 0) {
            std::lock_guard<std::mutex> lock(clientMutex);

            if (c == '\n') {
                if (msg != "") {
                    std::string fullMessage;
                    std::istringstream iss(msg);
                    std::string firstKey;
                    iss >> firstKey;
                    if (firstKey == "/exit") {
                        sendto(sockfd, (name + " /exit").c_str(), (name + " /exit").size(), 0, (sockaddr*)&serverAddr, sizeof(serverAddr));
                        running = false;
                        break;
                    }
                    sendto(sockfd, (name + " " + msg).c_str(), (name + " " + msg).size(), 0, (sockaddr*)&serverAddr, sizeof(serverAddr));
                    msg.clear();
                    std::cout << std::endl;
                    std::cout << "> " << std::flush;
                } else {
                    msg.clear();
                    std::cout << "Сообщение не отправлено. Пустое." << std::endl;
                    std::cout << "> " << std::flush;
                }
            } else if (c == 127) {
                if (!msg.empty()) {
                    // msg.pop_back();
                    popChar(msg);
                    std::cout << "\b \b" << std::flush;
                }
            } else {
                msg += c;
                std::cout << c << std::flush;
            }
        }
    }

    tcsetattr(STDIN_FILENO, TCSANOW, &oldTerminal);
    close(sockfd);
}

int main(int argc, char* argv[]) {
    if (argc != 4) {
        std::cerr << "Использование: " << argv[0] << "<login/singin><name><password>" << std::endl;
        return 1;
    }
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);

    std::ifstream file("settingsServer.txt");
    std::string ip, port;
    std::getline(file, ip);
    std::getline(file, port);

    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(atoi(port.c_str()));
    inet_pton(AF_INET, ip.c_str(), &serverAddr.sin_addr);

    std::string msgAuth = std::string(argv[1]) + " " + std::string(argv[2]) + " " + std::string(argv[3]); 
    sendto(sockfd, msgAuth.c_str(), msgAuth.size(), 0, (sockaddr*)&serverAddr, sizeof(serverAddr));
    while (true) {
        char buffer[bufferSize];
        int n = recvfrom(sockfd, buffer, bufferSize - 1, 0, nullptr, nullptr);
        if (n > 0) {
            {
                std::lock_guard<std::mutex> lock(clientMutex);
                buffer[n] = '\0';
                std::cout << "\r\033[K" << buffer << std::endl;
            }
            if (std::ranges::find(wrongConnect, buffer) != wrongConnect.end()) {
                close(sockfd);
                return 0;
            }
            break;
        }
    } 

    std::cout << "Чат запущен. Введите /exit для выхода." << std::endl;

    std::thread receiver(receiveMessages, sockfd);
    std::thread sender(sendMessages, sockfd, std::ref(serverAddr), std::string(argv[2]));
    
    sender.join();
    
    if (receiver.joinable()) {
        pthread_cancel(receiver.native_handle());
        receiver.join();
    }
    
    close(sockfd);
    return 0;
}
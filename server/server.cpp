#include <iostream>
#include <string>
#include <thread>
#include <mutex>
#include <unordered_map>
#include <vector>
#include <fstream>
#include <sstream>
#include <chrono>
#include <algorithm>
#include <unistd.h>
#include <arpa/inet.h>
#include <errno.h>

#define bufferSize 1024

class TcpSocket {
public:
    TcpSocket() = default;

    explicit TcpSocket(int socketFd) : fd(socketFd) {}

    ~TcpSocket() {
        closeSocket();
    }

    bool createServer(uint16_t port = 0) {
        fd = socket(AF_INET, SOCK_STREAM, 0);

        if (fd < 0) {
            return false;
        }

        int opt = 1;

        setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

        sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = INADDR_ANY;
        addr.sin_port = htons(port);

        if (bind(fd, (sockaddr*)&addr, sizeof(addr)) < 0) {
            return false;
        }

        if (listen(fd, 16) < 0) {
            return false;
        }

        return true;
    }

    int acceptClient() {
        sockaddr_in clientAddr{};
        socklen_t len = sizeof(clientAddr);

        return accept(fd, (sockaddr*)&clientAddr, &len);
    }

    bool sendLine(const std::string& line) {
        std::string data = line + "\n";

        const char* ptr = data.c_str();
        size_t left = data.size();

        while (left > 0) {
            ssize_t n = send(fd, ptr, left, 0);

            if (n < 0) {
                if (errno == EINTR) {
                    continue;
                }

                return false;
            }

            ptr += n;
            left -= static_cast<size_t>(n);
        }

        return true;
    }

    bool readLine(std::string& line) {
        while (true) {
            size_t pos = buffer.find('\n');

            if (pos != std::string::npos) {
                line = buffer.substr(0, pos);

                buffer.erase(0, pos + 1);

                if (!line.empty() && line.back() == '\r') {
                    line.pop_back();
                }

                return true;
            }

            char temp[bufferSize];

            ssize_t n = recv(fd, temp, sizeof(temp), 0);

            if (n == 0) {
                return false;
            }

            if (n < 0) {
                if (errno == EINTR) {
                    continue;
                }

                return false;
            }

            buffer.append(temp, static_cast<size_t>(n));
        }
    }

    uint16_t getPort() const {
        sockaddr_in addr{};
        socklen_t len = sizeof(addr);

        getsockname(fd, (sockaddr*)&addr, &len);

        return ntohs(addr.sin_port);
    }

    int getFd() const {
        return fd;
    }

    void closeSocket() {
        if (fd >= 0) {
            close(fd);
            fd = -1;
        }
    }

private:
    int fd{-1};
    std::string buffer;
};

class UserStore {
public:
    UserStore(const std::string& filename) : filePath(filename) {
        load();
    }

    bool exists(const std::string& login) const {
        return users.find(login) != users.end();
    }

    bool validate(const std::string& login, const std::string& password) const {
        auto it = users.find(login);

        return it != users.end() && it->second == password;
    }

    void add(const std::string& login, const std::string& password) {
        users[login] = password;

        std::ofstream file(filePath, std::ios::app);

        file << login << " " << password << "\n";
    }

private:
    std::unordered_map<std::string, std::string> users;

    std::string filePath;

    void load() {
        std::ifstream file(filePath);

        std::string line;

        while (std::getline(file, line)) {
            std::istringstream iss(line);

            std::string login;
            std::string password;

            iss >> login >> password;

            if (!login.empty()) {
                users[login] = password;
            }
        }
    }
};

class ClientSession {
public:
    ClientSession(int socketFd, const std::string& userName) : socket(socketFd),
          name(userName) {}

    TcpSocket socket;
    std::string name;
};

class ChatServer {
public:
    ChatServer() : users("users.txt") {}

    void run() {
        if (!serverSocket.createServer()) {
            std::cerr << "Ошибка запуска сервера" << std::endl;

            return;
        }

        writeSettings();

        std::thread(&ChatServer::printClientsLoop, this).detach();

        while (true) {
            int clientFd = serverSocket.acceptClient();

            if (clientFd < 0) {
                if (errno == EINTR) {
                    continue;
                }

                continue;
            }

            std::thread(&ChatServer::handleClient, this, clientFd).detach();
        }
    }

private:
    TcpSocket serverSocket;

    UserStore users;

    std::mutex clientsMutex;

    std::unordered_map<int, std::unique_ptr<ClientSession>> clients;

    std::vector<std::string> wrongConnect {
        "Сеанс уже существует",
        "Неверный пароль",
        "Нет пользователя",
        "Пользователь существует"
    };

    std::vector<std::string> correctConnect {
        "Успешный вход",
        "Успешная регистрация"
    };

    void writeSettings() {
        std::ofstream file("settingsServer.txt");

        file << "127.0.0.1" << std::endl;

        file << serverSocket.getPort() << std::endl;

        std::cout<< "Сервер запущен на порту: " << serverSocket.getPort() << std::endl;
    }

    bool isOnline(const std::string& name) {
        for (const auto& [fd, client] : clients) {
            if (client->name == name) {
                return true;
            }
        }

        return false;
    }

    void broadcast(const std::string& message, int excludeFd = -1) {
        std::vector<int> targets;

        {
            std::lock_guard<std::mutex> lock(clientsMutex);

            for (const auto& [fd, client] : clients) {
                if (fd != excludeFd) {
                    targets.push_back(fd);
                }
            }
        }

        for (int fd : targets) {
            auto it = clients.find(fd);

            if (it != clients.end()) {
                it->second->socket.sendLine(message);
            }
        }
    }

    void removeClient(int fd) {
        std::lock_guard<std::mutex> lock(clientsMutex);
        clients.erase(fd);
    }

    bool authenticate(TcpSocket& socket, int fd, const std::string& line, std::string& login) {
        std::istringstream iss(line);

        std::string command;
        std::string password;

        iss >> command >> login >> password;

        if (command == "login") {
            if (!users.exists(login)) {
                socket.sendLine(wrongConnect[2]);
                return false;
            }

            if (!users.validate(login, password)) {
                socket.sendLine(wrongConnect[1]);
                return false;
            }

            {
                std::lock_guard<std::mutex> lock(clientsMutex);

                if (isOnline(login)) {
                    socket.sendLine(wrongConnect[0]);
                    return false;
                }

                clients[fd] =
                    std::make_unique<ClientSession>(fd, login);
            }

            socket.sendLine(correctConnect[0]);

            std::string msg = "Пользователь " + login + " присоединился";

            std::cout<< msg << std::endl;

            broadcast(msg, fd);

            return true;
        }

        if (command == "singin") {
            if (users.exists(login)) {
                socket.sendLine(wrongConnect[3]);
                return false;
            }

            users.add(login, password);

            {
                std::lock_guard<std::mutex> lock(clientsMutex);

                clients[fd] = std::make_unique<ClientSession>(fd, login);
            }

            socket.sendLine(correctConnect[1]);

            std::string msg = "Новый пользователь " + login;

            std::cout<< msg << std::endl;

            broadcast(msg, fd);

            return true;
        }

        return false;
    }

    void handleClient(int clientFd) {
        TcpSocket socket(clientFd);

        std::string login;
        std::string line;

        if (!socket.readLine(line)) {
            return;
        }

        if (!authenticate(socket, clientFd, line, login)) {
            return;
        }

        while (socket.readLine(line)) {
            std::string sender;

            {
                std::lock_guard<std::mutex> lock(clientsMutex);

                auto it = clients.find(clientFd);

                if (it == clients.end()) {
                    break;
                }

                sender = it->second->name;
            }

            if (line == "/exit") {
                std::string msg = sender + " покидает чат.";

                std::cout<< msg << std::endl;

                removeClient(clientFd);

                broadcast(msg, clientFd);

                break;
            }

            std::string msg = sender + ": " + line;

            std::cout<< msg << std::endl;

            broadcast(msg, clientFd);
        }

        removeClient(clientFd);
    }

    void printClientsLoop() {
        while (true) {
            std::this_thread::sleep_for(std::chrono::seconds(10));

            std::lock_guard<std::mutex> lock(clientsMutex);

            std::cout<< "\n--- Активные клиенты (" << clients.size() << ") ---" << std::endl;

            for (const auto& [fd, client] : clients) {
                std::cout<< client->name << std::endl;
            }

            std::cout<< "-----------------------------\n" << std::endl;
        }
    }
};

int main() {
    ChatServer server;

    server.run();

    return 0;
}
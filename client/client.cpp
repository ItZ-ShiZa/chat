#include <iostream>
#include <string>
#include <thread>
#include <atomic>
#include <mutex>
#include <fstream>
#include <vector>
#include <algorithm>
#include <unistd.h>
#include <arpa/inet.h>
#include <termios.h>
#include <sys/select.h>
#include <errno.h>

#define bufferSize 1024

class TcpSocket {
public:
    TcpSocket() = default;

    ~TcpSocket() {
        closeSocket();
    }

    bool connectTo(const std::string& ip, uint16_t port) {
        fd = socket(AF_INET, SOCK_STREAM, 0);

        if (fd < 0) {
            return false;
        }

        sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_port = htons(port);

        inet_pton(AF_INET, ip.c_str(), &addr.sin_addr);

        return connect(fd, (sockaddr*)&addr, sizeof(addr)) >= 0;
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

    void shutdownSocket() {
        if (fd >= 0) {
            shutdown(fd, SHUT_RDWR);
        }
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

class TerminalMode {
public:
    TerminalMode() {
        tcgetattr(STDIN_FILENO, &oldTerm);

        newTerm = oldTerm;
        newTerm.c_lflag &= ~(ICANON | ECHO);

        tcsetattr(STDIN_FILENO, TCSANOW, &newTerm);
    }

    ~TerminalMode() {
        tcsetattr(STDIN_FILENO, TCSANOW, &oldTerm);
    }

private:
    termios oldTerm{};
    termios newTerm{};
};

class ChatClient {
public:
    bool run(const std::string& mode, const std::string& login, const std::string& password) {
        if (!loadServerSettings()) {
            return false;
        }

        if (!socket.connectTo(serverIp, serverPort)) {
            std::cerr << "Ошибка подключения" << std::endl;
            return false;
        }

        if (!authenticate(mode, login, password)) {
            return false;
        }

        std::cout << "Чат запущен. Введите /exit для выхода." << std::endl;

        std::thread receiver(&ChatClient::receiveLoop, this);
        std::thread sender(&ChatClient::sendLoop, this);

        sender.join();

        running = false;

        socket.shutdownSocket();

        if (receiver.joinable()) {
            receiver.join();
        }

        return true;
    }

private:
    TcpSocket socket;

    std::atomic<bool> running{true};

    std::mutex ioMutex;

    std::string currentMessage;

    std::string serverIp;
    uint16_t serverPort{0};

    std::vector<std::string> authErrors {
        "Сеанс уже существует",
        "Неверный пароль",
        "Нет пользователя",
        "Пользователь существует"
    };

    bool loadServerSettings() {
        std::ifstream file("settingsServer.txt");

        if (!file.is_open()) {
            return false;
        }

        std::string port;

        std::getline(file, serverIp);
        std::getline(file, port);

        serverPort = static_cast<uint16_t>(atoi(port.c_str()));

        return true;
    }

    bool authenticate(const std::string& mode, const std::string& login, const std::string& password) {
        std::string authMessage = mode + " " + login + " " + password;

        if (!socket.sendLine(authMessage)) {
            return false;
        }

        std::string response;

        if (!socket.readLine(response)) {
            return false;
        }

        std::cout << response << std::endl;

        return std::find(authErrors.begin(), authErrors.end(), response) == authErrors.end();
    }

    void receiveLoop() {
        std::string line;

        while (running) {
            if (!socket.readLine(line)) {
                running = false;
                break;
            }

            std::lock_guard<std::mutex> lock(ioMutex);

            std::cout << "\r\033[K" << line << std::endl;

            std::cout << "> " << currentMessage << std::flush;
        }
    }

    void sendLoop() {
        TerminalMode term;

        std::cout << "> " << std::flush;

        while (running) {
            fd_set set;

            FD_ZERO(&set);
            FD_SET(STDIN_FILENO, &set);

            timeval tv{};
            tv.tv_sec = 0;
            tv.tv_usec = 200000;

            int rv = select(STDIN_FILENO + 1, &set, nullptr, nullptr, &tv);

            if (rv <= 0) {
                continue;
            }

            char c;

            if (read(STDIN_FILENO, &c, 1) <= 0) {
                continue;
            }

            std::lock_guard<std::mutex> lock(ioMutex);

            if (c == '\n') {
                processInput();
            }
            else if (c == 127 || c == 8) {
                processBackspace();
            }
            else {
                currentMessage += c; std::cout << c << std::flush;
            }
        }
    }

    void processInput() {
        if (currentMessage.empty()) {
            std::cout << std::endl << "Сообщение не отправлено. Пустое." << std::endl << "> " << std::flush;

            return;
        }

        if (currentMessage == "/exit") {
            socket.sendLine("/exit");

            currentMessage.clear();

            running = false;

            socket.shutdownSocket();

            return;
        }

        socket.sendLine(currentMessage);

        currentMessage.clear();

        std::cout << std::endl << "> " << std::flush;
    }

    void processBackspace() {
        if (currentMessage.empty()) {
            return;
        }

        while (!currentMessage.empty() && (currentMessage.back() & 0xC0) == 0x80) {
            currentMessage.pop_back();
        }

        currentMessage.pop_back();

        std::cout << "\b \b" << std::flush;
    }
};

int main(int argc, char* argv[]) {
    if (argc != 4) {
        std::cerr << "Использование: " << argv[0] << " <login/singin> <name> <password>" << std::endl;

        return 1;
    }

    ChatClient client;

    client.run(argv[1], argv[2], argv[3]);

    return 0;
}
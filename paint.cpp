#include <iostream>
#include <filesystem>
#include <fstream>
#include <vector>

std::string name = "M1";
std::string pathToUserChatsList = std::filesystem::current_path().string() + "/chats/" + name;
std::string pathToUserChatsHistory = std::filesystem::current_path().string() + "/chats/" + name + "/chatsHistory/";
std::vector<std::string> chatsList;

int stringLength (std::string message) {
    int length = 0;
    for (int i = 0; i < static_cast<int>(message.size()); i++) {
        if ((message[i] & 0xC0) != 0x80) {
            length++;
        }
    }
    return length;
}

int substrUTF8 (std::string message, int n) {
    int bytes = 0;
    
    for (int chars = 0; chars < n && bytes < message.length(); chars++) {
        unsigned char c = message[bytes];
        if (c < 0x80) {
            bytes += 1;
        }
        else if ((c & 0xE0) == 0xC0) {
            bytes += 2;
        }
        else if ((c & 0xF0) == 0xE0) {
            bytes += 3;
        }
        else if ((c & 0xF8) == 0xF0) {
            bytes += 4;
        }
        else {
            return -1;
        }
    }
    return bytes;
}

void getChatsList () {
    std::string filePath = pathToUserChatsList + "/chatsList.txt";
    std::ifstream file(filePath);
    std::string chatName;
    while (std::getline(file, chatName)) {
        if (!chatName.empty()) {
            chatsList.push_back(chatName);
        }
    }
    file.close();
}

std::string getLineFromFile (std::string fileName) {
    std::string filePath = pathToUserChatsHistory + fileName + ".txt";
    std::ifstream file(filePath);
    std::string chatLine;
    std::getline(file, chatLine);
    file.close();
    return chatLine;
}

void getChatHistory (std::string chatName) {
    for (int i; i < 21; i++) {
        std::string line = getLineFromFile(chatName); 
    }

}

void writeFile (std::string fileName, std::string message) {
    std::string filePath;
    if (fileName != "chatsList.txt") {
        filePath = pathToUserChatsHistory + fileName;
    } else {
        filePath = pathToUserChatsList + "/chatsList.txt";
    }
    std::ofstream file(filePath, std::ios::app);
    file << message << std::endl;
}

void updateFile (std::string fileName, std::string message) {
    int n, pos = -1, check = 96;
    std::string tempMessage;

    while (message.size() > 0) {
        size_t start = message.find_first_not_of(" \t\n\r\f\v");
        if (start != std::string::npos) {
            message = message.substr(start);
        }
        n = substrUTF8(message, check);
        tempMessage = message.substr(0, n);
        pos = tempMessage.find_last_of(' ');
        if (pos != -1) {
            tempMessage = tempMessage.substr(0, pos);
            writeFile(fileName, tempMessage);
            message = message.substr(pos);
        } else {
            writeFile(fileName, tempMessage);
            n = substrUTF8(tempMessage, check);
            message = message.substr(n);
        }
    }




    // for (int i = 0; i < stringLength(message); i++) {
    //     if (n < 96) {
    //         if (message[i] != ' ') {
    //             tempWord += message[i];
    //         } else {
    //             pos = i;
    //             tempMessage += tempWord;
    //             tempWord = "";
    //         }
    //     } else {
    //         writeFile(fileName, tempMessage);
    //         tempMessage = "";
    //         if (pos != -1) {
    //             n = stringLength(tempWord);
    //             start = i - n;
    //         } else {
    //             tempWord = "";
    //             n = 0;
    //             start = i;
    //         }
    //     }
    // }
}

void goToXY (int x, int y) {
    std::cout << "\u001b[" << y << ";" << x << "H" << std::flush;
}
void print_a (const char* str, int lines, int countInLine, int spaces, int x = -1, int y = -1) {
    if (x >= 0 && y >= 0) {
        goToXY(x, y);
    }
    for (int i = 0; i < lines; i++) {
        for (int j = 0; j < countInLine; j++) {
            std::cout << str;
            if (countInLine - j > 1 && spaces > 0) {
                for (int k = 0; k < spaces/(countInLine - 1); k++) {
                    std::cout << " ";
                }
            }
        }
        if (i < lines - 1) {
            y++;
            goToXY(x, y);
        }
    }
}
void printChat () {
    print_a("┌", 1, 1, 0, 0, 0);
    print_a("─", 1, 98, 0);
    print_a("┐", 1, 1, 0);
    print_a("│", 23, 2, 98, 0, 2);
    print_a("└", 1, 1, 0, 0, 25);
    print_a("─", 1, 98, 0);
    print_a("┘", 1, 1, 0);
}
void printChatList () {
    print_a("┌", 1, 1, 0, 101, 0);
    print_a("─", 1, 38, 0);
    print_a("┐", 1, 1, 0);
    print_a("│", 23, 2, 38, 101, 2);
    print_a("└", 1, 1, 0, 101, 25);
    print_a("─", 1, 38, 0);
    print_a("┘", 1, 1, 0);

    getChatsList();
    for (int i = 0; i < chatsList.size(); i++) {
        goToXY(103, i + 2);
        std::cout << i + 1 << " - " << chatsList[i];
    }
    goToXY(142, 25);
}

int main() {
    std::cout << "\u001b[2J\u001b[H" << std::flush;
    printChat();
    printChatList();

    updateFile("M2.txt", "bкbкbкbкbкbкbкbкbкbкbкbкbкbкbкbкbкbкbкbкbкbкbкbкbкbкbкbкbкbкbкbкbкbкbкbкbкbкbкbкbкbкbкbкbкbкbкbкbкbкbкbкbкbкbкbкbкbкbкbкbкbкbкbкbкbкbкbкbкbкbкbкbкbкbкbкbкbкbкbкbкbкbкbкbкbкbкbкbкbкbкbкbкbкbкbкbк");

    std::cout << std::endl;
}

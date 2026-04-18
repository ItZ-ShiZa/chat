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
    int length, realLength;
    for (int i = 0; i < n; i++) {
        if ((message[i] & 0xC0) != 0x80) {
            length++;
        }
        realLength++;
    }
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

void getChatHistory (std::string chatName) {
    std::string filePath = pathToUserChatsHistory + chatName + ".txt";
    
}


void writeFile (std::string fileName, std::string message) {
    std::string filePath;
    if (fileName != "chatsList.txt") {
        filePath = pathToUserChatsList + fileName;
    } else {
        filePath = pathToUserChatsList + "/chatsList.txt";
    }
    std::ofstream file(filePath, std::ios::app);
    file << message << std::endl;
}

void updateFile (std::string fileName, std::string message) {
    std::string tempMessage;
    int n = 0, pos = -1, start = 0;

    while (message.size() > 0) {
        tempMessage = message.substr(0, 96);
        pos = tempMessage.find_last_of(' ');
        if (pos != -1) {
            writeFile(fileName, tempMessage);
        } else {
            tempMessage = tempMessage.substr(0, pos);
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

    updateFile("chatsList.txt", "M2");

    std::cout << std::endl;
}

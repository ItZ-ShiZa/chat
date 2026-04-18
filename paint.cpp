#include <iostream>
#include <filesystem>
#include <fstream>
#include <vector>

std::string name = "M1";
std::string pathToUserChatsList = std::filesystem::current_path().string() + "/paint/chats/" + name;
std::string pathToUserChatsHistory = std::filesystem::current_path().string() + "/paint/chats/" + name + "/chatsHistory/";
std::vector<std::string> chatsList;

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
    std::cout << std::endl;
}

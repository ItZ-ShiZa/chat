GREEN='\033[0;32m'
BLUE='\033[0;34m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
CYAN='\033[0;36m'
NC='\033[0m'
echo -e "${BLUE}=== Сборка проекта ===${NC}"
# Сборка сервера
if [ ! -f "server/server.cpp" ]; then
    echo -e "${RED}Ошибка: server/server.cpp не найден${NC}"
    exit 1
fi

echo -e "${GREEN}Сборка сервера...${NC}"
g++ server/server.cpp -o server/server
if [ $? -ne 0 ]; then
    echo -e "${RED}Ошибка сборки сервера${NC}"
    exit 1
fi

# Сборка клиента
if [ ! -f "client/client.cpp" ]; then
    echo -e "${RED}Ошибка: client/client.cpp не найден${NC}"
    exit 1
fi

echo -e "${GREEN}Сборка клиента...${NC}"
g++ -std=c++20 client/client.cpp -o client/client
if [ $? -ne 0 ]; then
    echo -e "${RED}Ошибка сборки клиента${NC}"
    exit 1
fi

echo -e "${GREEN}Сборка завершена${NC}"

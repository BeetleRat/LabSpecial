// This code is not protected by any copyright as far as I know
// It is based on misc public sources on the Internet, including MSDN
// Prepared by Mark Polyak in 2020

#include <stdio.h>
#include <winsock2.h> // Заголовочный файл, содержащий актуальные реализации функций для работы с сокетами
#include <iostream>
#include <vector>
#include <string>
#include <stdlib.h>
#include <queue>

// ipconfig ищем "Адаптер Ethernet VirtualBox Host-Only Network"
#define SERVER_IP "192.168.56.1"
//#define EXPRESSION_SIZE 255
#define RESULT_SIZE 255

int SERVER_PORT;
const int total_connections = 2;
SOCKET connections[total_connections];
std::queue<int> available_connections;

HANDLE threads[total_connections];
HANDLE semaphore;

enum LexemeType {
    LEFT_BRACKET, RIGHT_BRACKET,
    PLUS, MINUS, MULTIPLY, DIVIDE,
    NUMBER,
    END_OF_LINE
};

class Lexeme {
private:
    std::string value;
    LexemeType type;
public:
    Lexeme(std::string new_value, LexemeType new_type) {
        value = new_value;
        type = new_type;
    }
    std::string get_value() {
        return value;
    }
    LexemeType get_type() {
        return type;
    }
};

class LexemeBuffer {
private:
    int index;
    std::vector<Lexeme> lexeme_list;
public:
    LexemeBuffer(std::vector<Lexeme> new_lexeme_list) {
        index = 0;
        lexeme_list = new_lexeme_list;
    }

    Lexeme next() {
        Lexeme lexeme = lexeme_list[index];
        if (index < lexeme_list.size() - 1) {
            index++;
        }
        return lexeme;
    }

    Lexeme back() {
        index--;
        if (index < 0) {
            index = 0;
        }
        return lexeme_list[index];
    }

    int get_current_index() {
        return index;
    }
    int get_size() {
        return lexeme_list.size();
    }
};
std::vector<Lexeme> string_to_lexemes(std::string);
double expression(LexemeBuffer&);
double plus_or_minus(LexemeBuffer&);
double multiplication_or_division(LexemeBuffer&);
double number_or_expression_in_brackets(LexemeBuffer&);
std::string figure_expression(char*);
void thread_output(const std::string&);
bool is_string_equal(char*, const std::string&);
DWORD WINAPI client_handler(LPVOID);
void create_new_thread(intptr_t);
bool intit_WS2_32_lib();
bool create_server_socket(SOCKET&);
void listen_server_connection(const SOCKET&);


// Функция разбивает строку с выражением на массив лексем этого выражения
std::vector<Lexeme> string_to_lexemes(std::string expression_string){
    std::vector<Lexeme> lexemes;
    int index = 0;
    char current_char;
    while (expression_string[index]!=NULL) {
        current_char = expression_string[index];
        switch (current_char)
        {
        case '(': 
            lexemes.push_back(Lexeme("(", LexemeType::LEFT_BRACKET));
            break;
        case ')' : 
            lexemes.push_back(Lexeme(")", LexemeType::RIGHT_BRACKET));
            break;
        case '+' : 
            
            lexemes.push_back(Lexeme("+", LexemeType::PLUS));
            break;
        case '-' : 
            
            lexemes.push_back(Lexeme("-", LexemeType::MINUS));
            break;
        case '*': 
            
            lexemes.push_back(Lexeme("*", LexemeType::MULTIPLY));
            break;
        case '/': 
            
            lexemes.push_back(Lexeme("/", LexemeType::DIVIDE));
            break;
        default:
            if (current_char <= '9' && current_char >= '0') {
                std::string number_string = "";
                while (current_char <= '9' && current_char >= '0') {
                    number_string = number_string + current_char;
                    index++;
                    if (index >= expression_string.length()) {
                        break;
                    }
                    current_char = expression_string[index];                 
                }   
                
                lexemes.push_back(Lexeme(number_string, LexemeType::NUMBER));
                index--;
            }
            break;
        }
        index++;
    }
    // Убираем лишние знаки в конце выражения
    index = lexemes.size() - 1;
    while (lexemes[index].get_type() != LexemeType::NUMBER && lexemes[index].get_type() != LexemeType::RIGHT_BRACKET && index >= 0) {
        lexemes.erase(lexemes.begin() + index);
        index--;
    }

    lexemes.push_back(Lexeme("", LexemeType::END_OF_LINE));
    return lexemes;
}

// Вычисление выражения
double expression(LexemeBuffer& lexemes){

    Lexeme current_lexeme = lexemes.next();
    
    if (current_lexeme.get_type() == LexemeType::END_OF_LINE) {
        printf("В выражении отсутствуют лекемы\n");
        return 0.0;
    }
    else {
        lexemes.back();
        return plus_or_minus(lexemes);
    }
}

// После того как перемножили все минимальные части
// выполняем сложение и вычитание результатов умножения/деления
double plus_or_minus(LexemeBuffer& lexemes){

    double current_value = multiplication_or_division(lexemes);

    // Обходим все выражение
    while (lexemes.get_current_index() != lexemes.get_size() - 1) {

        Lexeme current_lexeme = lexemes.next();
        switch (current_lexeme.get_type())
        {
        case LexemeType::PLUS:   
            // Складываем текущий результат с следующим результатом умножения/деления
            current_value = current_value + multiplication_or_division(lexemes);
            break;
        case LexemeType::MINUS:
            // Вычитаем из текущего результата следующий результат умножения/деления
            current_value = current_value - multiplication_or_division(lexemes);
            break;            
        default:
            lexemes.back();
            return current_value;           
        }
    }
    return current_value;
}

// В выражении сначала выполняются операции деления и умножения
// минимальных составных частей
double multiplication_or_division(LexemeBuffer& lexemes){
    // Получаем минимальную составную часть
    double current_value = number_or_expression_in_brackets(lexemes);

    // Обходим все выражение
    while (lexemes.get_current_index() != lexemes.get_size() - 1) {

        Lexeme current_lexeme = lexemes.next();
            switch (current_lexeme.get_type())
            {
            case LexemeType::MULTIPLY:
                // Умножаем текущий результат умножения/деления на следующую минимальную составную часть
                current_value = current_value * number_or_expression_in_brackets(lexemes);
                break;                    
            case LexemeType::DIVIDE:
                // Делим текущий результат умножения/деления на следующую минимальную составную часть
                current_value = current_value / number_or_expression_in_brackets(lexemes);
                break;
            default:
                lexemes.back();
                return current_value;
                  
            }
    }
    return current_value;
}

// Минимальная составная часть выражения это цифра или выражение в скобках
// эту минимальную часть мы будем использовать в более сложных
double number_or_expression_in_brackets(LexemeBuffer& lexemes){

    Lexeme current_lexeme = lexemes.next();
    double value = 0.0;
    switch (current_lexeme.get_type())
    {
    case LexemeType::NUMBER:
        return std::atof(current_lexeme.get_value().c_str());
                    
    case LexemeType::LEFT_BRACKET:
        // Вычисляем выражение в скобках
        value = expression(lexemes);
        // Если нет закрывающей скобки
        if (lexemes.next().get_type() != LexemeType::RIGHT_BRACKET) {
            printf("Отсутствует закрывающая скобка");
            return 0.0;
        }
        return value;                    
    default:
        printf("В минимальную состовную часть попала сложная лексема");
        return 0.0;                    
    }             
}

std::string figure_expression(char* expression_string) {
    // Получить LexemeBuffer из входной строки
    LexemeBuffer lexemes = LexemeBuffer(string_to_lexemes(expression_string));
    // Вернуть результат вычисления LexemeBuffer
    return std::to_string(expression(lexemes));
}



void thread_output(const std::string& output) {    
    WaitForSingleObject(semaphore, INFINITE);
    std::cout << output << std::endl <<std::flush;
    if (!ReleaseSemaphore(semaphore,1,NULL))
    {
        printf("ReleaseSemaphore error: %d\n", GetLastError());
    }    
}

bool is_string_equal(char* char_string, const std::string& string_string) {
    int i = 0;
    while (char_string[i]!=NULL&&i< string_string.size())
    {
        if (char_string[i] != string_string[i]) {
            return false;
        }
        i++;
    }
    return true;
}
DWORD WINAPI client_handler(LPVOID connection_number) {
    int expression_size;
    char result[RESULT_SIZE];
    bool is_expression_correct;
    while (true) {
        is_expression_correct=true;
        // Получаем размер принимаемой строки
        recv(connections[(intptr_t)connection_number], (char*)&expression_size, sizeof(int), 0);
        //thread_output("Клиент " + std::to_string((int)connection_number) + ": хочет передать " + std::to_string(expression_size)+" символов.");
        char *expression=new char[expression_size+1];
        expression[expression_size] = '\0';
        // Получаем саму строку
        recv(connections[(intptr_t)connection_number], expression, expression_size, 0);
        
        if (is_string_equal(expression,"End of operation")) {
            thread_output("Клиент " + std::to_string((int)connection_number) + ": закончил сеанс.");
            delete[] expression;
            break;
        }
        else
        {
            thread_output("Клиент " + std::to_string((int)connection_number) + ": выражение: " + std::string(expression));
        }/*
        for (int i = 0; i < expression_size; i++) {
            thread_output("Клиент " + std::to_string((int)connection_number) + ": выражение[" + std::to_string(i) + "]: " + expression[i]);
        }*/

        for (int i = 0; i < expression_size; i++) {            
            if (((int)expression[i]<(int)'0' || (int)expression[i] > (int)'9')
                && expression[i] != ' ' && expression[i] != '+' && expression[i] != '-'
                && expression[i] != '*' && expression[i] != '/'
                && expression[i] != '(' && expression[i] != ')') {
                strcpy(result, "Uncorrect symbol");
                is_expression_correct = false;
                break;
            }
            if (i > 0) {
                if (expression[i - 1] == '+' || expression[i - 1] == '-' || expression[i - 1] == '*' || expression[i - 1] == '/' || expression[i - 1] == '(') {
                    if (((int)expression[i]<(int)'0' || (int)expression[i] > (int)'9')&& (int)expression[i]!='(') {
                        strcpy(result, "Uncorrect expression");
                        is_expression_correct = false;
                        break;
                    }
                }
                if (expression[i - 1] == ')') {
                    if (expression[i] != '+' && expression[i] != '-' && expression[i] != '*' && expression[i] != '/') {
                        strcpy(result, "Uncorrect expression");
                        is_expression_correct = false;
                        break;
                    }
                }
                if ((int)expression[i-1]>=(int)'0' && (int)expression[i-1] <= (int)'9') {
                    if (expression[i] == '(') {
                        strcpy(result, "Uncorrect expression");
                        is_expression_correct = false;
                        break;
                    }
                }
            }
        }
        if (is_expression_correct) {
            strcpy(result, figure_expression(expression).c_str());

        }
        send(connections[(intptr_t)connection_number], result, RESULT_SIZE, 0);
        thread_output("Клиент " + std::to_string((int)connection_number) + ": результат: " + std::string(result));
        delete [] expression;
    }
    available_connections.push((int)connection_number);
    return 0;
}

void create_new_thread(intptr_t thread_name) {
    DWORD new_thread_id;
    threads[thread_name] = CreateThread(NULL, 0, client_handler, (void*)thread_name, 0, &new_thread_id);
    if (threads[thread_name] == NULL)
        std::cout << "Поток " << thread_name << " не был создан." << std::endl;

}
bool intit_WS2_32_lib() {
    // Инициализация сокетного интерфейса
    // Структура WSADATA содержит информацию о реализации сокетов Windows
    WSADATA wsaData;
    // WSAStartup подключает библиотеку WS2_32.lib
    /*MAKEWORD(2, 2) запрашивает версию WinSock системы (от 2 до 2)
    и устанавливает ее как наивысшую допустимую версию сокетов Windows, которая может использоватся*/
    int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (iResult != NO_ERROR) {
        printf("Error at WSAStartup()\n");
        system("Pause");
        return false;
    }
    return true;
}

bool create_server_socket(SOCKET& created_socket) {
    
    // Инициализация сокета
    // TCP - transmission control protocol: надежный протокол, ориентированный на соединений потоков байтов или пакетов
    /* UPD - user data protocol : ненадежная передача пакетов.
    Пользователь получает доступ к голой сети.
    Пользователь сам реализует систему обработки ошибок.*/
    // AF_INET - семейство адресов интернет IPv4
    // SOCK_STREAM - соединение потоков байт(TCP)
    // SOCK_SEQPACKET - соединение потоков пакетов(TCP)
    // SOCK_RAW - сырые сокеты, с их помощью программа может формировать собственные заголовки в сообщении.
    // SOCK_DGRAM - передача данных в виде отдельных сообщений датаграм(UDP)
    // IPPROTO_TCP - протокол соединения TCP
    // IPPROTO_UDP - протокол соединения UPD
    // 0 протокол по умолчанию
    created_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    if (created_socket == INVALID_SOCKET) {
        printf("Error at socket(): %ld\n", WSAGetLastError());
        closesocket(created_socket);
        WSACleanup();// деинициализация сокетов Win32API
        system("Pause");
        return false;
    }

    // «Привязка» созданного сокета к конкретной паре IP-адрес/Порт 
    // с этого момента данный сокет (его имя) будет ассоциироваться с конкретным процессом, 
    // который «висит» по указанному адресу и порту.
    struct sockaddr_in server_address; // Объект структуры инкапсулирующий информацию о семействе адресов
    server_address.sin_family = AF_INET; // семейство адресов интернет
    /*
     Проверка функции inet_addr на ошибку
    in_addr ip_to_num;
    errorStatus = inet_pton(AF_INET, SERVER_IP, &ip_to_num);
    if (errorStatus <= 0) {
        cout << "Error in IP translation to special numeric format" << endl;
        return 1;
    }
    server_address.sin_addr.s_addr = ip_to_num;
    */
    server_address.sin_addr.s_addr = inet_addr(SERVER_IP); // Локальный IP с которым будет связан сокет
    server_address.sin_port = htons(SERVER_PORT); // Номер порта, с которым будет связан сокет

    if (bind(
        created_socket,// Созданный сокет
        (sockaddr*)&server_address, // Адрес к которому он привязывается
        sizeof(server_address)) == SOCKET_ERROR) {
        // Если не получилось привязать
        printf("bind() failed.\n");
        closesocket(created_socket);
        WSACleanup();// деинициализация сокетов Win32API
        system("Pause");
        return false;
    }

    // Прослушивание сокета, 
    int max_connected_process = total_connections; // Если больше клиентов попробует подключится, они будут сброшены
    if (listen(created_socket, SOMAXCONN_HINT(max_connected_process)) == SOCKET_ERROR) {
        printf("Ошибка прослушивания сокета.\n");
        closesocket(created_socket);
        WSACleanup();// деинициализация сокетов Win32API
        system("Pause");
        return false;
    }
    return true;
}



void listen_server_connection(const SOCKET& listening_socket) {

    // Accept / Подтверждение подключения (обычно на стороне сервера)/ Принятие запроса на установление соединения
    SOCKET AcceptSocket;// Временный сокет
    printf("Ожидание подключения клиентов...\n");
    while (1) {
        AcceptSocket = SOCKET_ERROR;
        // Бесконечное ожидание запроса на соединение
        while (AcceptSocket == SOCKET_ERROR) {
            AcceptSocket = accept(listening_socket, // "слушающий" сокет на стороне Сервера
                /*sockaddr* */  NULL, // указатель на пустую структуру sockaddr, в которую будет записана информация по подключившемуся Клиенту
                                NULL); // sizeof(sockaddr*)
        }
        if (available_connections.empty()) {
            printf("Ошибка. У сервера нет свободных потров\n");
        }
        else
        {
            int current_socket = available_connections.front();
            available_connections.pop();

            printf("Клиент %d подключился.\n", current_socket);

            connections[current_socket] = AcceptSocket;
            // Обработка клиентских запросов происходит в отдельном потоке
            create_new_thread(current_socket);
        }
    }
    
}

int main(int args, char*argv[]) {
    if (args > 1) {
        SERVER_PORT = 0;
        int i = 0;
        while (argv[1][i] != NULL) {
            if ((int)argv[1][i]<(int)'0' || (int)argv[1][i] > (int)'9') {
                SERVER_PORT = 10;
                break;
            }
            SERVER_PORT = (SERVER_PORT + (int)argv[1][i] - (int)'0') * 10;
            i++;
        }
        SERVER_PORT = SERVER_PORT / 10;
    }
    else {
        SERVER_PORT = 1;
    }
    std::cout << "SERVER_PORT: "<< SERVER_PORT << std::endl;
    setlocale(LC_ALL, "Rus");

    for (int i = 0; i < total_connections; i++) {
        available_connections.push(i);
    }

    semaphore=CreateSemaphore( NULL,0,1,NULL);
    if (semaphore == NULL)
        std::cout << "Ошибка создания семафора."<< std::endl;
    if (!ReleaseSemaphore(semaphore, 1, NULL))
    {
        printf("ReleaseSemaphore error: %d\n", GetLastError());
    }

    if (!intit_WS2_32_lib()) {
        return 0;
    }
        
    SOCKET server_socket;
    if (!create_server_socket(server_socket)) {
        return 0;
    }
    listen_server_connection(server_socket);

    WaitForMultipleObjects(total_connections, threads, TRUE, INFINITE);
    for (int i = 0; i < total_connections; i++) {
        closesocket(connections[i]);
    }
    WSACleanup();// деинициализация сокетов Win32API
    printf("Сервер завершил свою работу.\n");
    system("Pause");
    return 0;
}

// This code is not protected by any copyright as far as I know
// It is based on misc public sources on the Internet, including MSDN
// Prepared by Mark Polyak in 2020

#include <stdio.h>
#include <winsock2.h> // ������������ ����, ���������� ���������� ���������� ������� ��� ������ � ��������
#include <iostream>
#include <vector>
#include <string>
#include <stdlib.h>
#include <queue>

// ipconfig ���� "������� Ethernet VirtualBox Host-Only Network"
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


// ������� ��������� ������ � ���������� �� ������ ������ ����� ���������
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
    // ������� ������ ����� � ����� ���������
    index = lexemes.size() - 1;
    while (lexemes[index].get_type() != LexemeType::NUMBER && lexemes[index].get_type() != LexemeType::RIGHT_BRACKET && index >= 0) {
        lexemes.erase(lexemes.begin() + index);
        index--;
    }

    lexemes.push_back(Lexeme("", LexemeType::END_OF_LINE));
    return lexemes;
}

// ���������� ���������
double expression(LexemeBuffer& lexemes){

    Lexeme current_lexeme = lexemes.next();
    
    if (current_lexeme.get_type() == LexemeType::END_OF_LINE) {
        printf("� ��������� ����������� ������\n");
        return 0.0;
    }
    else {
        lexemes.back();
        return plus_or_minus(lexemes);
    }
}

// ����� ���� ��� ����������� ��� ����������� �����
// ��������� �������� � ��������� ����������� ���������/�������
double plus_or_minus(LexemeBuffer& lexemes){

    double current_value = multiplication_or_division(lexemes);

    // ������� ��� ���������
    while (lexemes.get_current_index() != lexemes.get_size() - 1) {

        Lexeme current_lexeme = lexemes.next();
        switch (current_lexeme.get_type())
        {
        case LexemeType::PLUS:   
            // ���������� ������� ��������� � ��������� ����������� ���������/�������
            current_value = current_value + multiplication_or_division(lexemes);
            break;
        case LexemeType::MINUS:
            // �������� �� �������� ���������� ��������� ��������� ���������/�������
            current_value = current_value - multiplication_or_division(lexemes);
            break;            
        default:
            lexemes.back();
            return current_value;           
        }
    }
    return current_value;
}

// � ��������� ������� ����������� �������� ������� � ���������
// ����������� ��������� ������
double multiplication_or_division(LexemeBuffer& lexemes){
    // �������� ����������� ��������� �����
    double current_value = number_or_expression_in_brackets(lexemes);

    // ������� ��� ���������
    while (lexemes.get_current_index() != lexemes.get_size() - 1) {

        Lexeme current_lexeme = lexemes.next();
            switch (current_lexeme.get_type())
            {
            case LexemeType::MULTIPLY:
                // �������� ������� ��������� ���������/������� �� ��������� ����������� ��������� �����
                current_value = current_value * number_or_expression_in_brackets(lexemes);
                break;                    
            case LexemeType::DIVIDE:
                // ����� ������� ��������� ���������/������� �� ��������� ����������� ��������� �����
                current_value = current_value / number_or_expression_in_brackets(lexemes);
                break;
            default:
                lexemes.back();
                return current_value;
                  
            }
    }
    return current_value;
}

// ����������� ��������� ����� ��������� ��� ����� ��� ��������� � �������
// ��� ����������� ����� �� ����� ������������ � ����� �������
double number_or_expression_in_brackets(LexemeBuffer& lexemes){

    Lexeme current_lexeme = lexemes.next();
    double value = 0.0;
    switch (current_lexeme.get_type())
    {
    case LexemeType::NUMBER:
        return std::atof(current_lexeme.get_value().c_str());
                    
    case LexemeType::LEFT_BRACKET:
        // ��������� ��������� � �������
        value = expression(lexemes);
        // ���� ��� ����������� ������
        if (lexemes.next().get_type() != LexemeType::RIGHT_BRACKET) {
            printf("����������� ����������� ������");
            return 0.0;
        }
        return value;                    
    default:
        printf("� ����������� ��������� ����� ������ ������� �������");
        return 0.0;                    
    }             
}

std::string figure_expression(char* expression_string) {
    // �������� LexemeBuffer �� ������� ������
    LexemeBuffer lexemes = LexemeBuffer(string_to_lexemes(expression_string));
    // ������� ��������� ���������� LexemeBuffer
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
        // �������� ������ ����������� ������
        recv(connections[(intptr_t)connection_number], (char*)&expression_size, sizeof(int), 0);
        //thread_output("������ " + std::to_string((int)connection_number) + ": ����� �������� " + std::to_string(expression_size)+" ��������.");
        char *expression=new char[expression_size+1];
        expression[expression_size] = '\0';
        // �������� ���� ������
        recv(connections[(intptr_t)connection_number], expression, expression_size, 0);
        
        if (is_string_equal(expression,"End of operation")) {
            thread_output("������ " + std::to_string((int)connection_number) + ": �������� �����.");
            delete[] expression;
            break;
        }
        else
        {
            thread_output("������ " + std::to_string((int)connection_number) + ": ���������: " + std::string(expression));
        }/*
        for (int i = 0; i < expression_size; i++) {
            thread_output("������ " + std::to_string((int)connection_number) + ": ���������[" + std::to_string(i) + "]: " + expression[i]);
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
        thread_output("������ " + std::to_string((int)connection_number) + ": ���������: " + std::string(result));
        delete [] expression;
    }
    available_connections.push((int)connection_number);
    return 0;
}

void create_new_thread(intptr_t thread_name) {
    DWORD new_thread_id;
    threads[thread_name] = CreateThread(NULL, 0, client_handler, (void*)thread_name, 0, &new_thread_id);
    if (threads[thread_name] == NULL)
        std::cout << "����� " << thread_name << " �� ��� ������." << std::endl;

}
bool intit_WS2_32_lib() {
    // ������������� ��������� ����������
    // ��������� WSADATA �������� ���������� � ���������� ������� Windows
    WSADATA wsaData;
    // WSAStartup ���������� ���������� WS2_32.lib
    /*MAKEWORD(2, 2) ����������� ������ WinSock ������� (�� 2 �� 2)
    � ������������� �� ��� ��������� ���������� ������ ������� Windows, ������� ����� �������������*/
    int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (iResult != NO_ERROR) {
        printf("Error at WSAStartup()\n");
        system("Pause");
        return false;
    }
    return true;
}

bool create_server_socket(SOCKET& created_socket) {
    
    // ������������� ������
    // TCP - transmission control protocol: �������� ��������, ��������������� �� ���������� ������� ������ ��� �������
    /* UPD - user data protocol : ���������� �������� �������.
    ������������ �������� ������ � ����� ����.
    ������������ ��� ��������� ������� ��������� ������.*/
    // AF_INET - ��������� ������� �������� IPv4
    // SOCK_STREAM - ���������� ������� ����(TCP)
    // SOCK_SEQPACKET - ���������� ������� �������(TCP)
    // SOCK_RAW - ����� ������, � �� ������� ��������� ����� ����������� ����������� ��������� � ���������.
    // SOCK_DGRAM - �������� ������ � ���� ��������� ��������� ��������(UDP)
    // IPPROTO_TCP - �������� ���������� TCP
    // IPPROTO_UDP - �������� ���������� UPD
    // 0 �������� �� ���������
    created_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    if (created_socket == INVALID_SOCKET) {
        printf("Error at socket(): %ld\n", WSAGetLastError());
        closesocket(created_socket);
        WSACleanup();// ��������������� ������� Win32API
        system("Pause");
        return false;
    }

    // ��������� ���������� ������ � ���������� ���� IP-�����/���� 
    // � ����� ������� ������ ����� (��� ���) ����� ��������������� � ���������� ���������, 
    // ������� ������ �� ���������� ������ � �����.
    struct sockaddr_in server_address; // ������ ��������� ��������������� ���������� � ��������� �������
    server_address.sin_family = AF_INET; // ��������� ������� ��������
    /*
     �������� ������� inet_addr �� ������
    in_addr ip_to_num;
    errorStatus = inet_pton(AF_INET, SERVER_IP, &ip_to_num);
    if (errorStatus <= 0) {
        cout << "Error in IP translation to special numeric format" << endl;
        return 1;
    }
    server_address.sin_addr.s_addr = ip_to_num;
    */
    server_address.sin_addr.s_addr = inet_addr(SERVER_IP); // ��������� IP � ������� ����� ������ �����
    server_address.sin_port = htons(SERVER_PORT); // ����� �����, � ������� ����� ������ �����

    if (bind(
        created_socket,// ��������� �����
        (sockaddr*)&server_address, // ����� � �������� �� �������������
        sizeof(server_address)) == SOCKET_ERROR) {
        // ���� �� ���������� ���������
        printf("bind() failed.\n");
        closesocket(created_socket);
        WSACleanup();// ��������������� ������� Win32API
        system("Pause");
        return false;
    }

    // ������������� ������, 
    int max_connected_process = total_connections; // ���� ������ �������� ��������� �����������, ��� ����� ��������
    if (listen(created_socket, SOMAXCONN_HINT(max_connected_process)) == SOCKET_ERROR) {
        printf("������ ������������� ������.\n");
        closesocket(created_socket);
        WSACleanup();// ��������������� ������� Win32API
        system("Pause");
        return false;
    }
    return true;
}



void listen_server_connection(const SOCKET& listening_socket) {

    // Accept / ������������� ����������� (������ �� ������� �������)/ �������� ������� �� ������������ ����������
    SOCKET AcceptSocket;// ��������� �����
    printf("�������� ����������� ��������...\n");
    while (1) {
        AcceptSocket = SOCKET_ERROR;
        // ����������� �������� ������� �� ����������
        while (AcceptSocket == SOCKET_ERROR) {
            AcceptSocket = accept(listening_socket, // "���������" ����� �� ������� �������
                /*sockaddr* */  NULL, // ��������� �� ������ ��������� sockaddr, � ������� ����� �������� ���������� �� ��������������� �������
                                NULL); // sizeof(sockaddr*)
        }
        if (available_connections.empty()) {
            printf("������. � ������� ��� ��������� ������\n");
        }
        else
        {
            int current_socket = available_connections.front();
            available_connections.pop();

            printf("������ %d �����������.\n", current_socket);

            connections[current_socket] = AcceptSocket;
            // ��������� ���������� �������� ���������� � ��������� ������
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
        std::cout << "������ �������� ��������."<< std::endl;
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
    WSACleanup();// ��������������� ������� Win32API
    printf("������ �������� ���� ������.\n");
    system("Pause");
    return 0;
}

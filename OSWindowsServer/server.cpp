// This code is not protected by any copyright as far as I know
// It is based on misc public sources on the Internet, including MSDN
// Prepared by Mark Polyak in 2020

#include <stdio.h>
#include <winsock2.h> // Заголовочный файл, содержащий актуальные реализации функций для работы с сокетами

int main() {
    // Инициализация сокетного интерфейса
    // Структура WSADATA содержит информацию о реализации сокетов Windows
    WSADATA wsaData;
    // WSAStartup подключает библиотеку WS2_32.lib
    /*MAKEWORD(2, 2) запрашивает версию WinSock системы (от 2 до 2)
    и устанавливает ее как наивысшую допустимую версию сокетов Windows, которая может использоватся*/
    int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if ( iResult != NO_ERROR )
        printf("Error at WSAStartup()\n");

    // Инициализация сокета
    // TCP - transmission control protocol: надежный протокол, ориентированный на соединений потоков байтов или пакетов
    /* UPD - user data protocol : ненадежная передача пакетов.
    Пользователь получает доступ к голой сети.
    Пользователь сам реализует систему обработки ошибок.*/ 
    SOCKET m_socket;// Структура сокет
    // AF_INET - семейство адресов интернет IPv4
    // SOCK_STREAM - соединение потоков байт(TCP)
    // SOCK_SEQPACKET - соединение потоков пакетов(TCP)
    // SOCK_RAW - сырые сокеты, с их помощью программа может формировать собственные заголовки в сообщении.
    // SOCK_DGRAM - передача данных в виде отдельных сообщений датаграм(UDP)
    // IPPROTO_TCP - протокол соединения TCP
    // IPPROTO_UDP - протокол соединения UPD
    // 0 протокол по умолчанию
    m_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    if ( m_socket == INVALID_SOCKET ) {
        printf("Error at socket(): %ld\n", WSAGetLastError() );
        WSACleanup();// деинициализация сокетов Win32API
        return 1;
    }

    // «Привязка» созданного сокета к конкретной паре IP-адрес/Порт 
    // с этого момента данный сокет (его имя) будет ассоциироваться с конкретным процессом, 
    // который «висит» по указанному адресу и порту.
    struct sockaddr_in service; // Объект структуры инкапсулирующий информацию о семействе адресов
    service.sin_family = AF_INET; // семейство адресов интернет
    /*
     Проверка функции inet_addr на ошибку
    in_addr ip_to_num;
    errorStatus = inet_pton(AF_INET, “127.0.0.1”, &ip_to_num);
    if (errorStatus <= 0) {
        cout << "Error in IP translation to special numeric format" << endl;
        return 1;
    }
    service.sin_addr.s_addr = ip_to_num;*/
    service.sin_addr.s_addr = inet_addr("127.0.0.1"); // Локальный IP с которым будет связан сокет
    service.sin_port = htons(27015); // Номер порта, с которым будет связан сокет

    if ( bind(
            m_socket,// Созданный сокет
            reinterpret_cast<SOCKADDR*> &service, // Адрес к которому он привязывается
            sizeof(service) )   == SOCKET_ERROR) {
        // Если не получилось привязать
        printf("bind() failed.\n");
        closesocket(m_socket);
        WSACleanup();// деинициализация сокетов Win32API
        return 1;
    }

    // Прослушивание сокета m_socket, 
    int maxConnectedProcess = 1; // Если больше клиентов попробует подключится, они будут сброшены
    if ( listen( m_socket, SOMAXCONN_HINT(maxConnectedProcess)) == SOCKET_ERROR )
        printf("Error listening on socket.\n");

    // Accept / Подтверждение подключения (обычно на стороне сервера)/ Принятие запроса на установление соединения
    SOCKET AcceptSocket;// Временный сокет

    printf("Waiting for a client to connect...\n");
    while (1) {
        AcceptSocket = SOCKET_ERROR;
        // Бесконечное ожидание запроса на соединение
        while ( AcceptSocket == SOCKET_ERROR ) {
            AcceptSocket = accept(m_socket, // "слушающий" сокет на стороне Сервера
             /*sockaddr* */ NULL, // указатель на пустую структуру sockaddr, в которую будет записана информация по подключившемуся Клиенту
                            NULL); // sizeof(sockaddr*)
        }
        printf("Client Connected.\n");
        // Передача управления от временного сокета, к основному
        m_socket = AcceptSocket;
        break;
    }

    // Обмен данными между процессами через установленное сокетное соединение.
    int bytesSent;
    int bytesRecv = SOCKET_ERROR;
    char sendbuf[32] = "Server: Sending Data.";
    char recvbuf[32] = "";

    // Считать 32 бита в из m_socket в recvbuf
    // флаги 0
    bytesRecv = recv(m_socket, recvbuf, 32, 0);
    printf("Bytes Recv: %ld\n", bytesRecv);
    // Отправить в m_socket все содержимое sendbuf
    // флаги 0
    bytesSent = send(m_socket, sendbuf, strlen(sendbuf), 0);
    printf("Bytes Sent: %ld\n", bytesSent);

    return 0;
}

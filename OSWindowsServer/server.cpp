// This code is not protected by any copyright as far as I know
// It is based on misc public sources on the Internet, including MSDN
// Prepared by Mark Polyak in 2020

#include <stdio.h>
#include <winsock2.h> // ������������ ����, ���������� ���������� ���������� ������� ��� ������ � ��������

int main() {
    // ������������� ��������� ����������
    // ��������� WSADATA �������� ���������� � ���������� ������� Windows
    WSADATA wsaData;
    // WSAStartup ���������� ���������� WS2_32.lib
    /*MAKEWORD(2, 2) ����������� ������ WinSock ������� (�� 2 �� 2)
    � ������������� �� ��� ��������� ���������� ������ ������� Windows, ������� ����� �������������*/
    int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if ( iResult != NO_ERROR )
        printf("Error at WSAStartup()\n");

    // ������������� ������
    // TCP - transmission control protocol: �������� ��������, ��������������� �� ���������� ������� ������ ��� �������
    /* UPD - user data protocol : ���������� �������� �������.
    ������������ �������� ������ � ����� ����.
    ������������ ��� ��������� ������� ��������� ������.*/ 
    SOCKET m_socket;// ��������� �����
    // AF_INET - ��������� ������� �������� IPv4
    // SOCK_STREAM - ���������� ������� ����(TCP)
    // SOCK_SEQPACKET - ���������� ������� �������(TCP)
    // SOCK_RAW - ����� ������, � �� ������� ��������� ����� ����������� ����������� ��������� � ���������.
    // SOCK_DGRAM - �������� ������ � ���� ��������� ��������� ��������(UDP)
    // IPPROTO_TCP - �������� ���������� TCP
    // IPPROTO_UDP - �������� ���������� UPD
    // 0 �������� �� ���������
    m_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    if ( m_socket == INVALID_SOCKET ) {
        printf("Error at socket(): %ld\n", WSAGetLastError() );
        WSACleanup();// ��������������� ������� Win32API
        return 1;
    }

    // ��������� ���������� ������ � ���������� ���� IP-�����/���� 
    // � ����� ������� ������ ����� (��� ���) ����� ��������������� � ���������� ���������, 
    // ������� ������ �� ���������� ������ � �����.
    struct sockaddr_in service; // ������ ��������� ��������������� ���������� � ��������� �������
    service.sin_family = AF_INET; // ��������� ������� ��������
    /*
     �������� ������� inet_addr �� ������
    in_addr ip_to_num;
    errorStatus = inet_pton(AF_INET, �127.0.0.1�, &ip_to_num);
    if (errorStatus <= 0) {
        cout << "Error in IP translation to special numeric format" << endl;
        return 1;
    }
    service.sin_addr.s_addr = ip_to_num;*/
    service.sin_addr.s_addr = inet_addr("127.0.0.1"); // ��������� IP � ������� ����� ������ �����
    service.sin_port = htons(27015); // ����� �����, � ������� ����� ������ �����

    if ( bind(
            m_socket,// ��������� �����
            reinterpret_cast<SOCKADDR*> &service, // ����� � �������� �� �������������
            sizeof(service) )   == SOCKET_ERROR) {
        // ���� �� ���������� ���������
        printf("bind() failed.\n");
        closesocket(m_socket);
        WSACleanup();// ��������������� ������� Win32API
        return 1;
    }

    // ������������� ������ m_socket, 
    int maxConnectedProcess = 1; // ���� ������ �������� ��������� �����������, ��� ����� ��������
    if ( listen( m_socket, SOMAXCONN_HINT(maxConnectedProcess)) == SOCKET_ERROR )
        printf("Error listening on socket.\n");

    // Accept / ������������� ����������� (������ �� ������� �������)/ �������� ������� �� ������������ ����������
    SOCKET AcceptSocket;// ��������� �����

    printf("Waiting for a client to connect...\n");
    while (1) {
        AcceptSocket = SOCKET_ERROR;
        // ����������� �������� ������� �� ����������
        while ( AcceptSocket == SOCKET_ERROR ) {
            AcceptSocket = accept(m_socket, // "���������" ����� �� ������� �������
             /*sockaddr* */ NULL, // ��������� �� ������ ��������� sockaddr, � ������� ����� �������� ���������� �� ��������������� �������
                            NULL); // sizeof(sockaddr*)
        }
        printf("Client Connected.\n");
        // �������� ���������� �� ���������� ������, � ���������
        m_socket = AcceptSocket;
        break;
    }

    // ����� ������� ����� ���������� ����� ������������� �������� ����������.
    int bytesSent;
    int bytesRecv = SOCKET_ERROR;
    char sendbuf[32] = "Server: Sending Data.";
    char recvbuf[32] = "";

    // ������� 32 ���� � �� m_socket � recvbuf
    // ����� 0
    bytesRecv = recv(m_socket, recvbuf, 32, 0);
    printf("Bytes Recv: %ld\n", bytesRecv);
    // ��������� � m_socket ��� ���������� sendbuf
    // ����� 0
    bytesSent = send(m_socket, sendbuf, strlen(sendbuf), 0);
    printf("Bytes Sent: %ld\n", bytesSent);

    return 0;
}

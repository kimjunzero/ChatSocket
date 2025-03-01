#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <WinSock2.h>
#include <windows.h>
#include <string.h>
#pragma comment(lib, "ws2_32.lib")
#define IS_CLIENT
#define SERVER_IP "127.0.0.1"
#define PORT 8080
#define BUFFER_SIZE 1024
#define QUEUE_SIZE 100
DWORD WINAPI thread_tx(LPVOID param);
DWORD WINAPI thread_rx(LPVOID param);
DWORD WINAPI thread_update(LPVOID param);
SOCKET sock, new_socket;
// 큐 구현
char queue[QUEUE_SIZE][BUFFER_SIZE];
int front = 0, rear = 0;
void enqueue(const char* msg) {
    if ((rear + 1) % QUEUE_SIZE == front) return;  // 큐가 가득 차면 무시
    strcpy(queue[rear], msg);
    rear = (rear + 1) % QUEUE_SIZE;
}int dequeue(char* msg) {
    if (front == rear) return 0;
    strcpy(msg, queue[front]);
    front = (front + 1) % QUEUE_SIZE;
    return 1;
}int main() {
    WSADATA wsa;
    WSAStartup(MAKEWORD(2, 2), &wsa);
    struct sockaddr_in address;
    int addrlen = sizeof(address);
    sock = socket(AF_INET, SOCK_STREAM, 0);
    address.sin_family = AF_INET;
    address.sin_port = htons(PORT);
#ifdef IS_CLIENT
    inet_pton(AF_INET, SERVER_IP, &address.sin_addr);
    connect(sock, (struct sockaddr*)&address, sizeof(address));
#else
    address.sin_addr.s_addr = INADDR_ANY;
    bind(sock, (struct sockaddr*)&address, sizeof(address));
    listen(sock, 3);
    new_socket = accept(sock, (struct sockaddr*)&address, &addrlen);
#endif
    // 스레드 생성
    HANDLE thread_transmit, thread_receive, thread_update_handle;
    thread_transmit = CreateThread(NULL, 0, thread_tx, NULL, 0, NULL);
    thread_receive = CreateThread(NULL, 0, thread_rx, NULL, 0, NULL);
    thread_update_handle = CreateThread(NULL, 0, thread_update, NULL, 0, NULL);
    while (1);
    closesocket(new_socket);
    closesocket(sock);
    CloseHandle(thread_transmit);
    CloseHandle(thread_receive);
    CloseHandle(thread_update_handle);
    return 0;
}// 송신 스레드
DWORD WINAPI thread_tx(LPVOID param) {
    char txBuffer[BUFFER_SIZE];
    while (1) {
        fgets(txBuffer, sizeof(txBuffer), stdin);
#ifdef IS_CLIENT
        send(sock, txBuffer, strlen(txBuffer), 0);
#else
        send(new_socket, txBuffer, strlen(txBuffer), 0);
#endif
    }
    return 0;
}// 수신 스레드
DWORD WINAPI thread_rx(LPVOID param) {
    char buffer[BUFFER_SIZE] = { 0 };
    while (1) {
        memset(buffer, 0, sizeof(buffer));
#ifdef IS_CLIENT
        recv(sock, buffer, BUFFER_SIZE, 0);
#else
        recv(new_socket, buffer, BUFFER_SIZE, 0);
#endif
        enqueue(buffer);
    }
    return 0;
}// 1초마다 printf 실행하는 스레드
DWORD WINAPI thread_update(LPVOID param) {
    char msg[BUFFER_SIZE];
    while (1) {
        Sleep(1000); // 1초 대기
        if (dequeue(msg)) {
            printf("출력: %s\n", msg);
        }
    }
    return 0;
}
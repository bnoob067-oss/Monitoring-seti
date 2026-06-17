#pragma once

#include <windows.h>
#include <iphlpapi.h>
#include <psapi.h>

#pragma comment(lib, "iphlpapi.lib")
#pragma comment(lib, "psapi.lib")

// Структура для хранения данных соединения
struct NativeConnectionData
{
    char LocalAddr[16];
    int LocalPort;
    char RemoteAddr[16];
    int RemotePort;
    char State[16];
    char Protocol[8];
    DWORD ProcessId;
};

// Получение TCP соединений
DWORD GetNativeTcpConnections(NativeConnectionData** connections, DWORD* count);

// Получение UDP соединений
DWORD GetNativeUdpConnections(NativeConnectionData** connections, DWORD* count);

// Освобождение памяти
void FreeNativeConnections(NativeConnectionData* connections);

// Получение имени процесса по PID
BOOL GetProcessNameByPid(DWORD pid, char* name, DWORD nameSize);
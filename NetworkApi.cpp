#define _CRT_SECURE_NO_WARNINGS
#include "NetworkApi.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

// Вспомогательная функция для преобразования порта
static int ConvertPort(DWORD port)
{
    return (port >> 8) | ((port & 0xFF) << 8);
}

DWORD GetNativeTcpConnections(NativeConnectionData** connections, DWORD* count)
{
    DWORD dwSize = 0;
    DWORD dwRetVal = 0;
    PMIB_TCPTABLE_OWNER_PID pTcpTable = NULL;

    // Первый вызов: узнаем необходимый размер буфера
    dwRetVal = GetExtendedTcpTable(NULL, &dwSize, FALSE, AF_INET, TCP_TABLE_OWNER_PID_ALL, 0);

    if (dwRetVal != ERROR_INSUFFICIENT_BUFFER)
    {
        return dwRetVal;
    }

    // Выделяем память
    pTcpTable = (PMIB_TCPTABLE_OWNER_PID)malloc(dwSize);
    if (pTcpTable == NULL)
    {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    // Второй вызов: получаем реальные данные
    dwRetVal = GetExtendedTcpTable(pTcpTable, &dwSize, FALSE, AF_INET, TCP_TABLE_OWNER_PID_ALL, 0);

    if (dwRetVal != NO_ERROR)
    {
        free(pTcpTable);
        return dwRetVal;
    }

    *count = pTcpTable->dwNumEntries;
    *connections = (NativeConnectionData*)malloc(sizeof(NativeConnectionData) * (*count));

    if (*connections == NULL)
    {
        free(pTcpTable);
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    // Заполняем данные для каждого TCP соединения
    for (DWORD i = 0; i < *count; i++)
    {
        MIB_TCPROW_OWNER_PID row = pTcpTable->table[i];
        NativeConnectionData* conn = &(*connections)[i];

        // Протокол
        strcpy_s(conn->Protocol, 8, "TCP");

        // Локальный адрес
        sprintf_s(conn->LocalAddr, 16, "%d.%d.%d.%d",
            (row.dwLocalAddr >> 0) & 0xFF,
            (row.dwLocalAddr >> 8) & 0xFF,
            (row.dwLocalAddr >> 16) & 0xFF,
            (row.dwLocalAddr >> 24) & 0xFF);

        // Локальный порт
        conn->LocalPort = ConvertPort(row.dwLocalPort);

        // Удаленный адрес
        sprintf_s(conn->RemoteAddr, 16, "%d.%d.%d.%d",
            (row.dwRemoteAddr >> 0) & 0xFF,
            (row.dwRemoteAddr >> 8) & 0xFF,
            (row.dwRemoteAddr >> 16) & 0xFF,
            (row.dwRemoteAddr >> 24) & 0xFF);

        // Удаленный порт
        conn->RemotePort = ConvertPort(row.dwRemotePort);

        // Состояние соединения
        switch (row.dwState)
        {
        case MIB_TCP_STATE_CLOSED:     strcpy_s(conn->State, 16, "CLOSED"); break;
        case MIB_TCP_STATE_LISTEN:     strcpy_s(conn->State, 16, "LISTENING"); break;
        case MIB_TCP_STATE_SYN_SENT:   strcpy_s(conn->State, 16, "SYN_SENT"); break;
        case MIB_TCP_STATE_SYN_RCVD:   strcpy_s(conn->State, 16, "SYN_RCVD"); break;
        case MIB_TCP_STATE_ESTAB:      strcpy_s(conn->State, 16, "ESTABLISHED"); break;
        case MIB_TCP_STATE_FIN_WAIT1:  strcpy_s(conn->State, 16, "FIN_WAIT1"); break;
        case MIB_TCP_STATE_FIN_WAIT2:  strcpy_s(conn->State, 16, "FIN_WAIT2"); break;
        case MIB_TCP_STATE_CLOSE_WAIT: strcpy_s(conn->State, 16, "CLOSE_WAIT"); break;
        case MIB_TCP_STATE_CLOSING:    strcpy_s(conn->State, 16, "CLOSING"); break;
        case MIB_TCP_STATE_LAST_ACK:   strcpy_s(conn->State, 16, "LAST_ACK"); break;
        case MIB_TCP_STATE_TIME_WAIT:  strcpy_s(conn->State, 16, "TIME_WAIT"); break;
        case MIB_TCP_STATE_DELETE_TCB: strcpy_s(conn->State, 16, "DELETE_TCB"); break;
        default:                       strcpy_s(conn->State, 16, "UNKNOWN"); break;
        }

        // PID процесса
        conn->ProcessId = row.dwOwningPid;
    }

    // Освобождаем временный буфер
    free(pTcpTable);
    return NO_ERROR;
}

DWORD GetNativeUdpConnections(NativeConnectionData** connections, DWORD* count)
{
    DWORD dwSize = 0;
    DWORD dwRetVal = 0;
    PMIB_UDPTABLE_OWNER_PID pUdpTable = NULL;

    // Первый вызов: узнаем необходимый размер буфера
    dwRetVal = GetExtendedUdpTable(NULL, &dwSize, FALSE, AF_INET, UDP_TABLE_OWNER_PID, 0);

    if (dwRetVal != ERROR_INSUFFICIENT_BUFFER)
    {
        return dwRetVal;
    }

    // Выделяем память
    pUdpTable = (PMIB_UDPTABLE_OWNER_PID)malloc(dwSize);
    if (pUdpTable == NULL)
    {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    // Второй вызов: получаем реальные данные
    dwRetVal = GetExtendedUdpTable(pUdpTable, &dwSize, FALSE, AF_INET, UDP_TABLE_OWNER_PID, 0);

    if (dwRetVal != NO_ERROR)
    {
        free(pUdpTable);
        return dwRetVal;
    }

    *count = pUdpTable->dwNumEntries;
    *connections = (NativeConnectionData*)malloc(sizeof(NativeConnectionData) * (*count));

    if (*connections == NULL)
    {
        free(pUdpTable);
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    // Заполняем данные для каждого UDP соединения
    for (DWORD i = 0; i < *count; i++)
    {
        MIB_UDPROW_OWNER_PID row = pUdpTable->table[i];
        NativeConnectionData* conn = &(*connections)[i];

        // Протокол
        strcpy_s(conn->Protocol, 8, "UDP");

        // Локальный адрес
        sprintf_s(conn->LocalAddr, 16, "%d.%d.%d.%d",
            (row.dwLocalAddr >> 0) & 0xFF,
            (row.dwLocalAddr >> 8) & 0xFF,
            (row.dwLocalAddr >> 16) & 0xFF,
            (row.dwLocalAddr >> 24) & 0xFF);

        // Локальный порт
        conn->LocalPort = ConvertPort(row.dwLocalPort);

        // Для UDP нет удаленного адреса и порта
        strcpy_s(conn->RemoteAddr, 16, "*");
        conn->RemotePort = 0;

        // Состояние для UDP всегда LISTENING
        strcpy_s(conn->State, 16, "LISTENING");

        // PID процесса
        conn->ProcessId = row.dwOwningPid;
    }

    // Освобождаем временный буфер
    free(pUdpTable);
    return NO_ERROR;
}

void FreeNativeConnections(NativeConnectionData* connections)
{
    if (connections != NULL)
    {
        free(connections);
    }
}

BOOL GetProcessNameByPid(DWORD pid, char* name, DWORD nameSize)
{
    HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pid);

    if (hProcess == NULL)
    {
        // Пробуем с меньшими правами
        hProcess = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, pid);
        if (hProcess == NULL)
        {
            if (name != NULL && nameSize > 0)
            {
                strncpy_s(name, nameSize, "System", nameSize - 1);
            }
            return FALSE;
        }
    }

    HMODULE hMod;
    DWORD cbNeeded;

    if (EnumProcessModules(hProcess, &hMod, sizeof(hMod), &cbNeeded))
    {
        GetModuleBaseNameA(hProcess, hMod, name, nameSize);
        CloseHandle(hProcess);
        return TRUE;
    }

    CloseHandle(hProcess);
    if (name != NULL && nameSize > 0)
    {
        strncpy_s(name, nameSize, "Unknown", nameSize - 1);
    }
    return FALSE;
}
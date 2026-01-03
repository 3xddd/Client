#pragma once
#include <afxsock.h>
#include <string>

class TcpClient
{
public:
    TcpClient();
    ~TcpClient();

    bool Connect(const CString& ip, int port);
    bool EnableAsync(HWND hWnd, UINT msg);
    void SendString(const std::string& data);
    int Receive(char* buf, int len);
    void Close();
    bool IsConnected() const;

private:
    CSocket m_socket;
    bool m_connected;
};

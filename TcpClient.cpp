#include "pch.h"
#include "TcpClient.h"
#pragma warning(disable:4996)
TcpClient::TcpClient()
    : m_connected(false)
{
    // 构造函数 初始化连接状态为未连接
}

TcpClient::~TcpClient()
{
    // 析构函数 释放资源 避免句柄泄露
    Close();
}

bool TcpClient::Connect(const CString& ip, int port)
{
    // 创建套接字对象
    if (!m_socket.Create())
        return false;

    // 连接到指定的服务器地址和端口
    if (!m_socket.Connect(ip, port))
        return false;

    // 连接成功 设置连接状态
    m_connected = true;
    return true;
}

bool TcpClient::EnableAsync(HWND hWnd, UINT msg)
{
    // 未连接则不能启用异步通知
    if (!m_connected) return false;

    // 启用异步通知
    // 当套接字有数据可读时触发 FD_READ
    // 当连接被关闭时触发 FD_CLOSE
    // 触发后会向指定窗口 hWnd 投递 msg 消息
    // 消息参数中包含事件类型和错误码 窗口过程里解析后再调用 Receive 或 Close
    if (::WSAAsyncSelect(m_socket.m_hSocket, hWnd, msg, FD_READ | FD_CLOSE) != 0)
        return false;

    return true;
}

void TcpClient::SendString(const std::string& data)
{
    // 未连接则不发送
    if (!m_connected) return;

    // 循环发送直到把数据尽量发送完
    // Send 可能一次只发送部分数据 所以要累加已发送长度
    int total = 0;
    int len = (int)data.size();
    while (total < len)
    {
        int n = m_socket.Send(data.c_str() + total, len - total);

        // 发送失败或连接异常则退出
        // 如果启用了异步模式 这里也可能出现暂时不可发送的情况
        if (n <= 0) break;

        // 更新已发送字节数
        total += n;
    }
}

int TcpClient::Receive(char* buf, int len)
{
    // 未连接则返回错误
    if (!m_connected) return -1;

    // 从套接字接收数据
    // 返回值为实际接收字节数 也可能为 0 表示对端关闭 或小于 0 表示错误
    return m_socket.Receive(buf, len);
}

void TcpClient::Close()
{
    // 只有在已连接状态才关闭
    if (m_connected)
    {
        // 关闭套接字 释放底层资源
        m_socket.Close();

        // 更新连接状态
        m_connected = false;
    }
}

bool TcpClient::IsConnected() const
{
    // 返回当前是否处于连接状态
    return m_connected;
}

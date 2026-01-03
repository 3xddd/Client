#pragma once
#include <vector>
#include <string>
#include "Patient.h"
#include "TcpClient.h"

// 定义套接字异步通知消息编号
// 该消息用于接收 WSAAsyncSelect 投递的网络事件通知
// 使用 WM_USER 自定义消息范围 避免与系统消息冲突
#ifndef WM_SOCKET_NOTIFY
#define WM_SOCKET_NOTIFY (WM_USER + 200)
#endif

// 主对话框类
class CClientAppDlg : public CDialogEx
{
public:
    // 构造函数 pParent 为父窗口指针 一般为 nullptr
    CClientAppDlg(CWnd* pParent = nullptr);

#ifdef AFX_DESIGN_TIME
    // 对话框资源 ID
    enum { IDD = IDD_CLIENTAPP_DIALOG };
#endif

protected:
    virtual void DoDataExchange(CDataExchange* pDX);
    virtual BOOL OnInitDialog();
    afx_msg void OnBnClickedButtonAddPatient();
    afx_msg void OnBnClickedButtonConnect();
    afx_msg void OnBnClickedButtonStart();
    afx_msg void OnTimer(UINT_PTR nIDEvent);

    // 套接字异步事件消息处理函数
    // wParam 一般包含触发事件的套接字句柄
    // lParam 包含事件类型和错误码 需要用 WSAGETSELECTEVENT 和 WSAGETSELECTERROR 解析
    afx_msg LRESULT OnSocketEvent(WPARAM wParam, LPARAM lParam);

    // MFC 消息映射声明
    DECLARE_MESSAGE_MAP()

private:
    std::vector<Patient> m_patients;
    TcpClient m_tcp;
    // 接收缓存 用于处理拆包粘包 将接收到的数据先缓存再按行解析
    std::string m_rxCache;
    void HandleServerLine(const std::string& line);
};

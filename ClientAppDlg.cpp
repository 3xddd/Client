#include "pch.h"
#include "ClientApp.h"
#include "ClientAppDlg.h"

#include <sstream>
#include <vector>

// CSV拆分 支持引号包裹字段 支持双引号转义
static std::vector<std::string> SplitCsvLine(const std::string& line)
{
    // out 保存拆分后的每一列
    std::vector<std::string> out;

    // cur 保存当前正在解析的字段内容
    std::string cur;

    // inQuote 表示当前是否在引号内部
    bool inQuote = false;

    for (size_t i = 0; i < line.size(); ++i)
    {
        char ch = line[i];

        // 如果当前在引号内部 逗号不作为分隔符
        if (inQuote)
        {
            // 遇到引号 可能是结束引号 也可能是转义引号
            if (ch == '"')
            {
                // 如果下一个字符还是引号 表示一个转义后的双引号
                if (i + 1 < line.size() && line[i + 1] == '"')
                {
                    cur.push_back('"');
                    ++i;
                }
                else
                {
                    // 结束引号
                    inQuote = false;
                }
            }
            else
            {
                // 引号内部的普通字符直接加入字段
                cur.push_back(ch);
            }
        }
        else
        {
            // 不在引号内部时 遇到引号表示进入引号模式
            if (ch == '"')
            {
                inQuote = true;
            }
            else if (ch == ',')
            {
                // 逗号作为字段分隔符 将当前字段放入结果并清空
                out.push_back(cur);
                cur.clear();
            }
            else
            {
                // 普通字符加入字段
                cur.push_back(ch);
            }
        }
    }

    // 最后一个字段也要加入结果
    out.push_back(cur);
    return out;
}

// UTF8 转 CString 宽字符
static CString Utf8ToCString(const std::string& u8)
{
    // 空字符串直接返回空 CString
    if (u8.empty()) return CString();

    // 先计算转换成宽字符需要的长度
    int wlen = MultiByteToWideChar(CP_UTF8, 0, u8.c_str(), -1, nullptr, 0);
    if (wlen <= 0) return CString();

    // 申请宽字符缓冲区
    std::wstring w(wlen, L'\0');

    // 进行转换
    MultiByteToWideChar(CP_UTF8, 0, u8.c_str(), -1, &w[0], wlen);

    // 去掉末尾的字符串结束符
    w.pop_back();

    // 构造 CString 返回
    return CString(w.c_str());
}

BEGIN_MESSAGE_MAP(CClientAppDlg, CDialogEx)
    // 添加病人按钮点击事件绑定
    ON_BN_CLICKED(IDC_BUTTON_ADD_PATIENT, &CClientAppDlg::OnBnClickedButtonAddPatient)

    // 连接服务器按钮点击事件绑定
    ON_BN_CLICKED(IDC_BUTTON_CONNECT, &CClientAppDlg::OnBnClickedButtonConnect)

    // 开始按钮点击事件绑定
    ON_BN_CLICKED(IDC_BUTTON_START, &CClientAppDlg::OnBnClickedButtonStart)

    // 定时器消息绑定
    ON_WM_TIMER()

    // 自定义套接字异步通知消息绑定
    // 当 socket 发生可读或关闭事件时 会投递 WM_SOCKET_NOTIFY 到本窗口
    ON_MESSAGE(WM_SOCKET_NOTIFY, &CClientAppDlg::OnSocketEvent)
END_MESSAGE_MAP()

CClientAppDlg::CClientAppDlg(CWnd* pParent)
    : CDialogEx(IDD_CLIENTAPP_DIALOG, pParent)
{
    // 构造函数 这里只做基础初始化
}

void CClientAppDlg::DoDataExchange(CDataExchange* pDX)
{
    // 控件和成员变量的数据交换
    CDialogEx::DoDataExchange(pDX);

    // 如果需要从界面输入端口等内容 可以在这里添加 DDX_Text
    //DDX_Text(pDX, IDC_EDIT_PORT, m_portStr);
}

BOOL CClientAppDlg::OnInitDialog()
{
    // 对话框初始化
    CDialogEx::OnInitDialog();

    // 初始化 MFC Socket 环境
    if (!AfxSocketInit())
        AfxMessageBox(_T("Socket初始化失败"));

    return TRUE;
}

void CClientAppDlg::OnBnClickedButtonAddPatient()
{
    // 从输入框读取病人姓名
    CString name;
    GetDlgItemText(IDC_EDIT_PATIENT_NAME, name);

    // 姓名为空则不处理
    if (name.IsEmpty()) return;

    // CString 转为窄字符串 便于后续网络发送
    CT2A nameA(name);

    // 添加到病人列表
    m_patients.emplace_back(std::string(nameA));

    // 提示添加成功
    CString msg;
    msg.Format(_T("%s 添加成功"), name);
    MessageBox(msg);
}

void CClientAppDlg::OnBnClickedButtonConnect()
{
    // 已连接则不重复连接
    if (m_tcp.IsConnected())
    {
        MessageBox(_T("已连接服务器"));
        return;
    }

    // 连接到本机服务器 端口 9000
    if (m_tcp.Connect(_T("127.0.0.1"), 9000))
    {
        // 开启异步通知 关注可读和关闭事件
        // 开启后 socket 事件会通过 WM_SOCKET_NOTIFY 消息通知到本窗口
        if (!m_tcp.EnableAsync(m_hWnd, WM_SOCKET_NOTIFY))
            MessageBox(_T("连接成功，但EnableAsync失败 无法接收报警"));
        else
            MessageBox(_T("服务器连接成功 已开启报警接收"));
    }
    else
    {
        // 连接失败提示
        MessageBox(_T("服务器连接失败"));
    }
}

void CClientAppDlg::OnBnClickedButtonStart()
{
    // 启动定时器
    // 计时器编号为 1 间隔 200 毫秒
    SetTimer(1, 200, nullptr);
}

void CClientAppDlg::OnTimer(UINT_PTR nIDEvent)
{
    // 定时器编号为 1 并且已连接服务器时 执行业务发送
    if (nIDEvent == 1 && m_tcp.IsConnected())
    {
        // 遍历所有病人 更新数据并发送给服务器
        for (auto& p : m_patients)
        {
            // 更新病人的模拟数据
            p.Update();

            // 生成要发送的字符串
            std::string data = p.BuildSendString();

            // 发送到服务器
            m_tcp.SendString(data);
        }
    }

    // 交给基类处理其他定时器逻辑
    CDialogEx::OnTimer(nIDEvent);
}

// Socket异步事件处理函数
// 负责接收服务器发来的报警信息
LRESULT CClientAppDlg::OnSocketEvent(WPARAM wParam, LPARAM lParam)
{
    // wParam 中一般是触发事件的 socket 句柄
    SOCKET hSocket = (SOCKET)wParam;

    // 从 lParam 解析事件类型
    int nEvent = WSAGETSELECTEVENT(lParam);

    // 从 lParam 解析错误码 0 表示无错误
    int nError = WSAGETSELECTERROR(lParam);

    // 如果发生错误 一般需要按连接关闭来处理
    if (nError != 0)
    {
        // 如果是关闭事件 关闭本地连接并提示
        if (nEvent == FD_CLOSE)
        {
            m_tcp.Close();
            MessageBox(_T("服务器断开连接"));
        }
        return 0;
    }

    // 可读事件 表示服务器发来了数据
    if (nEvent == FD_READ)
    {
        // 接收缓冲区
        char buf[2048];

        // 从 socket 接收数据
        int n = m_tcp.Receive(buf, sizeof(buf));

        // n 小于等于 0 表示无数据或连接异常
        if (n <= 0) return 0;

        // 追加到接收缓存中 解决分包粘包问题
        m_rxCache.append(buf, buf + n);

        // 按行解析 服务器用换行符分隔消息
        while (true)
        {
            // 查找一行结束位置
            size_t pos = m_rxCache.find('\n');
            if (pos == std::string::npos) break;

            // 取出一行数据
            std::string line = m_rxCache.substr(0, pos);

            // 从缓存中移除这一行和换行符
            m_rxCache.erase(0, pos + 1);

            // 兼容 Windows 行结尾 处理回车符
            if (!line.empty() && line.back() == '\r') line.pop_back();

            // 非空行才处理
            if (!line.empty())
                HandleServerLine(line);
        }
    }
    // 关闭事件 表示服务器主动断开
    else if (nEvent == FD_CLOSE)
    {
        m_tcp.Close();
        MessageBox(_T("服务器断开连接"));
    }

    return 0;
}

void CClientAppDlg::HandleServerLine(const std::string& line)
{
    // 服务器报警格式
    // ALARM,姓名,顶椎压力,腰椎压力,顶椎报警标志,腰椎报警标志
    if (line.rfind("ALARM,", 0) == 0)
    {
        // 按 CSV 拆分
        auto cols = SplitCsvLine(line);

        // 依次取字段 若不存在则给默认值
        std::string name = (cols.size() > 1) ? cols[1] : "未知";
        std::string top = (cols.size() > 2) ? cols[2] : "";
        std::string lum = (cols.size() > 3) ? cols[3] : "";
        int topFlag = (cols.size() > 4) ? atoi(cols[4].c_str()) : 0;
        int lumFlag = (cols.size() > 5) ? atoi(cols[5].c_str()) : 0;

        // UTF8 转 CString 便于界面显示中文
        CString wname = Utf8ToCString(name);
        CString wtop = Utf8ToCString(top);
        CString wlum = Utf8ToCString(lum);

        // 组合报警原因
        CString reason;
        if (topFlag) reason += _T("顶椎压力过高(>70N) ");
        if (lumFlag) reason += _T("腰椎压力过低(<15N)");

        // 组合报警提示内容
        CString msg;
        msg.Format(_T("报警提示！\r\n姓名：%s\r\n顶椎：%s N\r\n腰椎：%s N\r\n原因：%s"),
            wname, wtop, wlum, reason);

        // 弹出报警提示框
        AfxMessageBox(msg, MB_ICONWARNING);
        return;
    }

    // 如果以后服务器增加其他类型消息 可以在这里扩展处理逻辑
}

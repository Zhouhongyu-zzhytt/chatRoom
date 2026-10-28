#include"chatClient.h"

// 构造函数的定义
chatClient::chatClient(const char *ip, int port, const string &name) : name(name), running(true)
{
    // 创建客户端套接字
    cfd = socket(AF_INET, SOCK_STREAM, 0);
    if (cfd < 0)
    {
        perror("socket error");
        return;
    }

    // 连接服务器
    // 1、初始化地址信息结构体
    struct sockaddr_in sin;
    sin.sin_family = AF_INET;            // 通信域
    sin.sin_port = htons(port);          // 端口号
    sin.sin_addr.s_addr = inet_addr(ip); // 服务器ip

    // 2、连接服务器
    if (connect(cfd, (struct sockaddr *)&sin, sizeof(sin)) < 0)
    {
        perror("connect error");
        return;
    }

    // 程序执行至此，表示连接服务器成功
    // 顺便向服务器发送登录消息
    sendMsg(LOGIN);
}

// 析构函数的定义
chatClient::~chatClient()
{
    sendMsg(QUIT); // 向服务器发送退出消息
    close(cfd);    // 关闭套接字
}

//定义运行客户端函数
void chatClient::run()
{
    //创建一个分支线程，主要用于接收服务器端的数据
    thread recv_thread(&chatClient::recvMsg,this);

    //主线程向服务器发消息
    string text;
    while(running)
    {
        getline(cin,text);

        if(text=="quit")
        {
            running=false;
            
            break;
        }

        sendMsg(CHAT,text);
    }

    //回收线程资源
    recv_thread.join();
}

// 定义输出错误日志函数
void chatClient::errLog(const char *msg)
{
    // 输出错误的文件、功能、对应的错误行
    cerr << __FILE__ << " " << __func__ << " " << __LINE__ << endl;
    perror(msg);
}

//定义向服务器端发送消息的函数
void chatClient::sendMsg(int type,const string &text)
{
    MSG msg;
    msg.type=htonl(type);
    strncpy(msg.name,name.c_str(),sizeof(name));
    strncpy(msg.text,text.c_str(),sizeof(text));

    string data=msg.serialize();

    if (send(cfd, data.c_str(), data.size(), 0) < 0)
    {
        errLog("send error");
        return;
    }

    cout << "消息发送成功" << endl;

}

// 定义接收服务器发来消息的函数
void chatClient::recvMsg()
{
    // 用于接收消息
    char buffer[sizeof(MSG)];
    while (running)
    {
        // 非阻塞接收客户端消息
        int recv_len = recv(cfd, buffer, sizeof(buffer), MSG_DONTWAIT);
        if (recv_len < 0)
        {
            if (errno == EAGAIN)
            {
                // 休眠100ms
                usleep(100000);
            }
            else
            {
                // 错误
                errLog("recv error");
                running = false;
                break;
            }
        }
        else if (recv_len == 0)
        {
            // 错误
            errLog("recv error");
            running = false;
            break; // 连接关闭
        }
        else
        {
            // 将消息进行解析
            MSG msg;
            // 反序列化
            msg.deserialize(string(buffer, recv_len));
            cout << msg.name << "：" << msg.text << endl;
        }
    }
}



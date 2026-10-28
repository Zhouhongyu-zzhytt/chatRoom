//#if
#ifndef CHATSERVER_H
#define CHATSERVER_H

//include:线程池头文件+自定义头文件
#include<vector>
#include<queue>
#include<thread>
#include<mutex>
#include<condition_variable>
#include<functional>
#include<myhead.h>

//define宏定义；N，LOGIN，CHAT，QUIT
#define N 128    //消息缓冲区的大小
#define LOGIN 1
#define CHAT 2
#define QUIT 3


//自定义服务器类
//public:
//  struct MSG：包括消息类型，客户端名称，消息内容以及序列化反序列化函数
//  struct Client：包括客户端套接字和客户端地址信息结构体
//private：
//  服务器套接字，在线客户端列表，保护clients客户端列表的互斥锁
//  线程池相关变量
//  线程池相关函数以及错误信息日志函数
//public:
//  构造函数，析构函数
//  run(),handleClient(),broadcast()
class chatServer
{
public:
    struct MSG
    {
        int type;
        char name[20];
        char text[N];

        string serialize() const//const:这个函数承诺不修改对象的成员变量
        {
            string data;
            data.append(reinterpret_cast<const char*>(&type),sizeof(type));//按位重新解释类型
            data.append(name,sizeof(name));
            data.append(text,sizeof(text));
            return data;
        }

        void reserialize(const string &data)//引用（避免拷贝整份字节流）+ const（函数只读、允许 const 实参和临时对象、编译器兜底）
        {
            size_t offset=0;

            //data是对象不是指针，要转化成c风格字符串
            memcpy(&type,data.c_str()+offset,sizeof(type));
            offset+=sizeof(type);

            memcpy(&name,data.c_str()+offset,sizeof(name));
            offset+=sizeof(name);

            memcpy(&text,data.c_str()+offset,sizeof(text));

        }
    };
    struct Client
    {
        int Client_fd;
        struct sockaddr_in cin;
    };
private:
    int sfd;
    vector<Client> clients;
    mutex client_mutex;

    vector<thread> workers;
    queue<function<void()>> tasks;
    mutex task_mutex;
    condition_variable task_cv;
    bool stop;

    void startThreadPool(size_t numThreads);
    void addTask(function<void()> task);
    void errlog(const char* msg);
public:
    chatServer(const char* ip,int port,size_t threadPoolSize=4);
    ~chatServer();
    void run();
    void handleClient(int client_fd,struct sockaddr_in cin);
    void broadcast(const MSG &msg,int exclude_fd=-1);
};

//endif
#endif
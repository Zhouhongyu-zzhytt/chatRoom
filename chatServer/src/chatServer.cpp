#include "chatServer.h"
#include <myhead.h>

// 构造函数的定义：
// 初始化服务器，创建套接字，绑定地址信息结构体，启动监听，启动线程池
chatServer::chatServer(const char *ip, int port, size_t threadPoolSize) : stop(false)
{
    // 创建服务器套接字
    sfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sfd < 0)
    {
        perror("socker error");
        return;
    }

    // 端口号快速重用
    int reuse = 1;
    if (setsockopt(sfd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) == -1)
    {
        perror("setsockopt error");
        return;
    }

    // 地址信息结构体
    struct sockaddr_in sin;
    sin.sin_family = AF_INET;
    sin.sin_port = htons(port);
    sin.sin_addr.s_addr = inet_addr(ip);

    // bind绑定
    if (bind(sfd, (struct sockaddr *)&sin, sizeof(sin)) == -1)
    {
        perror("bind error");
        return;
    }

    // listen监听
    if (listen(sfd, 128) == -1)
    {
        perror("listen error");
        return;
    }

    startThreadPool(threadPoolSize);
}

// 析构函数的定义
chatServer::~chatServer()
{
    stop = true;
    task_cv.notify_all();
    for (auto &worker : workers)
    {
        worker.join();
    }

    close(sfd);
}

// 启动线程池函数的定义
void chatServer::startThreadPool(size_t numThreads)
{
    // 循环创建numthreads个线程
    for (int i = 0; i < numThreads; i++)
    {
        workers.emplace_back([this]
                             {
            while(1)
            {
                function<void()> task;
                {
                    unique_lock<mutex> lock(task_mutex);
                    task_cv.wait(lock,[this]
                    {
                        return stop || !tasks.empty();
                    });

                    if(stop&&tasks.empty())
                    {
                        return;
                    }
                    
                    task=move(tasks.front());
                    tasks.pop();
                }
                task();
            } });
    }
}

// 将任务添加到线程池函数的定义
void chatServer::addTask(function<void()> task)
{
    {
        unique_lock<mutex> lock(task_mutex);
        tasks.push(task);
    }

    task_cv.notify_one();
}

// 打印错误日志函数的定义
void chatServer::errlog(const char *msg)
{
    cerr << __FILE__ << " " << __func__ << " " << __LINE__ << endl;
    perror(msg);
}

// 启动服务器函数的定义
void chatServer::run()
{
    // 循环等待客户端连接请求
    while (1)
    {
        struct sockaddr_in cin;
        socklen_t socklen = sizeof(cin);

        // 阻塞接收客户端连接请求
        int client_fd = accept(sfd, (struct sockaddr *)&cin, &socklen);
        if (client_fd == -1)
        {
            errlog("accept error");
            continue;
        }

        addTask([this, client_fd, cin]
                { this->handleClient(client_fd, cin); });
    }
}

// 处理客户端消息函数的定义
void chatServer::handleClient(int client_fd, struct sockaddr_in cin)
{
    //定义一个消息类型的变量用于接收客户端信息msg buffer
    MSG msg;
    char buffer[sizeof(msg)];
    //循环读取客户端消息
    while(1)
    {
        //buffer读取
        int ret=recv(client_fd,buffer,sizeof(buffer),0);
        //当客户端下线或recv错误：将当前套接字从clients中移除，关闭套接字
        if(ret<=0)
        {
            auto it=clients.begin();
            while(it!=clients.end())
            {
                if(it->Client_fd==client_fd)
                {
                    clients.erase(it);
                    break;
                }
                ++it;
            }
            close(client_fd);
            break;
        }
        //msg反序列化
        msg.reserialize(string(buffer,ret));
        //对消息类型进行判断
        switch (ntohl(msg.type))
        {
        case LOGIN:
        {
            //互斥锁保护客户端容器
            unique_lock<mutex> lock(client_mutex);

            //获取新客户端信息
            Client new_client;
            new_client.Client_fd=client_fd;
            new_client.cin=cin;

            //将新客户端放入clients容器
            clients.push_back(new_client);

            //组装一条消息，广播给所有客户端
            
            sprintf(msg.text,"------%s 登陆成功--------",msg.name);
            broadcast(msg);

            break;
        }
        case CHAT:
        {
            //互斥锁保护客户端容器
            unique_lock<mutex> lock(client_mutex);
            //除了自己，将信息转发给其他所有人
            broadcast(msg,client_fd);

            break;
        }
        case QUIT:
        {
            // 表示客户端发的是退出消息
            // 定义一个互斥锁，保护客户端容器
            unique_lock<mutex> lock(client_mutex);
            auto it = clients.begin(); // 定义迭代器，遍历客户端容器
            while (it != clients.end())
            {
                if (it->Client_fd == client_fd)
                {
                    clients.erase(it); // 将当前用户清除容器
                    break;
                }
                ++it; // 继续遍历下一个
            }
            // 将消息广播给所有人
            sprintf(msg.text, "--------%s 退出聊天室--------", msg.name);
            broadcast(msg);
            close(client_fd); // 关闭当前客户端套接字
            break;
        }
        default:
            cout<<"消息类型有误"<<endl;
            break;
        }
        
    }
}

// 定义广播函数
void chatServer::broadcast(const MSG &msg, int exclude_fd)
{
    //将要广播的消息序列化操作
    string data=msg.serialize();
    //将字符串转发给所有客户端
    for(auto &client:clients)
    {
        if(client.Client_fd!=exclude_fd)
        {
            if(send(client.Client_fd,data.c_str(),data.size(),0)==-1)//bug
            {
                perror("send error");
                return;
            }
        }
    }
}
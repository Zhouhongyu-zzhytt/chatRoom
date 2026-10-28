# ChatRoom

一个基于 C++11 和 TCP Socket 的命令行聊天室示例，包含服务器端和客户端。服务器使用线程池处理客户端连接，并将聊天消息转发给其他在线客户端。

## 功能

- 客户端连接服务器时发送登录消息，服务器向聊天室广播登录提示。
- 客户端发送的聊天消息会广播给其他在线客户端。
- 在客户端输入 `quit` 可以退出聊天室，服务器会广播退出提示。
- 服务器使用线程池处理多个客户端连接。

## 项目结构

```text
chatRoom/
├── chatServer/
│   ├── header/chatServer.h
│   ├── src/main.cpp
│   ├── src/chatServer.cpp
│   └── CMakeLists.txt
└── chatClient/
    ├── header/chatClient.h
    ├── src/main.cpp
    ├── src/chatClient.cpp
    └── CMakeLists.txt
```

## 环境要求

- Linux 或其他提供 POSIX Socket API 的系统
- C++11 编译器（例如 GCC）
- CMake
- pthread
- 项目头文件 `myhead.h`

> 当前仓库源码包含 `#include <myhead.h>`，但仓库中没有提供该文件。请先补充此依赖，并确保其中包含代码所需的标准库和 POSIX 网络头文件，再进行编译。

## 编译

在项目根目录分别编译服务器和客户端：

```bash
cmake -S chatServer -B build/server
cmake --build build/server

cmake -S chatClient -B build/client
cmake --build build/client
```

构建完成后，可执行文件通常位于 `build/server/chatServer` 和 `build/client/chatClient`。

## 运行

先启动服务器：

```bash
./build/server/chatServer 0.0.0.0 8888
```

再在服务器所在机器或可访问该服务器的机器上启动客户端：

```bash
./build/client/chatClient 127.0.0.1 8888 Alice
```

将 `127.0.0.1` 替换为服务器 IP，将 `Alice` 替换为希望显示的昵称。多个客户端可使用相同的服务器地址和端口分别启动。输入普通文本发送聊天消息，输入 `quit` 退出。

## 消息格式

客户端与服务器交换定长二进制消息，包含消息类型、用户名和消息正文：

| 字段 | 大小 | 说明 |
| --- | ---: | --- |
| `type` | 4 字节 | 消息类型：`1` 登录、`2` 聊天、`3` 退出 |
| `name` | 20 字节 | 用户名 |
| `text` | 128 字节 | 消息内容 |

## 当前限制

- 这是用于学习 Socket 和多线程的示例项目，尚未包含身份验证、加密或持久化。
- 消息以固定大小的结构体序列化，客户端和服务器应运行在兼容的平台与字节序环境中。
- `myhead.h` 未随仓库提供；补齐该依赖是构建项目的前提。

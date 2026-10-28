#include<myhead.h>
#include"chatServer.h"

int main(int argc,const char* argv[])
{
    if(argc<3)
    {
        cout<<"请输入ip地址和端口号"<<endl;
        return -1;
    }

    chatServer server(argv[1],atoi(argv[2]));

    server.run();

    
    return 0;
}
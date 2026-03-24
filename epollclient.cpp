#include <iostream>
#include <string>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>

int main()
{
    // 1.创建套接字
    int cfd = socket(AF_INET, SOCK_STREAM, 0);
    if (cfd == -1)
    {
        perror("socket");
        return -1;
    }
    // 2.连接服务器
    struct sockaddr_in saddr;
    saddr.sin_family = AF_INET;
    saddr.sin_port = htons(9999);
    inet_pton(AF_INET, "127.0.0.1",  &saddr.sin_addr.s_addr);
    int ret = connect(cfd, (struct sockaddr*)&saddr, sizeof(saddr));
    if(ret == -1) {
        perror("connect");
        return -1;
    }  

    std::cout << "连接服务器成功！ 请输入内容（输入 'exit' 退出）：" << std::endl;

    //3.通信
    char buf[1024] = {0};
    while(1) {
        memset(buf, 0, sizeof(buf));
        std::string userInput;
        std::cout << "请输入内容：";
        std::getline(std::cin,userInput);

        if(userInput == "exit") {
            std::cout << "客户端请求退出..." << std::endl;
            break;
        }

        //发送数据
        send(cfd, userInput.c_str(), userInput.size(), 0);

        //接收数据
        int len = recv(cfd, buf, sizeof(buf)-1, 0);
        if(len == -1) {
            perror("recv");
            close(cfd);
            break;
        } else if(len == 0) {
            std::cout << "服务器断开连接..." << std::endl;
            close(cfd);
            break;
        }

        //打印数据
        buf[len] = '\0';
        std::cout << "服务器回复：" << buf << std::endl;
    }

    //4.关闭套接字
    close(cfd);
    return 0;

}
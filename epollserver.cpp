#include <iostream>
#include <string>
#include <cstring>
#include <mutex>
#include <thread>
#include <algorithm>
#include <sys/epoll.h>
#include <unistd.h>
#include <arpa/inet.h>

class TcpServer
{
public:
    TcpServer(unsigned short port) : m_lfd(-1), m_epfd(-1)
    {
        // 1.创建套接字
        m_lfd = socket(AF_INET, SOCK_STREAM, 0);
        if (m_lfd == -1)
        {
            perror("socket error");
            exit(-1);
        }

        // 设置端口复用
        int opt = 1;
        setsockopt(m_lfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

        // 初始化服务器 IP和地址
        struct sockaddr_in addr;
        addr.sin_family = AF_INET;
        addr.sin_port = htons(port);
        addr.sin_addr.s_addr = htonl(INADDR_ANY);

        // 2.绑定
        int ret = bind(m_lfd, (struct sockaddr *)&addr, sizeof(addr));
        if (ret == -1)
        {
            perror("bind error");
            exit(-1);
        }
        // 3.监听
        ret = listen(m_lfd, 128);
        if (ret == -1)
        {
            perror("listen error");
            exit(-1);
        }

        // 创建epoll实例
        m_epfd = epoll_create(1);
        if (m_epfd == -1)
        {
            perror("epoll_create error");
            exit(-1);
        }

        // 将监听套接字加入到epoll树上
        struct epoll_event ev;
        ev.data.fd = m_lfd;
        ev.events = EPOLLIN;
        if (epoll_ctl(m_epfd, EPOLL_CTL_ADD, m_lfd, &ev) == -1)
        {
            perror("epoll_ctl error");
            exit(-1);
        }

        std::cout << "服务器初始化成功，监听端口: " << port << std::endl;
    }
    // 析构函数，关闭套接字
    ~TcpServer()
    {
        if (m_lfd > 0)
            close(m_lfd);
        if (m_epfd > 0)
            close(m_epfd);
        std::cout << "服务器已关闭" << std::endl;
    }

    void run()
    {
        struct epoll_event evs[1024];
        int size = sizeof(evs) / sizeof(evs[0]);
        // 持续检测事件
        while (1)
        {
            // 阻塞等待，-1表示永久阻
            int num = epoll_wait(m_epfd, evs, size, -1); // 阻塞等待'
            if (num == -1)
            {
                perror("epoll_wait error");
                exit(1);
            }
            std::cout << "num = " << num << std::endl;
            for (int i = 0; i < num; ++i)
            {
                int fd = evs[i].data.fd;
                if (fd == m_lfd)
                { // 检测是否为监听套接字
                    acceptNewConnection();
                }
                else
                { // 否则为通信描述符，就两种
                    recvData(fd);
                }
            }
        }
    }

private:
    void acceptNewConnection()
    {

        int cfd = accept(m_lfd, NULL, NULL);
        if (cfd == -1)
        {
            perror("accept error");
            return;
        }
        // 将新连接加入到epoll实例
        struct epoll_event ev;
        ev.data.fd = cfd;
        ev.events = EPOLLIN;
        if (epoll_ctl(m_epfd, EPOLL_CTL_ADD, cfd, &ev) == -1) // cfd ev
        {
            perror("epoll_ctl error");
            return;
        }
        else
        {
            std::cout << "新客户端连接，cfd = " << cfd << std::endl;
        }
    }

    void recvData(int fd)
    {

        char buf[1024] = {0};
        int len = recv(fd, buf, sizeof(buf), 0);
        if (len == 0)
        {
            std::cout << "客户端关闭连接, fd = " << fd << std::endl;
            epoll_ctl(m_epfd, EPOLL_CTL_DEL, fd, NULL); // 从epoll树删除
            close(fd);                                  // 关闭文件描述符
        }
        else if (len == -1)
        {
            perror("recv error");
            // 如果是资源暂时不可用(EAGAIN)，不算严重错误，但在阻塞模式下通常不会发生
            // 其他错误建议关闭连接
            epoll_ctl(m_epfd, EPOLL_CTL_DEL, fd, NULL);
            close(fd);
        }

        std::cout << "Thread  " << std::this_thread::get_id() << "  Recv: " << buf << std::endl;
        for (int i = 0; i < len; i++)
        {
            buf[i] = toupper(buf[i]);
        }
        std::cout << "after buf = " << buf << std::endl;
        int ret = send(fd, buf, len, 0);
        if (ret == -1)
        {
            perror("send error");
        }
    }

private:
    int m_lfd;
    int m_epfd;
};

int main()
{
    TcpServer server(9999);
    server.run();
    return 0;
}

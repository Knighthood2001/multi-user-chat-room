#include <iostream>
#include <string>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>
#include <map>
#include <arpa/inet.h>  //inet_ntoa函数
#include <ctime>
//最大客户端连接数
const int MAX_COUT = 1024;

//保存客户端的信息
struct Client {
    int sockfd;
    std::string name;//名字
};


int main()
{
    //创建监听的socket
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("socket error");
        return -1;
    }
    //绑定本地的ip和端口
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(9999);

    int ret = bind(sockfd, (struct sockaddr*)&addr, sizeof(addr));
    if (sockfd < 0) {
        perror("bind error\n");
        return -1;
    }
    //监听客户端
    ret = listen(sockfd, 1024);
    if (ret < 0) {
        perror("listen error\n");
        return -1;
    }
    //创建epoll实例
    int epfd = epoll_create1(0);
    if (epfd < 0) {
        perror("epoll create error");
        return -1;
    }

    //将监听的socket加入epoll
    struct epoll_event ev;
    ev.events = EPOLLIN;
    ev.data.fd = sockfd;

    ret = epoll_ctl(epfd, EPOLL_CTL_ADD, sockfd, &ev);
    if (ret < 0) {
        perror("epoll_ctl error\n");
        return -1;
    }
    std::map<int, Client> clients;

    //开始循环监听
    while (true) {
        struct epoll_event events[MAX_COUT];
        int n = epoll_wait(epfd, events, MAX_COUT, -1); //阻塞着，直到有事件发生
        if (n < 0) {
            std::cout <<"epoll_ctl error\n"<< std::endl;
            break;
        }
        for (int i = 0;i < n; i++) { //遍历发生事件的文件描述符,根据不同的文件描述符进行相应的处理。
            int fd = events[i].data.fd;
            if (fd == sockfd) { //如果 fd 等于监听套接字 sockfd，表示有新的客户端连接请求。
                struct sockaddr_in client_addr;
                socklen_t client_addr_len = sizeof(client_addr);
                //accept 函数用于接受新的连接，返回一个新的套接字描述符 client_sockfd，用于与客户端进行通信。
                int client_sockfd = accept(sockfd, (struct sockaddr*)&client_addr, &client_addr_len);
                if (client_sockfd < 0) {
                    std::cout << "accept error\n" << std::endl;
                    continue;
                }
                //创建一个新的 epoll_event 结构体 ev_client，将其 events 字段设置为 EPOLLIN，表示监听该客户端套接字的读事件。
                //struct epoll_event ev_client;
                //ev_client.events = EPOLLIN;//检测客户端有没有消息过来
                //ev_client.data.fd = client_sockfd;
                ev.events = EPOLLIN;//检测客户端有没有消息过来
                ev.data.fd = client_sockfd;
                //使用 epoll_ctl 函数将该客户端套接字添加到 epoll 实例中进行监听。
                //ret = epoll_ctl(epfd, EPOLL_CTL_ADD, client_sockfd, &ev_client);
                ret = epoll_ctl(epfd, EPOLL_CTL_ADD, client_sockfd, &ev);
                if (ret < 0) {
                    std::cout << "epoll_ctl error\n" << std::endl;
                    break;
                }
                // 使用 inet_ntoa 转换 IP 地址
                std::cout << inet_ntoa(client_addr.sin_addr) << "正在连接..." << std::endl;

                //保存该客户端的信息
                Client client;
                client.sockfd = client_sockfd;
                client.name = "";
                clients[client_sockfd] = client;
            }
            else {//如果是客户端有消息了
                char buffer[1024];
                int n = read(fd, buffer, 1024);
                if (n < 0) {

                    break;
                }
                else if (n == 0) {
                    close(fd);
                    epoll_ctl(epfd, EPOLL_CTL_DEL, fd, 0);
                    clients.erase(fd);
                }
                else {
                    std::string msg(buffer, n);
                    //如果该客户端name为空，说明该消息是这个客户端的用户名
                    if (clients[fd].name == "") { 
                        clients[fd].name = msg;
                    }
                    else {//否则是聊天消息
                        std::string name = clients[fd].name;
                        std::time_t now = std::time(nullptr);
                        std::tm* local_time = std::localtime(&now);

                        char time_str[26];
                        std::strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", local_time);
                        std::string time_str_s(time_str);

                        std::string full_msg = "[" + time_str_s + "] [" + name + "]: " + msg;
            
                        //把消息发给其他所有客户端
                        for (auto& c : clients) {
                            if (c.first != fd) {
                                write(c.first, full_msg.c_str(), full_msg.size());

                            }
                        }

                    }
                }

            }
        }
    }
    close(epfd);
    close(sockfd);

    return 0;
}
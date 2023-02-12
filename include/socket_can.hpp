#pragma once
#include <net/if.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <linux/sockios.h>
#include <linux/can.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <signal.h>
#include <errno.h>
#include <ifaddrs.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include <string>
#include <iostream>
#include <chrono>
#include <thread>
#include <functional>
#include "string.h"

namespace chrono = std::chrono;

namespace can
{
    class socket_can
    {
    private:
        //定义一个socket can通信的地址结构体
        ifreq interface_request_{};
        //定义一个ifreq结构体，这个结构体用来配置和获取IP地址、掩码、MTU等接口信息的
        sockaddr_can address_{};
        //
        std::thread receiver_thread{};
    public:
        //socketCAN套接字
        int sock_fd_ = -1;
        //关闭接收线程标志位
        bool terminate_receiver_thread = false;
        //线程启动标志位
        bool receiver_thread_running = false;
        //收到消息时调用的函数
        std::function<void(const can_frame& frame)> reception_handler;
    public:
        socket_can() = default;
        ~socket_can();

        /**
         * @brief 初始化SocketCAN
         * 
         * @param interface CAN总线名称
         * @return true 成功初始化
         */
        bool open(const std::string& interface, std::function<void(const can_frame& frame)> handler);
        /**
         * @brief 关闭并解绑socket
         * 
         */
        void close();
        /**
         * @brief 确认是否开启socket
         * 
         */
        bool isOpen() const;
        /**
         * @brief 发送帧
         * 
         * @param frame can专用帧
         */
        void write(can_frame* frame) const;
        /**
         * @brief 创建线程，功能：等待socket活动
         * 
         */
        bool startReceiverThread();
    };







/*-----------------------以下为类函数实现-----------------------*/







    socket_can::~socket_can()
    {
        if (this->isOpen())
        {
            this->close();
        }
    }

    bool socket_can::open(const std::string& interface, std::function<void(const can_frame& frame)> handler)
    {
        reception_handler = std::move(handler);

        //创建socket关键字
        sock_fd_ = socket(PF_CAN, SOCK_RAW, CAN_RAW);
        if(sock_fd_ == -1)
        {
            std::cout<<"Error: Unable to create a CAN socket"<<std::endl;
            return false;
        }

        //指定设备
        char name[16] = {};
        strncpy(name, interface.c_str(), interface.size());
        strncpy(interface_request_.ifr_name, name, IFNAMSIZ);
        if(ioctl(sock_fd_, SIOCGIFINDEX, &interface_request_) == -1)
        {
            std::cout<<"Unable to select CAN interface "<<name<<": I/O control error"<<std::endl;
            //析构不可用的socket
            close();
            return false;
        }

        //将socket与can串口绑定
        address_.can_family = AF_CAN;
        address_.can_ifindex = interface_request_.ifr_ifindex;
        if (bind(sock_fd_, reinterpret_cast<struct sockaddr*>(&address_), sizeof(address_)) == -1)
        {
            std::cout<<"Failed to bind socket to "<<name<<" network interface"<<std::endl;
            //析构不可用的socket
            close();
            return false;
        }
        //为帧接收启动一个单独的事件驱动线程
        return startReceiverThread();
    }

    void socket_can::close()
    {
        //等待关闭任务线程
        terminate_receiver_thread = true;
        while (receiver_thread_running)
        {
            ;
        }
        

        if (!isOpen())
        {
            return;
        }
        //关闭socket
        ::close(sock_fd_);
        sock_fd_ = -1;
    }

    bool socket_can::isOpen() const
    {
        return (sock_fd_ != -1);
    }

    void socket_can::write(can_frame* frame) const
    {
        if (!isOpen())
        {
            std::cout<<"Unable to write: Socket "<<interface_request_.ifr_name<<" not open"<<std::endl;
            return;
        }
        
        if (::write(sock_fd_, frame, sizeof(can_frame)) == -1)
        {
            std::cout<<"Unable to write: The ";
            std::cout<<interface_request_.ifr_name;
            std::cout<<" tx buffer may be full"<<std::endl;
        }
    }

    bool socket_can::startReceiverThread()
    {
        terminate_receiver_thread = false;
        receiver_thread = std::thread(socketcan_receiver_thread, this);
        if (receiver_thread.joinable())
        {
            receiver_thread.detach();
            return true;
        }
        return false;
    }


    /**
     * @brief 监听socket，获取消息帧
     * 
     * @param argv 启动这个线程的socket_can指针
     */
    static void socketcan_receiver_thread(socket_can* argv)
    {
        /**
         * @brief 启动这个线程的socket_can指针
         * 
         */
        auto* sock = argv;
        /**
         * @brief 文件描述符集合
         * 
         */
        fd_set descriptors;
        /**
         * @brief 集合中标识符最高位
         * 
         */
        int maxfd = sock->sock_fd_;
        /**
         * @brief 设置超时时间
         * 
         */
        struct timeval timeout{};
        /**
         * @brief can帧，存储收到的帧
         * 
         */
        can_frame rx_frame{};
        /**
         * @brief 表示线程启动
         * 
         */
        sock->receiver_thread_running = true;

        while (!sock->terminate_receiver_thread)
        {
            //每次loop都要设置一次
            timeout.tv_sec = 1.;
            //清空标识符集合
            FD_ZERO(&descriptors);
            //添加socket标识符到集合中
            FD_SET(sock->sock_fd_, &descriptors);
            //开始监听，直到超时或收到消息
            if (select(maxfd+1, &descriptors, nullptr, nullptr, &timeout))
            {
                size_t len = read(sock->sock_fd_, &rx_frame, CAN_MTU);
                if (len < 0)
                {
                    continue;
                }
                if (sock->reception_handler)
                {
                    sock->reception_handler(rx_frame);
                }
            }
        }
        sock->receiver_thread_running = false;
        return;
    }

} // namespace can

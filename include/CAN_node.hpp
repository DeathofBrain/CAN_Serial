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
#include "string.h"

namespace can
{
    class CAN_node
    {
    private:
        ifreq interface_request_{};
        sockaddr_can address_{};
    
    public:
        int sock_fd_ = -1;

    public:
        CAN_node() = default;
        ~CAN_node();

        /**
         * @brief 初始化SocketCAN
         * 
         * @param interface CAN设备名称
         * @return true 成功初始化
         */
        bool open(const std::string& interface);
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
    };







/*-----------------------以下为类函数实现-----------------------*/







    CAN_node::~CAN_node()
    {
        if (this->isOpen())
        {
            this->close();
        }
    }

    bool CAN_node::open(const std::string& interface)
    {
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
        
        return true;
        //为帧接收启动一个单独的事件驱动线程
        //return startReceiverThread();
    }

    void CAN_node::close()
    {
        //预留：等待关闭任务线程
        //code

        if (!isOpen())
        {
            return;
        }
        //关闭socket
        ::close(sock_fd_);
        sock_fd_ = -1;
    }

    bool CAN_node::isOpen() const
    {
        return (sock_fd_ != -1);
    }

    void CAN_node::write(can_frame* frame) const
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


} // namespace can

#pragma once
#include "./socket_can.hpp"

#include <chrono>
#include <vector>
#include <mutex>
#include <thread>

typedef chrono::time_point<chrono::steady_clock> clock_stamp;

namespace can
{
    /**
     * @brief 含有时间戳的can帧
     * 
     */
    struct can_frame_stamp
    {
        can_frame frame;
        clock_stamp stamp;
    };
    

    class can_bus
    {
    public:
        can_bus() = delete;
        /**
         * @brief Construct a new can bus object
         * 
         * @param bus_name 设备名称
         */
        can_bus(const std::string& bus_name);

        ~can_bus() = default;
        /**
         * @brief 直接传输现有can帧
         * 
         * @param frame can帧
         */
        void write(can_frame* frame);
        /**
         * @brief 自动获取所需信息并发送can帧
         * 
         */
        void write();//TODO
        /**
         * @brief 从buffer中读取帧，并清空buffer
         * 
         * @param time chrono时间戳，采用steady_clock时钟
         */
        void read(clock_stamp time);
        /**
         * @brief 当socket接收can帧时，自动调用此函数并将帧传入buffer
         * 
         * @param frame socket获取的can帧
         */
        void frame_call_back(const can_frame& frame);
    private:
        /**
         * @brief can设备名称
         * 
         */
        const std::string bus_name;
        /**
         * @brief 串口socket
         * 
         */
        socket_can socket_can_;
        /**
         * @brief can帧缓冲区
         * 
         */
        std::vector<can_frame_stamp> read_buffer;

        mutable std::mutex _mutex;
    };
    







/*=======================================*/








    can_bus::can_bus(const std::string& bus_name)
        : bus_name(bus_name)
    {
        //初始化can串口
        while (!socket_can_.open(bus_name, std::bind(&can_bus::frame_call_back
                                                    ,this
                                                    ,std::placeholders::_1)))
        {
            std::this_thread::sleep_for(std::chrono::microseconds(500));
        }
        
        std::cout<<"Successfully connected to "<<bus_name.c_str()<<std::endl;
    }

    void can_bus::write(can_frame* frame)
    {
        socket_can_.write(frame);
    }

    void can_bus::write()
    {
        ;
    }

    void can_bus::read(clock_stamp stamp)
    {
        std::lock_guard<std::mutex> lg(_mutex);
        //读取can帧
        for (const auto& frame_stamp : read_buffer)
        {
            can_frame frame = frame_stamp.frame;
            std::cout<<frame.can_dlc<<frame.can_id;
        }
        read_buffer.clear();
    }

    void can_bus::frame_call_back(const can_frame& frame)
    {
        std::lock_guard<std::mutex> lg(_mutex);
        can_frame_stamp can_frame_stamp{.frame = frame, .stamp = std::chrono::steady_clock::now()};
        read_buffer.push_back(can_frame_stamp);
    }
} // namespace can

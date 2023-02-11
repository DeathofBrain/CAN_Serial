#pragma once
#include "./socket_can.hpp"

#include <chrono>
#include <mutex>
#include <thread>

namespace can
{

    class can_bus
    {
    private:
        /* data */
    public:
        can_bus() = delete;
        
        ~can_bus();
    };
    
    can_bus::can_bus(/* args */)
    {
    }
    
    can_bus::~can_bus()
    {
    }
    
} // namespace can

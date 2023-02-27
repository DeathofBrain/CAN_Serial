#include <iostream>
#include "../include/can_bus.hpp"

using namespace can;

int main(int argc, char const *argv[])
{
    can_bus cb("can0");
    int a;
    std::cin>>a;
    std::this_thread::sleep_for(std::chrono::seconds(3));
    cb.read();

}

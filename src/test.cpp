#include <iostream>
#include "../include/can_bus.hpp"

using namespace can;

int main(int argc, char const *argv[])
{
    can_bus cb("can0");
    std::cin.get();
    cb.read();
}

#include <ch/enet.hpp>

#include <enet/enet.h>
#include <stdexcept>

ch::enet::enet()
{
    if (enet_initialize() != 0)
    {
        throw std::runtime_error("Failed to initialize ENet");
    }
}

ch::enet::~enet()
{
    enet_deinitialize();
}

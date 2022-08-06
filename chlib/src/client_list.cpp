#include <ch/client_list.hpp>

ch::client_list::client_list()
{
    clients.fill(
        {.id = max_clients});
}

std::size_t ch::client_list::get_available_client()
{
    for (std::size_t i = 0; i < max_clients; i++)
    {
        if (clients.at(i).id == max_clients)
        {
            return i;
        }
    }

    return max_clients;
}

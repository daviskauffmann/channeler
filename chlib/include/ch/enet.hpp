#ifndef CH_ENET_HPP
#define CH_ENET_HPP

namespace ch
{
    class enet
    {
    public:
        enet();
        ~enet();
        enet(const enet &other) = delete;
        enet &operator=(const enet &other) = delete;
        enet(enet &&other) = delete;
        enet &operator=(enet &&other) = delete;
    };
}

#endif

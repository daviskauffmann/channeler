#ifndef CH_MIXER_HPP
#define CH_MIXER_HPP

namespace ch
{
    class mixer
    {
    public:
        mixer();
        mixer(mixer &&other) = delete;
        mixer(const mixer &other) = delete;
        mixer &operator=(mixer &&other) = delete;
        mixer &operator=(const mixer &other) = delete;
        ~mixer();
    };
}

#endif

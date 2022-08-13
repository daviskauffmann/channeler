#ifndef CH_AUDIO_HPP
#define CH_AUDIO_HPP

namespace ch
{
    class audio
    {
    public:
        audio();
        audio(audio &&other) = delete;
        audio(const audio &other) = delete;
        audio &operator=(audio &&other) = delete;
        audio &operator=(const audio &other) = delete;
        ~audio();
    };
}

#endif

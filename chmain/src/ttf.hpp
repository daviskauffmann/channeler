#ifndef CH_TTF_HPP
#define CH_TTF_HPP

namespace ch
{
    class ttf
    {
    public:
        ttf();
        ttf(ttf &&other) = delete;
        ttf(const ttf &other) = delete;
        ttf &operator=(ttf &&other) = delete;
        ttf &operator=(const ttf &other) = delete;
        ~ttf();
    };
}

#endif

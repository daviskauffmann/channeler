#ifndef CH_IMAGE_HPP
#define CH_IMAGE_HPP

namespace ch
{
    class image
    {
    public:
        image();
        image(image &&other) = delete;
        image(const image &other) = delete;
        image &operator=(image &&other) = delete;
        image &operator=(const image &other) = delete;
        ~image();
    };
}

#endif

#ifndef CH_VEC2_HPP
#define CH_VEC2_HPP

#include <cmath>

namespace ch
{
    struct vec2
    {
        float x = 0;
        float y = 0;

        vec2() {}
        vec2(float x, float y)
            : x(x), y(y) {}

        inline float length_sq() const
        {
            return std::powf(x, 2) + std::powf(y, 2);
        }

        inline float length() const
        {
            return std::sqrtf(length_sq());
        }

        inline float dot(const vec2 &v) const
        {
            return x * v.x + y * v.y;
        }

        inline vec2 operator-() const
        {
            return {-x, -y};
        }

        inline vec2 operator+(const vec2 &v) const
        {
            return {x + v.x, y + v.y};
        }

        inline vec2 operator-(const vec2 &v) const
        {
            return {x - v.x, y - v.y};
        }

        inline vec2 operator*(const float s) const
        {
            return {x * s, y * s};
        }

        inline vec2 operator/(const float s) const
        {
            return {x / s, y / s};
        }

        inline vec2 &operator+=(const vec2 &v)
        {
            x += v.x;
            y += v.y;

            return *this;
        }

        inline vec2 &operator-=(const vec2 &v)
        {
            x -= v.x;
            y -= v.y;

            return *this;
        }

        inline vec2 &operator*=(const float s)
        {
            x *= s;
            y *= s;

            return *this;
        }

        inline vec2 &operator/=(const float s)
        {
            x /= s;
            y /= s;

            return *this;
        }
    };

    inline vec2 operator*(const float s, const vec2 &v)
    {
        return v * s;
    }
}

#endif

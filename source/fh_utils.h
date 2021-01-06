#pragma once

#include "stdafx.h"

#define DEV

namespace ForHonor {

struct Color {
    union {
        COLORREF color;
        struct {
            BYTE b;
            BYTE g;
            BYTE r;
            BYTE a;
        };
    };

    Color(COLORREF color)
        : color(color)
    {}

    Color(BYTE r, BYTE g, BYTE b)
        : a(0xFF)
        , r(r)
        , g(g)
        , b(b)
    {}

    template<uint32_t limit = 360>
    uint32_t h()
    {
        return r * limit / 0xFF;
    }

    template<uint32_t limit = 100>
    uint32_t s()
    {
        return g * limit / 0xFF;
    }

    template<uint32_t limit = 100>
    uint32_t v()
    {
        return b * limit / 0xFF;
    }

    bool operator<=(Color o) const
    {
        return r <= o.r && g <= o.g && b <= o.b;
    }

    bool operator>=(Color o) const
    {
        return r >= o.r && g >= o.g && b >= o.b;
    }

    bool operator<(Color o) const
    {
        return r < o.r && g < o.g && b < o.b;
    }

    bool operator>(Color o) const
    {
        return r > o.r && g > o.g && b > o.b;
    }

    bool operator==(Color o) const
    {
        return color == o.color;
    }

    bool operator!=(Color o) const
    {
        return color != o.color;
    }

    Color toHsv()
    {
        Color hsv(0);

        BYTE rgbMin = r < g ? (r < b ? r : b) : (g < b ? g : b);
        BYTE rgbMax = r > g ? (r > b ? r : b) : (g > b ? g : b);

        hsv.b = rgbMax;
        if (hsv.b == 0) {
            hsv.r = 0;
            hsv.g = 0;
            return hsv;
        }

        hsv.g = 255 * long(rgbMax - rgbMin) / hsv.b;
        if (hsv.g == 0) {
            hsv.r = 0;
            return hsv;
        }

        if (rgbMax == r)
            hsv.r = 0 + 43 * (g - b) / (rgbMax - rgbMin);
        else if (rgbMax == g)
            hsv.r = 85 + 43 * (b - r) / (rgbMax - rgbMin);
        else
            hsv.r = 171 + 43 * (r - g) / (rgbMax - rgbMin);

        return hsv;
    }

    bool isDebug()
    {
#ifdef FH_DEV
        return color == 0xFF00FFFF || color == 0xFF0000FF;
#else
        return false;
#endif
    }
};

struct ColorRange {
    Color min;
    Color max;

    ColorRange(Color color, BYTE varR, BYTE varG, BYTE varB)
        : min { minOf(color.r, varR), minOf(color.g, varG), minOf(color.b, varB) }
        , max { maxOf(color.r, varR), maxOf(color.g, varG), maxOf(color.b, varB) }
    {}

    ColorRange(Color color, BYTE var = 0)
        : ColorRange { color, var, var, var }
    {}

    bool operator==(Color o) const
    {
        return o >= min && o <= max;
    }

    bool operator!=(Color o) const
    {
        return !operator==(o);
    }

private:
    BYTE minOf(BYTE c, BYTE var)
    {
        return (var > c) ? 0 : c - var;
    }

    BYTE maxOf(BYTE c, BYTE var)
    {
        return (var > 0xFF - c) ? 0xFF : c + var;
    }
};

template <class T>
struct vec2 {
    T x;
    T y;

    vec2(T x, T y) : x(x) , y(y)
    {}

    vec2(T v) : vec2(v, v)
    {}

    vec2() : vec2(0)
    {}

    template <class K>
    vec2(const vec2<K>& v) : vec2(static_cast<T>(v.x), static_cast<T>(v.y))
    {}

    operator bool() const
    {
        return x || y;
    }

    vec2 operator+(const vec2& o) const
    {
        return vec2(x + o.x, y + o.y);
    }

    vec2 operator-(const vec2& o) const
    {
        return vec2(x - o.x, y - o.y);
    }

    bool operator==(const vec2& o) const
    {
        return x == o.x && y == o.y;
    }

    T len2()
    {
        return x * x + y * y;
    }

    double len()
    {
        return std::sqrt(len2());
    }

    T distance2(const vec2& o)
    {
        return operator-(o).len2();
    }
};

using Pos = vec2<uint32_t>;

#define RTD(n) (n * 180 / 3.141592653589793238)

template <class T>
static T clamp(T v, T min, T max)
{
    return v < min ? min : v > max ? max : v;
}

}

#pragma once

#include "stdafx.h"
#include "fh_utils.h"

namespace ForHonor {

class Bitmap {
public:
    struct SearchResult {
        Bitmap* bmap;
        uint32_t index;

        SearchResult() : SearchResult(nullptr, 0)
        {}

        SearchResult(Bitmap* bmap, uint32_t index) : bmap(bmap), index(index)
        {}

        operator bool() const { return bmap != nullptr; }
        operator int() const { return static_cast<int>(index); }
        operator uint32_t() const { return index; }

        Bitmap& operator->() { return *bmap; };

        Pos asPos() const { return bmap ? bmap->posOf(index) : Pos{}; }
    };

    uint32_t posX1;
    uint32_t posX2;
    uint32_t posY1;
    uint32_t posY2;
    uint32_t width;
    uint32_t height;
    uint32_t size;

    LPCOLORREF buf = NULL;

    Bitmap(uint32_t posX1, uint32_t posY1, uint32_t posX2, uint32_t posY2);

    Bitmap();

    ~Bitmap();

    bool blit();

    void dump(size_t index, COLORREF marker);

    Pos posOf(uint32_t index) const
    { return Pos(index % width, index / width); }

    uint32_t indexOf(uint32_t x, uint32_t y) const
    { return x + y * width; }

    uint32_t indexOf(Pos pos) const
    { return pos.x + pos.y * width; }

    uint32_t indexOf(uint32_t index) const
    { return index; }

    COLORREF& at(uint32_t x, uint32_t y)
    { return buf[indexOf(x, y)]; }

    void drawQuad(int x1, int y1, int x2, int y2, COLORREF color);

    void drawQuadAround(int x, int y, int r, COLORREF color);

    template <class Index>
    Color get(Index index) const
    {
        return buf[indexOf(index)];
    }

    int stepOf(int vecX, int vecY)
    {
        int off = vecY == 0 ? vecX : vecX / vecY;
        return (width + off) * vecY;
    }

    template <int stepX, int stepY, class A>
    SearchResult search(int x1, int y1, int x2, int y2, const A& action)
    {
        clampRect(x1, y1, x2, y2);

        int w = x2 - x1, h = y2 - y1;

        int j1 = stepY < 0 ? y1 + h - 1 : y1;
        int j2 = stepY < 0 ? y1 - 1 : y1 + h;
        while (stepY < 0 ? (j1 > j2) : (j1 < j2)) {
            int i1 = (stepX < 0 ? x1 + w - 1 : x1) + j1 * width;
            int i2 = (stepX < 0 ? x1 - 1 : x1 + w) + j1 * width;
            while (stepX < 0 ? (i1 > i2) : (i1 < i2)) {
                if (action(i1))
                    return { this, static_cast<uint32_t>(i1) };
                i1 += stepX;
            }
            j1 += stepY;
        }
        return {};
    }

    template <class A>
    SearchResult searchLine(int x1, int y1, int x2, int y2, const A& action)
    {
        clampRect(x1, y1, x2, y2);

        int dx = abs(x2 - x1), sx = x1 < x2 ? 1 : -1;
        int dy = -abs(y2 - y1), sy = y1 < y2 ? 1 : -1;
        int err = dx + dy, e2; /* error value e_xy */

        for (;;) {  /* loop */
            uint32_t i = indexOf(x1, y1);
            if (action(i))
                return { this, i };
            if (x1 == x2 && y1 == y2)
                break;
            e2 = 2 * err;
            if (e2 >= dy) { 
                err += dy; x1 += sx;
            }
            if (e2 <= dx) { 
                err += dx; y1 += sy;
            }
        }

        return {};
    }

    template <class Index, class A>
    SearchResult searchVector(Index index, uint32_t len, int vecX, int vecY, const A& action)
    {
        uint32_t step = stepOf(vecX, vecY);
        for (uint32_t i = indexOf(index), s = i + len * step; i != s; i += step) {
            if (i < size && action(i))
                return { this, i };
        }
        return {};
    }

    template <class Index, class A>
    bool test(Index index, const A& action)
    {
        uint32_t i = indexOf(index);
        return (i < size) ? action(i) : false;
    }

    template <class Index, class A>
    bool testLine(Index index, uint32_t len, int vecX, int vecY, const A& action)
    {
        uint32_t step = stepOf(vecX, vecY);
        uint32_t start = indexOf(index);
        for (uint32_t i = start + step * len; i != start; i -= step) {
            if (i >= size || !action(i))
                return false;
        }
        return true;
    }

    template <BYTE Mul, class Index, class A>
    bool testLineMul(Index index, uint32_t len, int vecX, int vecY, const A& action)
    {
        return testLine(index, len / Mul, vecX * Mul, vecY * Mul, action);
    }

private:
    HDC hdc = NULL;
    HDC sdc = NULL;
    HBITMAP hbitmap_screen = NULL;
    HGDIOBJ sdc_orig_select = NULL;


    template <class T>
    void clampPos(T& x, T& y)
    {
        x = clamp(x, 0, static_cast<T>(width - 1));
        y = clamp(y, 0, static_cast<T>(height - 1));
    }

    template <class T>
    void clampRect(T& x1, T& y1, T& x2, T& y2)
    {
        clampPos(x1, y1);
        clampPos(x2, y2);
    }
};

} // namespace ForHonor

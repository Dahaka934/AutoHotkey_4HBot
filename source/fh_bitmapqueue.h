#pragma once

#include "stdafx.h"
#include "fh_bitmap.h"

namespace ForHonor {

template <size_t Size>
class BitmapQueue {
public:
    void init(uint32_t posX1, uint32_t posY1, uint32_t posX2, uint32_t posY2)
    {
        currIndex = 0;
        for (size_t i = 0; i < bmaps.size(); ++i) {
            bmaps[i] = std::make_unique<Bitmap>(posX1, posY1, posX2, posY2);
        }
    }

    void move()
    {
        currIndex = wrapIndex(currIndex + 1);
    }

    Bitmap& curr()
    {
        return at(currIndex);
    }

    Bitmap& get(size_t index)
    {
        return at(currIndex + index);
    }

    Bitmap& at(size_t index)
    {
        return *bmaps[wrapIndex(index)];
    }

    size_t index()
    {
        return currIndex;
    }

private:
    std::array<std::unique_ptr<Bitmap>, Size> bmaps;
    size_t currIndex = 0;

    size_t wrapIndex(size_t index)
    {
        return (index >= Size) ? 0 : index;
    }
};

} // namespace ForHonor

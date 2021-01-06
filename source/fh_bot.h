#pragma once

#include "stdafx.h"

namespace ForHonor {

struct Bot {
    using PropInt = int;
    using PropFlt = double;
    using PropStr = LPTSTR;

    PropInt* propInt(LPTSTR key);
    PropFlt* propFlt(LPTSTR key);
    PropStr* propStr(LPTSTR key);

    void attackSearchRect(int left, int top, int right, int bottom);

    int attackSearch(int deflect);

    static Bot& instance()
    {
        static Bot obj;
        return obj;
    }

private:
    Bot();
    ~Bot();

    Bot(const Bot&) = delete;
    Bot(Bot&&) = delete;
    
    Bot operator=(const Bot&) = delete;
    Bot operator=(Bot&&) = delete;

    struct Private;
    std::unique_ptr<Private> impl;
};

}

#include "stdafx.h"
#include "fh_bot.h"

#define FH_DEV1
#define FH_ASYNC

#include "fh_bitmap.h"
#include "fh_bitmapqueue.h"
#include "fh_utils.h"

using namespace ForHonor;

using std_clock = std::chrono::high_resolution_clock;

enum class Result : uint32_t {
    NONE = 0,
    ATK_U = 1,
    ATK_L = 2,
    ATK_R = 3,
    PARRY = 4,
    ATK_U_U = 5,
    ATK_L_U = 6,
    ATK_R_U = 7,
    PARRY_U = 8,
    GB = 9,
    BASH = 10,
    ATK_OTHER = 11,
};

#define ResultColor(color) std::pair<Bot::PropStr, COLORREF>(L#color, color | 0xff000000)

static std::array<std::pair<Bot::PropStr, COLORREF>, 12> resultColors {
    ResultColor(0x000000), // NONE
    ResultColor(0xFF0000), // ATK_U
    ResultColor(0xFFFF00), // ATK_L
    ResultColor(0x0000FF), // ATK_R
    ResultColor(0x00FF00), // PARRY
    ResultColor(0xFF0000), // ATK_U_U
    ResultColor(0xFFFF00), // ATK_L_U
    ResultColor(0x0000FF), // ATK_R_U
    ResultColor(0x00FF00), // PARRY_U
    ResultColor(0x00FFFF), // GB
    ResultColor(0xFF8F00), // BASH
    ResultColor(0x000000), // ATK_OTHER
};

static uint32_t resultInt(Result result)
{
    return static_cast<uint32_t>(result);
}

static Result resultWrap(Result result, bool unblock)
{
    return unblock ? static_cast<Result>(resultInt(result) + 4) : result;
}

bool operator!(Result e)
{
    return e == Result::NONE;
}

template <class T = std::chrono::milliseconds>
auto timediff(std_clock::time_point t1, std_clock::time_point t2 = std_clock::now())
{
    return std::chrono::duration_cast<T>(t2 - t1).count();
}

struct Bot::Private {
    union PropUnion {
        PropInt i;
        PropFlt f;
        PropStr s;
    };

    std::unordered_map<std::wstring, PropInt*> propsInt;
    std::unordered_map<std::wstring, PropFlt*> propsFlt;
    std::unordered_map<std::wstring, PropStr*> propsStr;

    #define REG_PROP(holder, prop) holder[L#prop] = &prop

    ColorRange colorBlockDark1 = { 0x989888, 0x10 };
    ColorRange colorBlockDark2 = { 0x505250, 0x10 };
    ColorRange colorBlockLight1 = { 0xF6F6F3, 0x04, 0x04, 0x10 };
    ColorRange colorBlockLight2 = { 0x090A08, 0x04, 0x04, 0x10 };
    ColorRange colorUnblockable = { 0xF05C03 };
    ColorRange colorGB = { 0xFF1914 };

    PropInt optBlock = 0;
    PropInt optGuardBrake = 0;
    PropInt optParry = 0;
    PropInt optEvade = 0;
    PropInt optBlockOther = 0;
    PropInt optParryOther = 0;
    PropInt optAttackUnblock = 0;
    PropInt optScreenshots = 0;

    PropInt cooldownGB = 0;
    PropInt outAtkPosX1 = 0;
    PropInt outAtkPosY1 = 0;
    PropInt outAtkPosX2 = 0;
    PropInt outAtkPosY2 = 0;
    PropInt outResult = 0;
    PropStr outColor = L"0x000000";
    PropInt outTmp = 0;

    Pos posAtk1 = 0;
    Pos posAtk2 = 0;
    Pos posBash = 0;
    Pos posAtkLast = 0;
    Pos posAtk3Last = 0;

    Result result = Result::NONE;

    Result resAtkLast = Result::NONE;
    Result resAtk3Last = Result::NONE;
    std_clock::time_point timeAtk = std_clock::now();
    std_clock::time_point timeAtk3 = std_clock::now();
    std_clock::time_point timeParry = std_clock::now();
    std_clock::time_point timeGB = std_clock::now();
    std_clock::time_point timeIter = std_clock::now();
    int unblockable = 0;
    int cooldownScreen = 0;
    int iterIndex = 0;
    int sizeAtk3 = 0;
    bool isParry3Now = false;

#ifdef FH_ASYNC
    BitmapQueue<3> bmaps;
#else
    BitmapQueue<1> bmaps;
#endif

#ifdef FH_DEV
    std::fstream log = std::fstream("ForHonorBot.txt", std::fstream::out);
#endif

    Bitmap* bmap = NULL;

    Private()
    {
        REG_PROP(propsInt, optBlock);
        REG_PROP(propsInt, optGuardBrake);
        REG_PROP(propsInt, optParry);
        REG_PROP(propsInt, optEvade);
        REG_PROP(propsInt, optBlockOther);
        REG_PROP(propsInt, optParryOther);
        REG_PROP(propsInt, optAttackUnblock);
        REG_PROP(propsInt, optScreenshots);
        REG_PROP(propsInt, cooldownGB);
        REG_PROP(propsInt, outAtkPosX1);
        REG_PROP(propsInt, outAtkPosY1);
        REG_PROP(propsInt, outAtkPosX2);
        REG_PROP(propsInt, outAtkPosY2);
        REG_PROP(propsInt, outResult);
        REG_PROP(propsStr, outColor);
        REG_PROP(propsInt, outTmp);
    }

    void initBitmap(uint32_t posX1, uint32_t posY1, uint32_t posX2, uint32_t posY2)
    {
        CreateDirectory(L"4H Screenshots", NULL);

        if (posX1 > posX2)
            std::swap(posX1, posX2);
        if (posY1 > posY2)
            std::swap(posY1, posY2);

        bmaps.init(posX1, posY1, posX2, posY2);
    }

    Result attackSearch(int checkBlock)
    {
        result = Result::NONE;
        iterIndex++;

        if (cooldownGB) {
            timeGB = std_clock::now();
            cooldownGB = 0;
        }

        bmap = &bmaps.curr();
#ifdef FH_DEV
        auto time1 = std_clock::now();
        makeScreenBlit();
        auto time2 = std_clock::now();
        result = makeScreenAnalize(checkBlock);
        auto time3 = std_clock::now();
        makeScreenDump();
        auto time4 = std_clock::now();
        log << iterIndex << ":" << resultInt(result)
            << " action: " << timediff(time2, time3) / 1000.0
            << " blit: " << timediff(time1, time2) / 1000.0
            << " screen: " << timediff(time3, time4) / 1000.0
            << " pass: " << timediff(timeIter, time1) / 1000.0
            << " all: " << timediff(timeIter, time4) / 1000.0
            << std::endl;

        log.flush();
        timeIter = std_clock::now();
#else
        makeScreenBlit();
        result = makeScreenAnalize(checkBlock);
        makeScreenDump();
#endif
        bmaps.move();

        outResult = resultInt(result);
        outColor = resultColors[outResult].first;
        outAtkPosX1 = posAtk1.x + bmap->posX1;
        outAtkPosY1 = posAtk1.y + bmap->posY1;
        outAtkPosX2 = posAtk2.x + bmap->posX1;
        outAtkPosY2 = posAtk2.y + bmap->posY1;

        return result;
    }

#ifdef FH_ASYNC
    std::future<void> futureScreenshot;
    std::future<void> futureBlit;

    void makeScreenBlit()
    {
        if (futureBlit.valid())
            futureBlit.get();
        futureBlit = std::async(std::launch::async, [&bmap = bmaps.get(1)]() {
            bmap.blit();
        });
    }
#else
    void makeScreenBlit()
    {
        bmaps.curr().blit();
    }
#endif

    void makeScreenDump()
    {
        if (!optScreenshots)
            return;

        if (resultInt(result))
            cooldownScreenReset();

        if (cooldownScreen) {
            cooldownScreen--;

            auto dump = [&bmap = bmaps.curr(), index = iterIndex, result = result]() {
                bmap.dump(index, resultColors[resultInt(result)].second);
            };

            bmaps.curr().drawQuadAround(posAtk1.x, posAtk1.y, 3, 0xFF00FFFF);
            bmaps.curr().drawQuadAround(posAtk2.x, posAtk2.y, 3, 0xFF0000FF);

#ifdef FH_ASYNC
            if (futureScreenshot.valid())
                futureScreenshot.get();
            futureScreenshot = std::async(std::launch::async, dump);
#else
            dump();
#endif
        }
    }

    template <class A>
    auto predInvert(const A& action)
    {
        return [&](uint32_t index) {
            return !action(index);
        };
    }

    auto predColor(const ColorRange& color)
    {
        return [&](uint32_t index) {
            Color rgb = bmap->get(index);
            return color == rgb || rgb.isDebug();
        };
    }

    template <BYTE minV, BYTE minS, bool debug = true>
    auto predAtk()
    {
        return [&](uint32_t index) {
            Color rgb = bmap->get(index);
            if (debug && rgb.isDebug())
                return true;

            Color hsv = rgb.toHsv();
            return hsv.r <= 2 && hsv.s() >= minS && hsv.s() <= 94 && hsv.v() >= minV;
        };
    }

    template <BYTE minV1, BYTE minV2, BYTE minS, bool bot>
    auto predAtkArrow()
    {
        return [&](uint32_t index) {
            if (!predAtk<minV1, minS, 0>()(index))
                return false;

            if (bmap->test(index + bmap->indexOf(+0, bot ? +3 : -3), predAtk<minV2, minS>()))
                return false;

            if (bmap->test(index + bmap->indexOf(+0, bot ? +7 : -7), predAtk<minV2, minS>()))
                return false;

            if (bmap->test(index + bmap->indexOf(+0, bot ? -3 : +3), predAtk<minV1, minS>()) == false)
                return false;

            return true;
        };
    }

    auto predBlockInternal(ColorRange& color1, ColorRange& color2)
    {
        return [&](uint32_t index) {
            if (!bmap->test(index, predColor(color1)))
                return false;

            if (bmap->test(index + bmap->indexOf(-2, +0), predColor(color1)) && bmap->test(index + bmap->indexOf(+2, +0), predColor(color2))) {
                // result = Result::ATK_U;
                return true;
            }

            if (bmap->test(index + bmap->indexOf(+1, -1), predColor(color1)) && bmap->test(index + bmap->indexOf(-1, +1), predColor(color2))) {
                // result = Result::ATK_R;
                return true;
            }

            if (bmap->test(index + bmap->indexOf(+2, +0), predColor(color1)) && bmap->test(index + bmap->indexOf(-1, -1), predColor(color2))) {
                // result = Result::ATK_L;
                return true;
            }

            return false;
        };
    }

    auto predBlock()
    {
        return predBlockInternal(colorBlockDark1, colorBlockLight1);
    }

    template <BYTE minV1, BYTE minV2, BYTE minV3, BYTE minS, BYTE pass>
    auto predAtkOrGB()
    {
        return [&](uint32_t index) {
            if (!predAtkArrow<minV1, minV2, minS, 1>()(index))
                return false;

            posAtk1 = bmap->posOf(index);
            cooldownScreenReset();

            uint32_t w = 110, h = 180;
            uint32_t x = posAtk1.x - 10 - w, y = posAtk1.y - h;
            posAtk2 = bmap->search<2, 2>(x, y, x + w, y + h, predAtkArrow<minV1, minV2, minS, 0>()).asPos();
            if (!posAtk2) {
                x = posAtk1.x + 10, y = posAtk1.y - h;
                posAtk2 = bmap->search<2, 2>(x, y, x + w, y + h, predAtkArrow<minV1, minV2, minS, 0>()).asPos();
                if (!posAtk2)
                    return false;
            }

            if (pass == 0 && optGuardBrake && timediff(timeGB) >= 1250 && !unblockable) {
                result = checkGuardBrake(posAtk2.x, posAtk2.y, posAtk1.x, posAtk1.y);
                if (!!result)
                    return true;
            }

            if (optBlock) {
                result = checkAttack<minV3, minS>(posAtk2.x, posAtk2.y, posAtk1.x, posAtk1.y);
                if (!!result) {
                    resAtkLast = result;
                    posAtkLast = { (posAtk1.x + posAtk2.x) / 2, (posAtk1.y + posAtk2.y) / 2 };
                    timeAtk = std_clock::now();
                    if (pass == 0) {
                        result = resultWrap(result, unblockable);
                        unblockable = !unblockable ? unblockable : -1000000;
                    }
                    return true;
                }
            }

            return false;
        };
    }

    Result makeScreenAnalize(int dontCheckBlock)
    {
        posAtk1 = posAtk2 = 0;

        if (timediff(timeParry) < 150) {
            return result;
        }

        // search bash or unblockable
        if (optEvade || optParry) {
            checkUnblockable();
        }

        if (optAttackUnblock && unblockable) {
            unblockable = -1000000;
            return Result::BASH;
        }

        // search attack or guard brake
        if (bmap->search<-1, -2>(0, 0, bmap->width, bmap->height, predAtkOrGB<88, 40, 55, 86, 0>())) {
            return result;
        }

        if (optEvade && unblockable > 1) {
            unblockable = -1000000;
            return Result::BASH;
        }

        // check parry
        if (optParry && posAtkLast && timediff(timeAtk) < 1000) {
            if (!!(result = checkParry(dontCheckBlock)))
                return result;
        }

        if (timediff(timeParry) > 500 &&
            bmap->search<-1, -2>(0, 0, bmap->width, bmap->height, predAtkOrGB<40, 40, 40, 90, 1>())) {
            return result;
        }

        // check third attacks
        if (optBlockOther) {
            if (!!(result = checkThirdAttack()))
                return result;
        }

        if (optBlockOther && optParryOther && posAtk3Last && timediff(timeAtk3) < 1500) {
            if (!!(result = checkThirdParry()))
                return result;
        }

        return result;
    }

    void checkUnblockable()
    {
        uint32_t w = bmap->width * 2 / 3, h = bmap->height * 2 / 3;
        uint32_t x = bmap->width - w, y = 0;
        posBash = bmap->search<+3, +3>(x, y, x + w, y + w, predColor(colorUnblockable)).asPos();
        if (posBash) {
            unblockable++;
            cooldownScreenReset();
        }
        else {
            unblockable = 0;
        }
    }

    Result checkGuardBrake(uint32_t x1, uint32_t y1, uint32_t x2, uint32_t y2)
    {
        int dx = int(x2) - int(x1);
        int dy = int(y2) - int(y1);
        double coef = double(abs(dy)) / double(abs(dx));

        if (coef < 3 || coef > 6)
            return Result::NONE;

        if (x1 > x2)
            return Result::NONE;

        if (x1 > (bmap->width * 2 / 3))
            return Result::NONE;

        if (y2 < (bmap->height * 1 / 3))
            return Result::NONE;

        auto pred = predColor(colorGB);

        int index1 = bmap->search<+2, +2>(x1, y1, x1 + 20, y1 + 20, pred);
        if (index1 < 0)
            return Result::NONE;

        if (!bmap->testLine(index1, 5, +2, +1, pred))
            return Result::NONE;

        int index2 = bmap->search<-2, -2>(x2 - 25, y2 - 25, x2, y2, pred);
        if (index2 < 0)
            return Result::NONE;

        if (!bmap->testLine(index2, 5, +0, -2, pred))
            return Result::NONE;

        posAtk1 = bmap->posOf(index2);
        posAtk2 = bmap->posOf(index1);

        // if (!bmap->searchLine(posAtk1.x, posAtk1.y, posAtk2.x, posAtk2.y, predInvert(pred)))
        //     return Result::NONE;

        return Result::GB;
    }

    template <BYTE minV, BYTE minS>
    Result checkAttack(uint32_t x1, uint32_t y1, uint32_t x2, uint32_t y2)
    {
        uint32_t distance = (Pos(x1, y1) - Pos(x2, y2)).len2();
        int dx = int(x2) - int(x1);
        int dy = int(y2) - int(y1);

        double coef = double(abs(dy)) / double(abs(dx));
        if (coef >= 4)
            return Result::NONE;

        uint32_t x3 = x1 + dx / 2;
        uint32_t y3 = y1 + dy / 2;
        int off = x1 < x2 ? 2 : -2;
        auto pred = predAtk<minV, minS>();
        auto predSideCommon = [&] {
            if (distance < 1500)
                return false;

            if (bmap->test(Pos(x2, y1), pred) || bmap->test(Pos(x3, y1), pred) || bmap->test(Pos(x2, y3), pred) || bmap->test(Pos(x1, y2), pred) || bmap->test(Pos(x1 - dx, y3), pred))
                return false;

            if (bmap->test(Pos(x3, y3), pred) || bmap->test(Pos(x3, y3 - dy / 4), pred))
                return false;

            if (!bmap->testLineMul<2>(Pos(x1, y1), dy / 2, 0, 1, pred))
                return false;

            return true;
        };

        if (coef <= 1.2) {
            if (distance < 1000 || coef <= 0.7)
                return Result::NONE;

            //if (bmap->test(Pos(x1, y2), pred))
            //    return Result::NONE;

            if (!(bmap->testLineMul<2>(Pos(x1, y1 + 1), dy * 3 / 4, +1, +1, pred) && bmap->testLineMul<2>(Pos(x1, y1 + 1), dy * 3 / 4, -1, +1, pred)) &&
                !(bmap->testLineMul<2>(Pos(x1, y1 + 3), dy * 3 / 4, +1, +1, pred) && bmap->testLineMul<2>(Pos(x1, y1 + 3), dy * 3 / 4, -1, +1, pred)))
                return Result::NONE;

            return Result::ATK_U;
        } else if (x1 < x2) {
            if (!(predSideCommon()))
                return Result::NONE;

            if (!bmap->testLineMul<2>(Pos(x2 - 0, y2), dx / 2, -1, -1, pred) && !bmap->testLineMul<2>(Pos(x2 - 2, y2), dx / 2, -1, -1, pred))
                return Result::NONE;

            return Result::ATK_L;
        } else {
            if (!(predSideCommon()))
                return Result::NONE;

            if (!bmap->testLineMul<2>(Pos(x2 - 0, y2), -dx / 2, +1, -1, pred) && !bmap->testLineMul<2>(Pos(x2 - 2, y2), -dx / 2, +1, -1, pred))
                return Result::NONE;

            return Result::ATK_R;
        }
    }

    Result checkParry(bool dontCheckBlock)
    {
        auto predParry = [&](uint32_t index) {
            Color rgb = bmap->get(index);
            if (rgb.isDebug())
                return true;

            if (rgb.r != 0xff || rgb.g >= 0xf0 || rgb.b >= 0xf0)
                return false;

            BYTE h = rgb.toHsv().r - 21;
            return h >= 0xFF - 21 - 21;
        };

        auto predParryArea = [&](uint32_t index) {
            if (bmap->testLine(index, 8, +3, +3, predParry)) {
                posAtk1 = bmap->posOf(index), posAtk2 = 0;
                return true;
            }

            if (bmap->testLine(index, 8, -3, +3, predParry)) {
                posAtk1 = bmap->posOf(index), posAtk2 = 0;
                return true;
            }

            return false;
        };

        auto checkBlockReady = [&] {
            uint32_t w = bmap->width * 3 / 4, h = bmap->height * 3 / 2;
            uint32_t x = 0, y = bmap->height - h;
            if (auto p = bmap->search<1, -1>(x, y, x + w, y + h, predBlock())) {
                return p.asPos();
                //return ret == resAtkLast ? p.asPos() : Pos(0);
            }
            return Pos(0);
        };

        auto parryPoint = [&] {
            uint32_t w = 200, h = 200;
            uint32_t x = posAtkLast.x - w / 2, y = posAtkLast.y - h / 2;
            return bmap->search<3, 3>(x, y, x + w, y + h, predParryArea).asPos();
        };

        cooldownScreenReset();
        if (dontCheckBlock || (posAtk1 = checkBlockReady())) {
            if ((posAtk2 = parryPoint())) {
                posAtkLast = 0;
                timeParry = std_clock::now();
                return resultWrap(Result::PARRY, unblockable);
            }
        }

        return Result::NONE;
    }

    Result checkThirdAttack()
    {
        auto predColor = [&](uint32_t index) {
            Color rgb = bmap->get(index);
            if (rgb.isDebug())
                return true;

            Color hsv = rgb.toHsv();
            return hsv.h() <= 15 && hsv.s() >= 65 && hsv.v() >= 95;
        };

        Pos maxPos;
        int maxSize = 0;
        Result result = Result::NONE;

        auto predFactory = [&](int stepX, int stepY, int len, Result res) {
            auto step = bmap->stepOf(stepX, stepY);
            return [&, stepX, stepY, len, res, step](uint32_t index) -> bool {
                if (auto sr = bmap->searchVector(index, len, stepX, stepY, predColor)) {
                    if (auto srLast = bmap->searchVector(sr.index, len, stepX, stepY, predInvert(predColor))) {
                        int size = (srLast.index - sr.index) / step;
                        if (size > maxSize) {
                            maxSize = size;
                            maxPos = sr.asPos();
                            result = res;
                        }
                    }
                }

                return true;
            };
        };

        uint32_t w, h, x, y;
        w = bmap->width / 2, h = bmap->height * 3 / 4;
        x = bmap->width - w, y = bmap->height - h;
        bmap->testLine(Pos(x, y), h - 10, 0, 1, predFactory(-4, 2, w / 4, Result::ATK_L));
        x = w / 2, y = bmap->height - h;
        bmap->testLine(Pos(x, y), h - 10, 0, 1, predFactory(+4, 2, w / 4, Result::ATK_R));

        sizeAtk3 = maxSize;
        if (maxSize > 19 && !!result) {
            posAtk3Last = posAtk1 = maxPos;
            resAtk3Last = result;
            timeAtk3 = std_clock::now();
            cooldownScreenReset();
            return result;
        }
        return Result::NONE;
    }

    Result checkThirdParry()
    {
        auto predColor = [&](uint32_t index) {
            Color rgb = bmap->get(index);
            if (rgb.isDebug())
                return true;

            if (rgb.r != 0xff || rgb.g < 120 || rgb.b < 100)
                return false;

            return true;
        };

        auto pred = [&](uint32_t index) {
            bool p = resAtk3Last == Result::ATK_L ? 1 : -1;

            if (!bmap->testLine(index, 13, +2 * p, +2, predColor))
                return false;

            if (!bmap->testLine(index, 8, -1 * p, -3, predColor))
                return false;

            return true;
        };

        Result result = Result::NONE;

        int w = 100, h = 100;
        int x = posAtk3Last.x - w / 2, y = posAtk3Last.y - h / 2;
        auto sr = bmap->search<2, 2>(x, y, x + w, x + h, pred);
        if (sr) {
            isParry3Now = true;
            posAtk1 = sr.asPos();
            cooldownScreenReset();
        }
        else if (isParry3Now) {
            isParry3Now = false;
            result = Result::PARRY;
            posAtk3Last = 0;
            resAtk3Last = Result::NONE;
            cooldownScreenReset();
        }

        return result;
    }

    void cooldownScreenReset()
    {
        cooldownScreen = 10;
    }
};

Bot::Bot() : impl(std::make_unique<Private>())
{}

Bot::~Bot() = default;

Bot::PropInt* Bot::propInt(LPTSTR key)
{ return impl->propsInt[key]; }

Bot::PropFlt* Bot::propFlt(LPTSTR key)
{ return impl->propsFlt[key]; }

Bot::PropStr* Bot::propStr(LPTSTR key)
{ return impl->propsStr[key]; }

void Bot::attackSearchRect(int left, int top, int right, int bottom)
{
    impl->initBitmap(left, top, right, bottom);
}

int Bot::attackSearch(int deflect)
{
    return resultInt(impl->attackSearch(deflect));
}

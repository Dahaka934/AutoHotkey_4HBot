#include "stdafx.h"
#include "fh_bitmap.h"

#include <d2d1.h>
#include <d2d1helper.h>
#include <dwrite.h>
#include <wincodec.h>

using namespace ForHonor;

Bitmap::Bitmap(uint32_t posX1, uint32_t posY1, uint32_t posX2, uint32_t posY2)
    : posX1(posX1)
    , posX2(posX2)
    , posY1(posY1)
    , posY2(posY2)
    , width(posX2 - posX1 + 1)
    , height(posY2 - posY1 + 1)
    , size(width * height)
{
    hdc = GetDC(NULL);
    if (!hdc)
        return;

    sdc = CreateCompatibleDC(hdc);
    if (!sdc)
        return;

    hbitmap_screen = CreateCompatibleBitmap(hdc, width, height);
    if (!hbitmap_screen)
        return;

    sdc_orig_select = SelectObject(sdc, hbitmap_screen);
    if (!sdc_orig_select)
        return;

    buf = new COLORREF[size];
}

Bitmap::Bitmap()
{
}

Bitmap::~Bitmap()
{
    if (hdc)
        ReleaseDC(NULL, hdc);
    if (sdc) {
        if (sdc_orig_select)
            SelectObject(sdc, sdc_orig_select);
        DeleteDC(sdc);
    }
    if (hbitmap_screen)
        DeleteObject(hbitmap_screen);
    if (buf)
        delete buf;

    hdc = NULL;
    sdc = NULL;
    hbitmap_screen = NULL;
    buf = NULL;
}

bool Bitmap::blit()
{
    if (!BitBlt(sdc, 0, 0, width, height, hdc, posX1, posY1, SRCCOPY))
        return false;

    struct BITMAPINFO3 {
        BITMAPINFOHEADER bmiHeader;
        RGBQUAD bmiColors[260];
    } bmi;

    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = width;
    bmi.bmiHeader.biHeight = -(int)height;
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;
    bmi.bmiHeader.biSizeImage = 0;

    return GetDIBits(sdc, hbitmap_screen, 0, height, buf, (LPBITMAPINFO)&bmi, DIB_RGB_COLORS);
}

static PBITMAPINFO CreateBitmapInfoStruct(HBITMAP hBmp)
{
    BITMAP bmp;
    PBITMAPINFO pbmi;
    WORD cClrBits;

    // Retrieve the bitmap color format, width, and height.
    GetObject(hBmp, sizeof(BITMAP), (LPSTR)&bmp);

    // Convert the color format to a count of bits.
    cClrBits = (WORD)(bmp.bmPlanes * bmp.bmBitsPixel);
    if (cClrBits == 1)
        cClrBits = 1;
    else if (cClrBits <= 4)
        cClrBits = 4;
    else if (cClrBits <= 8)
        cClrBits = 8;
    else if (cClrBits <= 16)
        cClrBits = 16;
    else if (cClrBits <= 24)
        cClrBits = 24;
    else
        cClrBits = 32;

    // Allocate memory for the BITMAPINFO structure. (This structure
    // contains a BITMAPINFOHEADER structure and an array of RGBQUAD
    // data structures.)

    if (cClrBits < 24)
        pbmi = (PBITMAPINFO)LocalAlloc(LPTR,
            sizeof(BITMAPINFOHEADER) + sizeof(RGBQUAD) * (1 << cClrBits));

    // There is no RGBQUAD array for these formats: 24-bit-per-pixel or 32-bit-per-pixel

    else
        pbmi = (PBITMAPINFO)LocalAlloc(LPTR,
            sizeof(BITMAPINFOHEADER));

    // Initialize the fields in the BITMAPINFO structure.

    pbmi->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    pbmi->bmiHeader.biWidth = bmp.bmWidth;
    pbmi->bmiHeader.biHeight = bmp.bmHeight;
    pbmi->bmiHeader.biPlanes = bmp.bmPlanes;
    pbmi->bmiHeader.biBitCount = bmp.bmBitsPixel;
    if (cClrBits < 24)
        pbmi->bmiHeader.biClrUsed = (1 << cClrBits);

    // If the bitmap is not compressed, set the BI_RGB flag.
    pbmi->bmiHeader.biCompression = BI_RGB;

    // Compute the number of bytes in the array of color
    // indices and store the result in biSizeImage.
    // The width must be DWORD aligned unless the bitmap is RLE
    // compressed.
    pbmi->bmiHeader.biSizeImage = ((pbmi->bmiHeader.biWidth * cClrBits + 31) & ~31) / 8
        * pbmi->bmiHeader.biHeight;
    // Set biClrImportant to 0, indicating that all of the
    // device colors are important.
    pbmi->bmiHeader.biClrImportant = 0;
    return pbmi;
}

void Bitmap::dump(size_t index, COLORREF marker)
{
    TCHAR name[64];
    swprintf(name, L"4H Screenshots\\img%03d.bmp", index);

    DWORD dwTmp;

    PBITMAPINFO pbi = CreateBitmapInfoStruct(hbitmap_screen);
    PBITMAPINFOHEADER pbih = (PBITMAPINFOHEADER)pbi;

    // Create the .BMP file.
    HANDLE hf = CreateFile(name,
        GENERIC_READ | GENERIC_WRITE,
        (DWORD)0,
        NULL,
        CREATE_ALWAYS,
        FILE_ATTRIBUTE_NORMAL,
        (HANDLE)NULL);

    BITMAPFILEHEADER hdr; // bitmap file-header
    hdr.bfType = 0x4d42; // 0x42 = "B" 0x4d = "M"
    // Compute the size of the entire file.
    hdr.bfSize = (DWORD)(sizeof(BITMAPFILEHEADER) + pbih->biSize + pbih->biClrUsed * sizeof(RGBQUAD) + pbih->biSizeImage);
    hdr.bfReserved1 = 0;
    hdr.bfReserved2 = 0;

    // Compute the offset to the array of color indices.
    hdr.bfOffBits = (DWORD)sizeof(BITMAPFILEHEADER) + pbih->biSize + pbih->biClrUsed * sizeof(RGBQUAD);

    // Copy the BITMAPFILEHEADER into the .BMP file.
    WriteFile(hf, &hdr, sizeof(BITMAPFILEHEADER),
        &dwTmp, NULL);

    // Copy the BITMAPINFOHEADER and RGBQUAD array into the file.
    WriteFile(hf, pbih, sizeof(BITMAPINFOHEADER) + pbih->biClrUsed * sizeof(RGBQUAD),
        &dwTmp, (NULL));

    if (marker)
        drawQuad(0, 3, width - 1, 5, marker);

    // Copy the array of color indices into the .BMP file.
    for (uint32_t i = 0; i < width; ++i) {
        for (uint32_t j = 0, jj = height - 1; j < height / 2; ++j, --jj) {
            std::swap(at(i, j), at(i, jj));
        }
    }

    WriteFile(hf, buf, pbih->biSizeImage, (LPDWORD)&dwTmp, NULL);

    // Close the .BMP file.
    CloseHandle(hf);
}

void Bitmap::drawQuad(int x1, int y1, int x2, int y2, COLORREF color)
{
    clampRect(x1, y1, x2, y2);

    if (x1 > x2)
        std::swap(x1, x2);
    if (y1 > y2)
        std::swap(y1, y2);

    for (int j = y1; j <= y2; ++j) {
        for (int i = x1; i <= x2; ++i) {
            at(i, j) = color;
        }
    }
}

void Bitmap::drawQuadAround(int x, int y, int r, COLORREF color)
{
    drawQuad(x - r, y - r, x + r, y + r, color);
}

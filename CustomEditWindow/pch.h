#pragma once
#include <SDKDDKVer.h>
#define GDIPVER 0x0110  // 또는 0x0100
#include <windows.h>
#include <cwchar>     // C++
#include <wchar.h>    // C
#include <imm.h>
#pragma comment(lib, "imm32")

#include <ObjIdl.h>
#include <gdiplus.h>
#include <gdiplusheaders.h>
#pragma comment (lib, "gdiplus.lib")
using namespace Gdiplus;
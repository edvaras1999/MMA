#pragma once
#define _WIN32_WINNT 0x0601
#define WINVER 0x0601
#include <cstdlib>
#include <cstdio>
#include <cstring>
#pragma warning(push)
#pragma warning(disable:4244)
#include "common/ITypes.h"
#include "common/IErrors.h"
#include "common/IDynamicCreate.h"
#include "common/IDebugLog.h"
#include "common/ISingleton.h"
#pragma warning(pop)
#include <winsock2.h>
#include <windows.h>
#include <commdlg.h>

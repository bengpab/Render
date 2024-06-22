#include "DxErrorHandling.h"

#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

bool DxEnsureImpl(long hr, const char* expression)
{
    if (hr == S_OK)
        return true;

    LPTSTR errorText = NULL;
    FormatMessage(
        FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL,
        (HRESULT)hr,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPTSTR)&errorText,
        0,
        NULL);

    if (errorText != NULL)
    {
        OutputDebugStringA(expression);
        OutputDebugString(errorText);
        if (IsDebuggerPresent())
        {
            DebugBreak();
        }
        else
        {           
            assert(0 && errorText);
        }
        LocalFree(errorText);
    }

    return false;
}

void DxAssertImpl(HRESULT hr, const char* expression)
{
    if (hr == S_OK)
        return;

    LPTSTR errorText = NULL;
    FormatMessage(
        FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL,
        hr,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPTSTR)&errorText,
        0,
        NULL);

    if (errorText != NULL)
    {
        OutputDebugStringA(expression);
        OutputDebugString(errorText);

        if (IsDebuggerPresent())
        {
            DebugBreak();
        }

        assert(0 && errorText);
        LocalFree(errorText);
    }
}
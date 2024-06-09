#pragma once

bool DxEnsureImpl(long hr, const char* expression);
#define DXENSURE(x) DxEnsureImpl(x, #x)

void DxAssertImpl(long hr, const char* expression);
#define DXASSERT(x) DxAssertImpl(x, #x)
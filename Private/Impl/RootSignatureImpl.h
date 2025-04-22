#pragma once

#include "Render.h"

namespace rl
{

bool RootSignature_CreateImpl(RootSignature_t rs, const RootSignatureDesc& desc);

void RootSignature_DestroyImpl(RootSignature_t rs);

}
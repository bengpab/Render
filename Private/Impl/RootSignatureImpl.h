#pragma once

#include "Render.h"

namespace tpr
{

bool RootSignature_CreateImpl(RootSignature_t rs, const RootSignatureDesc& desc);

void RootSignature_DestroyImpl(RootSignature_t rs);

}
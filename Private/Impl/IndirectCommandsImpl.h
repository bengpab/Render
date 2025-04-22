#pragma once

#include "IndirectCommands.h"
#include "RootSignature.h"

namespace rl
{
struct IndirectCommandDesc
{
	IndirectCommandType type;
	RootSignature_t rootSig;
};

bool CreateIndirectCommandImpl(IndirectCommand_t ic, const IndirectCommandDesc& desc);

void DestroyIndirectCommandImpl(IndirectCommand_t ic);
}
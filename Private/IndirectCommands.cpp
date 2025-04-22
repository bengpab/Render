#include "IndirectCommands.h"

#include "IDArray.h"
#include "RootSignature.h"

#include "Impl/IndirectCommandsImpl.h"

namespace rl
{

IDArray<IndirectCommand_t, IndirectCommandDesc> g_IndirectCommands;

IndirectCommand_t CreateIndirectCommand(IndirectCommandType type, RootSignature_t rootSig)
{
	IndirectCommandDesc desc;
	desc.type = type;
	desc.rootSig = rootSig;

	IndirectCommand_t ic = g_IndirectCommands.Create(desc);

	if (!CreateIndirectCommandImpl(ic, desc))
	{
		g_IndirectCommands.Release(ic);
		return IndirectCommand_t::INVALID;
	}

	return ic;
}

void RenderRef(IndirectCommand_t ic)
{
	g_IndirectCommands.AddRef(ic);
}

void RenderRelease(IndirectCommand_t ic)
{
	if (g_IndirectCommands.Release(ic))
	{
		DestroyIndirectCommandImpl(ic);
	}
}

size_t IndirectCommandLayoutSize(IndirectCommandType type)
{
	switch (type)
	{
	case IndirectCommandType::INDIRECT_DRAW: return sizeof(IndirectDrawLayout);
	case IndirectCommandType::INDIRECT_DRAW_INDEXED: return sizeof(IndirectDrawIndexedLayout);
	case IndirectCommandType::INDIRECT_DISPATCH: return sizeof(IndirectDispatchLayout);
	}

	assert(0 && "IndirectCommandLayoutSize invalid type");

	return 0;
}

IndirectCommandType GetIndirectCommandType(IndirectCommand_t ic)
{
	if (const IndirectCommandDesc* desc = g_IndirectCommands.Get(ic))
	{
		return desc->type;
	}

	return IndirectCommandType(0);
}

}
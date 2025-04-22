#include "Impl/IndirectCommandsImpl.h"

#include "RenderImpl.h"
#include "SparseArray.h"

namespace rl
{

SparseArray<ComPtr<ID3D12CommandSignature>, IndirectCommand_t> g_DxCommandSignatures;

D3D12_INDIRECT_ARGUMENT_TYPE Dx12_IndirectArgumentType(IndirectCommandType type)
{
	switch (type)
	{
	case IndirectCommandType::INDIRECT_DRAW: return D3D12_INDIRECT_ARGUMENT_TYPE_DRAW;
	case IndirectCommandType::INDIRECT_DRAW_INDEXED: return D3D12_INDIRECT_ARGUMENT_TYPE_DRAW_INDEXED;
	case IndirectCommandType::INDIRECT_DISPATCH: return D3D12_INDIRECT_ARGUMENT_TYPE_DISPATCH;
	}

	assert(0 && "Dx12_IndirectArgumentType invalid argument");

	return (D3D12_INDIRECT_ARGUMENT_TYPE)0;
}

bool RequiresRootSignature(IndirectCommandType type)
{
	return	type != IndirectCommandType::INDIRECT_DRAW &&
			type != IndirectCommandType::INDIRECT_DRAW_INDEXED &&
			type != IndirectCommandType::INDIRECT_DISPATCH;
}

bool CreateIndirectCommandImpl(IndirectCommand_t ic, const IndirectCommandDesc& desc)
{
	ID3D12RootSignature* dxRootSig = nullptr;
	
	if (RequiresRootSignature(desc.type))
	{
		Dx12_GetRootSignature(desc.rootSig != RootSignature_t::INVALID ? desc.rootSig : g_render.RootSignature);

		if (!dxRootSig)
		{
			return false;
		}
	}

	D3D12_INDIRECT_ARGUMENT_DESC dxArg = {};
	dxArg.Type = Dx12_IndirectArgumentType(desc.type);

	D3D12_COMMAND_SIGNATURE_DESC dxDesc = {};
	dxDesc.ByteStride = (UINT)IndirectCommandLayoutSize(desc.type);
	dxDesc.NodeMask = 0u;
	dxDesc.NumArgumentDescs = 1u;
	dxDesc.pArgumentDescs = &dxArg;

	ComPtr<ID3D12CommandSignature>& dxCommandSig = g_DxCommandSignatures.Alloc(ic);

	if (!DXENSURE(g_render.DxDevice->CreateCommandSignature(&dxDesc, dxRootSig, IID_PPV_ARGS(&dxCommandSig))))
	{
		return false;
	}

	return true;
}

ID3D12CommandSignature* Dx12_GetCommandSignature(IndirectCommand_t ic)
{
	if (g_DxCommandSignatures.Valid(ic))
	{
		return g_DxCommandSignatures[ic].Get();
	}

	return nullptr;
}

void DestroyIndirectCommandImpl(IndirectCommand_t ic)
{
	return g_DxCommandSignatures.Free(ic);
}

}
#include "CommandList.h"

#include "RenderImpl.h"

namespace tpr
{

std::vector<CommandListPtr> g_FreeCommandLists;

struct CommandListImpl
{
	ComPtr<ID3D11DeviceContext> context = nullptr;
	ComPtr<ID3D11CommandList> commandList = nullptr;
};

CommandList::CommandList(CommandListImpl* cl)
{
	impl = std::unique_ptr<CommandListImpl>(cl);
}

CommandList::~CommandList()
{
}

void CommandList::Begin()
{
	impl->context->ClearState();
}

void CommandList::Finish()
{
	impl->context->FinishCommandList(FALSE, &impl->commandList);
	LastPipeline = GraphicsPipelineState_t::INVALID;
}

void CommandList::SetRootSignature()
{
	SetRootSignature(g_render.MainRootSig);
}

void CommandList::SetRootSignature(RootSignature_t rs)
{
	if (rs != BoundRootSignature)
	{
		const auto* globalSamplers = Dx11_GetGlobalSamplers(rs);

		if (globalSamplers)
		{
			impl->context->VSSetSamplers(0, (UINT)globalSamplers->size(), globalSamplers->data());
			impl->context->CSSetSamplers(0, (UINT)globalSamplers->size(), globalSamplers->data());
			impl->context->PSSetSamplers(0, (UINT)globalSamplers->size(), globalSamplers->data());
		}

		BoundRootSignature = rs;
	}
}

void CommandList::ClearRenderTarget(RenderTargetView_t rtv, const float col[4])
{
	ID3D11RenderTargetView* dxRtv = Dx11_GetRenderTargetView(rtv);
	impl->context->ClearRenderTargetView(dxRtv, col);
}

void CommandList::ClearDepth(DepthStencilView_t dsv, float depth)
{
	ID3D11DepthStencilView* dxDsv = Dx11_GetDepthStencilView(dsv);
	impl->context->ClearDepthStencilView(dxDsv, D3D11_CLEAR_DEPTH, depth, 0);
}

void CommandList::SetRenderTargets(const RenderTargetView_t* const rtvs, size_t num, DepthStencilView_t dsv)
{
	assert(num <= 8);

	ID3D11RenderTargetView* dxRtvs[8];

	for(size_t i = 0; i < num; i++)
		dxRtvs[i] = Dx11_GetRenderTargetView(rtvs[i]);

	ID3D11DepthStencilView* dxDsv = Dx11_GetDepthStencilView(dsv);

	impl->context->OMSetRenderTargets((UINT)num, dxRtvs, dxDsv);
}

void CommandList::SetViewports(const Viewport* const vps, size_t num)
{
	assert(num <= 8);

	D3D11_VIEWPORT dxVps[8];
	for (size_t i = 0; i < num; i++)
	{
		dxVps[i].TopLeftX	= vps[i].topLeftX;
		dxVps[i].TopLeftY	= vps[i].topLeftY;
		dxVps[i].Width		= vps[i].width;
		dxVps[i].Height		= vps[i].height;
		dxVps[i].MinDepth	= vps[i].minDepth;
		dxVps[i].MaxDepth	= vps[i].maxDepth;
	}

	impl->context->RSSetViewports((UINT)num, dxVps);
}

void CommandList::SetDefaultScissor()
{
	D3D11_RECT rect = CD3D11_RECT(0, 0, LONG_MAX, LONG_MAX);
	impl->context->RSSetScissorRects(1, &rect);
}

void CommandList::SetScissors(const ScissorRect* const scissors, size_t num)
{
	assert(num <= 8);
	D3D11_RECT dxRects[8];
	for (size_t i = 0; i < num; i++)
	{
		dxRects[i].left = scissors[i].left;
		dxRects[i].top = scissors[i].top;
		dxRects[i].right = scissors[i].right;
		dxRects[i].bottom = scissors[i].bottom;
	}

	impl->context->RSSetScissorRects((UINT)num, dxRects);
}

void CommandList::SetPipelineState(GraphicsPipelineState_t pso)
{
	if (pso == LastPipeline)
		return;

	LastComputePipeline = ComputePipelineState_t::INVALID;
	LastPipeline = pso;

	Dx11GraphicsPipelineState* dxPso = Dx11_GetGraphicsPipelineState(pso);

	if (!dxPso)
	{
		return;
	}	

	impl->context->IASetPrimitiveTopology(dxPso->pt);
	impl->context->IASetInputLayout(dxPso->il.Get());
	impl->context->OMSetDepthStencilState(dxPso->dss.Get(), 0);
	impl->context->RSSetState(dxPso->rs.Get());
	impl->context->OMSetBlendState(dxPso->bs.Get(), nullptr, 0xffffffff);
	impl->context->VSSetShader(Dx11_GetVertexShader(dxPso->vs), nullptr, 0);
	impl->context->GSSetShader(Dx11_GetGeometryShader(dxPso->gs), nullptr, 0);
	impl->context->PSSetShader(Dx11_GetPixelShader(dxPso->ps), nullptr, 0);
	impl->context->CSSetShader(nullptr, nullptr, 0);
}

void CommandList::SetPipelineState(ComputePipelineState_t pso)
{
	if (pso == LastComputePipeline)
		return;

	LastPipeline = GraphicsPipelineState_t::INVALID;
	LastComputePipeline = pso;

	Dx11ComputePipelineState* dxPso = Dx11_GetComputePipelineState(pso);

	if(!dxPso)
	{
		return;
	}

	impl->context->CSSetShader(Dx11_GetComputeShader(dxPso->_cs), nullptr, 0);
}

void CommandList::SetVertexBuffers(uint32_t startSlot, uint32_t count, const VertexBuffer_t* const vbs, const uint32_t* const strides, const uint32_t* const offsets)
{
	const UINT endSlot = startSlot + count;
	UINT slot = startSlot;
	for (UINT i = 0; slot < endSlot; slot++, i++)
	{
		SetVertexBuffer(slot, vbs[i], strides[i], offsets[i]);
	}
}

void CommandList::SetVertexBuffers(uint32_t startSlot, uint32_t count, const DynamicBuffer_t* const vbs, const uint32_t* const strides, const uint32_t* const offsets)
{
	const UINT endSlot = startSlot + count;
	UINT slot = startSlot;
	for (UINT i = 0; slot < endSlot; slot++, i++)
	{
		SetVertexBuffer(slot, vbs[i], strides[i], offsets[i]);
	}
}

void CommandList::SetVertexBuffer(uint32_t slot, VertexBuffer_t vb, uint32_t stride, uint32_t offset)
{
	ID3D11Buffer* dxVb = Dx11_GetVertexBuffer(vb);
	impl->context->IASetVertexBuffers(slot, 1, &dxVb, (const UINT*)(&stride), (const UINT*)(&offset));
}

void CommandList::SetVertexBuffer(uint32_t slot, DynamicBuffer_t vb, uint32_t stride, uint32_t offset)
{
	ID3D11Buffer* dxVb = Dx11_GetDynamicBuffer(vb);
	impl->context->IASetVertexBuffers(slot, 1, &dxVb, (const UINT*)(&stride), (const UINT*)(&offset));
}

void CommandList::SetIndexBuffer(IndexBuffer_t ib, RenderFormat format, uint32_t indexOffset)
{
	ID3D11Buffer* dxIb = Dx11_GetIndexBuffer(ib);
	impl->context->IASetIndexBuffer(dxIb, Dx11_Format(format), (UINT)indexOffset);
}

void CommandList::SetIndexBuffer(DynamicBuffer_t ib, RenderFormat format, uint32_t indexOffset)
{
	ID3D11Buffer* dxIb = Dx11_GetDynamicBuffer(ib);
	impl->context->IASetIndexBuffer(dxIb, Dx11_Format(format), (UINT)indexOffset);
}

void CommandList::CopyTexture(Texture_t dst, Texture_t src)
{
	ID3D11Resource* dxDst = Dx11_GetTexture(dst);
	ID3D11Resource* dxSrc = Dx11_GetTexture(src);

	impl->context->CopyResource(dxDst, dxSrc);
}

void CommandList::DrawIndexedInstanced(uint32_t numIndices, uint32_t numInstances, uint32_t startIndex, uint32_t startVertex, uint32_t startInstance)
{
	impl->context->DrawIndexedInstanced((UINT)numIndices, (UINT)numInstances, (UINT)startIndex, (UINT)startVertex, (UINT)startInstance);
}

void CommandList::DrawInstanced(uint32_t numVerts, uint32_t numInstances, uint32_t startVertex, uint32_t startInstance)
{
	impl->context->DrawInstanced((UINT)numVerts, (UINT)numInstances, (UINT)startVertex, (UINT)startInstance);
}

void CommandList::Dispatch(uint32_t x, uint32_t y, uint32_t z)
{
	impl->context->Dispatch(x, y, z);
}

// Dx11 Style Bind Commands
void CommandList::BindVertexSRVs(uint32_t startSlot, uint32_t count, const ShaderResourceView_t* const srvs)
{
	const UINT endSlot = startSlot + count;
	UINT slot = startSlot;
	for (UINT i = 0; slot < endSlot; i++, slot++)
	{
		ID3D11ShaderResourceView* dxSrv = Dx11_GetShaderResourceView(srvs[i]);
		impl->context->VSSetShaderResources(slot, 1, &dxSrv);
	}
}

void CommandList::BindVertexCBVs(uint32_t startSlot, uint32_t count, const ConstantBuffer_t* const cbvs)
{
	const UINT endSlot = startSlot + count;
	UINT slot = startSlot;
	for (UINT i = 0; slot < endSlot; i++, slot++)
	{
		ID3D11Buffer* dxCbv = Dx11_GetConstantBuffer(cbvs[i]);
		impl->context->VSSetConstantBuffers(slot, 1, &dxCbv);
	}
}

void CommandList::BindVertexCBVs(uint32_t startSlot, uint32_t count, const DynamicBuffer_t* const cbvs)
{
	const UINT endSlot = startSlot + count;
	UINT slot = startSlot;
	for (UINT i = 0; slot < endSlot; i++, slot++)
	{
		ID3D11Buffer* dxCbv = Dx11_GetDynamicBuffer(cbvs[i]);
		impl->context->VSSetConstantBuffers(slot, 1, &dxCbv);
	}
}

void CommandList::BindGeometryCBVs(uint32_t startSlot, uint32_t count, const ConstantBuffer_t* const cbvs)
{
	const UINT endSlot = startSlot + count;
	UINT slot = startSlot;
	for (UINT i = 0; slot < endSlot; i++, slot++)
	{
		ID3D11Buffer* dxCbv = Dx11_GetConstantBuffer(cbvs[i]);
		impl->context->GSSetConstantBuffers(slot, 1, &dxCbv);
	}
}

void CommandList::BindGeometryCBVs(uint32_t startSlot, uint32_t count, const DynamicBuffer_t* const cbvs)
{
	const UINT endSlot = startSlot + count;
	UINT slot = startSlot;
	for (UINT i = 0; slot < endSlot; i++, slot++)
	{
		ID3D11Buffer* dxCbv = Dx11_GetDynamicBuffer(cbvs[i]);
		impl->context->GSSetConstantBuffers(slot, 1, &dxCbv);
	}
}

void CommandList::BindPixelSRVs(uint32_t startSlot, uint32_t count, const ShaderResourceView_t* const srvs)
{
	const UINT endSlot = startSlot + count;
	UINT slot = startSlot;
	for (UINT i = 0; slot < endSlot; i++, slot++)
	{
		ID3D11ShaderResourceView* dxSrv = Dx11_GetShaderResourceView(srvs[i]);
		impl->context->PSSetShaderResources(slot, 1, &dxSrv);
	}
}

void CommandList::BindPixelCBVs(uint32_t startSlot, uint32_t count, const ConstantBuffer_t* const cbvs)
{
	const UINT endSlot = startSlot + count;
	UINT slot = startSlot;
	for (UINT i = 0; slot < endSlot; i++, slot++)
	{
		ID3D11Buffer* dxCbv = Dx11_GetConstantBuffer(cbvs[i]);
		impl->context->PSSetConstantBuffers(slot, 1, &dxCbv);
	}
}

void CommandList::BindPixelCBVs(uint32_t startSlot, uint32_t count, const DynamicBuffer_t* const cbvs)
{
	const UINT endSlot = startSlot + count;
	UINT slot = startSlot;
	for (UINT i = 0; slot < endSlot; i++, slot++)
	{
		ID3D11Buffer* dxCbv = Dx11_GetDynamicBuffer(cbvs[i]);
		impl->context->PSSetConstantBuffers(slot, 1, &dxCbv);
	}
}

void CommandList::BindComputeSRVs(uint32_t startSlot, uint32_t count, const ShaderResourceView_t* const srvs)
{
	const UINT endSlot = startSlot + count;
	UINT slot = startSlot;
	for (UINT i = 0; slot < endSlot; i++, slot++)
	{
		ID3D11ShaderResourceView* dxSrv = Dx11_GetShaderResourceView(srvs[i]);
		impl->context->CSSetShaderResources(slot, 1, &dxSrv);
	}
}

void CommandList::BindComputeUAVs(uint32_t startSlot, uint32_t count, const UnorderedAccessView_t* const srvs)
{
	const UINT endSlot = startSlot + count;
	UINT slot = startSlot;
	for (UINT i = 0; slot < endSlot; i++, slot++)
	{
		ID3D11UnorderedAccessView* dxUav = Dx11_GetUnorderedAccessView(srvs[i]);
		impl->context->CSSetUnorderedAccessViews(slot, 1, &dxUav, nullptr);
	}
}

void CommandList::BindComputeCBVs(uint32_t startSlot, uint32_t count, const ConstantBuffer_t* const cbvs)
{
	const UINT endSlot = startSlot + count;
	UINT slot = startSlot;
	for (UINT i = 0; slot < endSlot; i++, slot++)
	{
		ID3D11Buffer* dxCbv = Dx11_GetConstantBuffer(cbvs[i]);
		impl->context->CSSetConstantBuffers(slot, 1, &dxCbv);
	}
}

void CommandList::BindComputeCBVs(uint32_t startSlot, uint32_t count, const DynamicBuffer_t* const cbvs)
{
	const UINT endSlot = startSlot + count;
	UINT slot = startSlot;
	for (UINT i = 0; slot < endSlot; i++, slot++)
	{
		ID3D11Buffer* dxCbv = Dx11_GetDynamicBuffer(cbvs[i]);
		impl->context->CSSetConstantBuffers(slot, 1, &dxCbv);
	}
}

void CommandList::SetGraphicsRootValue(uint32_t slot, uint32_t value) { assert(0); }
void CommandList::SetComputeRootValue(uint32_t slot, uint32_t value) { assert(0); }
void CommandList::SetGraphicsRootCBV(uint32_t slot, ConstantBuffer_t cb) { assert(0); }
void CommandList::SetComputeRootCBV(uint32_t slot, ConstantBuffer_t cb) { assert(0); }
void CommandList::SetGraphicsRootCBV(uint32_t slot, DynamicBuffer_t cb) { assert(0); }
void CommandList::SetComputeRootCBV(uint32_t slot, DynamicBuffer_t cb) { assert(0); }
void CommandList::SetGraphicsRootSRV(uint32_t slot, ShaderResourceView_t srv) { assert(0); }
void CommandList::SetComputeRootSRV(uint32_t slot, ShaderResourceView_t srv) { assert(0); }
void CommandList::SetGraphicsRootUAV(uint32_t slot, UnorderedAccessView_t uav) { assert(0); }
void CommandList::SetComputeRootUAV(uint32_t slot, UnorderedAccessView_t uav) { assert(0); }
void CommandList::SetGraphicsRootDescriptorTable(uint32_t slot) { assert(0); }
void CommandList::SetComputeRootDescriptorTable(uint32_t slot) { assert(0); }

void CommandList::TransitionResource(Texture_t tex, ResourceTransitionState before, ResourceTransitionState after) {}
void CommandList::UAVBarrier(Texture_t tex) {}

CommandListPtr CommandList::Create()
{
	if (!g_FreeCommandLists.empty())
	{
		CommandListPtr cl = g_FreeCommandLists.back();
		g_FreeCommandLists.pop_back();

		cl->Begin();

		return cl;
	}
	else
	{
		CommandListImpl* impl = new CommandListImpl;

		g_render.Device->CreateDeferredContext(0, &impl->context);

		CommandListPtr cl = std::make_shared<CommandList>(impl);

		cl->Begin();

		return cl;
	}
}

CommandListPtr CommandList::Create(CommandListType type)
{
	return CommandList::Create();
}

CommandList* CommandList::CreateRaw(CommandListType type)
{
	CommandListImpl* impl = new CommandListImpl;

	g_render.Device->CreateDeferredContext(0, &impl->context);

	CommandList* cl = new CommandList(impl);

	cl->Begin();

	return cl;
}

void CommandList::Execute(CommandListPtr& cl)
{
	assert(cl);

	cl->Finish();

	assert(cl->impl->commandList);

	g_render.DeviceContext->ExecuteCommandList(cl->impl->commandList.Get(), FALSE);

	g_FreeCommandLists.push_back(cl);
}

void CommandList::ExecuteAndStall(CommandListPtr& cl)
{
	D3D11_QUERY_DESC qDesc = {};
	qDesc.Query = D3D11_QUERY_EVENT;

	ComPtr<ID3D11Query> dxQuery;

	g_render.Device->CreateQuery(&qDesc, &dxQuery);

	cl->Finish();

	g_render.DeviceContext->ExecuteCommandList(cl->impl->commandList.Get(), FALSE);

	g_render.DeviceContext->End(dxQuery.Get());

	UINT32 queryData;
	while (g_render.DeviceContext->GetData(dxQuery.Get(), &queryData, sizeof(queryData), 0) != S_OK)
		Sleep(5);
}

void CommandList::ReleaseAll()
{
	g_FreeCommandLists.clear();
}

CommandListImpl* CommandList::GetCommandListImpl()
{
	return impl.get();
}

CommandList* CommandListSubmissionGroup::CreateCommandList()
{
	CommandLists.emplace_back(std::unique_ptr<CommandList>(CommandList::CreateRaw(Type)));

	return CommandLists.back().get();
}

void CommandListSubmissionGroup::Submit()
{
	for (size_t i = 0; i < CommandLists.size(); i++)
	{
		CommandListImpl* impl = CommandLists[i]->GetCommandListImpl();

		impl->context->FinishCommandList(FALSE, &impl->commandList);

		g_render.DeviceContext->ExecuteCommandList(impl->commandList.Get(), FALSE);
		
		//g_FreeCommandLists.push_back(CommandListPtr(CommandLists[i].release()));
	}
}

}
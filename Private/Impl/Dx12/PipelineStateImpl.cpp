#include "Impl/PipelineStateImpl.h"

#include "RenderImpl.h"

#include "SparseArray.h"

#include <dxcapi.h>

struct IDxcBlob;

namespace tpr
{

struct
{
	SparseArray<Dx12GraphicsPipelineStateDesc, GraphicsPipelineState_t> GraphicsPipelines;
	SparseArray<ComPtr<ID3D12PipelineState>, ComputePipelineState_t> ComputePipelines;
} g_pipelines;

static D3D12_BLEND GetBlend(BlendType b)
{
	switch (b)
	{
	case BlendType::ZERO:			return D3D12_BLEND_ZERO;
	case BlendType::ONE:			return D3D12_BLEND_ONE;
	case BlendType::SRC_COLOR:		return D3D12_BLEND_SRC_COLOR;
	case BlendType::INV_SRC_COLOR:	return D3D12_BLEND_INV_SRC_COLOR;
	case BlendType::SRC_ALPHA:		return D3D12_BLEND_SRC_ALPHA;
	case BlendType::INV_SRC_ALPHA:	return D3D12_BLEND_INV_SRC_ALPHA;
	case BlendType::DST_ALPHA:		return D3D12_BLEND_DEST_ALPHA;
	case BlendType::INV_DST_ALPHA:	return D3D12_BLEND_INV_DEST_ALPHA;
	case BlendType::DST_COLOR:		return D3D12_BLEND_DEST_COLOR;
	case BlendType::INV_DST_COLOR:	return D3D12_BLEND_INV_DEST_COLOR;
	case BlendType::SRC_ALPHA_SAT:	return D3D12_BLEND_SRC_ALPHA_SAT;
	}

	assert(0 && "Unsupported blend func");
	return (D3D12_BLEND)0;
}

static D3D12_BLEND_OP GetBlendOp(BlendOp o)
{
	switch (o)
	{
	case BlendOp::ADD:			return D3D12_BLEND_OP_ADD;
	case BlendOp::SUBTRACT:		return D3D12_BLEND_OP_SUBTRACT;
	case BlendOp::REV_SUBTRACT:	return D3D12_BLEND_OP_REV_SUBTRACT;
	case BlendOp::MIN:			return D3D12_BLEND_OP_MIN;
	case BlendOp::MAX:			return D3D12_BLEND_OP_MAX;
	}

	assert(0 && "Unsupported blend op");
	return (D3D12_BLEND_OP)0;
}

static D3D12_FILL_MODE GetFillMode(FillMode fm)
{
	switch (fm)
	{
	case FillMode::SOLID:
		return D3D12_FILL_MODE_SOLID;
	case FillMode::WIREFRAME:
		return D3D12_FILL_MODE_WIREFRAME;
	};

	assert(0 && "Unsupported fill mode");
	return (D3D12_FILL_MODE)0;
}

static D3D12_CULL_MODE GetCullMode(CullMode cm)
{
	switch (cm)
	{
	case CullMode::NONE:
		return D3D12_CULL_MODE_NONE;
	case CullMode::FRONT:
		return D3D12_CULL_MODE_FRONT;
	case CullMode::BACK:
		return D3D12_CULL_MODE_BACK;
	};

	assert(0 && "Unsupported cull mode");
	return (D3D12_CULL_MODE)0;
}

static D3D12_COMPARISON_FUNC GetComparisonFunc(ComparisionFunc f)
{
	switch (f)
	{
	case ComparisionFunc::NEVER:		return D3D12_COMPARISON_FUNC_NEVER;
	case ComparisionFunc::LESS:			return D3D12_COMPARISON_FUNC_LESS;
	case ComparisionFunc::EQUAL:		return D3D12_COMPARISON_FUNC_EQUAL;
	case ComparisionFunc::LESS_EQUAL:	return D3D12_COMPARISON_FUNC_LESS_EQUAL;
	case ComparisionFunc::GREATER:		return D3D12_COMPARISON_FUNC_GREATER;
	case ComparisionFunc::NOT_EQUAL:		return D3D12_COMPARISON_FUNC_NOT_EQUAL;
	case ComparisionFunc::GREATER_EQUAL:	return D3D12_COMPARISON_FUNC_GREATER_EQUAL;
	case ComparisionFunc::ALWAYS:		return D3D12_COMPARISON_FUNC_ALWAYS;
	}

	assert(0 && "Unsupported comparison func");
	return (D3D12_COMPARISON_FUNC)0;
}

static D3D12_INPUT_CLASSIFICATION GetInputClassification(InputClassification ic)
{
	switch (ic)
	{
	case InputClassification::PER_VERTEX: return D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
	case InputClassification::PER_INSTANCE: return D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA;
	}

	assert(0 && "Unsupported input classification");
	return (D3D12_INPUT_CLASSIFICATION)0;
}

D3D12_PRIMITIVE_TOPOLOGY_TYPE GetPrimTopoType(PrimitiveTopologyType pt)
{
	switch (pt)
	{
	case PrimitiveTopologyType::POINT:
		return D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;
	case PrimitiveTopologyType::LINE:
		return D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;
	case PrimitiveTopologyType::TRIANGLE:
		return D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	}

	assert(0 && "Unsupported primitive topology type");
	return D3D12_PRIMITIVE_TOPOLOGY_TYPE_UNDEFINED;
}

bool CompileGraphicsPipelineState(GraphicsPipelineState_t handle, const GraphicsPipelineStateDesc& desc, const InputElementDesc* inputs, size_t inputCount)
{
	D3D12_GRAPHICS_PIPELINE_STATE_DESC dxDesc = {};
	
	RootSignature_t rootSigToUse = desc.RootSignatureOverride;

	if (rootSigToUse == RootSignature_t::INVALID)
		rootSigToUse = g_render.RootSignature;

	dxDesc.pRootSignature = Dx12_GetRootSignature(rootSigToUse);
	
	if(desc.Vs == VertexShader_t::INVALID)
	{
		assert(0 && "CompileGraphicsPipelineState needs a valid vertex shader");
		return false;
	}
	else
	{
		ComPtr<IDxcBlob> vsBlob = Dx12_GetVertexShaderBlob(desc.Vs);
		assert(vsBlob && "CompileGraphicsPipelineState null vsBlob");

		dxDesc.VS.pShaderBytecode = vsBlob->GetBufferPointer();
		dxDesc.VS.BytecodeLength = vsBlob->GetBufferSize();
	}

	if (desc.Ps != PixelShader_t::INVALID)
	{
		ComPtr<IDxcBlob> psBlob = Dx12_GetPixelShaderBlob(desc.Ps);
		assert(psBlob && "CompileGraphicsPipelineState null psBlob");

		dxDesc.PS.pShaderBytecode = psBlob->GetBufferPointer();
		dxDesc.PS.BytecodeLength = psBlob->GetBufferSize();
	}

	if (desc.Gs != GeometryShader_t::INVALID)
	{
		ComPtr<IDxcBlob> gsBlob = Dx12_GetGeometryShaderBlob(desc.Gs);
		assert(gsBlob && "CompileGraphicsPipelineState null gsBlob");

		dxDesc.GS.pShaderBytecode = gsBlob->GetBufferPointer();
		dxDesc.GS.BytecodeLength = gsBlob->GetBufferSize();
	}

	dxDesc.BlendState.AlphaToCoverageEnable = FALSE;

	for (uint8_t i = 1; i < desc.TargetDesc.NumRenderTargets; i++)
	{
		if (desc.TargetDesc.Blends[i].Opaque != desc.TargetDesc.Blends[0].Opaque)
		{
			dxDesc.BlendState.IndependentBlendEnable = true;
			break;
		}
	}

	for (uint8_t i = 0; i < desc.TargetDesc.NumRenderTargets; i++)
	{
		dxDesc.BlendState.RenderTarget[i].BlendEnable = desc.TargetDesc.Blends[i].BlendEnabled;
		dxDesc.BlendState.RenderTarget[i].SrcBlend = GetBlend(desc.TargetDesc.Blends[i].SrcBlend);
		dxDesc.BlendState.RenderTarget[i].DestBlend = GetBlend(desc.TargetDesc.Blends[i].DstBlend);
		dxDesc.BlendState.RenderTarget[i].BlendOp = GetBlendOp(desc.TargetDesc.Blends[i].Op);
		dxDesc.BlendState.RenderTarget[i].SrcBlendAlpha = GetBlend(desc.TargetDesc.Blends[i].SrcBlendAlpha);
		dxDesc.BlendState.RenderTarget[i].DestBlendAlpha = GetBlend(desc.TargetDesc.Blends[i].DstBlendAlpha);
		dxDesc.BlendState.RenderTarget[i].BlendOpAlpha = GetBlendOp(desc.TargetDesc.Blends[i].OpAlpha);
		dxDesc.BlendState.RenderTarget[i].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
	}

	dxDesc.SampleMask = UINT_MAX;

	dxDesc.RasterizerState.FillMode = GetFillMode(desc.Fill);
	dxDesc.RasterizerState.CullMode = GetCullMode(desc.Cull);
	dxDesc.RasterizerState.FrontCounterClockwise = FALSE;
	dxDesc.RasterizerState.DepthBias = desc.DepthBias;
	dxDesc.RasterizerState.DepthBiasClamp = desc.DepthBiasClamp;
	dxDesc.RasterizerState.SlopeScaledDepthBias = desc.SlopeScaleDepthBias;
	dxDesc.RasterizerState.DepthClipEnable = TRUE;
	dxDesc.RasterizerState.MultisampleEnable = FALSE;
	dxDesc.RasterizerState.AntialiasedLineEnable = FALSE;
	dxDesc.RasterizerState.ForcedSampleCount = 0;
	dxDesc.RasterizerState.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;

	dxDesc.DepthStencilState.DepthEnable = desc.DepthEnabled;
	dxDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
	dxDesc.DepthStencilState.DepthFunc = GetComparisonFunc(desc.DepthCompare);
	dxDesc.DepthStencilState.StencilEnable = FALSE;
	dxDesc.DepthStencilState.StencilReadMask = 0;
	dxDesc.DepthStencilState.StencilWriteMask = 0;

	std::vector<D3D12_INPUT_ELEMENT_DESC> dxLayout;
	dxLayout.resize(inputCount);

	for (size_t i = 0; i < inputCount; i++)
	{
		dxLayout[i].SemanticName = (LPCSTR)inputs[i].semanticName;
		dxLayout[i].SemanticIndex = (UINT)inputs[i].semanticIndex;
		dxLayout[i].Format = Dx12_Format(inputs[i].format);
		dxLayout[i].InputSlot = (UINT)inputs[i].inputSlot;
		dxLayout[i].AlignedByteOffset = (UINT)inputs[i].alinedByteOffset;
		dxLayout[i].InputSlotClass = GetInputClassification(inputs[i].inputSlotClass);
		dxLayout[i].InstanceDataStepRate = (UINT)inputs[i].instanceDataStepRate;
	}

	dxDesc.InputLayout.NumElements = (UINT)inputCount;
	dxDesc.InputLayout.pInputElementDescs = dxLayout.data();

	dxDesc.IBStripCutValue = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED;
	dxDesc.PrimitiveTopologyType = GetPrimTopoType(desc.PrimTopo);

	dxDesc.NumRenderTargets = desc.TargetDesc.NumRenderTargets;
	
	for (uint32_t i = 0; i < desc.TargetDesc.NumRenderTargets; i++)
	{
		dxDesc.RTVFormats[i] = Dx12_Format(desc.TargetDesc.Formats[i]);
	}

	dxDesc.DSVFormat = Dx12_Format(desc.DsvFormat);

	dxDesc.SampleDesc.Count = 1;
	dxDesc.SampleDesc.Quality = 0;

	dxDesc.NodeMask = 1;

	Dx12GraphicsPipelineStateDesc& dxPso = g_pipelines.GraphicsPipelines.Alloc(handle);

	if (DXENSURE(g_render.DxDevice->CreateGraphicsPipelineState(&dxDesc, IID_PPV_ARGS(&dxPso.PSO))))
	{
		if(!desc.DebugName.empty())
			dxPso.PSO->SetName(desc.DebugName.c_str());

		dxPso.PrimTopo = Dx12_PrimitiveTopology(desc.PrimTopo);

		return true;
	}	

	return false;
}

bool CompileComputePipelineState(ComputePipelineState_t handle, const ComputePipelineStateDesc& desc)
{
	if (desc.Cs == ComputeShader_t::INVALID)
	{
		assert(0 && "CompileComputePipelineState needs a valid compute shader");
		return false;
	}

	D3D12_COMPUTE_PIPELINE_STATE_DESC dxDesc = {};
	
	RootSignature_t rootSigToUse = desc.RootSignatureOverride;

	if (rootSigToUse == RootSignature_t::INVALID)
		rootSigToUse = g_render.RootSignature;

	dxDesc.pRootSignature = Dx12_GetRootSignature(rootSigToUse);

	ComPtr<IDxcBlob> csBlob = Dx12_GetComputeShaderBlob(desc.Cs);
	assert(csBlob && "CompileComputePipelineState null csBlob");

	dxDesc.CS.pShaderBytecode = csBlob->GetBufferPointer();
	dxDesc.CS.BytecodeLength = csBlob->GetBufferSize();

	dxDesc.NodeMask = 1;

	ComPtr<ID3D12PipelineState>& dxPso = g_pipelines.ComputePipelines.Alloc(handle);

	if (DXENSURE(g_render.DxDevice->CreateComputePipelineState(&dxDesc, IID_PPV_ARGS(&dxPso))))
	{
		if (!desc.DebugName.empty())
			dxPso->SetName(desc.DebugName.c_str());

		return true;
	}		

	return false;
}

void DestroyGraphicsPipelineState(GraphicsPipelineState_t pso)
{
	g_pipelines.GraphicsPipelines.Free(pso);
}

void DestroyComputePipelineState(ComputePipelineState_t pso)
{
	g_pipelines.ComputePipelines.Free(pso);
}

Dx12GraphicsPipelineStateDesc* Dx12_GetPipelineState(GraphicsPipelineState_t pso)
{
	if (g_pipelines.GraphicsPipelines.Valid(pso))
	{
		return &g_pipelines.GraphicsPipelines[pso];
	}

	return nullptr;
}

ID3D12PipelineState* Dx12_GetPipelineState(ComputePipelineState_t pso)
{
	if (g_pipelines.ComputePipelines.Valid(pso))
	{
		return g_pipelines.ComputePipelines[pso].Get();
	}

	return nullptr;
}

}
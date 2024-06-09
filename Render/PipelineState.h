#pragma once

#include "Shaders.h"

#include "RenderTypes.h"
#include "RootSignature.h"

RENDER_TYPE(GraphicsPipelineState_t);
RENDER_TYPE(ComputePipelineState_t);

enum class PrimitiveTopologyType : uint8_t
{
	Undefined,
	Point,
	Line,
	Triangle,
	Patch
};

enum class FillMode : uint8_t
{
	Solid,
	Wireframe
};

enum class CullMode : uint8_t
{
	None,
	Front,
	Back
};

enum class BlendType : uint8_t
{
	Zero = 1,
	One = 2,
	SrcColor = 3,
	InvSrcColor = 4,
	SrcAlpha = 5,
	InvSrcAlpha = 6,
	DstAlpha = 7,
	InvDstAlpha = 8,
	DstColor = 9,
	InvDstColor = 10,
	SrcAlphaSat = 11,
	Count
};

enum class BlendOp : uint8_t
{
	Add = 1,
	Subtract = 2,
	RevSubtract = 3,
	Min = 4,
	Max = 5,
	Count
};

struct BlendMode
{
	union
	{
		struct
		{
			bool BlendEnabled : 1;
			BlendType SrcBlend : 4;
			BlendType DstBlend : 4;
			BlendOp Op : 3;
			BlendType SrcBlendAlpha : 4;
			BlendType DstBlendAlpha : 4;
			BlendOp OpAlpha : 3;
		};
		uint32_t Opaque = 0;
	};

	BlendMode& Enabled(bool e) noexcept { BlendEnabled = e; return *this; }
	BlendMode& Blend(BlendType s, BlendOp o, BlendType d) noexcept { SrcBlend = s; Op = o; DstBlend = d; return *this; }
	BlendMode& BlendAlpha(BlendType s, BlendOp o, BlendType d) noexcept { SrcBlendAlpha = s; OpAlpha = o; DstBlendAlpha = d; return *this; }

	constexpr BlendMode()
		: BlendEnabled(false)
		, SrcBlend(BlendType::One)
		, DstBlend(BlendType::One)
		, Op(BlendOp::Add)
		, SrcBlendAlpha(BlendType::One)
		, DstBlendAlpha(BlendType::One)
		, OpAlpha(BlendOp::Add)
	{}

	constexpr BlendMode(bool enabled, BlendType srcBlend, BlendType dstBlend, BlendOp op, BlendType srcBlendAlpha, BlendType dstBlendAlpha, BlendOp opAlpha)
		: BlendEnabled(enabled)
		, SrcBlend(srcBlend)
		, DstBlend(dstBlend)
		, Op(op)
		, SrcBlendAlpha(srcBlendAlpha)
		, DstBlendAlpha(dstBlendAlpha)
		, OpAlpha(opAlpha)
	{}

	static constexpr BlendMode Default() { return BlendMode(true, BlendType::SrcAlpha, BlendType::InvSrcAlpha, BlendOp::Add, BlendType::One, BlendType::One, BlendOp::Add); }
	static constexpr BlendMode Add() { return BlendMode(true, BlendType::One, BlendType::One, BlendOp::Add, BlendType::One, BlendType::One, BlendOp::Add); }
	static constexpr BlendMode None() { return BlendMode(); }
};

enum class ComparisionFunc : uint8_t
{
	Never = 1,
	Less = 2,
	Equal = 3,
	LessEqual = 4,
	Greater = 5,
	NotEqual = 6,
	GreaterEqual = 7,
	Always = 8
};

//struct RootSignature_t
//{
//	int x;
//};

struct GraphicsPipelineStateDesc
{
	static constexpr uint32_t MaxRenderTargets = 8u;

	// Rasterizer desc
	PrimitiveTopologyType PrimTopo = PrimitiveTopologyType::Undefined;
	FillMode Fill = FillMode::Solid;
	CullMode Cull = CullMode::Back;
	int DepthBias = 0;
	float DepthBiasClamp = 0.0f;
	float SlopeScaleDepthBias = 0.0f;

	// Depth desc
	bool DepthEnabled = false;
	ComparisionFunc DepthCompare = ComparisionFunc::Never;
	RenderFormat DsvFormat = RenderFormat::UNKNOWN;

	// Target Desc
	RenderFormat TargetFormats[MaxRenderTargets];
	BlendMode Blends[MaxRenderTargets];
	uint8_t NumRenderTargets = 1;
	
	VertexShader_t Vs = VertexShader_t::INVALID;
	GeometryShader_t Gs = GeometryShader_t::INVALID;
	PixelShader_t Ps = PixelShader_t::INVALID;

	RootSignature_t RootSignatureOverride = RootSignature_t::INVALID;

	std::wstring DebugName;

	GraphicsPipelineStateDesc& RasterizerDesc(PrimitiveTopologyType pt, FillMode fm, CullMode cm, int bias = 0, float biasClamp = 0.0f, float slopeScaleBias = 0.0f)
	{ 
		PrimTopo = pt; 
		Fill = fm; 
		Cull = cm; 
		DepthBias = bias;
		DepthBiasClamp = biasClamp;
		SlopeScaleDepthBias = slopeScaleBias;
		return *this; 
	}

	GraphicsPipelineStateDesc& DepthDesc(bool enabled, ComparisionFunc cf = ComparisionFunc::Never, RenderFormat dsvFormat = RenderFormat::UNKNOWN) 
	{
		DepthEnabled = enabled; 
		DepthCompare = cf;
		DsvFormat = dsvFormat;
		return *this; 
	}

	GraphicsPipelineStateDesc& TargetBlendDesc(std::initializer_list<RenderFormat> targetDescs, std::initializer_list<BlendMode> blends)
	{
		NumRenderTargets = (uint8_t)(targetDescs.size() < blends.size() ? targetDescs.size() : blends.size());

		memcpy(TargetFormats, targetDescs.begin(), NumRenderTargets * sizeof(RenderFormat));
		memcpy(Blends, blends.begin(), NumRenderTargets * sizeof(BlendMode));

		return *this;
	}

	GraphicsPipelineStateDesc& VertexShader(VertexShader_t vs) { Vs = vs; return *this; }
	GraphicsPipelineStateDesc& GeomstryShader(GeometryShader_t gs) { Gs = gs; return *this; }
	GraphicsPipelineStateDesc& PixelShader(PixelShader_t ps) { Ps = ps; return *this; }
	GraphicsPipelineStateDesc& RootSignature(RootSignature_t rs) { RootSignatureOverride = rs; return *this; }
};

struct ComputePipelineStateDesc
{
	ComputeShader_t cs = ComputeShader_t::INVALID;
	RootSignature_t RootSignatureOverride = RootSignature_t::INVALID;

	std::wstring DebugName;
};

GraphicsPipelineState_t CreateGraphicsPipelineState(const GraphicsPipelineStateDesc& desc, const InputElementDesc* inputs = nullptr, size_t inputCount = 0);
ComputePipelineState_t CreateComputePipelineState(const ComputePipelineStateDesc& desc);

const GraphicsPipelineStateDesc* GetGraphicsPipelineStateDesc(GraphicsPipelineState_t pso);
const ComputePipelineStateDesc* GetComputePipelineStateDesc(ComputePipelineState_t pso);

void Render_Release(GraphicsPipelineState_t pso);
void Render_Release(ComputePipelineState_t pso);

size_t PipelineStates_GetGraphicsPipelineStateCount();
size_t PipelineStates_GetComputePipelineStateCount();
#include "Impl/RootSignatureImpl.h"

#include "RenderImpl.h"
#include "SparseArray.h"

namespace rl
{

struct RootSignatureSamplers
{
	std::vector<ComPtr<ID3D11SamplerState>> DxSamplers;
	std::vector<ID3D11SamplerState*> DxSamplersRaw;
};

SparseArray<RootSignatureSamplers, RootSignature_t> g_GlobalSamplers;

static D3D11_TEXTURE_ADDRESS_MODE Dx11_AdressMode(SamplerAddressMode am)
{
	switch (am)
	{
	case SamplerAddressMode::WRAP:
		return D3D11_TEXTURE_ADDRESS_WRAP;
	case SamplerAddressMode::MIRROR:
		return D3D11_TEXTURE_ADDRESS_MIRROR;
	case SamplerAddressMode::CLAMP:
		return D3D11_TEXTURE_ADDRESS_CLAMP;
	case SamplerAddressMode::BORDER:
		return D3D11_TEXTURE_ADDRESS_BORDER;
	case SamplerAddressMode::MIRROR_ONCE:
		return D3D11_TEXTURE_ADDRESS_MIRROR_ONCE;
	}

	assert(0 && "Dx11_AdressMode unsupported");
	return (D3D11_TEXTURE_ADDRESS_MODE)0;
}

static D3D11_COMPARISON_FUNC Dx11_ComparisonFunc(SamplerComparisonFunc cf)
{
	switch (cf)
	{
	case SamplerComparisonFunc::NONE:			return (D3D11_COMPARISON_FUNC)0;
	case SamplerComparisonFunc::NEVER:			return D3D11_COMPARISON_NEVER;
	case SamplerComparisonFunc::LESS:			return D3D11_COMPARISON_LESS;
	case SamplerComparisonFunc::EQUAL:			return D3D11_COMPARISON_EQUAL;
	case SamplerComparisonFunc::LESS_EQUAL:		return D3D11_COMPARISON_LESS_EQUAL;
	case SamplerComparisonFunc::GREATER:		return D3D11_COMPARISON_GREATER;
	case SamplerComparisonFunc::NOT_EQUAL:		return D3D11_COMPARISON_NOT_EQUAL;
	case SamplerComparisonFunc::GREATER_EQUAL:	return D3D11_COMPARISON_GREATER_EQUAL;
	case SamplerComparisonFunc::ALWAYS:			return D3D11_COMPARISON_ALWAYS;
	}

	assert(0 && "Dx11_ComparisonFunc unsupported");
	return (D3D11_COMPARISON_FUNC)0;
}

static D3D11_FILTER_TYPE Dx11_FilterType(SamplerFilterMode fm)
{
	switch (fm)
	{
	case SamplerFilterMode::POINT:
		return D3D11_FILTER_TYPE_POINT;
	case SamplerFilterMode::LINEAR:
		return D3D11_FILTER_TYPE_LINEAR;
	}

	assert(0 && "Dx11_FilterType was likely passed an aniso type, this should get handled before calling this");
	return (D3D11_FILTER_TYPE)0;
}

static D3D11_FILTER Dx11_Filter(bool comparison, SamplerFilterMode min, SamplerFilterMode mag, SamplerFilterMode mip)
{
	const D3D11_FILTER_REDUCTION_TYPE dxReduction = comparison ? D3D11_FILTER_REDUCTION_TYPE_COMPARISON : D3D11_FILTER_REDUCTION_TYPE_STANDARD;
	if (min == SamplerFilterMode::ANISOTROPIC || mag == SamplerFilterMode::ANISOTROPIC || mip == SamplerFilterMode::ANISOTROPIC)
	{
		assert(min == SamplerFilterMode::ANISOTROPIC && mag == SamplerFilterMode::ANISOTROPIC && mip == SamplerFilterMode::ANISOTROPIC && "Dx11_Filter if one filter is aniso, they all must be");
		return D3D11_ENCODE_ANISOTROPIC_FILTER(dxReduction);
	}

	const D3D11_FILTER_TYPE dxMin = Dx11_FilterType(min);
	const D3D11_FILTER_TYPE dxMag = Dx11_FilterType(min);
	const D3D11_FILTER_TYPE dxMip = Dx11_FilterType(min);

	return D3D11_ENCODE_BASIC_FILTER(dxMin, dxMag, dxMip, dxReduction);
}

bool RootSignature_CreateImpl(RootSignature_t rs, const RootSignatureDesc& desc)
{
	RootSignatureSamplers& samplers = g_GlobalSamplers.Alloc(rs);

	samplers.DxSamplers.resize(desc.GlobalSamplers.size());
	samplers.DxSamplersRaw.resize(desc.GlobalSamplers.size());
	
	for (uint32_t i = 0; i < desc.GlobalSamplers.size(); i++)
	{
		const SamplerDesc& samp = desc.GlobalSamplers[i];

		D3D11_SAMPLER_DESC sd = {};
		sd.Filter = Dx11_Filter(samp.Comparison != SamplerComparisonFunc::NONE, samp.FilterMode.Min, samp.FilterMode.Mag, samp.FilterMode.Mip);
		sd.AddressU = Dx11_AdressMode(samp.AddressMode.U);
		sd.AddressV = Dx11_AdressMode(samp.AddressMode.V);
		sd.AddressW = Dx11_AdressMode(samp.AddressMode.W);
		sd.MipLODBias = samp.MipLODBias;
		sd.MaxAnisotropy = samp.MaxAnisotropy;
		sd.ComparisonFunc = Dx11_ComparisonFunc(samp.Comparison);
		sd.MinLOD = samp.MinLOD;
		sd.MaxLOD = samp.MaxLOD;

		switch (samp.BorderColor)
		{
		case SamplerBorderColor::TRANSPARENT_BLACK:
			sd.BorderColor[0] = sd.BorderColor[1] = sd.BorderColor[2] = sd.BorderColor[3] = 0;
			break;
		case SamplerBorderColor::OPAQUE_BLACK:
			sd.BorderColor[0] = sd.BorderColor[1] = sd.BorderColor[2] = 0; sd.BorderColor[3] = 1;
			break;
		case SamplerBorderColor::OPAQUE_WHITE:
			sd.BorderColor[0] = sd.BorderColor[1] = sd.BorderColor[2] = sd.BorderColor[3] = 1;
			break;
		}

		DXENSURE(g_render.Device->CreateSamplerState(&sd, &samplers.DxSamplers[i]));

		samplers.DxSamplersRaw[i] = samplers.DxSamplers[i].Get();
	}

	return true;
}

void RootSignature_DestroyImpl(RootSignature_t rs)
{
	g_GlobalSamplers.Free(rs);
}

const std::vector<ID3D11SamplerState*>* Dx11_GetGlobalSamplers(RootSignature_t rs)
{
	if (g_GlobalSamplers.Valid(rs))
	{
		return &g_GlobalSamplers[rs].DxSamplersRaw;
	}

	return nullptr;
}

}
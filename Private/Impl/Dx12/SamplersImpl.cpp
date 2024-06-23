#include "Samplers.h"

#include "RenderImpl.h"

#undef min

namespace tpr
{

D3D12_FILTER_TYPE Dx12_FilterType(SamplerFilterMode fm)
{
	switch (fm)
	{
	case SamplerFilterMode::POINT:
		return D3D12_FILTER_TYPE_POINT;
	case SamplerFilterMode::LINEAR:
		return D3D12_FILTER_TYPE_LINEAR;
	}

	assert(0 && "Dx12_FilterType was likely passed an aniso type, this should get handled before calling this");
	return (D3D12_FILTER_TYPE)0;
}

D3D12_FILTER Dx12_Filter(bool comparison, SamplerFilterMode min, SamplerFilterMode mag, SamplerFilterMode mip)
{
	D3D12_FILTER_REDUCTION_TYPE dxReduction = comparison ? D3D12_FILTER_REDUCTION_TYPE_COMPARISON : D3D12_FILTER_REDUCTION_TYPE_STANDARD;
	if (min == SamplerFilterMode::ANISOTROPIC || mag == SamplerFilterMode::ANISOTROPIC || mip == SamplerFilterMode::ANISOTROPIC)
	{
		assert(min == SamplerFilterMode::ANISOTROPIC && mag == SamplerFilterMode::ANISOTROPIC && mip == SamplerFilterMode::ANISOTROPIC && "Dx12_Filter if one filter is aniso, they all must be");
		return D3D12_ENCODE_ANISOTROPIC_FILTER(dxReduction);
	}

	const D3D12_FILTER_TYPE dxMin = Dx12_FilterType(min);
	const D3D12_FILTER_TYPE dxMag = Dx12_FilterType(min);
	const D3D12_FILTER_TYPE dxMip = Dx12_FilterType(min);

	return D3D12_ENCODE_BASIC_FILTER(dxMin, dxMag, dxMip, dxReduction);
}

D3D12_TEXTURE_ADDRESS_MODE Dx12_TextureAddressMode(SamplerAddressMode sm)
{
	switch (sm)
	{
	case SamplerAddressMode::WRAP:
		return D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	case SamplerAddressMode::MIRROR:
		return D3D12_TEXTURE_ADDRESS_MODE_MIRROR;
	case SamplerAddressMode::CLAMP:
		return D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	case SamplerAddressMode::BORDER:
		return D3D12_TEXTURE_ADDRESS_MODE_BORDER;
	case SamplerAddressMode::MIRROR_ONCE:
		return D3D12_TEXTURE_ADDRESS_MODE_MIRROR_ONCE;
	}

	assert(0 && "Dx12_AdressMode unsupported");
	return (D3D12_TEXTURE_ADDRESS_MODE)0;
}
D3D12_COMPARISON_FUNC Dx12_ComparisonFunc(SamplerComparisonFunc cf)
{
	switch (cf)
	{
	case SamplerComparisonFunc::NONE:
		return (D3D12_COMPARISON_FUNC)0;
	case SamplerComparisonFunc::NEVER:
		return D3D12_COMPARISON_FUNC_NEVER;
	case SamplerComparisonFunc::LESS:
		return D3D12_COMPARISON_FUNC_LESS;
	case SamplerComparisonFunc::EQUAL:
		return D3D12_COMPARISON_FUNC_EQUAL;
	case SamplerComparisonFunc::LESS_EQUAL:
		return D3D12_COMPARISON_FUNC_LESS_EQUAL;
	case SamplerComparisonFunc::GREATER:
		return D3D12_COMPARISON_FUNC_GREATER;
	case SamplerComparisonFunc::NOT_EQUAL:
		return D3D12_COMPARISON_FUNC_NOT_EQUAL;
	case SamplerComparisonFunc::GREATER_EQUAL:
		return D3D12_COMPARISON_FUNC_GREATER_EQUAL;
	case SamplerComparisonFunc::ALWAYS:
		return D3D12_COMPARISON_FUNC_ALWAYS;
	}

	assert(0 && "Dx12_ComparisonFunc unsupported");
	return (D3D12_COMPARISON_FUNC)0;
}

D3D12_STATIC_BORDER_COLOR Dx12_BorderColor(SamplerBorderColor bc)
{
	switch (bc)
	{
	case SamplerBorderColor::TRANSPARENT_BLACK:
		return D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
	case SamplerBorderColor::OPAQUE_BLACK:
		return D3D12_STATIC_BORDER_COLOR_OPAQUE_BLACK;
	case SamplerBorderColor::OPAQUE_WHITE:
		return D3D12_STATIC_BORDER_COLOR_OPAQUE_WHITE;
	}

	assert(0 && "Dx12_BorderColor unsupported");
	return (D3D12_STATIC_BORDER_COLOR)0;
}

}
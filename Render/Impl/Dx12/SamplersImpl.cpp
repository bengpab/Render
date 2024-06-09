#include "../../Samplers.h"

#include "RenderImpl.h"

D3D12_FILTER_TYPE Dx12_FilterType(SamplerFilterMode fm)
{
	switch (fm)
	{
	case SamplerFilterMode::Point:
		return D3D12_FILTER_TYPE_POINT;
	case SamplerFilterMode::Linear:
		return D3D12_FILTER_TYPE_LINEAR;
	}

	assert(0 && "Dx12_FilterType was likely passed an aniso type, this should get handled before calling this");
	return (D3D12_FILTER_TYPE)0;
}

D3D12_FILTER Dx12_Filter(bool comparison, SamplerFilterMode min, SamplerFilterMode mag, SamplerFilterMode mip)
{
	D3D12_FILTER_REDUCTION_TYPE dxReduction = comparison ? D3D12_FILTER_REDUCTION_TYPE_COMPARISON : D3D12_FILTER_REDUCTION_TYPE_STANDARD;
	if (min == SamplerFilterMode::Anisotropic || mag == SamplerFilterMode::Anisotropic || mip == SamplerFilterMode::Anisotropic)
	{
		assert(min == SamplerFilterMode::Anisotropic && mag == SamplerFilterMode::Anisotropic && mip == SamplerFilterMode::Anisotropic && "Dx12_Filter if one filter is aniso, they all must be");
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
	case SamplerAddressMode::Wrap:
		return D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	case SamplerAddressMode::Mirror:
		return D3D12_TEXTURE_ADDRESS_MODE_MIRROR;
	case SamplerAddressMode::Clamp:
		return D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	case SamplerAddressMode::Border:
		return D3D12_TEXTURE_ADDRESS_MODE_BORDER;
	case SamplerAddressMode::MirrorOnce:
		return D3D12_TEXTURE_ADDRESS_MODE_MIRROR_ONCE;
	}

	assert(0 && "Dx12_AdressMode unsupported");
	return (D3D12_TEXTURE_ADDRESS_MODE)0;
}
D3D12_COMPARISON_FUNC Dx12_ComparisonFunc(SamplerComparisonFunc cf)
{
	switch (cf)
	{
	case SamplerComparisonFunc::None:
		return (D3D12_COMPARISON_FUNC)0;
	case SamplerComparisonFunc::Never:
		return D3D12_COMPARISON_FUNC_NEVER;
	case SamplerComparisonFunc::Less:
		return D3D12_COMPARISON_FUNC_LESS;
	case SamplerComparisonFunc::Equal:
		return D3D12_COMPARISON_FUNC_EQUAL;
	case SamplerComparisonFunc::LessEqual:
		return D3D12_COMPARISON_FUNC_LESS_EQUAL;
	case SamplerComparisonFunc::Greater:
		return D3D12_COMPARISON_FUNC_GREATER;
	case SamplerComparisonFunc::NotEqual:
		return D3D12_COMPARISON_FUNC_NOT_EQUAL;
	case SamplerComparisonFunc::GreaterEqual:
		return D3D12_COMPARISON_FUNC_GREATER_EQUAL;
	case SamplerComparisonFunc::Always:
		return D3D12_COMPARISON_FUNC_ALWAYS;
	}

	assert(0 && "Dx12_ComparisonFunc unsupported");
	return (D3D12_COMPARISON_FUNC)0;
}

D3D12_STATIC_BORDER_COLOR Dx12_BorderColor(SamplerBorderColor bc)
{
	switch (bc)
	{
	case SamplerBorderColor::TransparentBlack:
		return D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
	case SamplerBorderColor::OpaqueBlack:
		return D3D12_STATIC_BORDER_COLOR_OPAQUE_BLACK;
	case SamplerBorderColor::OpaqueWhite:
		return D3D12_STATIC_BORDER_COLOR_OPAQUE_WHITE;
	}

	assert(0 && "Dx12_BorderColor unsupported");
	return (D3D12_STATIC_BORDER_COLOR)0;
}

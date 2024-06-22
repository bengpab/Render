#pragma once

#include "RenderTypes.h"

namespace tpr
{

enum class SamplerAddressMode : uint8_t
{
	WRAP,
	MIRROR,
	CLAMP,
	BORDER,
	MIRROR_ONCE
};

enum class SamplerFilterMode : uint8_t
{
	POINT,
	LINEAR,
	ANISOTROPIC,
};

enum class SamplerComparisonFunc : uint8_t
{
	NONE,
	NEVER,
	LESS,
	EQUAL,
	LESS_EQUAL,
	GREATER,
	NOT_EQUAL,
	GREATER_EQUAL,
	ALWAYS,
};

enum class SamplerBorderColor : uint8_t
{
	TRANSPARENT_BLACK,
	OPAQUE_BLACK,
	OPAQUE_WHITE,
};

struct SamplerDesc
{
	union
	{
		struct
		{
			SamplerAddressMode U = SamplerAddressMode::WRAP;
			SamplerAddressMode V = SamplerAddressMode::WRAP;
			SamplerAddressMode W = SamplerAddressMode::WRAP;
		} AddressMode;
		uint32_t Opaque;
	};

	union
	{
		struct
		{
			SamplerFilterMode Min = SamplerFilterMode::LINEAR;
			SamplerFilterMode Mag = SamplerFilterMode::LINEAR;
			SamplerFilterMode Mip = SamplerFilterMode::LINEAR;
		} FilterMode;
		uint32_t Opaque;
	};

	SamplerComparisonFunc Comparison = SamplerComparisonFunc::NONE;
	float MinLOD = 0.0f;
	float MaxLOD = 0.0f;
	float MipLODBias = 0.0f;
	SamplerBorderColor BorderColor = SamplerBorderColor::TRANSPARENT_BLACK;
	uint32_t MaxAnisotropy = 0;

	inline SamplerDesc& AddressModeUVW(SamplerAddressMode am)
	{
		AddressMode.U = AddressMode.V = AddressMode.W = am; return *this;		
	}

	inline SamplerDesc& FilterModeMinMagMip(SamplerFilterMode fm)
	{
		FilterMode.Min = FilterMode.Mag = FilterMode.Mip = fm; return *this;
	}

	inline SamplerDesc& ComparisonFunc(SamplerComparisonFunc cf)
	{
		Comparison = cf; return *this;
	}

	inline SamplerDesc& BorderCol(SamplerBorderColor col)
	{
		BorderColor = col; return *this;
	}
};

}
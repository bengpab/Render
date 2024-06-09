#pragma once

#include "Render/Render.h"
#include "../../RenderTypes.h"

#include "Render/Impl/Dx/DxErrorHandling.h"

#include <d3d12.h>

DXGI_FORMAT Dx12_Format(RenderFormat format);
void Dx12_CalculatePitch(DXGI_FORMAT format, UINT width, UINT height, UINT* rowPitch, UINT* slicePitch);

D3D12_DESCRIPTOR_RANGE_TYPE Dx12_DescriptorRangeType(RootSignatureDescriptorTableType type);

D3D12_FILTER Dx12_Filter(bool comparison, SamplerFilterMode min, SamplerFilterMode mag, SamplerFilterMode mip);
D3D12_TEXTURE_ADDRESS_MODE Dx12_TextureAddressMode(SamplerAddressMode sm);
D3D12_COMPARISON_FUNC Dx12_ComparisonFunc(SamplerComparisonFunc cf);
D3D12_STATIC_BORDER_COLOR Dx12_BorderColor(SamplerBorderColor bc);

D3D12_PRIMITIVE_TOPOLOGY Dx12_PrimitiveTopology(PrimitiveTopologyType ptt);
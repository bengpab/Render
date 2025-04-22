#include "Impl/RootSignatureImpl.h"

#include "RenderImpl.h"
#include "SparseArray.h"

namespace rl
{

SparseArray<ComPtr<ID3D12RootSignature>, RootSignature_t> g_DxRootSignatures;

bool RootSignature_CreateImpl(RootSignature_t rs, const RootSignatureDesc& desc)
{
	D3D12_FEATURE_DATA_ROOT_SIGNATURE featureData = {};
	featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;
	if (FAILED(g_render.DxDevice->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &featureData, sizeof(featureData))))
	{
		OutputDebugStringA("Only root sig version 1.1 supported");
		return false;
	}

	D3D12_ROOT_SIGNATURE_FLAGS flags =
		D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_AMPLIFICATION_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_MESH_SHADER_ROOT_ACCESS;

	if (((uint32_t)desc.Flags & (uint32_t)RootSignatureFlags::ALLOW_INPUT_LAYOUT) != 0)
		flags |= D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

	std::vector<D3D12_ROOT_PARAMETER1> rootParams;
	rootParams.resize(desc.Slots.size());

	std::vector<std::vector<D3D12_DESCRIPTOR_RANGE1>> rangeTable;
	rangeTable.resize(desc.Slots.size());

	for (uint32_t slotIt = 0; slotIt < desc.Slots.size(); slotIt++)
	{
		const RootSignatureSlot& slotDesc = desc.Slots[slotIt];

		D3D12_ROOT_PARAMETER1& rootParam = rootParams[slotIt];

		switch (slotDesc.Type)
		{
		case RootSignatureSlotType::CONSTANTS:
		{
			rootParam.ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
			rootParam.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
			rootParam.Constants.Num32BitValues = slotDesc.Num32BitVals;
			rootParam.Constants.ShaderRegister = slotDesc.BaseRegister;
			rootParam.Constants.RegisterSpace = slotDesc.BaseRegisterSpace;
		}
		break;
		case RootSignatureSlotType::CBV:
		{
			rootParam.ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
			rootParam.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
			rootParam.Descriptor.ShaderRegister = slotDesc.BaseRegister;
			rootParam.Descriptor.RegisterSpace = slotDesc.BaseRegisterSpace;
			rootParam.Descriptor.Flags = D3D12_ROOT_DESCRIPTOR_FLAG_NONE;
		}
		break;
		case RootSignatureSlotType::SRV:
		{
			rootParam.ParameterType = D3D12_ROOT_PARAMETER_TYPE_SRV;
			rootParam.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
			rootParam.Descriptor.ShaderRegister = slotDesc.BaseRegister;
			rootParam.Descriptor.RegisterSpace = slotDesc.BaseRegisterSpace;
			rootParam.Descriptor.Flags = D3D12_ROOT_DESCRIPTOR_FLAG_NONE;
		}
		break;
		case RootSignatureSlotType::UAV:
		{
			rootParam.ParameterType = D3D12_ROOT_PARAMETER_TYPE_UAV;
			rootParam.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
			rootParam.Descriptor.ShaderRegister = slotDesc.BaseRegister;
			rootParam.Descriptor.RegisterSpace = slotDesc.BaseRegisterSpace;
			rootParam.Descriptor.Flags = D3D12_ROOT_DESCRIPTOR_FLAG_NONE;
		}
		break;
		case RootSignatureSlotType::DESCRIPTOR_TABLE:
		{
			std::vector<D3D12_DESCRIPTOR_RANGE1>& ranges = rangeTable[slotIt];
			ranges.resize(slotDesc.RangeCount);
			for (uint32_t rangeIt = 0; rangeIt < slotDesc.RangeCount; rangeIt++)
			{
				D3D12_DESCRIPTOR_RANGE1& range = ranges[rangeIt];
				range.RangeType = Dx12_DescriptorRangeType(slotDesc.DescriptorTableType);
				range.NumDescriptors = UINT_MAX;
				range.BaseShaderRegister = slotDesc.BaseRegister;
				range.RegisterSpace = slotDesc.BaseRegisterSpace + rangeIt;
				range.Flags = D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE;
				range.OffsetInDescriptorsFromTableStart = 0u;
			}

			rootParam.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
			rootParam.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
			rootParam.DescriptorTable.NumDescriptorRanges = slotDesc.RangeCount;
			rootParam.DescriptorTable.pDescriptorRanges = rangeTable[slotIt].data();
		}
		break;
		default:
			DXASSERT(-1); // Invalid slot type
		}
	}

	std::vector<D3D12_STATIC_SAMPLER_DESC> dxStaticSamplers;
	dxStaticSamplers.resize(desc.GlobalSamplers.size());

	for (uint32_t sampIt = 0; sampIt < desc.GlobalSamplers.size(); sampIt++)
	{
		const SamplerDesc& sd = desc.GlobalSamplers[sampIt];
		D3D12_STATIC_SAMPLER_DESC& desc = dxStaticSamplers[sampIt];
		desc.Filter = Dx12_Filter(sd.Comparison != SamplerComparisonFunc::NONE, sd.FilterMode.Min, sd.FilterMode.Mag, sd.FilterMode.Mip);
		desc.AddressU = Dx12_TextureAddressMode(sd.AddressMode.U);
		desc.AddressV = Dx12_TextureAddressMode(sd.AddressMode.V);
		desc.AddressW = Dx12_TextureAddressMode(sd.AddressMode.W);
		desc.MipLODBias = sd.MipLODBias;
		desc.MaxAnisotropy = sd.MaxAnisotropy;
		desc.ComparisonFunc = Dx12_ComparisonFunc(sd.Comparison);
		desc.BorderColor = Dx12_BorderColor(sd.BorderColor);
		desc.MinLOD = sd.MinLOD;
		desc.MaxLOD = sd.MaxLOD;
		desc.ShaderRegister = sampIt;
		desc.RegisterSpace = 0u;
		desc.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
	}

	D3D12_VERSIONED_ROOT_SIGNATURE_DESC dxRootSigDesc = {};
	dxRootSigDesc.Version = featureData.HighestVersion;

	dxRootSigDesc.Desc_1_1.NumParameters = (UINT)rootParams.size();
	dxRootSigDesc.Desc_1_1.pParameters = rootParams.data();
	dxRootSigDesc.Desc_1_1.NumStaticSamplers = (UINT)dxStaticSamplers.size();
	dxRootSigDesc.Desc_1_1.pStaticSamplers = dxStaticSamplers.data();

	dxRootSigDesc.Desc_1_1.Flags = D3D12_ROOT_SIGNATURE_FLAG_NONE;

	if ((desc.Flags & RootSignatureFlags::ALLOW_INPUT_LAYOUT) != RootSignatureFlags::NONE)
	{
		dxRootSigDesc.Desc_1_1.Flags |= D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
	}

	ComPtr<ID3DBlob> dxSerializedRootSig;
	ComPtr<ID3DBlob> errorBlob;
	if (!DXENSURE(D3D12SerializeVersionedRootSignature(&dxRootSigDesc, &dxSerializedRootSig, &errorBlob)))
	{
		if (errorBlob)
		{
			OutputDebugStringA((LPCSTR)errorBlob->GetBufferPointer());
		}
		return false;
	}

	ComPtr<ID3D12RootSignature>& dxRootSig = g_DxRootSignatures.Alloc(rs);

	if (!DXENSURE(g_render.DxDevice->CreateRootSignature(0u, dxSerializedRootSig->GetBufferPointer(), dxSerializedRootSig->GetBufferSize(), IID_PPV_ARGS(&dxRootSig))))
	{
		return false;
	}

	return true;
}

ID3D12RootSignature* Dx12_GetRootSignature(RootSignature_t rs)
{
	if (g_DxRootSignatures.Valid(rs))
	{
		return g_DxRootSignatures[rs].Get();
	}

	return nullptr;
}

void RootSignature_DestroyImpl(RootSignature_t rs)
{
	g_DxRootSignatures.Free(rs);
}

}
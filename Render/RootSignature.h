#pragma once

#include "RenderTypes.h"
#include "Samplers.h"

enum class RootSignatureFlags : uint32_t
{
	None = 0u,
	AllowInputLayout = (1u << 0u),
};
IMPLEMENT_FLAGS(RootSignatureFlags, uint32_t);

enum class RootSignatureSlotType : uint32_t
{
	None = 0u,
	Constants,
	CBV,
	SRV,
	UAV,
	DescriptorTable,
};

enum class RootSignatureDescriptorTableType : uint32_t
{
	None,
	SRV,
	UAV,
};

struct RootSignatureSlot
{
	RootSignatureSlotType Type = RootSignatureSlotType::None;

	uint32_t BaseRegister = 0;
	uint32_t BaseRegisterSpace = 0;

	// Descriptor Table Settings
	RootSignatureDescriptorTableType DescriptorTableType = RootSignatureDescriptorTableType::None;
	uint32_t RangeCount;

	// Constants Settings
	uint32_t Num32BitVals;

	static constexpr RootSignatureSlot MakeSlot(RootSignatureSlotType type, uint32_t reg)
	{
		RootSignatureSlot slot = {};
		slot.Type = type;
		slot.BaseRegister = reg;
		return slot;
	}

	static constexpr RootSignatureSlot ConstantsSlot(uint32_t num32BitVals, uint32_t reg)
	{
		RootSignatureSlot slot = MakeSlot(RootSignatureSlotType::Constants, reg);
		slot.Num32BitVals = num32BitVals;
		return slot;
	}

	static constexpr RootSignatureSlot CBVSlot(uint32_t reg, uint32_t space)
	{
		RootSignatureSlot slot = MakeSlot(RootSignatureSlotType::CBV, reg);
		slot.BaseRegisterSpace = space;
		return slot;
	}

	static constexpr RootSignatureSlot SRVSlot(uint32_t reg, uint32_t space)
	{
		RootSignatureSlot slot = MakeSlot(RootSignatureSlotType::SRV, reg);
		slot.BaseRegisterSpace = space;
		return slot;
	}

	static constexpr RootSignatureSlot DescriptorTableSlot(uint32_t baseReg, uint32_t baseSpace, RootSignatureDescriptorTableType tableType, uint32_t rangeCount = (uint32_t)0xffff)
	{
		RootSignatureSlot slot = MakeSlot(RootSignatureSlotType::DescriptorTable, baseReg);
		slot.DescriptorTableType = tableType;
		slot.RangeCount = rangeCount;
		slot.BaseRegisterSpace = baseSpace;
		return slot;
	}
};

struct RootSignatureDesc
{
	RootSignatureFlags Flags = RootSignatureFlags::None;
	std::vector<RootSignatureSlot> Slots;
	std::vector<SamplerDesc> GlobalSamplers;
};

RENDER_TYPE(RootSignature_t);

RootSignature_t CreateRootSignature(const RootSignatureDesc& desc);

void Render_Ref(RootSignature_t rs);
void Render_Release(RootSignature_t rs);

const RootSignatureDesc* RootSignature_GetDesc(RootSignature_t rs);
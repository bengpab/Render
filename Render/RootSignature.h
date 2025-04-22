#pragma once

#include "RenderTypes.h"
#include "Samplers.h"

namespace rl
{

enum class RootSignatureFlags : uint32_t
{
	NONE = 0u,
	ALLOW_INPUT_LAYOUT = (1u << 0u),
};
IMPLEMENT_FLAGS(RootSignatureFlags, uint32_t);

enum class RootSignatureSlotType : uint32_t
{
	NONE,
	CONSTANTS,
	CBV,
	SRV,
	UAV,
	DESCRIPTOR_TABLE,
};

enum class RootSignatureDescriptorTableType : uint32_t
{
	NONE,
	SRV,
	UAV,
};

struct RootSignatureSlot
{
	RootSignatureSlotType Type = RootSignatureSlotType::NONE;

	uint32_t BaseRegister = 0;
	uint32_t BaseRegisterSpace = 0;

	// Descriptor Table Settings
	RootSignatureDescriptorTableType DescriptorTableType = RootSignatureDescriptorTableType::NONE;
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
		RootSignatureSlot slot = MakeSlot(RootSignatureSlotType::CONSTANTS, reg);
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
		RootSignatureSlot slot = MakeSlot(RootSignatureSlotType::DESCRIPTOR_TABLE, baseReg);
		slot.DescriptorTableType = tableType;
		slot.RangeCount = rangeCount;
		slot.BaseRegisterSpace = baseSpace;
		return slot;
	}
};

struct RootSignatureDesc
{
	RootSignatureFlags Flags = RootSignatureFlags::NONE;
	std::vector<RootSignatureSlot> Slots;
	std::vector<SamplerDesc> GlobalSamplers;
};

RENDER_TYPE(RootSignature_t);

RootSignature_t CreateRootSignature(const RootSignatureDesc& desc);

void RenderRef(RootSignature_t rs);
void RenderRelease(RootSignature_t rs);

}
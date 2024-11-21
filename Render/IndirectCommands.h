#pragma once

#include "RenderTypes.h"

namespace tpr
{
struct IndirectDrawLayout
{
	uint32_t numVerts;
	uint32_t numInstances;
	uint32_t startVertex;
	uint32_t startInstance;
};

struct IndirectDrawIndexedLayout
{
	uint32_t numIndices;
	uint32_t numInstances;
	uint32_t startIndex;
	uint32_t startVertex;
	uint32_t startInstance;
};

struct IndirectDispatchLayout
{
	uint32_t x;
	uint32_t y;
	uint32_t z;
};

enum class IndirectCommandType : uint32_t
{
	INDIRECT_DRAW,
	INDIRECT_DRAW_INDEXED,
	INDIRECT_DISPATCH
};

RENDER_TYPE(IndirectCommand_t);

IndirectCommand_t CreateIndirectCommand(IndirectCommandType type, RootSignature_t rootSig = {});

inline IndirectCommand_t CreateIndirectDrawCommand() { return CreateIndirectCommand(IndirectCommandType::INDIRECT_DRAW, {}); }
inline IndirectCommand_t CreateIndirectDrawIndexedCommand() { return CreateIndirectCommand(IndirectCommandType::INDIRECT_DRAW_INDEXED, {}); }
inline IndirectCommand_t CreateIndirectDispatchCommand() { return CreateIndirectCommand(IndirectCommandType::INDIRECT_DISPATCH, {}); }

void RenderRef(IndirectCommand_t ic);
void RenderRelease(IndirectCommand_t ic);

size_t IndirectCommandLayoutSize(IndirectCommandType type);
IndirectCommandType GetIndirectCommandType(IndirectCommand_t ic);
}
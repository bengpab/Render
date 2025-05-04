#pragma once

#include "Raytracing.h"

namespace rl
{
bool CreateRaytracingGeometryImpl(RaytracingGeometry_t RtGeometry, VertexBuffer_t VertexBuffer, RenderFormat VertexFormat, uint32_t VertexCount, uint32_t VertexStride, IndexBuffer_t IndexBuffer, RenderFormat IndexFormat, uint32_t IndexCount);
bool CreateRaytracingSceneImpl(RaytracingScene_t RtScene);

bool CreateRaytracingPipelineStateImpl(RaytracingPipelineState_t RtPSO, const RaytracingPipelineStateDesc& Desc);

void DestroyRaytracingGeometryImpl(RaytracingGeometry_t RtGeometry);
void DestroyRaytracingSceneImpl(RaytracingScene_t RtScene);
}
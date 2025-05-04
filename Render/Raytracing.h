#pragma once

#include "RenderTypes.h"
#include "Shaders.h"

namespace rl
{
// Geometry
RENDER_TYPE(RaytracingGeometry_t);
RENDER_TYPE(RaytracingScene_t);

// RTPSO
RENDER_TYPE(RaytracingPipelineState_t);

// Shaders


struct RaytracingPipelineStateDesc
{
	RaytracingRayGenShader_t RayGenShader = {};
	RaytracingMissShader_t MissShader = {};
	RaytracingAnyHitShader_t AnyHitShader = {};
	RaytracingClosestHitShader_t ClosestHitShader = {};
	uint32_t MaxRayRecursion = 2;

	std::wstring DebugName;
};

RaytracingGeometry_t CreateRaytracingGeometry(VertexBuffer_t VertexBuffer, RenderFormat VertexFormat, uint32_t VertexCount, uint32_t VertexStride);
RaytracingGeometry_t CreateRaytracingGeometry(VertexBuffer_t VertexBuffer, RenderFormat VertexFormat, uint32_t VertexCount, uint32_t VertexStride, IndexBuffer_t IndexBuffer, RenderFormat IndexFormat, uint32_t IndexCount);

RaytracingScene_t CreateRaytracingScene();

RaytracingPipelineState_t CreateRaytracingPipelineState(const RaytracingPipelineStateDesc& Desc);

void AddRaytracingGeometryToScene(RaytracingGeometry_t Geometry, RaytracingScene_t Scene);
void RemoveRaytracingGeometryFromScene(RaytracingGeometry_t Geometry, RaytracingScene_t Scene);

void RenderRef(RaytracingGeometry_t geometry);
void RenderRef(RaytracingScene_t scene);

void RenderRelease(RaytracingGeometry_t geometry);
void RenderRelease(RaytracingScene_t scene);


}
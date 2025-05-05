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

struct RaytracingPipelineStateDesc
{
	RaytracingRayGenShader_t RayGenShader = {};
	RaytracingMissShader_t MissShader = {};
	RaytracingAnyHitShader_t AnyHitShader = {};
	RaytracingClosestHitShader_t ClosestHitShader = {};
	uint32_t MaxRayRecursion = 2;

	std::wstring DebugName;
};

struct RaytracingGeometryDesc
{
	// Must supply exactly one of either vertex or structured buffer
	VertexBuffer_t VertexBuffer = {};
	StructuredBuffer_t StructuredVertexBuffer = {};

	// Required
	RenderFormat VertexFormat = RenderFormat::UNKNOWN;
	uint32_t VertexCount = 0;
	uint32_t VertexStride = 0;

	// Optional, supply either index or structured buffer
	IndexBuffer_t IndexBuffer = {};
	StructuredBuffer_t StructuredIndexBuffer = {};
	RenderFormat IndexFormat = RenderFormat::UNKNOWN;
	uint32_t IndexCount = 0;
	uint32_t IndexOffset = 0;
};

RaytracingGeometry_t CreateRaytracingGeometry(const RaytracingGeometryDesc& Desc);

RaytracingScene_t CreateRaytracingScene();

RaytracingPipelineState_t CreateRaytracingPipelineState(const RaytracingPipelineStateDesc& Desc);

void AddRaytracingGeometryToScene(RaytracingGeometry_t Geometry, RaytracingScene_t Scene);
void RemoveRaytracingGeometryFromScene(RaytracingGeometry_t Geometry, RaytracingScene_t Scene);

void RenderRef(RaytracingGeometry_t geometry);
void RenderRef(RaytracingScene_t scene);
void RenderRef(RaytracingPipelineState_t RTPipelineState);

void RenderRelease(RaytracingGeometry_t geometry);
void RenderRelease(RaytracingScene_t scene);
void RenderRelease(RaytracingPipelineState_t RTPipelineState);

}
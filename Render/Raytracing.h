#pragma once

#include "RenderTypes.h"

namespace rl
{
RENDER_TYPE(RaytracingGeometry_t);
RENDER_TYPE(RaytracingScene_t);

RaytracingGeometry_t CreateRaytracingGeometry(VertexBuffer_t VertexBuffer, RenderFormat VertexFormat, uint32_t VertexCount, uint32_t VertexStride);
RaytracingGeometry_t CreateRaytracingGeometry(VertexBuffer_t VertexBuffer, RenderFormat VertexFormat, uint32_t VertexCount, uint32_t VertexStride, IndexBuffer_t IndexBuffer, RenderFormat IndexFormat, uint32_t IndexCount);

RaytracingScene_t CreateRaytracingScene();

void RenderRef(RaytracingGeometry_t geometry);
void RenderRef(RaytracingScene_t scene);

void RenderRelease(RaytracingGeometry_t geometry);
void RenderRelease(RaytracingScene_t scene);
}
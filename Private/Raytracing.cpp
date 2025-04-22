#include "Raytracing.h"

#include "Buffers.h"
#include "IDArray.h"
#include "Impl/RaytracingImpl.h"

namespace rl
{

struct RaytracingGeometryData
{
    RenderFormat VertexFormat = RenderFormat::UNKNOWN;
    uint32_t VertexCount = 0u;
    uint32_t VertexStride = 0u;
    RenderFormat IndexFormat = RenderFormat::UNKNOWN;
    uint32_t IndexCount = 0u;
};

struct RaytracingSceneData
{
    std::vector<RaytracingGeometryPtr> Geometry;

    bool NeedsUpdate = false;
};

IDArray<RaytracingGeometry_t, RaytracingGeometryData> g_RaytracingGeometry;
IDArray<RaytracingScene_t, RaytracingSceneData> g_RaytracingScenes;

RaytracingGeometry_t CreateRaytracingGeometry(VertexBuffer_t VertexBuffer, RenderFormat VertexFormat, uint32_t VertexCount, uint32_t VertexStride)
{
    return CreateRaytracingGeometry(VertexBuffer, VertexFormat, VertexCount, VertexStride, IndexBuffer_t::INVALID, RenderFormat::UNKNOWN, 0);
}

RaytracingGeometry_t CreateRaytracingGeometry(VertexBuffer_t VertexBuffer, RenderFormat VertexFormat, uint32_t VertexCount, uint32_t VertexStride, IndexBuffer_t IndexBuffer, RenderFormat IndexFormat, uint32_t IndexCount)
{
    RaytracingGeometryData Data = {};
    Data.VertexFormat = VertexFormat;
    Data.VertexCount = VertexCount;
    Data.IndexFormat = IndexFormat;
    Data.IndexCount = IndexCount;

    RaytracingGeometry_t Handle = g_RaytracingGeometry.Create(Data);

    if (!CreateRaytracingGeometryImpl(Handle, VertexBuffer, VertexFormat, VertexCount, VertexStride, IndexBuffer, IndexFormat, IndexCount))
    {
        g_RaytracingGeometry.Release(Handle);
        return RaytracingGeometry_t::INVALID;
    }

    return Handle;
}

RaytracingScene_t CreateRaytracingScene()
{
    RaytracingScene_t Handle = g_RaytracingScenes.Create();

    if (!CreateRaytracingSceneImpl(Handle))
    {
        g_RaytracingScenes.Release(Handle);
        return RaytracingScene_t::INVALID;
    }

    return Handle;
}

void RenderRef(RaytracingGeometry_t geometry)
{
    g_RaytracingGeometry.AddRef(geometry);
}

void RenderRef(RaytracingScene_t scene)
{
    g_RaytracingScenes.AddRef(scene);
}

void RenderRelease(RaytracingGeometry_t geometry)
{
    if (g_RaytracingGeometry.Release(geometry))
    {
        DestroyRaytracingGeometryImpl(geometry);
    }
}

void RenderRelease(RaytracingScene_t scene)
{
    if (g_RaytracingScenes.Release(scene))
    {
        DestroyRaytracingSceneImpl(scene);
    }
}

}

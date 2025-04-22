#include "Impl/RaytracingImpl.h"

#include "RenderImpl.h"
#include "SparseArray.h"

namespace rl
{

SparseArray<ComPtr<ID3D12Resource>, RaytracingGeometry_t> g_DxRaytracingGeometry;

struct BLAS
{
    // We must store strong pointers to these so we can rebuild, the callee is responsible for ensuring they maintain the ray trace and raster scenes
    VertexBufferPtr VertexBuffer = {};
    IndexBufferPtr IndexBuffer = {};

    D3D12_RAYTRACING_GEOMETRY_DESC DxDesc = {};
};
    
struct AccelerationStructure
{
    std::vector<BLAS> GeometryDescs;

    bool Dirty = false;
};

AccelerationStructure g_AccelerationStructure;

bool CreateRaytracingGeometryImpl(RaytracingGeometry_t RtGeometry, VertexBuffer_t VertexBuffer, RenderFormat VertexFormat, uint32_t VertexCount, uint32_t VertexStride, IndexBuffer_t IndexBuffer, RenderFormat IndexFormat, uint32_t IndexCount)
{
    return true;

    //D3D12_GPU_VIRTUAL_ADDRESS VertexBufferGpuAddress = Dx12_GetVbAddress(VertexBuffer);
    //if (VertexBufferGpuAddress == 0)
    //{
    //    return false;
    //}

    //BLAS Desc = {};
    //Desc.VertexBuffer = VertexBuffer;
    //Desc.IndexBuffer = IndexBuffer;
    //Desc.DxDesc.Type = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
    //Desc.DxDesc.Triangles.VertexBuffer.StartAddress = VertexBufferGpuAddress;
    //Desc.DxDesc.Triangles.VertexCount = VertexCount;
    //Desc.DxDesc.Triangles.VertexFormat = Dx12_Format(VertexFormat);
    //Desc.DxDesc.Triangles.IndexBuffer = Dx12_GetIbAddress(IndexBuffer);
    //Desc.DxDesc.Triangles.IndexFormat = Dx12_Format(IndexFormat);
    //Desc.DxDesc.Triangles.IndexCount = IndexCount;
    //Desc.DxDesc.Triangles.Transform3x4 = 0;
    //Desc.DxDesc.Flags = D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE; // TODO: support transparent objects

    //g_AccelerationStructure.GeometryDescs.push_back(Desc);
    //g_AccelerationStructure.Dirty = true;

    //return true;
}

bool CreateRaytracingSceneImpl(RaytracingScene_t RtScene)
{
    return true;
}

void DestroyRaytracingGeometryImpl(RaytracingGeometry_t RtGeometry)
{
}

void DestroyRaytracingSceneImpl(RaytracingScene_t RtScene)
{
}

}

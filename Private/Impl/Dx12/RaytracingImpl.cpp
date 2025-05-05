#include "Impl/RaytracingImpl.h"

#include "Impl/Dx/d3dx12.h"
#include "RenderImpl.h"
#include "SparseArray.h"

#include <dxcapi.h>

// TODO https://developer.nvidia.com/blog/managing-memory-for-acceleration-structures-in-dxr/
// Shared buffer allocation strategy for BLAS

namespace rl
{

struct AccelerationStructure
{    
    D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO DxPrebuildInfo = {};

    ComPtr<ID3D12Resource> DxBuffer;
};

struct BLAS : public AccelerationStructure
{
    // We must store strong pointers to these so we can rebuild, the callee is responsible for ensuring they maintain the ray trace and raster scenes
    VertexBufferPtr VertexBuffer = {};
    IndexBufferPtr IndexBuffer = {};

    D3D12_RAYTRACING_GEOMETRY_DESC DxDesc = {};

    bool Dirty = true;

};

struct TLAS : public AccelerationStructure
{
    uint32_t GeometryCount;

    std::vector<RaytracingGeometry_t> Meshes;
};

struct Dx12ShaderTable
{
    Dx12StaticBufferAllocation GpuShaderTable;
};

SparseArray<BLAS, RaytracingGeometry_t> g_BLAS;
SparseArray<TLAS, RaytracingScene_t> g_TLAS;
SparseArray<Dx12ShaderTable, RaytracingShaderTable_t> g_ShaderTables;
SparseArray<ComPtr<ID3D12StateObject>, RaytracingPipelineState_t> g_RTPSOs;

bool CreateRaytracingGeometryImpl(RaytracingGeometry_t Handle, const RaytracingGeometryDesc& Desc)
{
    D3D12_GPU_VIRTUAL_ADDRESS VertexBufferGpuAddress = Dx12_GetVbAddress(Desc.VertexBuffer);
    if (VertexBufferGpuAddress == 0)
    {
        VertexBufferGpuAddress = Dx12_GetSbAddress(Desc.StructuredVertexBuffer);
        if (VertexBufferGpuAddress == 0)
        {
            return false;
        }
    }

    BLAS& GeomDesc = g_BLAS.Alloc(Handle);

    D3D12_GPU_VIRTUAL_ADDRESS IndexBufferGpuAddress = Dx12_GetIbAddress(Desc.IndexBuffer);
    if (IndexBufferGpuAddress == 0)
    {
        IndexBufferGpuAddress = Dx12_GetSbAddress(Desc.StructuredIndexBuffer);
    }

    if (Desc.IndexOffset > 0)
    {
        IndexBufferGpuAddress += Desc.IndexOffset * (Desc.IndexFormat == RenderFormat::R32_UINT ? 4 : 2);
    }

    GeomDesc.VertexBuffer = Desc.VertexBuffer;
    GeomDesc.IndexBuffer = Desc.IndexBuffer;
    GeomDesc.DxDesc.Type = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
    GeomDesc.DxDesc.Triangles.VertexBuffer.StartAddress = VertexBufferGpuAddress;
    GeomDesc.DxDesc.Triangles.VertexBuffer.StrideInBytes = Desc.VertexStride;
    GeomDesc.DxDesc.Triangles.VertexCount = Desc.VertexCount;
    GeomDesc.DxDesc.Triangles.VertexFormat = Dx12_Format(Desc.VertexFormat);
    GeomDesc.DxDesc.Triangles.IndexBuffer = IndexBufferGpuAddress;
    GeomDesc.DxDesc.Triangles.IndexFormat = Dx12_Format(Desc.IndexFormat);
    GeomDesc.DxDesc.Triangles.IndexCount = Desc.IndexCount;
    GeomDesc.DxDesc.Triangles.Transform3x4 = 0;
    GeomDesc.DxDesc.Flags = D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE; // TODO: support transparent objects

    return true;
}

bool CreateRaytracingSceneImpl(RaytracingScene_t RtScene)
{
    TLAS& Desc = g_TLAS.Alloc(RtScene);

    return true;
}

bool CreateRaytracingPipelineStateImpl(RaytracingPipelineState_t RtPSO, const RaytracingPipelineStateDesc& Desc)
{
    CD3DX12_STATE_OBJECT_DESC StateObjectDesc(D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE);

    auto rootSignatureSubObject = StateObjectDesc.CreateSubobject<CD3DX12_GLOBAL_ROOT_SIGNATURE_SUBOBJECT>();
    rootSignatureSubObject->SetRootSignature(Dx12_GetRootSignature(g_render.RootSignature)); // TODO: Allow overrides on desc

    UINT MaxRayRecursion = static_cast<UINT>(Desc.MaxRayRecursion);
    auto configurationSubObject = StateObjectDesc.CreateSubobject<CD3DX12_RAYTRACING_PIPELINE_CONFIG_SUBOBJECT>();
    configurationSubObject->Config(MaxRayRecursion);

    auto shaderConfigStateObject = StateObjectDesc.CreateSubobject<CD3DX12_RAYTRACING_SHADER_CONFIG_SUBOBJECT>();
    shaderConfigStateObject->Config(8, 8);

    auto AddDxilLibary = [&](IDxcBlob* ShaderBlob, LPCWSTR ExportName)
    {
        assert(ShaderBlob && "Ray Shader is not valid for PSO compilation");

        CD3DX12_DXIL_LIBRARY_SUBOBJECT* LibSubObject = StateObjectDesc.CreateSubobject<CD3DX12_DXIL_LIBRARY_SUBOBJECT>();
        D3D12_SHADER_BYTECODE Shader = CD3DX12_SHADER_BYTECODE(ShaderBlob->GetBufferPointer(), ShaderBlob->GetBufferSize());
        LibSubObject->SetDXILLibrary(&Shader);
        LibSubObject->DefineExport(ExportName, L"main");
    };

    if (Desc.RayGenShader != RaytracingRayGenShader_t::INVALID)
    {
        AddDxilLibary(Dx12_GetRayGenShaderBlob(Desc.RayGenShader), L"RayGen");
    }

    ComPtr<ID3D12StateObject> DxRTPSO = g_RTPSOs.Alloc(RtPSO);

    if (DXENSURE(g_render.DxDevice->CreateStateObject(StateObjectDesc, IID_PPV_ARGS(&DxRTPSO))))
    {
        if (!Desc.DebugName.empty())
            DxRTPSO->SetName(Desc.DebugName.c_str());

        return true;
    }

    return false;
}

bool CreateRaytracingShaderTableImpl(RaytracingShaderTable_t ShaderTable, RaytracingPipelineState_t RTPipelineState, const RaytracingShaderTableLayout& Layout)
{
    ID3D12StateObject* DxPipelineState = Dx12_GetRaytracingStateObject(RTPipelineState);
    if (!DxPipelineState)
        return false;

    Dx12ShaderTable& DxShaderTable = g_ShaderTables.Alloc(ShaderTable);

    std::vector<byte> BufferData;

    auto AddShaderRecord = [&](LPCWSTR ExportName)
    {
        ComPtr<ID3D12StateObjectProperties> StateObjectProperties = nullptr;
        if (DXENSURE(DxPipelineState->QueryInterface(IID_PPV_ARGS(&StateObjectProperties))))
        {
            void* ShaderIdentifierData = StateObjectProperties->GetShaderIdentifier(ExportName);
            size_t BufIt = BufferData.size();
            BufferData.resize(BufferData.size() + D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
            memcpy(&BufferData[BufIt], ShaderIdentifierData, D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
        }
    };

    for (const RaytracingShaderRecord& Record : Layout.GetRecords())
    {
        if (Record.Type == RaytracingShaderRecordType::RAYGEN)
        {
            AddShaderRecord(L"RayGen");
        }
        else if (Record.Type == RaytracingShaderRecordType::MISS)
        {
            AddShaderRecord(L"Miss");
        }
        else if (Record.Type == RaytracingShaderRecordType::HITGROUP)
        {
            AddShaderRecord(L"HitGroup");
        }
        else if (Record.Type == RaytracingShaderRecordType::DATA)
        {
            size_t BufIt = BufferData.size();
            BufferData.resize(BufferData.size() + 16);
            memcpy(&BufferData[BufIt], Record.Data.Data, 16);
        }
    }

    DxShaderTable.GpuShaderTable = Dx12_CreateByteBuffer(BufferData.data(), BufferData.size());

    return DxShaderTable.GpuShaderTable.pGPUMem != 0;
}

void AddRaytracingGeometryToSceneImpl(RaytracingGeometry_t Geometry, RaytracingScene_t Scene)
{
    if (TLAS* DxScene = g_TLAS.Get(Scene))
    {
        if (std::find(DxScene->Meshes.begin(), DxScene->Meshes.end(), Geometry) == DxScene->Meshes.end())
        {
            DxScene->Meshes.push_back(Geometry);
        }
        else
        {
            OutputDebugStringA("Duplicated meshes or instancing not supported");
        }
    }
}

void RemoveRaytracingGeometryFromSceneImpl(RaytracingGeometry_t Geometry, RaytracingScene_t Scene)
{
    if (TLAS* DxScene = g_TLAS.Get(Scene))
    {
        auto FoundIt = std::find(DxScene->Meshes.begin(), DxScene->Meshes.end(), Geometry);

        if (FoundIt == DxScene->Meshes.end())
        {
            OutputDebugStringA("Can't remove geometry, not present in scene");
        }
        else
        {
            DxScene->Meshes.erase(FoundIt);
        }
    }
}

void DestroyRaytracingGeometryImpl(RaytracingGeometry_t RtGeometry)
{
    g_BLAS.Free(RtGeometry);
}

void DestroyRaytracingSceneImpl(RaytracingScene_t RtScene)
{
    g_TLAS.Free(RtScene);
}

void DestroyRaytracingPipelineStateImpl(RaytracingPipelineState_t RTPipelineState)
{
    g_RTPSOs.Free(RTPipelineState);
}

void Dx12_BuildRaytracingScene(CommandList* cl, RaytracingScene_t scene)
{
    // Assume scene is dirty
    if (!g_TLAS.Valid(scene))
        return;

    TLAS& SceneAS = g_TLAS[scene];
    
    D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO TopLevelPrebuildInfo = {};
    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC TopLevelAccelerationStructureDesc = {};
    TopLevelAccelerationStructureDesc.Inputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;
    TopLevelAccelerationStructureDesc.Inputs.NumDescs = SceneAS.GeometryCount;
    TopLevelAccelerationStructureDesc.Inputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE;
    TopLevelAccelerationStructureDesc.Inputs.pGeometryDescs = nullptr;
    TopLevelAccelerationStructureDesc.Inputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
    g_render.DxDevice->GetRaytracingAccelerationStructurePrebuildInfo(&TopLevelAccelerationStructureDesc.Inputs, &TopLevelPrebuildInfo);    

    ComPtr<ID3D12Resource> TopLevelScratchBuffer = Dx12_CreateBuffer(TopLevelPrebuildInfo.ScratchDataSizeInBytes, D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
    SceneAS.DxBuffer = Dx12_CreateBuffer(TopLevelPrebuildInfo.ScratchDataSizeInBytes, D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);

    TopLevelAccelerationStructureDesc.DestAccelerationStructureData = SceneAS.DxBuffer->GetGPUVirtualAddress();
    TopLevelAccelerationStructureDesc.ScratchAccelerationStructureData = TopLevelScratchBuffer->GetGPUVirtualAddress();

    std::vector<D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC> BottomLevelDescs(SceneAS.Meshes.size());
    std::vector<D3D12_RAYTRACING_INSTANCE_DESC> InstanceDescs(SceneAS.Meshes.size());
    std::vector<ComPtr<ID3D12Resource>> BottomLevelScratchBuffers(SceneAS.Meshes.size());

    for (size_t GeomIt = 0; GeomIt < SceneAS.Meshes.size(); GeomIt++)
    {
        BLAS& Geom = g_BLAS[SceneAS.Meshes[GeomIt]];

        D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC& BottomLevelDesc = BottomLevelDescs[GeomIt];
        BottomLevelDesc.Inputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;
        BottomLevelDesc.Inputs.NumDescs = 1u;
        BottomLevelDesc.Inputs.pGeometryDescs = &Geom.DxDesc;
        BottomLevelDesc.Inputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE;
        BottomLevelDesc.Inputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;

        D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO BottomLevelPrebuildInfo = {};
        g_render.DxDevice->GetRaytracingAccelerationStructurePrebuildInfo(&BottomLevelDesc.Inputs, &BottomLevelPrebuildInfo);

        Geom.DxBuffer = Dx12_CreateBuffer(BottomLevelPrebuildInfo.ResultDataMaxSizeInBytes, D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
        BottomLevelDesc.DestAccelerationStructureData = Geom.DxBuffer->GetGPUVirtualAddress();

        BottomLevelScratchBuffers[GeomIt] = Dx12_CreateBuffer(BottomLevelPrebuildInfo.ScratchDataSizeInBytes, D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);

        D3D12_RAYTRACING_INSTANCE_DESC InstanceDesc = InstanceDescs[GeomIt];

        // Sample allocs a UAV here

        ZeroMemory(InstanceDesc.Transform, sizeof(InstanceDesc.Transform));
        InstanceDesc.Transform[0][0] = 1.0f;
        InstanceDesc.Transform[1][1] = 1.0f;
        InstanceDesc.Transform[2][2] = 1.0f;

        InstanceDesc.AccelerationStructure = Geom.DxBuffer->GetGPUVirtualAddress();
        InstanceDesc.Flags = 0;
        InstanceDesc.InstanceID = 0;
        InstanceDesc.InstanceMask = 1;
        InstanceDesc.InstanceContributionToHitGroupIndex = GeomIt;
    }

    // Upload instance data
    ComPtr<ID3D12Resource> InstanceDataBuffer = Dx12_CreateBuffer(InstanceDescs.size(), D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
    DynamicBuffer_t InstanceDataBufferInitData = CreateDynamicByteBufferFromArray(InstanceDescs.data(), InstanceDescs.size());

    ID3D12GraphicsCommandList* DxCl = Dx12_GetCommandList(cl);

    {
        const D3D12_RESOURCE_BARRIER Barrier = Dx12_TransitionBarrier(InstanceDataBuffer.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST);
        DxCl->ResourceBarrier(1u, &Barrier);
    }

    size_t CopySrcOffset;
    ID3D12Resource* CopySrcResource = Dx12_GetDynamicBufferResource(InstanceDataBufferInitData, &CopySrcOffset);
    DxCl->CopyBufferRegion(InstanceDataBuffer.Get(), 0u, CopySrcResource, CopySrcOffset, InstanceDescs.size() * sizeof(D3D12_RAYTRACING_INSTANCE_DESC));

    {
        const D3D12_RESOURCE_BARRIER Barrier = Dx12_TransitionBarrier(InstanceDataBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_GENERIC_READ);
        DxCl->ResourceBarrier(1u, &Barrier);
    }

    TopLevelAccelerationStructureDesc.Inputs.InstanceDescs = InstanceDataBuffer->GetGPUVirtualAddress();
    TopLevelAccelerationStructureDesc.Inputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;

    // Build structures
    ComPtr<ID3D12GraphicsCommandList4> DxRtCL;
    DxCl->QueryInterface(IID_PPV_ARGS(&DxRtCL));

    // Assume descriptor heaps are already set

    std::vector<D3D12_RESOURCE_BARRIER> UAVBarriers(BottomLevelDescs.size());
    for (uint32_t BLASIt = 0; BLASIt < BottomLevelDescs.size(); BLASIt++)
    {
        DxRtCL->BuildRaytracingAccelerationStructure(&BottomLevelDescs[BLASIt], 0, nullptr);

        BLAS& Geom = g_BLAS[SceneAS.Meshes[BLASIt]];
        UAVBarriers[BLASIt] = Dx12_UavBarrier(Geom.DxBuffer.Get());
    }

    DxRtCL->ResourceBarrier((UINT)UAVBarriers.size(), UAVBarriers.data());

    DxRtCL->BuildRaytracingAccelerationStructure(&TopLevelAccelerationStructureDesc, 0, nullptr);
}

ID3D12StateObject* Dx12_GetRaytracingStateObject(RaytracingPipelineState_t RaytracingPipelineState)
{
    return g_RTPSOs.Valid(RaytracingPipelineState) ? g_RTPSOs[RaytracingPipelineState].Get() : nullptr;
}

}

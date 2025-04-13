#include "PipelineState.h"
#include "Impl/PipelineStateImpl.h"
#include "IDArray.h"

namespace tpr
{

struct GraphicsPipelineStateData
{
    GraphicsPipelineStateDesc Desc;
    std::vector<InputElementDesc> Inputs;
};

struct ComputePipelineStateData
{
    ComputePipelineStateDesc Desc;
};

GraphicsPipelineTargetDesc::GraphicsPipelineTargetDesc(std::initializer_list<RenderFormat> formats, std::initializer_list<BlendMode> blends, RenderFormat depthFormat)
{
    assert(formats.size() == blends.size() && "GraphicsPipelineTargetDesc ctor target desc and blend mismatch");

    NumRenderTargets = (uint8_t)(formats.size());

    memcpy(Formats, formats.begin(), NumRenderTargets * sizeof(RenderFormat));
    memcpy(Blends, blends.begin(), NumRenderTargets * sizeof(BlendMode));

    DepthFormat = depthFormat;
}

template<typename T>
inline void hash_combine(uint64_t& hash, const T& value)
{
    std::hash<T> h;
    hash ^= h(value) + 0x9e3779b9 + (hash << 9) + (hash >> 2);
}

uint64_t GraphicsPipelineTargetDesc::Hash() const
{
    if (Hashed != 0)
    {
        return Hashed;
    }

    hash_combine(Hashed, NumRenderTargets);

    for (uint32_t i = 0; i < NumRenderTargets; i++)
    {
        hash_combine(Hashed, (uint32_t)Formats[i]);
        hash_combine(Hashed, Blends[i].Opaque);
    }

    hash_combine(Hashed, DepthFormat);

    return Hashed;
}

IDArray<GraphicsPipelineState_t, GraphicsPipelineStateData> g_GraphicsPipelineStates;
IDArray<ComputePipelineState_t, ComputePipelineStateData> g_ComputePipelineStates;

GraphicsPipelineState_t CreateGraphicsPipelineState(const GraphicsPipelineStateDesc& desc, const InputElementDesc* inputs, size_t inputCount)
{
    GraphicsPipelineState_t pso = g_GraphicsPipelineStates.Create();

    if (!CompileGraphicsPipelineState(pso, desc, inputs, inputCount))
    {
        g_GraphicsPipelineStates.Release(pso);
        return GraphicsPipelineState_t::INVALID;
    }

    // TODO Multithread: If we can accept the input elements as a slice or similar we can forward the args right into the Create() call, avoiding a second lock.
    {
        auto lock = g_GraphicsPipelineStates.ReadScopeLock();

        GraphicsPipelineStateData* data = g_GraphicsPipelineStates.Get(pso);

        data->Desc = desc;
        data->Inputs.resize(inputCount);
        memcpy(data->Inputs.data(), inputs, inputCount * sizeof(InputElementDesc));
    }
    
    return pso;
}

ComputePipelineState_t CreateComputePipelineState(const ComputePipelineStateDesc& desc)
{
    ComputePipelineState_t pso = g_ComputePipelineStates.Create({ desc });

    if (!CompileComputePipelineState(pso, desc))
    {
        g_ComputePipelineStates.Release(pso);
        return ComputePipelineState_t::INVALID;
    }

    return pso;
}

void RenderRef(GraphicsPipelineState_t pso)
{
    g_GraphicsPipelineStates.AddRef(pso);
}

void RenderRef(ComputePipelineState_t pso)
{
    g_ComputePipelineStates.AddRef(pso);
}

void RenderRelease(GraphicsPipelineState_t pso)
{
    if (g_GraphicsPipelineStates.Release(pso))
        DestroyGraphicsPipelineState(pso);
}

void RenderRelease(ComputePipelineState_t pso)
{
    if (g_ComputePipelineStates.Release(pso))
        DestroyComputePipelineState(pso);
}

size_t GetGraphicsPipelineStateCount()
{
    return g_GraphicsPipelineStates.UsedSize();
}

size_t GetComputePipelineStateCount()
{
    return g_ComputePipelineStates.UsedSize();
}

void ReloadPipelines()
{
    g_GraphicsPipelineStates.ForEachValid([](GraphicsPipelineState_t Handle, const GraphicsPipelineStateData& Data)
    {
         return CompileGraphicsPipelineState(Handle, Data.Desc, Data.Inputs.data(), Data.Inputs.size());
    });

    g_ComputePipelineStates.ForEachValid([](ComputePipelineState_t Handle, const ComputePipelineStateData& Data)
    {
        return CompileComputePipelineState(Handle, Data.Desc);
    });
}

}

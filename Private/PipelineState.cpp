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

GraphicsPipelineTargetDesc::GraphicsPipelineTargetDesc(std::initializer_list<RenderFormat> formats, std::initializer_list<BlendMode> blends)
{
    assert(formats.size() == blends.size() && "GraphicsPipelineTargetDesc ctor target desc and blend mismatch");

    NumRenderTargets = (uint8_t)(formats.size());

    memcpy(Formats, formats.begin(), NumRenderTargets * sizeof(RenderFormat));
    memcpy(Blends, blends.begin(), NumRenderTargets * sizeof(BlendMode));
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

    GraphicsPipelineStateData* data = g_GraphicsPipelineStates.Get(pso);

    data->Desc = desc;
    data->Inputs.resize(inputCount);
    memcpy(data->Inputs.data(), inputs, inputCount * sizeof(InputElementDesc));
    
    return pso;
}

ComputePipelineState_t CreateComputePipelineState(const ComputePipelineStateDesc& desc)
{
    ComputePipelineState_t pso = g_ComputePipelineStates.Create();

    if (!CompileComputePipelineState(pso, desc))
    {
        g_ComputePipelineStates.Release(pso);
        return ComputePipelineState_t::INVALID;
    }

    ComputePipelineStateData* data = g_ComputePipelineStates.Get(pso);

    data->Desc = desc;

    return pso;
}

const GraphicsPipelineStateDesc* GetGraphicsPipelineStateDesc(GraphicsPipelineState_t pso)
{
    if (const GraphicsPipelineStateData* data = g_GraphicsPipelineStates.Get(pso))
    {
        return &data->Desc;
    }

    return nullptr;
}

const ComputePipelineStateDesc* GetComputePipelineStateDesc(ComputePipelineState_t pso)
{
    if (const ComputePipelineStateData* data = g_ComputePipelineStates.Get(pso))
    {
        return &data->Desc;
    }

    return nullptr;
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

}

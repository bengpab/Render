#include "Impl/RaytracingImpl.h"

namespace rl
{

bool CreateRaytracingGeometryImpl(RaytracingGeometry_t Handle, const RaytracingGeometryDesc& Desc)
{
    return false;
}

bool CreateRaytracingSceneImpl(RaytracingScene_t RtScene)
{
    return false;
}

bool CreateRaytracingPipelineStateImpl(RaytracingPipelineState_t RtPSO, const RaytracingPipelineStateDesc& Desc)
{
    return false;
}

bool CreateRaytracingShaderTableImpl(RaytracingShaderTable_t ShaderTable, RaytracingPipelineState_t RTPipelineState, const RaytracingShaderTableLayout& Layout)
{
    return false;
}

void AddRaytracingGeometryToSceneImpl(RaytracingGeometry_t Geometry, RaytracingScene_t Scene)
{
}

void RemoveRaytracingGeometryFromSceneImpl(RaytracingGeometry_t Geometry, RaytracingScene_t Scene)
{
}

void DestroyRaytracingGeometryImpl(RaytracingGeometry_t RtGeometry)
{
}

void DestroyRaytracingSceneImpl(RaytracingScene_t RtScene)
{
}

void DestroyRaytracingPipelineStateImpl(RaytracingPipelineState_t RTPipelineState)
{
}

}

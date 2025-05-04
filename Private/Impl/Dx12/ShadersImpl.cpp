#include "Impl/ShadersImpl.h"

#include "Impl/Dxc/DxCompiler.h"
#include "RenderTypes.h"
#include "RenderImpl.h"
#include "SparseArray.h"

#include <dxcapi.h>

namespace rl
{

struct
{
	SparseArray<ComPtr<IDxcBlob>, VertexShader_t>				CompiledVertexBlobs;
	SparseArray<ComPtr<IDxcBlob>, PixelShader_t>				CompiledPixelBlobs;
	SparseArray<ComPtr<IDxcBlob>, GeometryShader_t>				CompiledGeometryBlobs;
	SparseArray<ComPtr<IDxcBlob>, MeshShader_t>					CompiledMeshBlobs;
	SparseArray<ComPtr<IDxcBlob>, AmplificationShader_t>		CompiledAmplificationBlobs;
	SparseArray<ComPtr<IDxcBlob>, ComputeShader_t>				CompiledComputeBlobs;
	SparseArray<ComPtr<IDxcBlob>, RaytracingRayGenShader_t>		CompiledRayGenBlobs;
} g_shaders;

bool CompileShaderInternal(ShaderProfile target, const char* path, const char* includeDirectory, const ShaderMacros& macros, ComPtr<IDxcBlob>& shaderBlob)
{	
	ComPtr<IDxcResult> result = CompileShaderFromFile(path, includeDirectory, target, macros);

	if (!result)
	{
		return false;
	}

	if (!result->HasOutput(DXC_OUT_OBJECT))
	{
		OutputDebugStringA("CompileShader failed - result->HasOutput(DXC_OUT_OBJECT)");

		return false;
	}		

	ComPtr<IDxcBlobUtf16> outputName;
	if (!DXENSURE(result->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&shaderBlob), &outputName)))
		return false;

	return true;
}

bool CompileShader(VertexShader_t handle, const char* path, const char* includeDirectory, const ShaderMacros& macros)
{
	return CompileShaderInternal(ShaderProfile::VS_6_0, path, includeDirectory, macros, g_shaders.CompiledVertexBlobs.Alloc(handle));
}

bool CompileShader(PixelShader_t handle, const char* path, const char* includeDirectory, const ShaderMacros& macros)
{
	return CompileShaderInternal(ShaderProfile::PS_6_0, path, includeDirectory, macros, g_shaders.CompiledPixelBlobs.Alloc(handle));
}

bool CompileShader(GeometryShader_t handle, const char* path, const char* includeDirectory, const ShaderMacros& macros)
{
	return CompileShaderInternal(ShaderProfile::GS_6_0, path, includeDirectory, macros, g_shaders.CompiledGeometryBlobs.Alloc(handle));
}

bool CompileShader(MeshShader_t handle, const char* path, const char* includeDirectory, const ShaderMacros& macros)
{
	return CompileShaderInternal(ShaderProfile::MS_6_0, path, includeDirectory, macros, g_shaders.CompiledMeshBlobs.Alloc(handle));
}

bool CompileShader(AmplificationShader_t handle, const char* path, const char* includeDirectory, const ShaderMacros& macros)
{
	return CompileShaderInternal(ShaderProfile::AS_6_0, path, includeDirectory, macros, g_shaders.CompiledAmplificationBlobs.Alloc(handle));
}

bool CompileShader(ComputeShader_t handle, const char* path, const char* includeDirectory, const ShaderMacros& macros)
{
	return CompileShaderInternal(ShaderProfile::CS_6_0, path, includeDirectory, macros, g_shaders.CompiledComputeBlobs.Alloc(handle));
}

bool CompileShader(RaytracingRayGenShader_t handle, const char* path, const char* includeDirectory, const ShaderMacros& macros)
{
	return CompileShaderInternal(ShaderProfile::LIB_6_3, path, includeDirectory, macros, g_shaders.CompiledRayGenBlobs.Alloc(handle));
}

IDxcBlob* Dx12_GetVertexShaderBlob(VertexShader_t vs)
{
	return g_shaders.CompiledVertexBlobs[vs].Get();
}

IDxcBlob* Dx12_GetPixelShaderBlob(PixelShader_t ps)
{
	return g_shaders.CompiledPixelBlobs[ps].Get();
}

IDxcBlob* Dx12_GetGeometryShaderBlob(GeometryShader_t gs)
{
	return g_shaders.CompiledGeometryBlobs[gs].Get();
}

IDxcBlob* Dx12_GetMeshShaderBlob(MeshShader_t ms)
{
	return g_shaders.CompiledMeshBlobs[ms].Get();
}

IDxcBlob* Dx12_GetAmplificationShaderBlob(AmplificationShader_t as)
{
	return g_shaders.CompiledAmplificationBlobs[as].Get();
}

IDxcBlob* Dx12_GetComputeShaderBlob(ComputeShader_t cs)
{
	return g_shaders.CompiledComputeBlobs[cs].Get();
}

IDxcBlob* Dx12_GetRayGenShaderBlob(RaytracingRayGenShader_t rgs)
{
	return g_shaders.CompiledRayGenBlobs[rgs].Get();
}

}

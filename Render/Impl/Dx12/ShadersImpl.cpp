#include "../ShadersImpl.h"

#include "Impl/Dxc/DxCompiler.h"
#include "RenderTypes.h"
#include "RenderImpl.h"
#include "SparseArray.h"

#include <dxcapi.h>

struct
{
	SparseArray<ComPtr<IDxcBlob>, VertexShader_t> CompiledVertexBlobs;
	SparseArray<ComPtr<IDxcBlob>, PixelShader_t> CompiledPixelBlobs;
	SparseArray<ComPtr<IDxcBlob>, GeometryShader_t> CompiledGeometryBlobs;
	SparseArray<ComPtr<IDxcBlob>, ComputeShader_t> CompiledComputeBlobs;
} g_shaders;

bool CompileShader(ShaderProfile target, const char* path, const ShaderMacros& macros, ComPtr<IDxcBlob>& shaderBlob)
{	
	ComPtr<IDxcResult> result = CompileShaderFromFile(path, target, macros);
	if (!result->HasOutput(DXC_OUT_OBJECT))
	{
		OutputDebugStringA("CompileShader failed - result->HasOutput(DXC_OUT_OBJECT)");

		return false;
	}		

	if (!DXENSURE(result->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(shaderBlob.GetAddressOf()), nullptr)))
		return false;

	return true;
}

bool CompileVertexShader(VertexShader_t handle, const char* path, const ShaderMacros& macros)
{
	return CompileShader(ShaderProfile::VS_6_0, path, macros, g_shaders.CompiledVertexBlobs.Alloc(handle));
}

bool CompilePixelShader(PixelShader_t handle, const char* path, const ShaderMacros& macros)
{
	return CompileShader(ShaderProfile::PS_6_0, path, macros, g_shaders.CompiledPixelBlobs.Alloc(handle));
}

bool CompileGeometryShader(GeometryShader_t handle, const char* path, const ShaderMacros& macros)
{
	return CompileShader(ShaderProfile::GS_6_0, path, macros, g_shaders.CompiledGeometryBlobs.Alloc(handle));
}

bool CompileComputeShader(ComputeShader_t handle, const char* path, const ShaderMacros& macros)
{
	return CompileShader(ShaderProfile::CS_6_0, path, macros, g_shaders.CompiledComputeBlobs.Alloc(handle));
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

IDxcBlob* Dx12_GetComputeShaderBlob(ComputeShader_t cs)
{
	return g_shaders.CompiledComputeBlobs[cs].Get();
}

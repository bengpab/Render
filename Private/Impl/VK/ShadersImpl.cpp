#include "Impl/ShadersImpl.h"

#include "SpirvShaderCompiler.h"
#include "SparseArray.h"
#include <stdexcept>
#include "RenderImpl.h"

namespace rl
{
	struct
	{
		SparseArray<std::vector<char>, VertexShader_t>			CompiledVertexBlobs;
		SparseArray<std::vector<char>, PixelShader_t>			CompiledPixelBlobs;
		SparseArray<std::vector<char>, GeometryShader_t>		CompiledGeometryBlobs;
		SparseArray<std::vector<char>, MeshShader_t>			CompiledMeshBlobs;
		SparseArray<std::vector<char>, AmplificationShader_t>	CompiledAmplificationBlobs;
		SparseArray<std::vector<char>, ComputeShader_t>			CompiledComputeBlobs;
	} g_shaders;

	bool CompileShader(VertexShader_t handle, const char* path, const char* directory, const ShaderMacros& macros) 
	{
		std::vector<char>& shaderBlob = g_shaders.CompiledVertexBlobs.Alloc(handle);
		
		CompileSpirVShaderFromFile(path, directory, macros, "VertexShader", shaderBlob);
		return true;
	}


	bool CompileShader(PixelShader_t handle, const char* path, const char* directory, const ShaderMacros& macros)
	{
		std::vector<char>& shaderBlob = g_shaders.CompiledPixelBlobs.Alloc(handle);

		CompileSpirVShaderFromFile(path, directory, macros, "PixelShader", shaderBlob);
		return true;
	}

	bool CompileShader(GeometryShader_t handle, const char* path, const char* directory, const ShaderMacros& macros)
	{
		throw std::runtime_error("Unimplemented");
	}

	bool CompileShader(MeshShader_t handle, const char* path, const char* directory, const ShaderMacros& macros)
	{
		throw std::runtime_error("Unimplemented");
	}

	bool CompileShader(AmplificationShader_t handle, const char* path, const char* directory, const ShaderMacros& macros)
	{
		throw std::runtime_error("Unimplemented");
	}

	bool CompileShader(ComputeShader_t handle, const char* path, const char* directory, const ShaderMacros& macros)
	{
		std::vector<char>& shaderBlob = g_shaders.CompiledComputeBlobs.Alloc(handle);

		CompileSpirVShaderFromFile(path, directory, macros, "ComputeShader", shaderBlob);
		return true;
	}


	std::vector<char>* Vk_GetVertexShaderBlob(VertexShader_t vs)
	{
		return &g_shaders.CompiledVertexBlobs[vs];
	}

	std::vector<char>* Vk_GetPixelShaderBlob(PixelShader_t ps)
	{
		return &g_shaders.CompiledPixelBlobs[ps];
	}

	std::vector<char>* Vk_GetComputeShaderBlob(ComputeShader_t cs)
	{
		return &g_shaders.CompiledComputeBlobs[cs];
	}

	std::vector<char>* rl::Vk_GetGeometryShaderBlob(GeometryShader_t gs)
	{
		throw std::runtime_error("Unimplemented");
		return nullptr;
	}
	std::vector<char>* rl::Vk_GetMeshShaderBlob(MeshShader_t ms)
	{
		throw std::runtime_error("Unimplemented");
		return nullptr;
	}
	std::vector<char>* Vk_GetAmplificationShaderBlob(AmplificationShader_t as)
	{
		throw std::runtime_error("Unimplemented");
		return nullptr;
	}
}
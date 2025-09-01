#include "SpirvShaderCompiler.h"

#include "shaderc/shaderc.hpp"
#include <stdexcept>
#include <fstream>
namespace rl
{
void CompileSpirVShaderFromBuffer(const std::string& shaderCode, const char* includeDirectory, const ShaderMacros& macros, ShaderType Type, const char* DebugName, std::vector<char>& outSpirVCode)
{
	shaderc::Compiler compiler;
	shaderc::CompileOptions options;
	for (const auto& macro : macros)
	{
		options.AddMacroDefinition(macro._define, macro._value);
	}
	if (includeDirectory)
	{
		//TODO: Find how to add includes: use https://github.com/beaumanvienna/vulkan/commit/617f70e1a311c6f498ec69507dcc9d4aadb86612
		//shaderc::CompileOptions::IncluderInterface includerInterface;


		//options.a(includeDirectory);
	}

	shaderc_shader_kind kind = shaderc_glsl_infer_from_source;

	switch (Type) 
	{
	case ShaderType::VERTEX: kind = shaderc_vertex_shader; break; 
	case ShaderType::PIXEL:	kind = shaderc_fragment_shader; break;
	case ShaderType::COMPUTE: kind = shaderc_compute_shader; break;
	}

	auto result = compiler.CompileGlslToSpv(
		shaderCode.c_str(), kind, DebugName, options);
	if (result.GetCompilationStatus() != shaderc_compilation_status_success)
	{
		throw std::runtime_error("Shader compilation failed: " + result.GetErrorMessage());
	}

	outSpirVCode.assign(result.cbegin(), result.cend());
}

void CompileSpirVShaderFromFile(const std::string& path, const char* includeDirectory, const ShaderMacros& macros, const char* DebugName, std::vector<char>& outSpirVCode)
{
	std::ifstream file(path, std::ios::ate);
	
	if (!file.is_open())
	{
		throw std::runtime_error("Failed to open shader file: " + path);
	}
	
	std::vector<char> shaderCode((size_t)file.tellg());
	
	file.seekg(0);
	file.read(shaderCode.data(), shaderCode.size());

	std::string shaderCodeStr(shaderCode.begin(), shaderCode.end());

	ShaderType type = ShaderType::VERTEX; // Default to vertex shader, can be changed based on file extension or other logic
	if (path.find(".frag") != std::string::npos)
	{
		type = ShaderType::PIXEL;
	}
	else if (path.find(".comp") != std::string::npos)
	{
		type = ShaderType::COMPUTE;
	}


	CompileSpirVShaderFromBuffer(shaderCodeStr, includeDirectory, macros, type, DebugName, outSpirVCode);
}

}
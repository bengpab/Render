#pragma once

#include "RenderTypes.h"
#include "Shaders.h"

namespace rl
{
enum class ShaderType : uint8_t
{
	VERTEX,
	PIXEL,
	COMPUTE,
};

void CompileSpirVShaderFromBuffer(const std::string& shaderCode, const char* includeDirectory, const ShaderMacros& macros, ShaderType Type, const char* DebugName, std::vector<char>& outSpirVCode);
void CompileSpirVShaderFromFile(const std::string& path, const char* includeDirectory, const ShaderMacros& macros, const char* DebugName, std::vector<char>& outSpirVCode);
}

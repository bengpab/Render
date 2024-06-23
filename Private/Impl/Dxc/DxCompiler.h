#pragma once

#include "RenderTypes.h"
#include "Shaders.h"

struct IDxcResult;

namespace tpr
{

enum class ShaderProfile : uint8_t
{
	CS_5_0,
	PS_5_0,
	VS_5_0,
	GS_5_0,
	CS_5_1,
	PS_5_1,
	VS_5_1,
	GS_5_1,
	CS_6_0,
	PS_6_0,
	VS_6_0,
	GS_6_0,
	COUNT
};

ComPtr<IDxcResult> CompileShaderFromFile(const std::string& path, ShaderProfile profile, const ShaderMacros& macros);
ComPtr<IDxcResult> CompileShader(const std::string& shaderCode, ShaderProfile profile, const ShaderMacros& macros);

}
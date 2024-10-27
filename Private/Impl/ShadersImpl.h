#pragma once

#include "Shaders.h"

namespace tpr
{

bool CompileShader(VertexShader_t handle, const char* path, const char* directory, const ShaderMacros& macros);
bool CompileShader(PixelShader_t handle, const char* path, const char* directory, const ShaderMacros& macros);
bool CompileShader(GeometryShader_t handle, const char* path, const char* directory, const ShaderMacros& macros);
bool CompileShader(ComputeShader_t handle, const char* path, const char* directory, const ShaderMacros& macros);

}
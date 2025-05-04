#pragma once

#include "Shaders.h"

namespace rl
{

bool CompileShader(VertexShader_t handle, const char* path, const char* directory, const ShaderMacros& macros);
bool CompileShader(PixelShader_t handle, const char* path, const char* directory, const ShaderMacros& macros);
bool CompileShader(GeometryShader_t handle, const char* path, const char* directory, const ShaderMacros& macros);
bool CompileShader(MeshShader_t handle, const char* path, const char* directory, const ShaderMacros& macros);
bool CompileShader(AmplificationShader_t handle, const char* path, const char* directory, const ShaderMacros& macros);
bool CompileShader(ComputeShader_t handle, const char* path, const char* directory, const ShaderMacros& macros);
bool CompileShader(RaytracingRayGenShader_t handle, const char* path, const char* directory, const ShaderMacros& macros);

}
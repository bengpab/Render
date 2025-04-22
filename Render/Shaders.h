#pragma once

#include "RenderTypes.h"

namespace rl
{

RENDER_TYPE(VertexShader_t);
RENDER_TYPE(PixelShader_t);
RENDER_TYPE(GeometryShader_t);
RENDER_TYPE(MeshShader_t);
RENDER_TYPE(AmplificationShader_t);
RENDER_TYPE(ComputeShader_t);

struct ShaderMacro
{
	std::string _define;
	std::string _value;

	ShaderMacro() {}

	ShaderMacro(const char* define)
		: _define(define)
		, _value("1")
	{}

	ShaderMacro(const char* define, const char* value)
		: _define(define)
		, _value(value)
	{}

	ShaderMacro(const char* define, uint32_t value);
};

VertexShader_t			CreateVertexShader(const char* path, const ShaderMacros& macros = {});
PixelShader_t			CreatePixelShader(const char* path, const ShaderMacros& macros = {});
GeometryShader_t		CreateGeometryShader(const char* path, const ShaderMacros& macros = {});
MeshShader_t			CreateMeshShader(const char* path, const ShaderMacros& macros = {});
AmplificationShader_t	CreateAmplificationShader(const char* path, const ShaderMacros& macros = {});
ComputeShader_t			CreateComputeShader(const char* path, const ShaderMacros& macros = {});

size_t GetVertexShaderCount();
size_t GetPixelShaderCount();
size_t GetGeometryShaderCount();
size_t GetMeshShaderCount();
size_t GetAmplificationShaderCount();
size_t GetComputeShaderCount();

void ReloadShaders();

void RenderRef(VertexShader_t vs);
void RenderRef(PixelShader_t ps);
void RenderRef(GeometryShader_t gs);
void RenderRef(MeshShader_t ms);
void RenderRef(AmplificationShader_t as);
void RenderRef(ComputeShader_t cs);

void RenderRelease(VertexShader_t vs);
void RenderRelease(PixelShader_t ps);
void RenderRelease(GeometryShader_t gs);
void RenderRelease(MeshShader_t ms);
void RenderRelease(AmplificationShader_t as);
void RenderRelease(ComputeShader_t cs);

}
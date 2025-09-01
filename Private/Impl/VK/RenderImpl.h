#pragma once

#include "vulkan/vulkan.h"
#include "RenderTypes.h"
#include <optional>
#include <vector>

namespace rl
{

struct VKRenderGlobals
{
	VkInstance Instance;
	VkPhysicalDevice PhysicalDevice = VK_NULL_HANDLE;
	VkDevice Device;

	VkQueue presentQueue;
	VkQueue GraphicsQueue;
	VkQueue ComputeQueue;

	VkExtent2D MainViewExtent{};
};

extern VKRenderGlobals g_render;

void CreateInstance(bool debug);
void GetPhysicalDevice();
void CreateDevice();

bool IsDeviceSuitable(VkPhysicalDevice device);

std::vector<char>* Vk_GetVertexShaderBlob(VertexShader_t vs);
std::vector<char>* Vk_GetPixelShaderBlob(PixelShader_t ps);
std::vector<char>* Vk_GetGeometryShaderBlob(GeometryShader_t gs);
std::vector<char>* Vk_GetMeshShaderBlob(MeshShader_t ms);
std::vector<char>* Vk_GetAmplificationShaderBlob(AmplificationShader_t as);
std::vector<char>* Vk_GetComputeShaderBlob(ComputeShader_t cs);

}
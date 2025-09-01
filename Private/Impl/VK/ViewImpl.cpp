#include "View.h"

#include "RenderImpl.h"

#include "VKHelpers.h"

#define VOLK_IMPLEMENTATION
#include "volk.h"
#include "glm/glm.hpp"

#include <map>

namespace rl
{
std::map<intptr_t, RenderView*> g_views;

struct RenderViewImpl
{
	VkSurfaceKHR Surface;
	VkSwapchainKHR Swapchain;
	VkFormat swapChainImageFormat;
	VkExtent2D swapChainExtent;

	std::vector<VkImage> swapChainImages;
	std::vector<VkImageView> swapChainImageViews;
};

RenderView::RenderView()
	: Impl(std::make_unique<RenderViewImpl>())
{
}

RenderView::~RenderView()
{
	for (uint32_t i = 0; i < RenderView::NumBackBuffers; i++)
	{
	}

	for (auto& imageView : Impl->swapChainImageViews)
	{
		vkDestroyImageView(g_render.Device, imageView, nullptr);
	}

	vkDestroySwapchainKHR(g_render.Device, Impl->Swapchain, nullptr);
	vkDestroySurfaceKHR(g_render.Instance, Impl->Surface, nullptr);
}

void RenderView::Sync()
{
}

void RenderView::Resize(uint32_t x, uint32_t y)
{
}

void RenderView::Present(bool vsync, bool sync)
{
}

struct SwapChainSupportDetails
{
	VkSurfaceCapabilitiesKHR capabilities;
	std::vector<VkSurfaceFormatKHR> formats;
	std::vector<VkPresentModeKHR> presentModes;
};

static SwapChainSupportDetails GetSwapchainSupportDetails(RenderView* renderView)
{
	SwapChainSupportDetails details;

	VkPhysicalDevice& device = g_render.PhysicalDevice;

	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, renderView->Impl->Surface, &details.capabilities);

	uint32_t formatCount;
	vkGetPhysicalDeviceSurfaceFormatsKHR(device, renderView->Impl->Surface, &formatCount, nullptr);

	if (formatCount != 0) 
	{
		details.formats.resize(formatCount);
		vkGetPhysicalDeviceSurfaceFormatsKHR(device, renderView->Impl->Surface, &formatCount, details.formats.data());
	}

	uint32_t presentModeCount;
	vkGetPhysicalDeviceSurfacePresentModesKHR(device, renderView->Impl->Surface, &presentModeCount, nullptr);

	if (presentModeCount != 0) 
	{
		details.presentModes.resize(presentModeCount);
		vkGetPhysicalDeviceSurfacePresentModesKHR(device, renderView->Impl->Surface, &presentModeCount, details.presentModes.data());
	}

	return details;
}

static VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) 
{
	for (const auto& availableFormat : availableFormats) 
	{
		if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) 
		{
			return availableFormat;
		}
	}

	return availableFormats[0];
}

static VkPresentModeKHR ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes) 
{
	for (const auto& availablePresentMode : availablePresentModes) 
	{
		if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) 
		{
			return availablePresentMode;
		}
	}

	return VK_PRESENT_MODE_FIFO_KHR;
}

static VkExtent2D ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities, const RenderView* renderView) 
{

	VkExtent2D actualExtent = 
	{
		static_cast<uint32_t>(renderView->Width),
		static_cast<uint32_t>(renderView->Height)
	};

	actualExtent.width = glm::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
	actualExtent.height = glm::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

	return actualExtent;
}

static void CreateImageViews(RenderView* renderView)
{
	renderView->Impl->swapChainImageViews.resize(renderView->Impl->swapChainImages.size());

	for (size_t i = 0; i < renderView->Impl->swapChainImages.size(); i++)
	{
		VkImageViewCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		createInfo.image = renderView->Impl->swapChainImages[i];
		createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		createInfo.format = renderView->Impl->swapChainImageFormat;
		createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		createInfo.subresourceRange.baseMipLevel = 0;
		createInfo.subresourceRange.levelCount = 1;
		createInfo.subresourceRange.baseArrayLayer = 0;
		createInfo.subresourceRange.layerCount = 1;
		CHECK_VK_RESULT(vkCreateImageView(g_render.Device, &createInfo, nullptr, &renderView->Impl->swapChainImageViews[i]), "vkCreateImageView");
	}
}

void CreateSwapchain(RenderView* view) 
{
	SwapChainSupportDetails swapchainDetails = GetSwapchainSupportDetails(view);

	VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapchainDetails.formats);
	VkPresentModeKHR presentMode = ChooseSwapPresentMode(swapchainDetails.presentModes);
	VkExtent2D extent = ChooseSwapExtent(swapchainDetails.capabilities, view);
	uint32_t imageCount = swapchainDetails.capabilities.minImageCount + 1;

	g_render.MainViewExtent = extent;

	if (swapchainDetails.capabilities.maxImageCount > 0 && imageCount > swapchainDetails.capabilities.maxImageCount) {
		imageCount = swapchainDetails.capabilities.maxImageCount;
	}

	VkSwapchainCreateInfoKHR createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	createInfo.surface = view->Impl->Surface;

	createInfo.minImageCount = imageCount;
	createInfo.imageFormat = surfaceFormat.format;
	createInfo.imageColorSpace = surfaceFormat.colorSpace;
	createInfo.imageExtent = extent;
	createInfo.imageArrayLayers = 1;
	createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

	//TODO: use VK_SHARING_MODE_CONCURRENT if queue families for graphics and present are different.
	createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	createInfo.queueFamilyIndexCount = 0; // Optional
	createInfo.pQueueFamilyIndices = nullptr; // Optional

	createInfo.preTransform = swapchainDetails.capabilities.currentTransform;
	createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;\
	createInfo.presentMode = presentMode;
	createInfo.clipped = VK_TRUE;
	createInfo.oldSwapchain = VK_NULL_HANDLE;

	CHECK_VK_RESULT(vkCreateSwapchainKHR(g_render.Device, &createInfo, nullptr, &view->Impl->Swapchain), "vkCreateSwapchainKHR");

	view->Impl->swapChainImageFormat = surfaceFormat.format;
	view->Impl->swapChainExtent = extent;

	vkGetSwapchainImagesKHR(g_render.Device, view->Impl->Swapchain, &imageCount, nullptr);
	view->Impl->swapChainImages.resize(imageCount);
	vkGetSwapchainImagesKHR(g_render.Device, view->Impl->Swapchain, &imageCount, view->Impl->swapChainImages.data());

}

bool IsSurfaceSuitable(RenderView* renderView)
{
	uint32_t queueFamilyCount = 0;

	vkGetPhysicalDeviceQueueFamilyProperties(g_render.PhysicalDevice, &queueFamilyCount, nullptr);

	std::vector<VkQueueFamilyProperties> queueFamiliesProps(queueFamilyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(g_render.PhysicalDevice, &queueFamilyCount, queueFamiliesProps.data());

	std::optional<uint32_t> graphicsFamily;
	std::optional<uint32_t> presentFamily;

	for (uint32_t i = 0; i < queueFamilyCount; i++)
	{
		if (queueFamiliesProps[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
		{
			graphicsFamily = i;
		}

		//TODO: Look at how to make surface in between and query for presenting
		VkBool32 presentSupport = false;
		vkGetPhysicalDeviceSurfaceSupportKHR(g_render.PhysicalDevice, i, renderView->Impl->Surface, &presentSupport);

		if (presentSupport && graphicsFamily.has_value() && graphicsFamily.value() == i)
		{
			presentFamily = i;
			//return true;
		}
	}

	//Only allow for gpus with combined graphics and present queue families.
	return graphicsFamily.has_value() && presentFamily.has_value() &&
		graphicsFamily.value() == presentFamily.value();
}

RenderViewPtr CreateRenderViewPtr(intptr_t hwnd) 
{
	return std::shared_ptr<RenderView>(CreateRenderView(hwnd));
}

RenderView* CreateRenderView(intptr_t hwnd)
{
	RenderView* rv = new RenderView;

	rv->Hwnd = hwnd;

	VkWin32SurfaceCreateInfoKHR createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
	createInfo.hwnd = (HWND)hwnd;
	createInfo.hinstance = GetModuleHandle(nullptr);

	CHECK_VK_RESULT(vkCreateWin32SurfaceKHR(g_render.Instance, &createInfo, nullptr, &rv->Impl->Surface), "vkCreateWin32SurfaceKHR");

	if (!IsSurfaceSuitable(rv)) 
	{
		throw std::runtime_error("Surface is not suitable for rendering");
	}

	CreateSwapchain(rv);

	g_views[hwnd] = rv;

	return rv;
}
}

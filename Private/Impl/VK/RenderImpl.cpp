#include "RenderImpl.h"

#include "Render.h"

#include "volk.h"
#include "SparseArray.h"
#include "VKHelpers.h"

namespace rl
{
VKRenderGlobals g_render;

static const std::vector<const char*> validationLayers = 
{
    "VK_LAYER_KHRONOS_validation"
};


static bool checkValidationLayerSupport() 
{
    uint32_t layerCount;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

    std::vector<VkLayerProperties> availableLayers(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

    for (const char* layerName : validationLayers) {
        bool layerFound = false;

        for (const auto& layerProperties : availableLayers) {
            if (strcmp(layerName, layerProperties.layerName) == 0) {
                layerFound = true;
                break;
            }
        }

        if (!layerFound) {
            return false;
        }
    }

    return true;
}

struct QueueFamilyIndices
{
    std::optional<uint32_t> graphicsFamily;
    std::optional<uint32_t> computeFamily;
};

static QueueFamilyIndices FindFamilyIndices()
{
    QueueFamilyIndices ret{};
    uint32_t queueFamilyCount = 0;

    vkGetPhysicalDeviceQueueFamilyProperties(g_render.PhysicalDevice, &queueFamilyCount, nullptr);

    std::vector<VkQueueFamilyProperties> queueFamiliesProps(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(g_render.PhysicalDevice, &queueFamilyCount, queueFamiliesProps.data());

    for (uint32_t i = 0; i < queueFamilyCount; i++) 
    {
        if (queueFamiliesProps[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) 
        {
            ret.graphicsFamily = i;
        }

        if (queueFamiliesProps[i].queueFlags & VK_QUEUE_COMPUTE_BIT) 
        {
            ret.computeFamily = i;
        }
    }

    if (!ret.computeFamily.has_value() || !ret.graphicsFamily.has_value())
    {
        throw std::runtime_error("No suitable queue families found!");
    }

    return ret;
}

bool Render_Init(const RenderInitParams& params) 
{
    if (volkInitialize() != VK_SUCCESS)
        return false;

    CreateInstance(params.DebugEnabled);

	volkLoadInstance(g_render.Instance);

    GetPhysicalDevice();
    CreateDevice();

	return true;
}

void Render_ShutDown()
{
	if (g_render.Instance != VK_NULL_HANDLE) {
		vkDestroyInstance(g_render.Instance, nullptr);
		g_render.Instance = VK_NULL_HANDLE;
	}

	if (g_render.Device != VK_NULL_HANDLE) {
		vkDestroyDevice(g_render.Device, nullptr);
		g_render.Device = VK_NULL_HANDLE;
	}
}
void CreateInstance(bool debug)
{
    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "Hello TPR";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "No Engine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_0;

    std::vector<const char*> enabledExtensions = { VK_KHR_SURFACE_EXTENSION_NAME };
    enabledExtensions.push_back(VK_KHR_WIN32_SURFACE_EXTENSION_NAME);

    VkInstanceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;
    createInfo.enabledExtensionCount = static_cast<uint32_t>(enabledExtensions.size());
    createInfo.ppEnabledExtensionNames = enabledExtensions.data();

    if (debug && checkValidationLayerSupport())
    {
        createInfo.enabledLayerCount = 1;
        createInfo.ppEnabledLayerNames = validationLayers.data();
    }
    else
    {
        createInfo.enabledLayerCount = 0;
    }

    CHECK_VK_RESULT(vkCreateInstance(&createInfo, nullptr, &g_render.Instance), "vkCreateInstance");

}
void GetPhysicalDevice()
{
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(g_render.Instance, &deviceCount, nullptr);

	if (deviceCount == 0) {
		throw std::runtime_error("Failed to find GPUs with Vulkan support!");
	}
	std::vector<VkPhysicalDevice> devices(deviceCount);
	vkEnumeratePhysicalDevices(g_render.Instance, &deviceCount, devices.data());

	if (!IsDeviceSuitable(devices[0]))
    {
        throw std::runtime_error("No suitable physical device found!");
	}

	g_render.PhysicalDevice = devices[0];
}

void CreateDevice()
{
    QueueFamilyIndices indices = FindFamilyIndices();

    std::vector<const char*> enabledExtensions;
    enabledExtensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);

    float queuePriority = 1.0f;
    VkDeviceQueueCreateInfo queueCreateInfo[2] = {};
    queueCreateInfo[0].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueCreateInfo[0].queueFamilyIndex = indices.graphicsFamily.value();
    queueCreateInfo[0].queueCount = 1;
    queueCreateInfo[0].pQueuePriorities = &queuePriority;
    queueCreateInfo[1].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueCreateInfo[1].queueFamilyIndex = indices.computeFamily.value();
    queueCreateInfo[1].queueCount = 1;
    queueCreateInfo[1].pQueuePriorities = &queuePriority;

	VkPhysicalDeviceFeatures deviceFeatures{};
	//deviceFeatures.geometryShader = VK_TRUE;
    
	VkDeviceCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	createInfo.queueCreateInfoCount = 2;
	createInfo.pQueueCreateInfos = queueCreateInfo;
	createInfo.pEnabledFeatures = &deviceFeatures;
	createInfo.enabledExtensionCount = static_cast<uint32_t>(enabledExtensions.size());
	createInfo.ppEnabledExtensionNames = enabledExtensions.data();
    createInfo.pNext = nullptr;

    CHECK_VK_RESULT(vkCreateDevice(g_render.PhysicalDevice, &createInfo, nullptr, &g_render.Device), "vkCreateDevice")

    volkLoadDevice(g_render.Device);

    vkGetDeviceQueue(g_render.Device, indices.graphicsFamily.value(), 0, &g_render.GraphicsQueue);
    vkGetDeviceQueue(g_render.Device, indices.graphicsFamily.value(), 0, &g_render.presentQueue);
    vkGetDeviceQueue(g_render.Device, indices.computeFamily.value(), 0, &g_render.ComputeQueue);
}

bool IsDeviceSuitable(VkPhysicalDevice device)
{
    VkPhysicalDeviceProperties deviceProperties;
    VkPhysicalDeviceFeatures deviceFeatures;
    vkGetPhysicalDeviceProperties(device, &deviceProperties);
    vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

    return deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU &&
        deviceFeatures.geometryShader;
}

}
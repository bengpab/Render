#pragma once

#include <stdexcept>
#include <string>

#include "volk.h"


#define CHECK_VK_RESULT(Func, FuncName) \
	{	\
		VkResult result = Func; \
		if (result != VK_SUCCESS) { \
			std::string errorMessage = "Vulkan error in " + std::string(FuncName) + ": " + std::to_string(result); \
			throw std::runtime_error(errorMessage); \
		} \
	}
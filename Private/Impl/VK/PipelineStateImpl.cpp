#include "Impl/PipelineStateImpl.h"
#include "volk.h"
#include "RenderImpl.h"
#include "VKFormats.h"
#include "SparseArray.h"
#include <stdexcept>

namespace rl
{
	struct
	{
		SparseArray<VkPipelineLayout, GraphicsPipelineState_t> GraphicsPipelineLayouts;
		//SparseArray<ComPtr<ID3D12PipelineState>, ComputePipelineState_t> ComputePipelines;
	} g_pipelines;

	static VkShaderModule CreateShaderModule(const std::vector<char>& shaderCode)
	{
		VkShaderModuleCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		createInfo.codeSize = shaderCode.size();
		createInfo.pCode = reinterpret_cast<const uint32_t*>(shaderCode.data());
		VkShaderModule shaderModule;
		if (vkCreateShaderModule(g_render.Device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
			throw std::runtime_error("failed to create shader module");
		}

		return shaderModule;
	}

	static VkVertexInputRate GetInputClassification(InputClassification c)
	{
		switch (c)
		{
		case InputClassification::PER_VERTEX:	return VK_VERTEX_INPUT_RATE_VERTEX;
		case InputClassification::PER_INSTANCE:	return VK_VERTEX_INPUT_RATE_INSTANCE;
		}
		assert(0 && "Unsupported InputClassification");
		return (VkVertexInputRate)0;
	}

	static VkPrimitiveTopology GetTopology(PrimitiveTopologyType primTopo)
	{
		switch (primTopo)
		{
		case PrimitiveTopologyType::POINT:	return VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
		case PrimitiveTopologyType::LINE:	return VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
		case PrimitiveTopologyType::TRIANGLE:	return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		}
		assert(0 && "Unsupported PrimitiveTopologyType");
		return (VkPrimitiveTopology)0;
	}
	
	static VkPolygonMode GetPolygonFillMode(FillMode fm)
	{
		switch (fm)
		{
		case FillMode::SOLID:		return VK_POLYGON_MODE_FILL;
		case FillMode::WIREFRAME:	return VK_POLYGON_MODE_LINE;
		}
		assert(0 && "Unsupported FillMode");
		return (VkPolygonMode)0;
	}

	static VkCullModeFlagBits GetCullMode(CullMode cm)
	{
		switch (cm)
		{
		case CullMode::NONE:		return VK_CULL_MODE_NONE;
		case CullMode::FRONT:		return VK_CULL_MODE_FRONT_BIT;
		case CullMode::BACK:		return VK_CULL_MODE_BACK_BIT;
		}
		assert(0 && "Unsupported CullMode");
		return (VkCullModeFlagBits)0;
	}

	static VkBlendFactor GetBlendFactor(BlendType type)
	{
		switch (type)
		{
		case BlendType::ZERO:			return VK_BLEND_FACTOR_ZERO;
		case BlendType::ONE:			return VK_BLEND_FACTOR_ONE;
		case BlendType::SRC_COLOR:		return VK_BLEND_FACTOR_SRC_COLOR;
		case BlendType::INV_SRC_COLOR:	return VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR;
		case BlendType::SRC_ALPHA:		return VK_BLEND_FACTOR_SRC_ALPHA;
		case BlendType::INV_SRC_ALPHA:	return VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
		case BlendType::DST_ALPHA:		return VK_BLEND_FACTOR_DST_ALPHA;
		case BlendType::INV_DST_ALPHA:	return VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA;
		case BlendType::DST_COLOR:		return VK_BLEND_FACTOR_DST_COLOR;
		case BlendType::INV_DST_COLOR:	return VK_BLEND_FACTOR_ONE_MINUS_DST_COLOR;
		case BlendType::SRC_ALPHA_SAT:	return VK_BLEND_FACTOR_SRC_ALPHA_SATURATE;
		}
		assert(0 && "Unsupported BlendType");
		return (VkBlendFactor)0;
	}

	static VkBlendOp GetBlendOp(BlendOp op)
	{
		switch (op)
		{
		case BlendOp::ADD:			return VK_BLEND_OP_ADD;
		case BlendOp::SUBTRACT:		return VK_BLEND_OP_SUBTRACT;
		case BlendOp::REV_SUBTRACT:	return VK_BLEND_OP_REVERSE_SUBTRACT;
		case BlendOp::MIN:			return VK_BLEND_OP_MIN;
		case BlendOp::MAX:			return VK_BLEND_OP_MAX;
		}
		assert(0 && "Unsupported BlendOp");
		return (VkBlendOp)0;
	}

	bool CompileGraphicsPipelineState(GraphicsPipelineState_t handle, const GraphicsPipelineStateDesc& desc, const InputElementDesc* inputs, size_t inputCount)
	{
		VkShaderModule vertShaderModule;
		VkShaderModule pixShaderModule;
		if (desc.VS != VertexShader_t::INVALID) 
		{
			std::vector<char>* vsBlob = Vk_GetVertexShaderBlob(desc.VS);
			assert(vsBlob && "CompileGraphicsPipelineState null vsBlob");

			vertShaderModule = CreateShaderModule(*vsBlob);
		}
		else
		{
			assert(0 && "CompileGraphicsPipelineState needs a valid vertex shader");
			return false;
		}

		if (desc.PS != PixelShader_t::INVALID)
		{
			std::vector<char>* psBlob = Vk_GetPixelShaderBlob(desc.PS);
			assert(psBlob && "CompileGraphicsPipelineState null psBlob");
			pixShaderModule = CreateShaderModule(*psBlob);
		}

		VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
		vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
		vertShaderStageInfo.module = vertShaderModule;
		vertShaderStageInfo.pName = "main";

		VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
		fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		fragShaderStageInfo.module = pixShaderModule;
		fragShaderStageInfo.pName = "main";

		VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };

		std::vector<VkDynamicState> dynamicStates = {
			VK_DYNAMIC_STATE_VIEWPORT,
			VK_DYNAMIC_STATE_SCISSOR
		};

		VkPipelineDynamicStateCreateInfo dynamicState{};
		dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
		dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
		dynamicState.pDynamicStates = dynamicStates.data();


		//Use max for now, maybe change into array?
		VkVertexInputBindingDescription bindingDescriptions[16];
		VkVertexInputAttributeDescription attributeDescriptions[16];

		const uint32_t vertexInputDescriptionCount = inputCount < 16u ? (uint32_t)inputCount : 16u;
		for (uint32_t i = 0; i < vertexInputDescriptionCount; i++)
		{
			bindingDescriptions[i].binding = inputs[i].inputSlot;
			bindingDescriptions[i].stride = inputs[i].alignedByteOffset;
			bindingDescriptions[i].inputRate = GetInputClassification(inputs[i].inputSlotClass);
			attributeDescriptions[i].binding = inputs[i].inputSlot;
			attributeDescriptions[i].location = inputs[i].semanticIndex; //TODO: Maybe change this?
			attributeDescriptions[i].format = GetVKFormat(inputs[i].format);
			attributeDescriptions[i].offset = inputs[i].alignedByteOffset;
		}

		VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
		vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		vertexInputInfo.vertexBindingDescriptionCount = vertexInputDescriptionCount;
		vertexInputInfo.pVertexBindingDescriptions = bindingDescriptions;
		vertexInputInfo.vertexAttributeDescriptionCount = vertexInputDescriptionCount;
		vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions;

		VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
		inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		inputAssembly.topology = GetTopology(desc.PrimTopo);
		inputAssembly.primitiveRestartEnable = VK_FALSE;

		//Viewport scissor always at full screen
		VkViewport viewport{};
		viewport.x = 0.0f;
		viewport.y = 0.0f;
		viewport.width = (float)g_render.MainViewExtent.width;
		viewport.height = (float)g_render.MainViewExtent.height;
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;

		VkRect2D scissor{};
		scissor.offset = { 0, 0 };
		scissor.extent = g_render.MainViewExtent;

		VkPipelineViewportStateCreateInfo viewportState{}; //TODO: Maybe not needed with dynamic states?
		viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		viewportState.viewportCount = 1;
		viewportState.pViewports = &viewport;
		viewportState.scissorCount = 1;
		viewportState.pScissors = &scissor;

		VkPipelineRasterizationStateCreateInfo rasterizer{};
		rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		rasterizer.rasterizerDiscardEnable = VK_FALSE;
		rasterizer.polygonMode = GetPolygonFillMode(desc.Fill); // TODO: Wireframe requires GPU feature
		rasterizer.lineWidth = 1.0f;
		rasterizer.cullMode = GetCullMode(desc.Cull);
		rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
		rasterizer.depthClampEnable = desc.DepthBias > 0 ? VK_TRUE : VK_FALSE; // TODO: This requires GPU feature
		rasterizer.depthBiasConstantFactor = 0.0f; //TODO not qeuivalent in api
		rasterizer.depthBiasClamp = desc.DepthBiasClamp;
		rasterizer.depthBiasSlopeFactor = desc.SlopeScaleDepthBias;

		//Disabled
		VkPipelineMultisampleStateCreateInfo multisampling{};
		multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		multisampling.sampleShadingEnable = VK_FALSE;
		multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
		multisampling.minSampleShading = 1.0f; // Optional
		multisampling.pSampleMask = nullptr; // Optional
		multisampling.alphaToCoverageEnable = VK_FALSE; // Optional
		multisampling.alphaToOneEnable = VK_FALSE; // Optional

		//TODO: Depth stencil state


		VkPipelineColorBlendAttachmentState colorBlendAttachment[8]; //Support a max of 8 like dx12

		const uint32_t numColorAttachments = desc.TargetDesc.NumRenderTargets < 8 ? desc.TargetDesc.NumRenderTargets : 8;
		for (uint32_t i = 0; i < numColorAttachments; i++)
		{
			const BlendMode& blend = desc.TargetDesc.Blends[i];
			colorBlendAttachment[i].blendEnable = blend.BlendEnabled ? VK_TRUE : VK_FALSE;
			colorBlendAttachment[i].srcColorBlendFactor = GetBlendFactor(blend.SrcBlend);
			colorBlendAttachment[i].dstColorBlendFactor = GetBlendFactor(blend.DstBlend);
			colorBlendAttachment[i].colorBlendOp = GetBlendOp(blend.Op);
			colorBlendAttachment[i].srcAlphaBlendFactor = GetBlendFactor(blend.SrcBlendAlpha);
			colorBlendAttachment[i].dstAlphaBlendFactor = GetBlendFactor(blend.DstBlendAlpha);
			colorBlendAttachment[i].alphaBlendOp = GetBlendOp(blend.OpAlpha);
			colorBlendAttachment[i].colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
		}

		VkPipelineColorBlendStateCreateInfo colorBlending{};
		colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		colorBlending.logicOpEnable = VK_FALSE;
		colorBlending.logicOp = VK_LOGIC_OP_COPY; // Optional
		colorBlending.attachmentCount = numColorAttachments;
		colorBlending.pAttachments = colorBlendAttachment;
		colorBlending.blendConstants[0] = 0.0f; // Optional
		colorBlending.blendConstants[1] = 0.0f; // Optional
		colorBlending.blendConstants[2] = 0.0f; // Optional
		colorBlending.blendConstants[3] = 0.0f; // Optional

		//For uniforms
		VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
		pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutInfo.setLayoutCount = 0; // Optional
		pipelineLayoutInfo.pSetLayouts = nullptr; // Optional
		pipelineLayoutInfo.pushConstantRangeCount = 0; // Optional
		pipelineLayoutInfo.pPushConstantRanges = nullptr; // Optional

		
		VkPipelineLayout& pipelineLayout = g_pipelines.GraphicsPipelineLayouts.Alloc(handle);
		if (vkCreatePipelineLayout(g_render.Device, &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS) {
			throw std::runtime_error("failed to create pipeline layout!");
		}

		return true;
	}

	bool CompileComputePipelineState(ComputePipelineState_t handle, const ComputePipelineStateDesc& desc)
	{
		return true;
	}

	void DestroyGraphicsPipelineState(GraphicsPipelineState_t pso)
	{
	}

	void DestroyComputePipelineState(ComputePipelineState_t pso)
	{
	}


}
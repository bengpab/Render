add_library(RenderDx11
            "Binding.h"
            "Buffers.h"
            "CommandList.h"
            "PipelineState.h"
            "Render.h"
            "RenderTypes.h"
            "RootSignature.h"
            "Samplers.h"
            "Shaders.h"
            "Textures.h"
            "View.h"
)

add_library(RenderDx12
            "Binding.h"
            "Buffers.h"
            "CommandList.h"
            "PipelineState.h"
            "Render.h"
            "RenderTypes.h"
            "RootSignature.h"
            "Samplers.h"
            "Shaders.h"
            "Textures.h"
            "View.h"
)

set_target_properties(RenderDx11 PROPERTIES OUTPUT_NAME_DEBUG RenderExampleDx11_Debug)
set_target_properties(RenderDx12 PROPERTIES OUTPUT_NAME_DEBUG RenderExampleDx12_Debug)

target_include_directories(RenderDx11 PRIVATE 
                            "" 
                            "Private"
)

target_include_directories(RenderDx12 PRIVATE 
                            "" 
                            "Private"
)

target_sources(RenderDx11 PRIVATE
                "Private/Binding.cpp"
                "Private/Buffers.cpp"
                "Private/IDArray.h"
                "Private/PipelineState.cpp"
                "Private/RootSignature.cpp"
                "Private/Shaders.cpp"
                "Private/SparseArray.h"
                "Private/Textures.cpp"
                "Private/Impl/BindingImpl.h"
                "Private/Impl/BuffersImpl.h"
                "Private/Impl/PipelineStateImpl.h"
                "Private/Impl/RootSignatureImpl.h"
                "Private/Impl/ShadersImpl.h"
                "Private/Impl/TexturesImpl.h"
)

target_sources(RenderDx12 PRIVATE
                "Private/Binding.cpp"
                "Private/Buffers.cpp"
                "Private/IDArray.h"
                "Private/PipelineState.cpp"
                "Private/RootSignature.cpp"
                "Private/Shaders.cpp"
                "Private/SparseArray.h"
                "Private/Textures.cpp"
                "Private/Impl/BindingImpl.h"
                "Private/Impl/BuffersImpl.h"
                "Private/Impl/PipelineStateImpl.h"
                "Private/Impl/RootSignatureImpl.h"
                "Private/Impl/ShadersImpl.h"
                "Private/Impl/TexturesImpl.h"
)

target_sources(RenderDx11 PRIVATE
                "Private/Impl/Dx/DxErrorHandling.cpp"
                "Private/Impl/Dx/DxErrorHandling.h"
                "Private/Impl/Dx11/BindingImpl.cpp"
                "Private/Impl/Dx11/BuffersImpl.cpp"
                "Private/Impl/Dx11/CommandListImpl.cpp"
                "Private/Impl/Dx11/Dx11Types.cpp"
                "Private/Impl/Dx11/Dx11Types.h"
                "Private/Impl/Dx11/PipelineStateImpl.cpp"
                "Private/Impl/Dx11/RenderImpl.cpp"
                "Private/Impl/Dx11/RenderImpl.h"
                "Private/Impl/Dx11/RootSignatureImpl.cpp"
                "Private/Impl/Dx11/ShadersImpl.cpp"
                "Private/Impl/Dx11/TexturesImpl.cpp"
                "Private/Impl/Dx11/ViewImpl.cpp"
)

target_sources(RenderDx12 PRIVATE
                "Private/Impl/Dx/DxErrorHandling.cpp"
                "Private/Impl/Dx/DxErrorHandling.h"
                "Private/Impl/Dx12/BindingImpl.cpp"
                "Private/Impl/Dx12/CommandListImpl.cpp"
                "Private/Impl/Dx12/Dx12Types.cpp"
                "Private/Impl/Dx12/Dx12Types.h"
                "Private/Impl/Dx12/DynamicBuffersImpl.cpp"
                "Private/Impl/Dx12/PipelineStateImpl.cpp"
                "Private/Impl/Dx12/RenderImpl.cpp"
                "Private/Impl/Dx12/RenderImpl.h"
                "Private/Impl/Dx12/RootSignatureImpl.cpp"
                "Private/Impl/Dx12/StaticBuffersImpl.cpp"
                "Private/Impl/Dx12/ShadersImpl.cpp"
                "Private/Impl/Dx12/TexturesImpl.cpp"
                "Private/Impl/Dx12/ViewImpl.cpp"
                "Private/Impl/Dxc/DXCompiler.cpp"
                "Private/Impl/Dxc/DXCompiler.h"
)

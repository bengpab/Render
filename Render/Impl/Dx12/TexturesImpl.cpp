#include "../TexturesImpl.h"

#include "RenderImpl.h"
#include "SparseArray.h"

struct Dx12Texture
{
	ComPtr<ID3D12Resource> DxResource = {};
	uint64_t GraphicsFence = 0u;
	uint64_t ComputeFence = 0u;
	uint64_t CopyFence = 0u;
};

SparseArray<Dx12Texture, Texture_t> g_DxTextures;

std::vector<Dx12Texture> g_UploadResources;
std::vector<Dx12Texture> g_FreeQueue;

D3D12_RESOURCE_DIMENSION Dx12_ResourceDimension(TextureDimension td)
{
	switch (td)
	{
	case TextureDimension::Unknown:
		return D3D12_RESOURCE_DIMENSION_UNKNOWN;
	case TextureDimension::Tex1D:
		return D3D12_RESOURCE_DIMENSION_TEXTURE1D;
	case TextureDimension::Tex2D:
		return D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	case TextureDimension::Tex3D:
		return D3D12_RESOURCE_DIMENSION_TEXTURE3D;
	}

	assert(0 && "Dx12_ResourceDimension unsupported");
	return (D3D12_RESOURCE_DIMENSION)0;
}

bool AllocTextureImpl(Texture_t tex)
{
	g_DxTextures.Alloc(tex);

	return true;
}

bool CreateTextureImpl(Texture_t tex, const TextureCreateDescEx& desc)
{
	Dx12Texture& texture = g_DxTextures.Alloc(tex);	

	D3D12_RESOURCE_DESC resourceDesc = {};

	resourceDesc.Dimension = Dx12_ResourceDimension(desc.dimension);
	resourceDesc.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
	resourceDesc.Width = desc.width;
	resourceDesc.Height = desc.height;
	resourceDesc.DepthOrArraySize = desc.depthOrArraySize;
	resourceDesc.MipLevels = desc.mipCount;
	resourceDesc.Format = Dx12_Format(desc.resourceFormat);
	resourceDesc.SampleDesc.Count = 1u;
	resourceDesc.SampleDesc.Quality = 0u;
	resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_64KB_UNDEFINED_SWIZZLE;

	resourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

	if (!HasEnumFlags(desc.flags, RenderResourceFlags::SRV))
		resourceDesc.Flags |= D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE;

	if (HasEnumFlags(desc.flags, RenderResourceFlags::RTV))
		resourceDesc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

	if (HasEnumFlags(desc.flags, RenderResourceFlags::DSV))
		resourceDesc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

	if(HasEnumFlags(desc.flags, RenderResourceFlags::UAV))
		resourceDesc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

	D3D12_HEAP_PROPERTIES heapProps = Dx12_HeapProps(D3D12_HEAP_TYPE_DEFAULT);

	//D3D12_RESOURCE_STATES initState = desc.data ? D3D12_RESOURCE_STATE_COPY_DEST : D3D12_RESOURCE_STATE_COMMON;
	D3D12_RESOURCE_STATES initState = D3D12_RESOURCE_STATE_COMMON;

	if (FAILED(g_render.DxDevice->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &resourceDesc, initState, nullptr, IID_PPV_ARGS(&texture.DxResource))))
	{
		assert(0 && "CreateTextureImpl failed to create committed resource");
		return false;
	}

	if (!desc.DebugName.empty())
	{
		texture.DxResource->SetName(desc.DebugName.c_str());
	}

	if (desc.data)
	{
		UINT64 requiredUploadSize = 0u;
		g_render.DxDevice->GetCopyableFootprints(&resourceDesc, 0, resourceDesc.MipLevels * resourceDesc.DepthOrArraySize, 0u, nullptr, nullptr, nullptr, &requiredUploadSize);

		ComPtr<ID3D12Resource> uploadResource = Dx12_CreateBuffer(requiredUploadSize, D3D12_HEAP_TYPE_UPLOAD, D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_FLAG_NONE);

		if (!uploadResource)
		{
			assert(0 && "CreateTextureImpl failed to create upload resource");
			return false;
		}

		void* pMapped;
		if (FAILED(uploadResource->Map(0u, nullptr, &pMapped)))
		{
			assert(0 && "CreateTextureImpl failed to map upload resource");
			return false;
		}

		const size_t numSubResources = desc.mipCount * desc.depthOrArraySize;

		std::vector<D3D12_PLACED_SUBRESOURCE_FOOTPRINT> layouts;
		std::vector<UINT64> rowSizesInBytes;
		std::vector<UINT> numRows;

		layouts.resize(numSubResources);
		rowSizesInBytes.resize(numSubResources);
		numRows.resize(numSubResources);

		UINT64 requiredSize = 0u;
		g_render.DxDevice->GetCopyableFootprints(&resourceDesc, 0u, (UINT)numSubResources, 0u, layouts.data(), numRows.data(), rowSizesInBytes.data(), &requiredSize);

		std::vector<D3D12_SUBRESOURCE_DATA> subResData;
		subResData.resize(desc.mipCount * desc.depthOrArraySize);

		for (uint32_t d = 0u; d < desc.depthOrArraySize; d++)
		{
			for (uint32_t m = 0u; m < desc.mipCount; m++)
			{
				uint32_t subResIndex = d * desc.mipCount + m;
				D3D12_SUBRESOURCE_DATA& subRes = subResData[subResIndex];
				const MipData& mipData = desc.data[subResIndex];

				subRes.pData = mipData.data;
				subRes.RowPitch = mipData.rowPitch;
				subRes.SlicePitch = mipData.slicePitch;

				const D3D12_PLACED_SUBRESOURCE_FOOTPRINT& layout = layouts[subResIndex];
				UINT64 rowSizeBytes = rowSizesInBytes[subResIndex];
				UINT rowCount = numRows[subResIndex];

				D3D12_MEMCPY_DEST destData = { (BYTE*)pMapped + layout.Footprint.RowPitch, (SIZE_T)layout.Footprint.RowPitch * (SIZE_T)rowCount };

				auto pDestSlice = static_cast<BYTE*>(pMapped) + layout.Offset;
				auto pSrcSlice = static_cast<const BYTE*>(mipData.data);
				for (UINT y = 0; y < rowCount; ++y)
				{
					memcpy(pDestSlice + layout.Footprint.RowPitch * y,
						pSrcSlice + mipData.rowPitch * LONG_PTR(y),
						rowSizeBytes);
				}
			}
		}

		uploadResource->Unmap(0u, nullptr);

		CommandListPtr uploadCl = CommandList::Create(CommandListType::COPY);

		ID3D12GraphicsCommandList* dxcl = Dx12_GetCommandList(uploadCl.get());

		for(uint32_t i = 0; i < desc.depthOrArraySize * desc.mipCount; i++)
		{
			D3D12_TEXTURE_COPY_LOCATION dst = {};
			dst.pResource = texture.DxResource.Get();
			dst.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
			dst.SubresourceIndex = i;

			D3D12_TEXTURE_COPY_LOCATION src = {};
			src.pResource = uploadResource.Get();
			src.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
			src.PlacedFootprint = layouts[i];

			dxcl->CopyTextureRegion(&dst, 0, 0, 0, &src, nullptr);
		}	

		uploadCl->TransitionResource(tex, ResourceTransitionState::COPY_DEST, ResourceTransitionState::COMMON);

		g_UploadResources.emplace_back(uploadResource, Dx12_Signal(CommandListType::COPY));

		CommandList::Execute(uploadCl);
	}

	return true;
}

bool UpdateTextureImpl(Texture_t tex, const void* const data, uint32_t width, uint32_t height, RenderFormat format)
{
	return false;
}

void DestroyTexture(Texture_t tex)
{
	g_DxTextures[tex].CopyFence = g_render.CopyQueue.FenceValue;
	g_DxTextures[tex].GraphicsFence = g_render.DirectQueue.FenceValue;
	g_DxTextures[tex].ComputeFence = g_render.ComputeQueue.FenceValue;

	g_FreeQueue.emplace_back(std::move(g_DxTextures[tex]));

	g_DxTextures.Free(tex);
}

TextureResourceAccessScope::TextureResourceAccessScope(Texture_t resource, TextureResourceAccessMethod method, uint32_t subResourceIndex)
	: mappedTex(resource)
	, subResIdx(subResourceIndex)
{

}

TextureResourceAccessScope::~TextureResourceAccessScope()
{
}

void Dx12_TexturesBeginFrame()
{
	Dx12_TexturesProcessPendingDeletes(false);
}

void Dx12_TexturesProcessPendingDeletes(bool flush)
{
	if (flush)
	{
		Dx12_FlushQueue(g_render.CopyQueue);
		Dx12_FlushQueue(g_render.DirectQueue);
		Dx12_FlushQueue(g_render.ComputeQueue);
	}

	const uint64_t CopyFrameFence = g_render.CopyQueue.DxFence->GetCompletedValue();
	const uint64_t DirectFrameFence = g_render.DirectQueue.DxFence->GetCompletedValue();
	const uint64_t ComputeFrameFence = g_render.ComputeQueue.DxFence->GetCompletedValue();

	for (int32_t i = (int32_t)g_FreeQueue.size() - 1; i >= 0; --i)
	{
		if (g_FreeQueue[i].CopyFence <= CopyFrameFence && g_FreeQueue[i].GraphicsFence <= DirectFrameFence && g_FreeQueue[i].ComputeFence <= ComputeFrameFence)
		{
			g_FreeQueue.erase(g_FreeQueue.begin() + i);
		}
	}

	for (int32_t i = (int32_t)g_UploadResources.size() - 1; i >= 0; --i)
	{
		if (g_UploadResources[i].CopyFence <= CopyFrameFence)
		{
			g_UploadResources.erase(g_UploadResources.begin() + i);
		}
	}
}

ID3D12Resource* Dx12_GetTextureResource(Texture_t tex)
{
	if (g_DxTextures.Valid(tex))
	{
		return g_DxTextures[tex].DxResource.Get();
	}
	
	return nullptr;
}

void Dx12_SetTextureResource(Texture_t tex, const ComPtr<ID3D12Resource>& resource)
{
	if (g_DxTextures.Valid(tex))
	{
		g_DxTextures[tex].DxResource = resource;
	}
}

void Dx12_TexturesMarkAsUsedByQueue(Texture_t tex, CommandListType type, uint64_t fenceValue)
{
	if (g_DxTextures.Valid(tex))
	{
		switch (type)
		{
		case CommandListType::COPY:		g_DxTextures[tex].CopyFence = fenceValue;		break;
		case CommandListType::GRAPHICS: g_DxTextures[tex].GraphicsFence = fenceValue;	break;
		case CommandListType::COMPUTE:	g_DxTextures[tex].ComputeFence = fenceValue;	break;
		}
	}
}

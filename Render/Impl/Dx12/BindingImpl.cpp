#include "Impl/BindingImpl.h"

#include "RenderImpl.h"
#include "Impl/TexturesImpl.h"
#include "Textures.h"
#include "Render/IDArray.h"

#include "SparseArray.h"

#include <queue>
#include <vector>

RENDER_TYPE(SRVUAV_t);

enum class DescriptorType
{
	SRV,
	UAV
};

struct SRVUAVDescriptor
{
	DescriptorType Type;
	union
	{
		D3D12_SHADER_RESOURCE_VIEW_DESC SrvDesc;
		D3D12_UNORDERED_ACCESS_VIEW_DESC UavDesc;
	};
	ID3D12Resource* Resource;
};

struct RTVDescriptor
{
	D3D12_RENDER_TARGET_VIEW_DESC RtvDesc;
	ID3D12Resource* Resource;
};

struct DSVDescriptor
{
	D3D12_DEPTH_STENCIL_VIEW_DESC DsvDesc;
	ID3D12Resource* Resource;
};

IDArray<SRVUAV_t, SRVUAVDescriptor> g_SrvUavDescriptors;
SparseArray<RTVDescriptor, RenderTargetView_t> g_RtvDescriptors;
SparseArray<DSVDescriptor, DepthStencilView_t> g_DsvDescriptors;

struct DescriptorHeaps
{
	uint32_t DescriptorCount = 0u;
	UINT DescriptorHandleIncrement = 0u;

	std::vector<Dx12DescriptorHeap> FreeHeaps;
	std::queue<Dx12DescriptorHeap> PendingDeletion;
	std::queue<Dx12DescriptorHeap> InFlightHeaps;

	virtual Dx12DescriptorHeap CreateHeap() = 0;

	void BeginFrame()
	{
		if (!InFlightHeaps.empty() || !PendingDeletion.empty())
		{
			const uint64_t directFenceCurrent = g_render.DirectQueue.DxFence->GetCompletedValue();
			const uint64_t computeFenceCurrent = g_render.ComputeQueue.DxFence->GetCompletedValue();

			while (!InFlightHeaps.empty())
			{
				if (InFlightHeaps.front().ComputeFenceValue < computeFenceCurrent && InFlightHeaps.front().DirectFenceValue < directFenceCurrent)
				{
					FreeHeaps.emplace_back(std::move(InFlightHeaps.front()));
					InFlightHeaps.pop();
				}
				else
				{
					break;
				}
			}

			while (!PendingDeletion.empty())
			{
				if (PendingDeletion.front().ComputeFenceValue <= computeFenceCurrent && PendingDeletion.front().DirectFenceValue <= directFenceCurrent)
				{
					PendingDeletion.pop();
				}
				else
				{
					break;
				}

				if (PendingDeletion.empty())
				{
					break;
				}
			}
		}
	}

	Dx12DescriptorHeap AccquireHeap()
	{
		Dx12DescriptorHeap heap = {};

		if (!FreeHeaps.empty())
		{
			heap = std::move(FreeHeaps.back());
			FreeHeaps.pop_back();
		}
		else
		{
			heap = CreateHeap();
		}

		return heap;
	}

	void HeapChanged(uint32_t desciptorCount)
	{
		FreeHeaps.clear();

		while(!InFlightHeaps.empty())
		{
			PendingDeletion.emplace(std::move(InFlightHeaps.front()));
			InFlightHeaps.pop();
		}

		DescriptorCount = desciptorCount;
	}
};

struct SrvUavDescriptorHeaps : public DescriptorHeaps
{
	virtual Dx12DescriptorHeap CreateHeap() override
	{
		Dx12DescriptorHeap heap = {};

		D3D12_DESCRIPTOR_HEAP_DESC desc = {};
		desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
		desc.NumDescriptors = DescriptorCount;
		desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
		desc.NodeMask = 1u;

		if (!DXENSURE(g_render.DxDevice->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&heap.DxHeap))))
		{
			assert(0 && "SrvUavDescriptorHeaps : CreateHeap failed");

			return {};
		}

		DescriptorHandleIncrement = g_render.DxDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

		D3D12_CPU_DESCRIPTOR_HANDLE handle = heap.DxHeap->GetCPUDescriptorHandleForHeapStart();

		D3D12_SHADER_RESOURCE_VIEW_DESC nullSrvDesc = {};
		nullSrvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
		nullSrvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		nullSrvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

		D3D12_UNORDERED_ACCESS_VIEW_DESC nullUavDesc = {};
		nullUavDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		nullUavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;

		g_SrvUavDescriptors.ForEachNullIfValid([&](SRVUAVDescriptor* descriptor) 
		{
			if (!descriptor)
			{
				g_render.DxDevice->CreateShaderResourceView(nullptr, &nullSrvDesc, handle);
			}
			else
			{
				if (descriptor->Type == DescriptorType::SRV)
				{
					g_render.DxDevice->CreateShaderResourceView(descriptor->Resource, descriptor->Resource ? &descriptor->SrvDesc : &nullSrvDesc, handle);
				}
				else if (descriptor->Type == DescriptorType::UAV)
				{
					g_render.DxDevice->CreateUnorderedAccessView(descriptor->Resource, nullptr, descriptor->Resource ? &descriptor->UavDesc : &nullUavDesc, handle);
				}
			}

			handle.ptr += DescriptorHandleIncrement;
		});

		return heap;
	}
};

struct RtvDescriptorHeap : public DescriptorHeaps
{
	virtual Dx12DescriptorHeap CreateHeap() override
	{
		Dx12DescriptorHeap heap = {};

		D3D12_DESCRIPTOR_HEAP_DESC desc = {};
		desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
		desc.NumDescriptors = DescriptorCount;
		desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		desc.NodeMask = 1u;

		if (!DXENSURE(g_render.DxDevice->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&heap.DxHeap))))
		{
			assert(0 && "RtvDescriptorHeap : CreateHeap failed");

			return {};
		}

		DescriptorHandleIncrement = g_render.DxDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

		D3D12_CPU_DESCRIPTOR_HANDLE handle = heap.DxHeap->GetCPUDescriptorHandleForHeapStart();

		D3D12_RENDER_TARGET_VIEW_DESC nullDesc = {};
		nullDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
		nullDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;

		for(const RTVDescriptor& descriptor : g_RtvDescriptors)
		{
			g_render.DxDevice->CreateRenderTargetView(descriptor.Resource, descriptor.Resource ? &descriptor.RtvDesc : &nullDesc, handle);

			handle.ptr += DescriptorHandleIncrement;
		}

		return heap;
	}
};

struct DsvDescriptorHeap : public DescriptorHeaps
{
	virtual Dx12DescriptorHeap CreateHeap() override
	{
		Dx12DescriptorHeap heap = {};

		D3D12_DESCRIPTOR_HEAP_DESC desc = {};
		desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
		desc.NumDescriptors = DescriptorCount;
		desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		desc.NodeMask = 1u;

		if (!DXENSURE(g_render.DxDevice->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&heap.DxHeap))))
		{
			assert(0 && "DsvDescriptorHeap : CreateHeap failed");

			return {};
		}

		DescriptorHandleIncrement = g_render.DxDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);

		D3D12_CPU_DESCRIPTOR_HANDLE handle = heap.DxHeap->GetCPUDescriptorHandleForHeapStart();

		D3D12_DEPTH_STENCIL_VIEW_DESC nullDesc = {};
		nullDesc.Format = DXGI_FORMAT_D16_UNORM;
		nullDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;

		for(const DSVDescriptor& descriptor : g_DsvDescriptors)
		{
			g_render.DxDevice->CreateDepthStencilView(descriptor.Resource, descriptor.Resource ? &descriptor.DsvDesc : &nullDesc, handle);

			handle.ptr += DescriptorHandleIncrement;
		}

		return heap;
	}
};

SrvUavDescriptorHeaps g_SrvUavHeap;
RtvDescriptorHeap g_RtvHeap;
DsvDescriptorHeap g_DsvHeap;

SparseArray<SRVUAV_t, ShaderResourceView_t> g_SrvDescriptorRemap;
SparseArray<SRVUAV_t, UnorderedAccessView_t> g_UavDescriptorRemap;

bool CreateTextureSRVImpl(ShaderResourceView_t srv, Texture_t tex, RenderFormat format, TextureDimension dim, uint32_t mipLevels, uint32_t depthOrArraySize)
{
	SRVUAV_t heapHandle = g_SrvUavDescriptors.Create();

	g_SrvDescriptorRemap.AllocCopy(srv, heapHandle);

	SRVUAVDescriptor& descriptor = g_SrvUavDescriptors[heapHandle];

	descriptor.Type = DescriptorType::SRV;
	descriptor.Resource = Dx12_GetTextureResource(tex);

	descriptor.SrvDesc.Format = Dx12_Format(format);	
	descriptor.SrvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

	if (dim == TextureDimension::Tex1D)
	{
		descriptor.SrvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE1D;
		descriptor.SrvDesc.Texture1D.MostDetailedMip = 0u;
		descriptor.SrvDesc.Texture1D.MipLevels = mipLevels;
		descriptor.SrvDesc.Texture1D.ResourceMinLODClamp = 0.0f;
	}
	else if (dim == TextureDimension::Tex1DArray)
	{
		descriptor.SrvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE1DARRAY;
		descriptor.SrvDesc.Texture1DArray.MostDetailedMip = 0u;
		descriptor.SrvDesc.Texture1DArray.MipLevels = mipLevels;
		descriptor.SrvDesc.Texture1DArray.ArraySize = depthOrArraySize;
		descriptor.SrvDesc.Texture1DArray.ResourceMinLODClamp = 0.0f;
	}
	else if (dim == TextureDimension::Tex2D)
	{
		descriptor.SrvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		descriptor.SrvDesc.Texture2D.MostDetailedMip = 0u;
		descriptor.SrvDesc.Texture2D.MipLevels = mipLevels;
		descriptor.SrvDesc.Texture2D.PlaneSlice = 0u;
		descriptor.SrvDesc.Texture2D.ResourceMinLODClamp = 0.0f;
	}
	else if (dim == TextureDimension::Tex2DArray)
	{
		descriptor.SrvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
		descriptor.SrvDesc.Texture2DArray.MostDetailedMip = 0u;
		descriptor.SrvDesc.Texture2DArray.MipLevels = mipLevels;
		descriptor.SrvDesc.Texture2DArray.FirstArraySlice = 0u;
		descriptor.SrvDesc.Texture2DArray.ArraySize = depthOrArraySize;
		descriptor.SrvDesc.Texture2DArray.PlaneSlice = 0u;
		descriptor.SrvDesc.Texture2DArray.ResourceMinLODClamp = 0.0f;
	}
	else if (dim == TextureDimension::Cubemap)
	{
		descriptor.SrvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
		descriptor.SrvDesc.TextureCube.MostDetailedMip = 0u;
		descriptor.SrvDesc.TextureCube.MipLevels = mipLevels;
		descriptor.SrvDesc.TextureCube.ResourceMinLODClamp = 0.0f;
	}
	else if (dim == TextureDimension::CubemapArray)
	{
		descriptor.SrvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBEARRAY;
		descriptor.SrvDesc.TextureCubeArray.MostDetailedMip = 0u;
		descriptor.SrvDesc.TextureCubeArray.MipLevels = mipLevels;
		descriptor.SrvDesc.TextureCubeArray.First2DArrayFace = 0u;
		descriptor.SrvDesc.TextureCubeArray.NumCubes = depthOrArraySize;
		descriptor.SrvDesc.TextureCubeArray.ResourceMinLODClamp = 0.0f;
	}
	else if (dim == TextureDimension::Tex3D)
	{
		descriptor.SrvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE3D;
		descriptor.SrvDesc.Texture3D.MostDetailedMip = 0u;
		descriptor.SrvDesc.Texture3D.MipLevels = mipLevels;
		descriptor.SrvDesc.Texture3D.ResourceMinLODClamp = 0.0f;
	}
	else
	{
		g_SrvUavDescriptors.Release(heapHandle);
		g_SrvDescriptorRemap.Free(srv);
		return false;
	}

	g_SrvUavHeap.HeapChanged((uint32_t)g_SrvUavDescriptors.Size());

	return true;
}

bool CreateTextureUAVImpl(UnorderedAccessView_t uav, Texture_t tex, RenderFormat format, TextureDimension dim, uint32_t depthOrArraySize)
{
	SRVUAV_t heapHandle = g_SrvUavDescriptors.Create();

	g_UavDescriptorRemap.AllocCopy(uav, heapHandle);

	SRVUAVDescriptor& descriptor = g_SrvUavDescriptors[heapHandle];

	descriptor.Type = DescriptorType::UAV;
	descriptor.Resource = Dx12_GetTextureResource(tex);
	descriptor.UavDesc.Format = Dx12_Format(format);
	
	if (dim == TextureDimension::Tex1D)
	{
		descriptor.UavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE1D;
		descriptor.UavDesc.Texture1D.MipSlice = 0u;
	}
	else if (dim == TextureDimension::Tex1DArray)
	{
		descriptor.UavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE1DARRAY;
		descriptor.UavDesc.Texture1DArray.MipSlice = 0u;
		descriptor.UavDesc.Texture1DArray.FirstArraySlice = 0u;
		descriptor.UavDesc.Texture1DArray.ArraySize = depthOrArraySize;		
	}
	else if (dim == TextureDimension::Tex2D)
	{
		descriptor.UavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
		descriptor.UavDesc.Texture2D.MipSlice = 0u;
		descriptor.UavDesc.Texture2D.PlaneSlice = 0u;
	}
	else if (dim == TextureDimension::Tex2DArray)
	{
		descriptor.UavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2DARRAY;
		descriptor.UavDesc.Texture2DArray.MipSlice = 0u;
		descriptor.UavDesc.Texture2DArray.FirstArraySlice = 0u;
		descriptor.UavDesc.Texture2DArray.ArraySize = depthOrArraySize;
		descriptor.UavDesc.Texture2DArray.PlaneSlice = 0u;
	}
	else if (dim == TextureDimension::Cubemap)
	{
		descriptor.UavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2DARRAY;
		descriptor.UavDesc.Texture2DArray.MipSlice = 0u;
		descriptor.UavDesc.Texture2DArray.FirstArraySlice = 0u;
		descriptor.UavDesc.Texture2DArray.ArraySize = 6u;
		descriptor.UavDesc.Texture2DArray.PlaneSlice = 0u;
	}
	else if (dim == TextureDimension::CubemapArray)
	{
		descriptor.UavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2DARRAY;
		descriptor.UavDesc.Texture2DArray.MipSlice = 0u;
		descriptor.UavDesc.Texture2DArray.FirstArraySlice = 0u;
		descriptor.UavDesc.Texture2DArray.ArraySize = 6u * depthOrArraySize;
		descriptor.UavDesc.Texture2DArray.PlaneSlice = 0u;
	}
	else if (dim == TextureDimension::Tex3D)
	{
		descriptor.UavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE3D;
		descriptor.UavDesc.Texture3D.MipSlice = 0u;
		descriptor.UavDesc.Texture3D.FirstWSlice = 0u;
		descriptor.UavDesc.Texture3D.WSize = -1;
	}
	else
	{
		g_SrvUavDescriptors.Release(heapHandle);
		g_UavDescriptorRemap.Free(uav);

		return false;
	}

	g_SrvUavHeap.HeapChanged((uint32_t)g_SrvUavDescriptors.Size());

	return true;
}

bool CreateTextureRTVImpl(RenderTargetView_t rtv, Texture_t tex, RenderFormat format, TextureDimension dim, uint32_t depthOrArraySize)
{
	RTVDescriptor& descriptor = g_RtvDescriptors.Alloc(rtv);

	descriptor.Resource = Dx12_GetTextureResource(tex);

	descriptor.RtvDesc.Format = Dx12_Format(format);

	if (dim == TextureDimension::Tex1D)
	{
		descriptor.RtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE1D;
		descriptor.RtvDesc.Texture1D.MipSlice = 0u;
	}
	else if (dim == TextureDimension::Tex1DArray)
	{
		descriptor.RtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE1DARRAY;
		descriptor.RtvDesc.Texture1DArray.MipSlice = 0u;
		descriptor.RtvDesc.Texture1DArray.FirstArraySlice = 0u;
		descriptor.RtvDesc.Texture1DArray.ArraySize = depthOrArraySize;
	}
	else if (dim == TextureDimension::Tex2D)
	{
		descriptor.RtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
		descriptor.RtvDesc.Texture2D.MipSlice = 0u;
		descriptor.RtvDesc.Texture2D.PlaneSlice = 0u;
	}
	else if (dim == TextureDimension::Tex2DArray)
	{
		descriptor.RtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DARRAY;
		descriptor.RtvDesc.Texture2DArray.MipSlice = 0u;
		descriptor.RtvDesc.Texture2DArray.FirstArraySlice = 0u;
		descriptor.RtvDesc.Texture2DArray.ArraySize = depthOrArraySize;
		descriptor.RtvDesc.Texture2DArray.PlaneSlice = 0u;
	}
	else if (dim == TextureDimension::Cubemap)
	{
		descriptor.RtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DARRAY;
		descriptor.RtvDesc.Texture2DArray.MipSlice = 0u;
		descriptor.RtvDesc.Texture2DArray.FirstArraySlice = 0u;
		descriptor.RtvDesc.Texture2DArray.ArraySize = 6u;
		descriptor.RtvDesc.Texture2DArray.PlaneSlice = 0u;
	}
	else if (dim == TextureDimension::CubemapArray)
	{
		descriptor.RtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DARRAY;
		descriptor.RtvDesc.Texture2DArray.MipSlice = 0u;
		descriptor.RtvDesc.Texture2DArray.FirstArraySlice = 0u;
		descriptor.RtvDesc.Texture2DArray.ArraySize = 6u * depthOrArraySize;
		descriptor.RtvDesc.Texture2DArray.PlaneSlice = 0u;
	}
	else if (dim == TextureDimension::Tex3D)
	{
		descriptor.RtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE3D;
		descriptor.RtvDesc.Texture3D.MipSlice = 0u;
		descriptor.RtvDesc.Texture3D.FirstWSlice = 0u;
		descriptor.RtvDesc.Texture3D.WSize = -1;
	}
	else
	{
		g_RtvDescriptors.Free(rtv);

		return false;
	}

	g_RtvHeap.HeapChanged((uint32_t)g_RtvDescriptors.Size());

	return true;
}

bool CreateTextureDSVImpl(DepthStencilView_t dsv, Texture_t tex, RenderFormat format, TextureDimension dim, uint32_t depthOrArraySize)
{
	DSVDescriptor& descriptor = g_DsvDescriptors.Alloc(dsv);

	descriptor.Resource = Dx12_GetTextureResource(tex);

	descriptor.DsvDesc.Format = Dx12_Format(format);

	if (dim == TextureDimension::Tex1D)
	{
		descriptor.DsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE1D;
		descriptor.DsvDesc.Texture1D.MipSlice = 0u;
	}
	else if (dim == TextureDimension::Tex1DArray)
	{
		descriptor.DsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE1DARRAY;
		descriptor.DsvDesc.Texture1DArray.MipSlice = 0u;
		descriptor.DsvDesc.Texture1DArray.FirstArraySlice = 0u;
		descriptor.DsvDesc.Texture1DArray.ArraySize = depthOrArraySize;
	}
	else if (dim == TextureDimension::Tex2D)
	{
		descriptor.DsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
		descriptor.DsvDesc.Texture2D.MipSlice = 0u;
	}
	else if (dim == TextureDimension::Tex2DArray)
	{
		descriptor.DsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DARRAY;
		descriptor.DsvDesc.Texture2DArray.MipSlice = 0u;
		descriptor.DsvDesc.Texture2DArray.FirstArraySlice = 0u;
		descriptor.DsvDesc.Texture2DArray.ArraySize = depthOrArraySize;
	}
	else if (dim == TextureDimension::Cubemap)
	{
		descriptor.DsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DARRAY;
		descriptor.DsvDesc.Texture2DArray.MipSlice = 0u;
		descriptor.DsvDesc.Texture2DArray.FirstArraySlice = 0u;
		descriptor.DsvDesc.Texture2DArray.ArraySize = 6u;
	}
	else if (dim == TextureDimension::CubemapArray)
	{
		descriptor.DsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DARRAY;
		descriptor.DsvDesc.Texture2DArray.MipSlice = 0u;
		descriptor.DsvDesc.Texture2DArray.FirstArraySlice = 0u;
		descriptor.DsvDesc.Texture2DArray.ArraySize = 6u * depthOrArraySize;
	}
	else
	{
		g_DsvDescriptors.Free(dsv);

		return false;
	}

	g_DsvHeap.HeapChanged((uint32_t)g_DsvDescriptors.Size());

	return true;
}

void BindTextureSRVImpl(ShaderResourceView_t srv, Texture_t tex)
{
	SRVUAV_t handle = g_SrvDescriptorRemap[srv];
	SRVUAVDescriptor& descriptor = g_SrvUavDescriptors[handle];

	descriptor.Resource = Dx12_GetTextureResource(tex);

	g_SrvUavHeap.HeapChanged((uint32_t)g_SrvUavDescriptors.Size());
}

void BindTextureUAVImpl(UnorderedAccessView_t uav, Texture_t tex)
{
	SRVUAV_t handle = g_UavDescriptorRemap[uav];
	SRVUAVDescriptor& descriptor = g_SrvUavDescriptors[handle];

	descriptor.Resource = Dx12_GetTextureResource(tex);

	g_SrvUavHeap.HeapChanged((uint32_t)g_SrvUavDescriptors.Size());
}

void BindTextureRTVImpl(RenderTargetView_t rtv, Texture_t tex)
{
	RTVDescriptor& descriptor = g_RtvDescriptors[rtv];

	descriptor.Resource = Dx12_GetTextureResource(tex);

	g_RtvHeap.HeapChanged((uint32_t)g_RtvDescriptors.Size());
}

void BindTextureDSVImpl(DepthStencilView_t dsv, Texture_t tex)
{
	DSVDescriptor& descriptor = g_DsvDescriptors[dsv];

	descriptor.Resource = Dx12_GetTextureResource(tex);

	g_DsvHeap.HeapChanged((uint32_t)g_DsvDescriptors.Size());
}

bool CreateStructuredBufferSRVImpl(ShaderResourceView_t srv, StructuredBuffer_t buf, uint32_t firstElement, uint32_t numElements)
{
	return false;
}

bool CreateStructuredBufferUAVImpl(UnorderedAccessView_t uav, StructuredBuffer_t buf, uint32_t firstElement, uint32_t numElements)
{
	return false;
}

void DestroySRV(ShaderResourceView_t srv)
{
	g_SrvUavDescriptors.Release(g_SrvDescriptorRemap[srv]);

	g_SrvDescriptorRemap.Free(srv);

	g_SrvUavHeap.HeapChanged((uint32_t)g_SrvUavDescriptors.Size());
}

void DestroyUAV(UnorderedAccessView_t uav)
{
	g_SrvUavDescriptors.Release(g_UavDescriptorRemap[uav]);

	g_UavDescriptorRemap.Free(uav);

	g_SrvUavHeap.HeapChanged((uint32_t)g_SrvUavDescriptors.Size());
}

void DestroyRTV(RenderTargetView_t rtv)
{
	g_RtvDescriptors.Free(rtv);

	g_RtvHeap.HeapChanged((uint32_t)g_RtvDescriptors.Size());
}

void DestroyDSV(DepthStencilView_t dsv)
{
	g_DsvDescriptors.Free(dsv);

	g_DsvHeap.HeapChanged((uint32_t)g_DsvDescriptors.Size());
}

void Dx12_DescriptorsBeginFrame()
{
	g_SrvUavHeap.BeginFrame();
	g_RtvHeap.BeginFrame();
	g_DsvHeap.BeginFrame();
}

Dx12DescriptorHeap Dx12_AccquireSrvUavHeap()
{
	return g_SrvUavHeap.AccquireHeap();
}

Dx12DescriptorHeap Dx12_AccquireRtvHeap()
{
	return g_RtvHeap.AccquireHeap();
}

Dx12DescriptorHeap Dx12_AccquireDsvHeap()
{
	return g_DsvHeap.AccquireHeap();
}

D3D12_CPU_DESCRIPTOR_HANDLE Dx12_RtvDescriptorHandle(ID3D12DescriptorHeap* heap, RenderTargetView_t rtv)
{
	D3D12_CPU_DESCRIPTOR_HANDLE handle = heap->GetCPUDescriptorHandleForHeapStart();

	handle.ptr += (UINT)rtv * g_RtvHeap.DescriptorHandleIncrement;

	return handle;
}

D3D12_CPU_DESCRIPTOR_HANDLE Dx12_DsvDescriptorHandle(ID3D12DescriptorHeap* heap, DepthStencilView_t dsv)
{
	D3D12_CPU_DESCRIPTOR_HANDLE handle = heap->GetCPUDescriptorHandleForHeapStart();

	handle.ptr += (UINT)dsv * g_DsvHeap.DescriptorHandleIncrement;

	return handle;
}

D3D12_GPU_VIRTUAL_ADDRESS Dx12_GetSrvAddress(ID3D12DescriptorHeap* heap, ShaderResourceView_t srv)
{
	D3D12_GPU_VIRTUAL_ADDRESS ptr = heap->GetGPUDescriptorHandleForHeapStart().ptr;
	ptr += (UINT)srv * g_SrvUavHeap.DescriptorHandleIncrement;

	return ptr;
}

D3D12_GPU_VIRTUAL_ADDRESS Dx12_GetUavAddress(ID3D12DescriptorHeap* heap, UnorderedAccessView_t uav)
{
	D3D12_GPU_VIRTUAL_ADDRESS ptr = heap->GetGPUDescriptorHandleForHeapStart().ptr;
	ptr += (UINT)uav * g_SrvUavHeap.DescriptorHandleIncrement;

	return ptr;
}

D3D12_GPU_DESCRIPTOR_HANDLE Dx12_GetSrvUavTableHandle(ID3D12DescriptorHeap* heap)
{
	return heap->GetGPUDescriptorHandleForHeapStart();
}

void Dx12_SubmitSrvUavDescriptorHeap(Dx12DescriptorHeap&& heap, uint64_t graphicsFenceValue, uint64_t computeFenceValue)
{
	heap.DirectFenceValue = graphicsFenceValue;
	heap.ComputeFenceValue = computeFenceValue;

	g_SrvUavHeap.InFlightHeaps.emplace(std::move(heap));
}

void Dx12_SubmitRtvDescriptorHeap(Dx12DescriptorHeap&& heap, uint64_t graphicsFenceValue, uint64_t computeFenceValue)
{
	heap.DirectFenceValue = graphicsFenceValue;
	heap.ComputeFenceValue = computeFenceValue;

	g_RtvHeap.InFlightHeaps.emplace(std::move(heap));
}

void Dx12_SubmitDsvDescriptorHeap(Dx12DescriptorHeap&& heap, uint64_t graphicsFenceValue, uint64_t computeFenceValue)
{
	heap.DirectFenceValue = graphicsFenceValue;
	heap.ComputeFenceValue = computeFenceValue;

	g_DsvHeap.InFlightHeaps.emplace(std::move(heap));
}

uint32_t Binding_GetDescriptorIndexImpl(ShaderResourceView_t srv)
{
	return static_cast<uint32_t>(g_SrvDescriptorRemap[srv]);
}

uint32_t Binding_GetDescriptorIndexImpl(UnorderedAccessView_t uav)
{
	return static_cast<uint32_t>(g_UavDescriptorRemap[uav]);
}

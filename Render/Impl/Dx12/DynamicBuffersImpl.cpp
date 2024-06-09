#include "../BuffersImpl.h"

#include "RenderImpl.h"
#include "Utils/SurfMath.h"

#include <deque>

static const size_t AllocationPageSize = 2u * 1024u * 1024u;

struct DynamicAllocation
{
	void* pCpuMem = nullptr;
	D3D12_GPU_VIRTUAL_ADDRESS pGpuMem = (D3D12_GPU_VIRTUAL_ADDRESS)0;
	uint32_t Size = 0u;
};

struct DynamicUploadBuffer
{
	explicit DynamicUploadBuffer() {}

	static const size_t GetPageSize() { return AllocationPageSize; }

	DynamicAllocation Allocate(size_t sizeInBytes, size_t alignment)
	{
		assert(sizeInBytes < AllocationPageSize && "DynamicUploadBuffer::AllocateBuffer allocation exceeds max page size for dynamic buffers");

		if (!CurrentPage || !CurrentPage->HasSpace(sizeInBytes, alignment))
		{
			CurrentPage = RequestPage();
		}

		return CurrentPage->Allocate(sizeInBytes, alignment);
	}

	DynamicAllocation AllocateBuffer(size_t sizeInBytes)
	{
		return Allocate(sizeInBytes, D3D12_RAW_UAV_SRV_BYTE_ALIGNMENT);
	}

	DynamicAllocation AllocateConstantBuffer(size_t sizeInBytes)
	{
		return Allocate(sizeInBytes, D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);
	}

	void Reset(uint64_t frameFence)
	{
		CurrentPage = nullptr;
		AvailablePages = Pages;
		FrameFence = frameFence;

		for (auto& page : AvailablePages)
		{
			page->Reset();
		}
	}

	bool InFlight(uint64_t frameFence) const
	{
		return frameFence <= FrameFence;
	}

private:

	struct AllocationPage
	{
		AllocationPage()
		{
			DxRes = Dx12_CreateBuffer(AllocationPageSize, D3D12_HEAP_TYPE_UPLOAD, D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_FLAG_NONE);

			pGpuMem = DxRes->GetGPUVirtualAddress();
			DxRes->Map(0, nullptr, &pCpuMem);
		}

		~AllocationPage()
		{
			DxRes->Unmap(0, nullptr);
		}

		bool HasSpace(size_t sizeInBytes, size_t alignment) const
		{
			size_t alignedSize = AlignUp(sizeInBytes, alignment);
			size_t alignedOffset = AlignUp(Offset, alignment);

			return (alignedOffset + alignedSize) <= AllocationPageSize;
		}

		DynamicAllocation Allocate(size_t sizeInBytes, size_t alignment)
		{
			size_t alignedSize = AlignUp(sizeInBytes, alignment);
			Offset = AlignUp(Offset, alignment);

			DynamicAllocation alloc;
			alloc.pCpuMem = static_cast<uint8_t*>(pCpuMem) + Offset;
			alloc.pGpuMem = pGpuMem + Offset;
			alloc.Size = (uint32_t)alignedSize;

			Offset += alignedSize;

			return alloc;
		}

		void Reset()
		{
			Offset = 0;
		}

	private:
		ComPtr<ID3D12Resource> DxRes;

		void* pCpuMem = nullptr;
		D3D12_GPU_VIRTUAL_ADDRESS pGpuMem = (D3D12_GPU_VIRTUAL_ADDRESS)0;

		size_t Offset = 0;
	};

	using PagePool = std::deque<std::shared_ptr<AllocationPage>>;

	AllocationPage* RequestPage()
	{
		AllocationPage* page = nullptr;

		if (AvailablePages.empty())
		{
			Pages.emplace_back(std::make_shared<AllocationPage>());
			page = Pages.back().get();
		}
		else
		{
			page = AvailablePages.front().get();
			AvailablePages.pop_front();
		}

		return page;
	}

	PagePool Pages;
	PagePool AvailablePages;

	AllocationPage* CurrentPage = nullptr;

	uint64_t FrameFence = 0u;
};

DynamicUploadBuffer* g_dynamicUploadBuffer = nullptr;
std::vector<DynamicAllocation> g_dynamicBuffers;

std::vector<DynamicUploadBuffer> g_InFlightDynamicBuffers;
std::vector<DynamicUploadBuffer> g_AvailableDynamicBuffers;

DynamicBuffer_t CreateDynamicBuffer(const void* const data, size_t size, bool isConstantBuf)
{
	assert(g_dynamicUploadBuffer && "g_dynamicUploadBuffer null, DynamicBuffers_NewFrame not called");

	auto alloc = isConstantBuf ? g_dynamicUploadBuffer->AllocateConstantBuffer(size) : g_dynamicUploadBuffer->AllocateBuffer(size);

	memcpy(alloc.pCpuMem, data, size);

	g_dynamicBuffers.push_back(alloc);

	return (DynamicBuffer_t)(g_dynamicBuffers.size() - 1u);
}

DynamicBuffer_t CreateDynamicVertexBuffer(const void* const data, size_t size)
{
	return CreateDynamicBuffer(data, size, false);
}

DynamicBuffer_t CreateDynamicIndexBuffer(const void* const data, size_t size)
{
	return CreateDynamicBuffer(data, size, false);
}

DynamicBuffer_t CreateDynamicConstantBuffer(const void* const data, size_t size)
{
	return CreateDynamicBuffer(data, size, true);
}

void DynamicBuffers_NewFrame()
{
	g_dynamicBuffers.clear();

	const uint64_t frameFence = g_render.FrameFenceValue;

	for (int32_t i = (int32_t)g_InFlightDynamicBuffers.size() - 1; i >= 0; --i)
	{
		if (!g_InFlightDynamicBuffers[i].InFlight(frameFence))
		{
			g_AvailableDynamicBuffers.emplace_back(std::move(g_InFlightDynamicBuffers[i]));
			g_InFlightDynamicBuffers.erase(g_InFlightDynamicBuffers.begin() + i);
		}
	}

	if (g_AvailableDynamicBuffers.empty())
	{
		g_AvailableDynamicBuffers.resize(1);
	}

	g_InFlightDynamicBuffers.emplace_back(std::move(g_AvailableDynamicBuffers.back()));

	g_AvailableDynamicBuffers.pop_back();

	g_dynamicUploadBuffer = &g_InFlightDynamicBuffers.back();

	g_dynamicUploadBuffer->Reset(g_render.FrameFenceValue + 1);
}

D3D12_VERTEX_BUFFER_VIEW Dx12_GetVertexBufferView(DynamicBuffer_t db, uint32_t offset, uint32_t stride)
{
	assert((size_t)db < g_dynamicBuffers.size() && "Dx12_GetIndexBufferView invalid db");

	const DynamicAllocation& alloc = g_dynamicBuffers[(size_t)db];

	D3D12_VERTEX_BUFFER_VIEW view;
	view.BufferLocation = alloc.pGpuMem + (D3D12_GPU_VIRTUAL_ADDRESS)offset;
	view.SizeInBytes = (UINT)alloc.Size;
	view.StrideInBytes = (UINT)stride;

	return view;
}

D3D12_GPU_VIRTUAL_ADDRESS Dx12_GetCbvAddress(DynamicBuffer_t db)
{
	assert((size_t)db < g_dynamicBuffers.size() && "Dx12_GetIndexBufferView invalid db");

	const DynamicAllocation& alloc = g_dynamicBuffers[(size_t)db];

	return alloc.pGpuMem;
}

D3D12_INDEX_BUFFER_VIEW Dx12_GetIndexBufferView(DynamicBuffer_t db, RenderFormat format, uint32_t offset)
{
	assert((size_t)db < g_dynamicBuffers.size() && "Dx12_GetIndexBufferView invalid db");

	const DynamicAllocation& alloc = g_dynamicBuffers[(size_t)db];

	D3D12_INDEX_BUFFER_VIEW view;
	view.BufferLocation = alloc.pGpuMem + (D3D12_GPU_VIRTUAL_ADDRESS)offset;
	view.SizeInBytes = (UINT)alloc.Size;
	view.Format = Dx12_Format(format);

	return view;
}
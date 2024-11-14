#include "Impl/BuffersImpl.h"

#include "IDArray.h"
#include "RenderImpl.h"
#include "SparseArray.h"

#include <map>
#include <set>
#include <vector>

namespace tpr
{

static const size_t AllocationPageSize = 2u * 1024u * 1024u;

template<typename T>
static constexpr T AlignUpPowerOfTwo(T size, T alignment)
{
	const T mask = alignment - 1;
	return (size + mask) & ~mask;
}

static constexpr size_t AlignUp(size_t size, size_t alignment)
{
	const size_t remainder = size % alignment;
	return remainder ? size + (alignment - remainder) : size;
}

struct BufferAllocationBlock;
using FreeBlocksByOffsetMap = std::map<size_t, BufferAllocationBlock>;
using FreeBlocksBySizeMap = std::multimap<size_t, FreeBlocksByOffsetMap::iterator>;

struct BufferAllocation
{
	D3D12_GPU_VIRTUAL_ADDRESS pGPUMem = D3D12_GPU_VIRTUAL_ADDRESS{ 0 };

	size_t Offset = 0u;
	size_t Size = 0u;

	ID3D12Resource* pResource;

	bool SingleBuffer = false;
};

struct BufferAllocationUploadRequest
{
	BufferAllocation Allocation;

	ID3D12Resource* pDst = nullptr;
	ID3D12Resource* pSrc = nullptr;
};

std::vector<BufferAllocationUploadRequest> g_uploadRequests;
std::set<ID3D12Resource*> g_uploadTargetBuffers;

static void RequestUploadAlloc(const BufferAllocation& alloc, ID3D12Resource* pDst, ID3D12Resource* pSrc)
{
	g_uploadRequests.emplace_back(alloc, pDst, pSrc);
	g_uploadTargetBuffers.emplace(pDst);
}

struct BufferAllocationBlock
{
	FreeBlocksBySizeMap::iterator OrderBySizeIt;

	size_t Size() { return OrderBySizeIt->first; }
};

struct BufferAllocationPage
{
private:
	size_t FreeSize = AllocationPageSize;

	void* pCpuMemory = nullptr;
	D3D12_GPU_VIRTUAL_ADDRESS pGpuMemory = D3D12_GPU_VIRTUAL_ADDRESS{ 0 };

	ComPtr<ID3D12Resource> pUploadBuffer = nullptr;
	ComPtr<ID3D12Resource> pBuffer = nullptr;

	FreeBlocksByOffsetMap FreeBlocksByOffset;
	FreeBlocksBySizeMap FreeBlocksBySize;

	void AddFreeBlock(size_t offset, size_t size)
	{
		FreeBlocksByOffsetMap::iterator newBlockIt = FreeBlocksByOffset.emplace(offset, BufferAllocationBlock{}).first;
		FreeBlocksBySizeMap::iterator orderIt = FreeBlocksBySize.emplace(size, newBlockIt);

		newBlockIt->second.OrderBySizeIt = orderIt;
	}

public:

	BufferAllocationPage()
	{
		pUploadBuffer = Dx12_CreateBuffer(AllocationPageSize, D3D12_HEAP_TYPE_UPLOAD, D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_FLAG_NONE);
		pBuffer = Dx12_CreateBuffer(AllocationPageSize, D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_FLAG_NONE);

		pUploadBuffer->Map(0, nullptr, &pCpuMemory);

		pGpuMemory = pBuffer->GetGPUVirtualAddress();

		AddFreeBlock(0u, AllocationPageSize);
	}

	void Release()
	{
		pUploadBuffer->Unmap(0, nullptr);
		pCpuMemory = nullptr;
		pGpuMemory = D3D12_GPU_VIRTUAL_ADDRESS{ 0 };
	}

	D3D12_GPU_VIRTUAL_ADDRESS GetGpuAddress() { return pGpuMemory; }

	BufferAllocation Alloc(size_t size, size_t alignment, const void* const pData)
	{
		size_t alignedSize = AlignUp(size, alignment);

		FreeBlocksBySizeMap::const_iterator smallestBlockBySizeIt = FreeBlocksBySize.lower_bound(alignedSize);
		FreeBlocksByOffsetMap::const_iterator smallestBlockIt = FreeBlocksByOffset.cend();

		size_t alignedOffset = 0;
		size_t offsetDiff = 0;

		size_t blockOffset = 0;
		size_t blockSize = 0;

		while (smallestBlockBySizeIt != FreeBlocksBySize.cend())
		{
			smallestBlockIt = smallestBlockBySizeIt->second;

			blockSize = smallestBlockBySizeIt->first;
			blockOffset = smallestBlockIt->first;

			alignedOffset = AlignUp(blockOffset, alignment);

			// Calculate the change in alignment, may leave a small initial block.
			offsetDiff = alignedOffset - blockOffset;

			if ((offsetDiff + alignedSize) <= blockSize)
			{
				break;
			}

			smallestBlockBySizeIt++;
		}

		if (smallestBlockBySizeIt == FreeBlocksBySize.cend())
		{
			return {};
		}

		if (alignedOffset + alignedSize > AllocationPageSize)
		{
			__debugbreak();
		}

		FreeBlocksBySize.erase(smallestBlockBySizeIt);
		FreeBlocksByOffset.erase(smallestBlockIt);

		const size_t newOffset = alignedOffset + alignedSize;
		const size_t newSize = blockSize - alignedSize;

		if (offsetDiff > 0)
		{
			AddFreeBlock(blockOffset, offsetDiff);
		}

		if (newSize > 0)
		{
			AddFreeBlock(newOffset, newSize);
		}

		// Using size instead of aligned size is intentional so that the source alloc doesnt also need to be aligned.
		memcpy((uint8_t*)pCpuMemory + alignedOffset, pData, size);

		BufferAllocation alloc = { pGpuMemory, alignedOffset, alignedSize, pBuffer.Get()};

		RequestUploadAlloc(alloc, pBuffer.Get(), pUploadBuffer.Get());

		//FreeSize -= size;

		alloc.SingleBuffer = false;

		return alloc;
	}

	void Update(BufferAllocation alloc, const void* const pData)
	{
		memcpy((uint8_t*)pCpuMemory + alloc.Offset, pData, alloc.Size);

		RequestUploadAlloc(alloc, pBuffer.Get(), pUploadBuffer.Get());
	}

	void Free(BufferAllocation alloc)
	{
		FreeBlocksByOffsetMap::iterator nextBlockIt = FreeBlocksByOffset.upper_bound(alloc.Offset);
		FreeBlocksByOffsetMap::iterator prevBlockIt = nextBlockIt != FreeBlocksByOffset.begin() ? --nextBlockIt : FreeBlocksByOffset.end();

		size_t newSize, newOffset;

		if (prevBlockIt != FreeBlocksByOffset.end() && alloc.Offset == prevBlockIt->first + prevBlockIt->second.Size())
		{
			newSize = prevBlockIt->second.Size() + alloc.Size;
			newOffset = prevBlockIt->first;

			if (nextBlockIt != FreeBlocksByOffset.end() && alloc.Offset + alloc.Size == nextBlockIt->first)
			{
				newSize += nextBlockIt->second.Size();

				FreeBlocksBySize.erase(prevBlockIt->second.OrderBySizeIt);
				FreeBlocksBySize.erase(nextBlockIt->second.OrderBySizeIt);

				FreeBlocksByOffset.erase(prevBlockIt, ++nextBlockIt);
			}
			else
			{
				FreeBlocksBySize.erase(prevBlockIt->second.OrderBySizeIt);

				FreeBlocksByOffset.erase(prevBlockIt);
			}
		}
		else if (nextBlockIt != FreeBlocksByOffset.end() && alloc.Offset + alloc.Size == nextBlockIt->first)
		{
			newSize = alloc.Size + nextBlockIt->second.Size();
			newOffset = alloc.Offset;

			FreeBlocksBySize.erase(nextBlockIt->second.OrderBySizeIt);

			FreeBlocksByOffset.erase(nextBlockIt);
		}
		else
		{
			newSize = alloc.Size;
			newOffset = alloc.Offset;
		}

		AddFreeBlock(newOffset, newSize);

		//FreeSize += newSize;
	}
};

struct BufferAllocationSingleBuffer
{
private:
	size_t Size = 0;
	void* pCpuMemory = nullptr;
	D3D12_GPU_VIRTUAL_ADDRESS pGpuMemory = D3D12_GPU_VIRTUAL_ADDRESS{ 0 };

	ComPtr<ID3D12Resource> pUploadBuffer = nullptr;
	ComPtr<ID3D12Resource> pBuffer = nullptr;

public:

	BufferAllocationSingleBuffer(size_t size, bool uav)
		: Size(size)
	{
		pUploadBuffer = Dx12_CreateBuffer(AllocationPageSize, D3D12_HEAP_TYPE_UPLOAD, D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_FLAG_NONE);
		pBuffer = Dx12_CreateBuffer(AllocationPageSize, D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_COMMON, uav ? D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS : D3D12_RESOURCE_FLAG_NONE);

		pUploadBuffer->Map(0, nullptr, &pCpuMemory);

		pGpuMemory = pBuffer->GetGPUVirtualAddress();
	}

	void Release()
	{
		pUploadBuffer->Unmap(0, nullptr);
		pCpuMemory = nullptr;
		pGpuMemory = D3D12_GPU_VIRTUAL_ADDRESS{ 0 };
	}

	D3D12_GPU_VIRTUAL_ADDRESS GetGpuAddress() { return pGpuMemory; }

	BufferAllocation Alloc(size_t size, const void* const pData)
	{
		if (size > Size)
		{
			return BufferAllocation{};
		}

		memcpy((uint8_t*)pCpuMemory, pData, size);

		BufferAllocation alloc = { pGpuMemory, 0, size, pBuffer.Get(), true };

		RequestUploadAlloc(alloc, pBuffer.Get(), pUploadBuffer.Get());

		return alloc;
	}

	void Update(const void* const data)
	{
		memcpy((uint8_t*)pCpuMemory, data, Size);

		BufferAllocation alloc = { pGpuMemory, 0, Size, pBuffer.Get(), true};

		RequestUploadAlloc(alloc, pBuffer.Get(), pUploadBuffer.Get());
	}
};

struct BufferAllocationPool
{
	std::vector<std::unique_ptr<BufferAllocationPage>> Pages;
	std::vector<BufferAllocationSingleBuffer> SingleBuffers;

	BufferAllocation AllocSmallBuffer(size_t size, size_t alignment, const void* const pData)
	{
		assert(size > 0 && pData != nullptr);

		BufferAllocation alloc = {};

		for (std::unique_ptr<BufferAllocationPage>& page : Pages)
		{
			alloc = page->Alloc(size, alignment, pData);

			if (alloc.Size > 0)
			{
				return alloc;
			}
		}

		std::unique_ptr<BufferAllocationPage>& page = Pages.emplace_back(std::make_unique<BufferAllocationPage>());

		alloc = page->Alloc(size, alignment, pData);

		if (alloc.Size < size)
		{
			assert(0 && "Failed to allocate small buffer");
		}

		return alloc;
	}

	BufferAllocation AllocLargeBuffer(size_t size, const void* const pData)
	{
		assert(size > 0 && pData != nullptr);

		BufferAllocationSingleBuffer& buffer = SingleBuffers.emplace_back(size, false);

		return buffer.Alloc(size, pData);
	}

	BufferAllocation AllocRW(size_t size, const void* const pData)
	{
		assert(size > 0 && pData != nullptr);

		BufferAllocationSingleBuffer& buffer = SingleBuffers.emplace_back(size, true);

		return buffer.Alloc(size, pData);
	}

	BufferAllocation Alloc(size_t size, size_t alignment, const void* const pData)
	{
		assert(size > 0 && pData != nullptr);

		if (size < AllocationPageSize)
		{
			return AllocSmallBuffer(size, alignment, pData);
		}
		else
		{
			return AllocLargeBuffer(size, pData);
		}
	}

	void Update(BufferAllocation alloc, const void* const pData)
	{
		assert(alloc.Size > 0 && pData != nullptr);

		if (alloc.SingleBuffer)
		{
			auto it = SingleBuffers.begin();
			for (; it != SingleBuffers.end(); it++)
			{
				if (it->GetGpuAddress() == alloc.pGPUMem)
				{
					it->Update(pData);

					return;
				}
			}
		}
		else
		{
			auto it = Pages.begin();
			for (; it != Pages.end(); it++)
			{
				if (it->get()->GetGpuAddress() == alloc.pGPUMem)
				{
					it->get()->Update(alloc, pData);

					return;
				}
			}
		}

		assert(0 && "Trying to free a non-existent allocation");
	}

	void Free(BufferAllocation alloc)
	{
		if (alloc.SingleBuffer)
		{
			auto it = SingleBuffers.begin();
			for (; it != SingleBuffers.end(); it++)
			{
				if (it->GetGpuAddress() == alloc.pGPUMem)
				{
					break;
				}
			}

			if (it != SingleBuffers.end())
			{
				it->Release();
				SingleBuffers.erase(it);
			}
			else
			{
				assert(0 && "Trying to free a non-existent allocation");
			}
		}
		else
		{
			auto it = Pages.begin();
			for (; it != Pages.end(); it++)
			{
				if (it->get()->GetGpuAddress() == alloc.pGPUMem)
				{
					it->get()->Free(alloc);
				}
			}
		}
	}
};

BufferAllocationPool g_BufferAllocator;

SparseArray<BufferAllocation, VertexBuffer_t> g_DxVertexBuffers;
SparseArray<BufferAllocation, IndexBuffer_t> g_DxIndexBuffers;
SparseArray<BufferAllocation, StructuredBuffer_t> g_DxStructuredBuffers;
SparseArray<BufferAllocation, ConstantBuffer_t> g_DxConstantBuffers;

bool CreateVertexBufferImpl(VertexBuffer_t vb, const void* const data, size_t size)
{
	g_DxVertexBuffers.Alloc(vb);

	g_DxVertexBuffers[vb] = g_BufferAllocator.Alloc(size, D3D12_RAW_UAV_SRV_BYTE_ALIGNMENT, data);

	return g_DxVertexBuffers[vb].pGPUMem != 0;
}

bool CreateIndexBufferImpl(IndexBuffer_t ib, const void* const data, size_t size)
{
	g_DxIndexBuffers.Alloc(ib);

	g_DxIndexBuffers[ib] = g_BufferAllocator.Alloc(size, D3D12_RAW_UAV_SRV_BYTE_ALIGNMENT, data);

	return g_DxIndexBuffers[ib].pGPUMem != 0;
}

bool CreateStructuredBufferImpl(StructuredBuffer_t sb, const void* const data, size_t size, size_t stride, RenderResourceFlags flags)
{
	g_DxStructuredBuffers.Alloc(sb);

	if (HasEnumFlags(flags, RenderResourceFlags::UAV))
	{
		g_DxStructuredBuffers[sb] = g_BufferAllocator.AllocRW(size, data);
	}
	else
	{
		size_t alignment = stride < D3D12_RAW_UAV_SRV_BYTE_ALIGNMENT ? D3D12_RAW_UAV_SRV_BYTE_ALIGNMENT : stride;

		g_DxStructuredBuffers[sb] = g_BufferAllocator.Alloc(size, alignment, data);
	}

	return g_DxStructuredBuffers[sb].pGPUMem != 0;
}

bool CreateConstantBufferImpl(ConstantBuffer_t cb, const void* const data, size_t size)
{
	g_DxConstantBuffers.Alloc(cb);

	g_DxConstantBuffers[cb] = g_BufferAllocator.Alloc(size, D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT, data);

	return g_DxConstantBuffers[cb].pGPUMem != 0;
}

void UpdateVertexBufferImpl(VertexBuffer_t vb, const void* const data, size_t size)
{
	g_BufferAllocator.Update(g_DxVertexBuffers[vb], data);
}

void UpdateIndexBufferImpl(IndexBuffer_t ib, const void* const data, size_t size)
{
	g_BufferAllocator.Update(g_DxIndexBuffers[ib], data);
}

void UpdateConstantBufferImpl(ConstantBuffer_t cb, const void* const data, size_t size)
{
	g_BufferAllocator.Update(g_DxConstantBuffers[cb], data);
}

void UpdateStructuredBufferImpl(StructuredBuffer_t sb, const void* const data, size_t size)
{
	g_BufferAllocator.Update(g_DxStructuredBuffers[sb], data);
}

void DestroyVertexBuffer(VertexBuffer_t vb)
{
	g_BufferAllocator.Free(g_DxVertexBuffers[vb]);
	g_DxVertexBuffers.Free(vb);
}

void DestroyIndexBuffer(IndexBuffer_t ib)
{
	g_BufferAllocator.Free(g_DxIndexBuffers[ib]);
	g_DxIndexBuffers.Free(ib);
}

void DestroyStructuredBuffer(StructuredBuffer_t sb)
{
	g_BufferAllocator.Free(g_DxStructuredBuffers[sb]);
	g_DxStructuredBuffers.Free(sb);
}

void DestroyConstantBuffer(ConstantBuffer_t cb)
{
	g_BufferAllocator.Free(g_DxConstantBuffers[cb]);
	g_DxConstantBuffers.Free(cb);
}

void UploadBuffers(CommandList* cl)
{
	if (g_uploadRequests.empty())
		return;

	ID3D12GraphicsCommandList* dxcl = Dx12_GetCommandList(cl);

	std::vector<D3D12_RESOURCE_BARRIER> barriers;
	barriers.reserve(g_uploadTargetBuffers.size());

	for (ID3D12Resource* pDst : g_uploadTargetBuffers)
	{
		barriers.emplace_back(Dx12_TransitionBarrier(pDst, D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_COPY_DEST));
	}

	dxcl->ResourceBarrier((UINT)barriers.size(), barriers.data());

	for (BufferAllocationUploadRequest& request : g_uploadRequests)
	{
		dxcl->CopyBufferRegion(request.pDst, request.Allocation.Offset, request.pSrc, request.Allocation.Offset, request.Allocation.Size);
	}

	for (D3D12_RESOURCE_BARRIER& barrier : barriers)
	{
		barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
		barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_GENERIC_READ;
	}

	dxcl->ResourceBarrier((UINT)barriers.size(), barriers.data());

	g_uploadRequests.clear();
	g_uploadTargetBuffers.clear();
}

D3D12_VERTEX_BUFFER_VIEW Dx12_GetVertexBufferView(VertexBuffer_t vb, uint32_t offset, uint32_t stride)
{
	if (!g_DxVertexBuffers.Valid(vb))
	{
		return {};
	}

	const BufferAllocation& alloc = g_DxVertexBuffers[vb];

	D3D12_VERTEX_BUFFER_VIEW view;
	view.BufferLocation = alloc.pGPUMem + (D3D12_GPU_VIRTUAL_ADDRESS)alloc.Offset + (D3D12_GPU_VIRTUAL_ADDRESS)offset;
	view.SizeInBytes = (UINT)alloc.Size;
	view.StrideInBytes = (UINT)stride;

	return view;
}

D3D12_INDEX_BUFFER_VIEW Dx12_GetIndexBufferView(IndexBuffer_t ib, RenderFormat format, uint32_t offset)
{
	if (!g_DxIndexBuffers.Valid(ib))
	{
		return {};
	}

	const BufferAllocation& alloc = g_DxIndexBuffers[ib];

	D3D12_INDEX_BUFFER_VIEW view;
	view.BufferLocation = alloc.pGPUMem + (D3D12_GPU_VIRTUAL_ADDRESS)alloc.Offset + (D3D12_GPU_VIRTUAL_ADDRESS)offset;
	view.SizeInBytes = (UINT)alloc.Size;
	view.Format = Dx12_Format(format);

	return view;
}

D3D12_GPU_VIRTUAL_ADDRESS Dx12_GetCbvAddress(ConstantBuffer_t cb)
{
	if (!g_DxConstantBuffers.Valid(cb))
	{
		return {};
	}

	const BufferAllocation& alloc = g_DxConstantBuffers[cb];

	return alloc.pGPUMem + (D3D12_GPU_VIRTUAL_ADDRESS)alloc.Offset;
}

ID3D12Resource* Dx12_GetBufferResource(StructuredBuffer_t sb)
{
	if (!g_DxStructuredBuffers.Valid(sb))
	{
		return {};
	}

	const BufferAllocation& alloc = g_DxStructuredBuffers[sb];

	return alloc.pResource;
}

uint32_t Dx12_GetBufferOffset(StructuredBuffer_t sb)
{
	if (!g_DxStructuredBuffers.Valid(sb))
	{
		return {};
	}

	const BufferAllocation& alloc = g_DxStructuredBuffers[sb];

	return (uint32_t)alloc.Offset;
}

}
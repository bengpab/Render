#include "Buffers.h"
#include "IDArray.h"

#include "Impl/BuffersImpl.h"

namespace rl
{

struct BufferData
{
	size_t size;
	BufferData() = default;
	BufferData(size_t s)
		: size(s)
	{}
};

IDArray<VertexBuffer_t, BufferData> g_VertexBuffers;
IDArray<IndexBuffer_t, BufferData> g_IndexBuffers;
IDArray<StructuredBuffer_t, BufferData> g_StructuredBuffers;
IDArray<ConstantBuffer_t, BufferData> g_ConstantBuffers;

VertexBuffer_t CreateVertexBuffer(const void* const data, size_t size)
{
	VertexBuffer_t newBuf = g_VertexBuffers.Create(size);

	if (!CreateVertexBufferImpl(newBuf, data, size))
	{
		g_VertexBuffers.Release(newBuf);
		return VertexBuffer_t::INVALID;
	}

	return newBuf;
}

IndexBuffer_t CreateIndexBuffer(const void* const data, size_t size)
{
	IndexBuffer_t newBuf = g_IndexBuffers.Create(size);

	if (!CreateIndexBufferImpl(newBuf, data, size))
	{
		g_IndexBuffers.Release(newBuf);
		return IndexBuffer_t::INVALID;
	}

	return newBuf;
}

StructuredBuffer_t CreateStructuredBuffer(const void* const data, size_t size, size_t stride, RenderResourceFlags flags)
{
	StructuredBuffer_t newBuf = g_StructuredBuffers.Create();

	if (!CreateStructuredBufferImpl(newBuf, data, size, stride, flags))
	{
		g_StructuredBuffers.Release(newBuf);
		return StructuredBuffer_t::INVALID;
	}

	return newBuf;
}

ConstantBuffer_t CreateConstantBuffer(const void* const data, size_t size)
{
	ConstantBuffer_t newBuf = g_ConstantBuffers.Create(size);

	if (!CreateConstantBufferImpl(newBuf, data, size))
	{
		g_ConstantBuffers.Release(newBuf);
		return ConstantBuffer_t::INVALID;
	}

	return newBuf;
}

void UpdateVertexBuffer(VertexBuffer_t vb, const void* const data, size_t size)
{
	if (g_VertexBuffers.Valid(vb))
	{
		UpdateVertexBufferImpl(vb, data, size);
	}
}

void UpdateIndexBuffer(IndexBuffer_t ib, const void* const data, size_t size)
{
	if (g_IndexBuffers.Valid(ib))
	{
		UpdateIndexBufferImpl(ib, data, size);
	}
}

void UpdateConstantBuffer(ConstantBuffer_t cb, const void* const data, size_t size)
{
	if (g_ConstantBuffers.Valid(cb))
	{
		UpdateConstantBufferImpl(cb, data, size);
	}
}

void UpdateStructuredBuffer(StructuredBuffer_t sb, const void* const data, size_t size)
{
	if (g_StructuredBuffers.Valid(sb))
	{
		UpdateStructuredBufferImpl(sb, data, size);
	}
}

void RenderRelease(VertexBuffer_t vb)
{
	if (g_VertexBuffers.Release(vb))
	{
		DestroyVertexBuffer(vb);
	}
}

void RenderRelease(IndexBuffer_t ib)
{
	if (g_IndexBuffers.Release(ib))
	{
		DestroyIndexBuffer(ib);
	}
}

void RenderRelease(StructuredBuffer_t sb)
{
	if (g_StructuredBuffers.Release(sb))
	{
		DestroyStructuredBuffer(sb);
	}
}

void RenderRelease(ConstantBuffer_t cb)
{
	if (g_ConstantBuffers.Release(cb))
	{
		DestroyConstantBuffer(cb);
	}
}

void RenderRef(VertexBuffer_t vb)
{
	g_VertexBuffers.AddRef(vb);
}

void RenderRef(IndexBuffer_t ib)
{
	g_IndexBuffers.AddRef(ib);
}

void RenderRef(StructuredBuffer_t sb)
{
	g_StructuredBuffers.AddRef(sb);
}

void RenderRef(ConstantBuffer_t cb)
{
	g_ConstantBuffers.AddRef(cb);
}

size_t GetVertexBufferCount()
{
	return g_VertexBuffers.UsedSize();
}

size_t GetIndexBufferCount()
{
	return g_IndexBuffers.UsedSize();
}

size_t GetStructuredBufferCount()
{
	return g_StructuredBuffers.UsedSize();
}

size_t GetConstantBufferCount()
{
	return g_ConstantBuffers.UsedSize();
}

}
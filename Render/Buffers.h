#pragma once

#include "RenderTypes.h"

namespace tpr
{

RENDER_TYPE(VertexBuffer_t);
RENDER_TYPE(IndexBuffer_t);
RENDER_TYPE(StructuredBuffer_t);
RENDER_TYPE(ConstantBuffer_t);
RENDER_TYPE(DynamicBuffer_t);

VertexBuffer_t CreateVertexBuffer(const void* const data, size_t size);
IndexBuffer_t CreateIndexBuffer(const void* const data, size_t size);
StructuredBuffer_t CreateStructuredBuffer(const void* const data, size_t size, size_t stride, RenderResourceFlags flags);
ConstantBuffer_t CreateConstantBuffer(const void* const data, size_t size);

DynamicBuffer_t CreateDynamicVertexBuffer(const void* const data, size_t size);
DynamicBuffer_t CreateDynamicIndexBuffer(const void* const data, size_t size);
DynamicBuffer_t CreateDynamicConstantBuffer(const void* const data, size_t size);

void UpdateVertexBuffer(VertexBuffer_t vb, const void* const data, size_t size);
void UpdateIndexBuffer(IndexBuffer_t ib, const void* const data, size_t size);
void UpdateConstantBuffer(ConstantBuffer_t cb, const void* const data, size_t size);
void UpdateStructuredBuffer(StructuredBuffer_t sb, const void* const data, size_t size);

void RenderRelease(VertexBuffer_t vb);
void RenderRelease(IndexBuffer_t ib);
void RenderRelease(StructuredBuffer_t sb);
void RenderRelease(ConstantBuffer_t cb);

void RenderRef(VertexBuffer_t vb);
void RenderRef(IndexBuffer_t ib);
void RenderRef(StructuredBuffer_t sb);
void RenderRef(ConstantBuffer_t cb);

void DynamicBuffers_NewFrame();
void DynamicBuffers_EndFrame();

struct CommandList;
void UploadBuffers(CommandList* cl);

size_t GetVertexBufferCount();
size_t GetIndexBufferCount();
size_t GetStructuredBufferCount();
size_t GetConstantBufferCount();

}
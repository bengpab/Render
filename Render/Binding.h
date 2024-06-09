#pragma once

#include "RenderTypes.h"

RENDER_TYPE(ShaderResourceView_t);
RENDER_TYPE(UnorderedAccessView_t);
RENDER_TYPE(RenderTargetView_t);
RENDER_TYPE(DepthStencilView_t);

FWD_RENDER_TYPE(Texture_t);
FWD_RENDER_TYPE(StructuredBuffer_t);

enum class TextureDimension : uint8_t;

ShaderResourceView_t CreateTextureSRV(Texture_t tex, RenderFormat format, TextureDimension dim, uint32_t mipLevels, uint32_t depthOrArraySize);
UnorderedAccessView_t CreateTextureUAV(Texture_t tex, RenderFormat format, TextureDimension dim, uint32_t depthOrArraySize);
RenderTargetView_t CreateTextureRTV(Texture_t tex, RenderFormat format, TextureDimension dim, uint32_t depthOrArraySize);
DepthStencilView_t CreateTextureDSV(Texture_t tex, RenderFormat format, TextureDimension dim, uint32_t depthOrArraySize);

ShaderResourceView_t CreateStructuredBufferSRV(StructuredBuffer_t buf, uint32_t firstElem, uint32_t numElems);
UnorderedAccessView_t CreateStructuredBufferUAV(StructuredBuffer_t buf, uint32_t firstElem, uint32_t numElems);

//TODO: Buffer SRV and UAV
ShaderResourceView_t AllocSRV(RenderFormat format, TextureDimension dim, uint32_t mipLevels, uint32_t depthOrArraySize);
UnorderedAccessView_t AllocUAV(RenderFormat format, TextureDimension dim, uint32_t depthOrArraySize);
RenderTargetView_t AllocRTV(RenderFormat format, TextureDimension dim, uint32_t depthOrArraySize);
DepthStencilView_t AllocDSV(RenderFormat format, TextureDimension dim, uint32_t depthOrArraySize);

void BindSRV(ShaderResourceView_t srv, Texture_t texture);
void BindUAV(UnorderedAccessView_t uav, Texture_t texture);
void BindRTV(RenderTargetView_t rtv, Texture_t texture);
void BindDSV(DepthStencilView_t dsv, Texture_t texture);

RenderFormat GetSRVFormat(ShaderResourceView_t srv);
RenderFormat GetUAVFormat(UnorderedAccessView_t uav);
RenderFormat GetRTVFormat(RenderTargetView_t rtv);
RenderFormat GetDSVFormat(DepthStencilView_t dsv);

void ReleaseSRV(ShaderResourceView_t srv);
void ReleaseUAV(UnorderedAccessView_t uav);
void ReleaseRTV(RenderTargetView_t rtv);
void ReleaseDSV(DepthStencilView_t dsv);

void Render_Release(ShaderResourceView_t srv);
void Render_Release(UnorderedAccessView_t uav);
void Render_Release(RenderTargetView_t rtv);
void Render_Release(DepthStencilView_t dsv);

size_t Bindings_GetShaderResourceViewCount();
size_t Bindings_GetUnorderedAccessViewCount();
size_t Bindings_GetRenderTargetViewCount();
size_t Bindings_GetDepthStencilViewCount();

uint32_t Binding_GetDescriptorIndex(ShaderResourceView_t srv);
uint32_t Binding_GetDescriptorIndex(UnorderedAccessView_t uav);
#include "Binding.h"
#include "Textures.h"
#include "Impl/BindingImpl.h"
#include "IDArray.h"

// TODO: Bindings should hold resource references, currently its the other way round

enum class ViewResourceType : uint8_t
{
	Unknown = 0,
	Texture,
	StructuredBuffer
};

struct TextureViewData
{
	Texture_t Handle;
	RenderFormat Format;
	uint32_t MipLevels;
	uint32_t DepthOrArraySize;
	TextureViewData(Texture_t inTexture, RenderFormat inFormat, uint32_t inDepthOrArraySize = 1, uint32_t mipLevels = uint32_t(-1))
		: Handle(inTexture)
		, Format(inFormat)
		, MipLevels(mipLevels)
		, DepthOrArraySize(inDepthOrArraySize)
	{}
};

struct BufferViewData
{
	StructuredBuffer_t Handle;
	uint32_t FirstElem;
	uint32_t NumElems;
	BufferViewData(StructuredBuffer_t inBuffer, uint32_t inFirstElem, uint32_t inNumElems)
		: Handle(inBuffer)
		, FirstElem(inFirstElem)
		, NumElems(inNumElems)
	{}
};

struct ViewData
{
	ViewResourceType Type = ViewResourceType::Unknown;
	union
	{
		TextureViewData Texture;
		BufferViewData Buffer;
	};

	ViewData()
		: Type(ViewResourceType::Unknown)
	{}

	ViewData(Texture_t tex, RenderFormat format, uint32_t depthOrArraySize)
		: Texture(tex, format, depthOrArraySize)
		, Type(ViewResourceType::Texture)
	{}

	ViewData(StructuredBuffer_t buf, uint32_t firstElem, uint32_t numElems)
		: Buffer(buf, firstElem, numElems)
		, Type(ViewResourceType::StructuredBuffer)
	{}
};

IDArray<ShaderResourceView_t, ViewData> g_SRVs;
IDArray<UnorderedAccessView_t, ViewData> g_UAVs;
IDArray<RenderTargetView_t, ViewData> g_RTVs;
IDArray<DepthStencilView_t, ViewData> g_DSVs;

ShaderResourceView_t CreateTextureSRV(Texture_t tex, RenderFormat format, TextureDimension dim, uint32_t mipLevels, uint32_t depthOrArraySize)
{
	ShaderResourceView_t srv = g_SRVs.Create(ViewData(tex, format, depthOrArraySize));

	if (!CreateTextureSRVImpl(srv, tex, format, dim, mipLevels, depthOrArraySize))
	{
		g_SRVs.Release(srv);
		return ShaderResourceView_t::INVALID;
	}

	return srv;
}

UnorderedAccessView_t CreateTextureUAV(Texture_t tex, RenderFormat format, TextureDimension dim, uint32_t depthOrArraySize)
{
	UnorderedAccessView_t uav = g_UAVs.Create(ViewData(tex, format, depthOrArraySize));

	if (!CreateTextureUAVImpl(uav, tex, format, dim, depthOrArraySize))
	{
		g_UAVs.Release(uav);
		return UnorderedAccessView_t::INVALID;
	}

	return uav;
}

RenderTargetView_t CreateTextureRTV(Texture_t tex, RenderFormat format, TextureDimension dim, uint32_t depthOrArraySize)
{
	RenderTargetView_t rtv = g_RTVs.Create(ViewData(tex, format, depthOrArraySize));

	if (!CreateTextureRTVImpl(rtv, tex, format, dim, depthOrArraySize))
	{
		g_RTVs.Release(rtv);
		return RenderTargetView_t::INVALID;
	}

	return rtv;
}

DepthStencilView_t CreateTextureDSV(Texture_t tex, RenderFormat format, TextureDimension dim, uint32_t depthOrArraySize)
{
	DepthStencilView_t dsv = g_DSVs.Create(ViewData(tex, format, depthOrArraySize));

	if (!CreateTextureDSVImpl(dsv, tex, format, dim, depthOrArraySize))
	{
		g_DSVs.Release(dsv);
		return DepthStencilView_t::INVALID;
	}

	return dsv;
}

ShaderResourceView_t CreateStructuredBufferSRV(StructuredBuffer_t buf, uint32_t firstElem, uint32_t numElems)
{
	ShaderResourceView_t srv = g_SRVs.Create(ViewData(buf, firstElem, numElems));

	if (!CreateStructuredBufferSRVImpl(srv, buf, firstElem, numElems))
	{
		g_SRVs.Release(srv);
		return ShaderResourceView_t::INVALID;
	}

	return srv;
}

UnorderedAccessView_t CreateStructuredBufferUAV(StructuredBuffer_t buf, uint32_t firstElem, uint32_t numElems)
{
	UnorderedAccessView_t uav = g_UAVs.Create(ViewData(buf, firstElem, numElems));

	if (!CreateStructuredBufferUAVImpl(uav, buf, firstElem, numElems))
	{
		g_UAVs.Release(uav);
		return UnorderedAccessView_t::INVALID;
	}

	return uav;
}

static RenderFormat GetViewDataFormat(const ViewData* const data)
{
	if (data && data->Type == ViewResourceType::Texture)
		return data->Texture.Format;

	return RenderFormat::UNKNOWN;
}

RenderTargetView_t AllocTextureRTV(RenderFormat format, TextureDimension dim, uint32_t depthOrArraySize)
{
	RenderTargetView_t rtv = g_RTVs.Create(ViewData(Texture_t::INVALID, format, depthOrArraySize));

	if (!CreateTextureRTVImpl(rtv, Texture_t::INVALID, format, dim, depthOrArraySize))
	{
		g_RTVs.Release(rtv);
		return RenderTargetView_t::INVALID;
	}

	return rtv;
}

ShaderResourceView_t AllocSRV(RenderFormat format, TextureDimension dim, uint32_t mipLevels, uint32_t depthOrArraySize)
{
	ShaderResourceView_t srv = g_SRVs.Create(ViewData(Texture_t::INVALID, format, depthOrArraySize));

	if (!CreateTextureSRVImpl(srv, Texture_t::INVALID, format, dim, depthOrArraySize, mipLevels))
	{
		g_SRVs.Release(srv);
		return ShaderResourceView_t::INVALID;
	}

	return srv;
}

UnorderedAccessView_t AllocUAV(RenderFormat format, TextureDimension dim, uint32_t depthOrArraySize)
{
	UnorderedAccessView_t uav = g_UAVs.Create(ViewData(Texture_t::INVALID, format, depthOrArraySize));

	if (!CreateTextureUAVImpl(uav, Texture_t::INVALID, format, dim, depthOrArraySize))
	{
		g_UAVs.Release(uav);
		return UnorderedAccessView_t::INVALID;
	}

	return uav;
}

RenderTargetView_t AllocRTV(RenderFormat format, TextureDimension dim, uint32_t depthOrArraySize)
{
	RenderTargetView_t rtv = g_RTVs.Create(ViewData(Texture_t::INVALID, format, depthOrArraySize));

	if (!CreateTextureRTVImpl(rtv, Texture_t::INVALID, format, dim, depthOrArraySize))
	{
		g_RTVs.Release(rtv);
		return RenderTargetView_t::INVALID;
	}

	return rtv;
}

DepthStencilView_t AllocDSV(RenderFormat format, TextureDimension dim, uint32_t depthOrArraySize)
{
	DepthStencilView_t dsv = g_DSVs.Create(ViewData(Texture_t::INVALID, format, depthOrArraySize));

	if (!CreateTextureDSVImpl(dsv, Texture_t::INVALID, format, dim, depthOrArraySize))
	{
		g_DSVs.Release(dsv);
		return DepthStencilView_t::INVALID;
	}

	return dsv;
}

// TODO: Validate bind functions
void BindSRV(ShaderResourceView_t srv, Texture_t texture)
{
	if(srv != ShaderResourceView_t::INVALID)
		BindTextureSRVImpl(srv, texture);
}

void BindUAV(UnorderedAccessView_t uav, Texture_t texture)
{	
	if(uav != UnorderedAccessView_t::INVALID)
		BindTextureUAVImpl(uav, texture);
}

void BindRTV(RenderTargetView_t rtv, Texture_t texture)
{
	if(rtv != RenderTargetView_t::INVALID)
		BindTextureRTVImpl(rtv, texture);
}

void BindDSV(DepthStencilView_t dsv, Texture_t texture)
{
	if(dsv != DepthStencilView_t::INVALID)
		BindTextureDSVImpl(dsv, texture);
}

RenderFormat GetSRVFormat(ShaderResourceView_t srv)
{
	return GetViewDataFormat(g_SRVs.Get(srv));
}

RenderFormat GetUAVFormat(UnorderedAccessView_t uav)
{
	return GetViewDataFormat(g_UAVs.Get(uav));
}

RenderFormat GetRTVFormat(RenderTargetView_t rtv)
{
	return GetViewDataFormat(g_RTVs.Get(rtv));
}

RenderFormat GetDSVFormat(DepthStencilView_t dsv)
{
	return GetViewDataFormat(g_DSVs.Get(dsv));
}

void ReleaseSRV(ShaderResourceView_t srv)
{
	if (g_SRVs.Release(srv))
		DestroySRV(srv);
}

void ReleaseUAV(UnorderedAccessView_t uav)
{
	if (g_UAVs.Release(uav))
		DestroyUAV(uav);
}

void ReleaseRTV(RenderTargetView_t rtv)
{
	if (g_RTVs.Release(rtv))
		DestroyRTV(rtv);
}

void ReleaseDSV(DepthStencilView_t dsv)
{
	if (g_DSVs.Release(dsv))
		DestroyDSV(dsv);
}

void Render_Release(ShaderResourceView_t srv)
{
	ReleaseSRV(srv);
}

void Render_Release(UnorderedAccessView_t uav)
{
	ReleaseUAV(uav);
}

void Render_Release(RenderTargetView_t rtv)
{
	ReleaseRTV(rtv);
}

void Render_Release(DepthStencilView_t dsv)
{
	ReleaseDSV(dsv);
}

size_t Bindings_GetShaderResourceViewCount()
{
	return g_SRVs.UsedSize();
}

size_t Bindings_GetUnorderedAccessViewCount()
{
	return g_UAVs.UsedSize();
}

size_t Bindings_GetRenderTargetViewCount()
{
	return g_RTVs.UsedSize();
}

size_t Bindings_GetDepthStencilViewCount()
{
	return g_DSVs.UsedSize();
}

uint32_t Binding_GetDescriptorIndex(ShaderResourceView_t srv)
{
	if (g_SRVs.Get(srv))
	{
		return Binding_GetDescriptorIndexImpl(srv);
	}

	return 0u;
}

uint32_t Binding_GetDescriptorIndex(UnorderedAccessView_t uav)
{
	if (g_UAVs.Get(uav))
	{
		return Binding_GetDescriptorIndexImpl(uav);
	}

	return 0u;
}

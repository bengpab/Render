#include "View.h"

#include "RenderImpl.h"

#include "Binding.h"
#include "CommandList.h"
#include "Textures.h"

#include <dxgi1_2.h>
#include <map>

#ifdef _MSC_VER
#pragma comment(lib, "dxgi.lib")
#endif

namespace tpr
{

std::map<intptr_t, RenderView*> g_views;

struct RenderViewImpl
{
	ComPtr<IDXGISwapChain1> DxSwapChain;
	Texture_t Textures[RenderView::NumBackBuffers];
	RenderTargetView_t Rtv[RenderView::NumBackBuffers];
	uint32_t FrameId = 0;
};

RenderView::RenderView()
	: Impl(std::make_unique<RenderViewImpl>())
{
}

RenderView::~RenderView()
{
	for (uint32_t i = 0; i < RenderView::NumBackBuffers; i++)
	{
		RenderRelease(Impl->Rtv[i]);
		RenderRelease(Impl->Textures[i]);
	}

	g_views.erase(Hwnd);
}

void RenderView::Resize(uint32_t x, uint32_t y)
{
	x = x > 0 ? x : 1;
	y = y > 0 ? y : 1;

	if (Width == x && Height == y)
		return;

	Width = x;
	Height = y;

	for (uint32_t i = 0; i < RenderView::NumBackBuffers; i++)
	{
		RenderRelease(Impl->Rtv[i]);
		RenderRelease(Impl->Textures[i]);

		Impl->Rtv[i] = RenderTargetView_t::INVALID;
		Impl->Textures[i] = Texture_t::INVALID;
	}

	CommandList::ReleaseAll();

	Impl->DxSwapChain->ResizeBuffers(0, (UINT)Width, (UINT)Height, DXGI_FORMAT_UNKNOWN, 0);

	//for (uint32_t i = 0; i < RenderView::NumBackBuffers; i++)
	for (uint32_t i = 0; i < 1; i++)
	{
		ComPtr<ID3D11Resource> backBuffer = nullptr;

		Impl->DxSwapChain->GetBuffer(i, IID_PPV_ARGS(&backBuffer));

		Impl->Textures[i] = AllocTexture();

		Dx11_SetTextureResource(Impl->Textures[i], backBuffer);

		Impl->Rtv[i] = CreateTextureRTV(Impl->Textures[i], BackBufferFormat, TextureDimension::TEX2D, 1);
	}
}

void RenderView::Sync()
{

}

void RenderView::Present(bool vsync, bool sync)
{
	(void)sync;

	Impl->DxSwapChain->Present(vsync, 0);

	Impl->FrameId++;
}

RenderViewPtr CreateRenderViewPtr(intptr_t hwnd)
{
	return std::shared_ptr<RenderView>(CreateRenderView(hwnd));
}

RenderView* CreateRenderView(intptr_t hwnd)
{
	RenderView* rv = new RenderView;

	rv->Hwnd = hwnd;

	DXGI_SWAP_CHAIN_DESC1 sd;
	ZeroMemory(&sd, sizeof(sd));

	sd.Width = 0;
	sd.Height = 0;
	sd.Format = Dx11_Format(RenderView::BackBufferFormat);
	sd.Stereo = FALSE;
	sd.SampleDesc.Count = 1;
	sd.SampleDesc.Quality = 0;
	sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	sd.BufferCount = RenderView::NumBackBuffers;
	sd.Scaling = DXGI_SCALING_STRETCH;
	sd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	sd.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;

	ComPtr<IDXGIFactory2> factory;
	if (FAILED(CreateDXGIFactory(IID_PPV_ARGS(&factory))))
		return nullptr;

	if (FAILED(factory->CreateSwapChainForHwnd(g_render.Device.Get(), (HWND)hwnd, &sd, NULL, NULL, &rv->Impl->DxSwapChain)))
		return nullptr;

	g_views[hwnd] = rv;

	return rv;
}

RenderView* GetRenderViewForHwnd(intptr_t hwnd)
{
	auto iter = g_views.find(hwnd);
	return iter != g_views.end() ? iter->second : nullptr;
}

void RenderView::ClearCurrentBackBufferTarget( CommandList* cl )
{
	constexpr float DefaultClearCol[4] = { 0.0f, 0.0f, 0.2f, 0.0f };
	ClearCurrentBackBufferTarget( cl, DefaultClearCol );
}

void RenderView::ClearCurrentBackBufferTarget(CommandList* cl, const float clearCol[4])
{
	RenderTargetView_t rtv = GetCurrentBackBufferRTV();
	if (rtv != RenderTargetView_t::INVALID)
	{
		cl->ClearRenderTarget(rtv, clearCol);
	}
}

Texture_t RenderView::GetCurrentBackBufferTexture() const
{
	//return Impl->Textures[Impl->FrameId % RenderView::NumBackBuffers];
	return Impl->Textures[0];
}

RenderTargetView_t RenderView::GetCurrentBackBufferRTV() const
{
	//return Impl->Rtv[Impl->FrameId % RenderView::NumBackBuffers];
	return Impl->Rtv[0];
}

}

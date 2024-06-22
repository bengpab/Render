#pragma once

#include "RenderTypes.h"

namespace tpr
{

FWD_RENDER_TYPE(Texture_t);
FWD_RENDER_TYPE(RenderTargetView_t);

#include <memory>

struct CommandList;
struct RenderViewImpl;

struct RenderView
{
	explicit RenderView();
	~RenderView();

	void Resize(uint32_t x, uint32_t y);
	void Present(bool vsync);

	void ClearCurrentBackBufferTarget(CommandList* cl);
	void ClearCurrentBackBufferTarget(CommandList* cl, const float clearCol[4]);

	Texture_t GetCurrentBackBufferTexture() const;
	RenderTargetView_t GetCurrentBackBufferRTV() const;

	intptr_t Hwnd;
	std::unique_ptr<RenderViewImpl> Impl;
	uint32_t Width = 0;
	uint32_t Height = 0;

	static constexpr RenderFormat BackBufferFormat = RenderFormat::R8G8B8A8_UNORM;
	static constexpr uint32_t NumBackBuffers = 2u;
};

typedef std::shared_ptr<RenderView> RenderViewPtr;

RenderViewPtr CreateRenderViewPtr(intptr_t hwnd);
RenderView* CreateRenderView(intptr_t hwnd);
RenderView* GetRenderViewForHwnd(intptr_t hwnd);

}
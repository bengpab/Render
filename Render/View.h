#pragma once

#include "RenderTypes.h"

namespace rl
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

	// Call Sync() to ensure we have finished presenting this back buffer before using it.
	// Default behaviour of Present syncs at end of frame but this is not efficient in
	// multi-viewport setups where the next frame we process would be a part of a different
	// back buffer.
	void Sync();

	void Resize(uint32_t x, uint32_t y);
	void Present(bool vsync = true, bool sync = true);

	void ClearCurrentBackBufferTarget(CommandList* cl);
	void ClearCurrentBackBufferTarget(CommandList* cl, const float clearCol[4]);

	Texture_t GetCurrentBackBufferTexture() const;
	RenderTargetView_t GetCurrentBackBufferRTV() const;

	uint64_t GetFrameID() const;

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
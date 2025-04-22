#include "RenderImpl.h"
#include "Render.h"
#include "Buffers.h"

namespace rl
{

Dx11RenderGlobals g_render;

bool CreateDeviceAndContext(bool debug)
{
	UINT createDeviceFlags = 0;

	if (debug)
	{
		createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
	}	

	D3D_FEATURE_LEVEL featureLevel;
	D3D_FEATURE_LEVEL supportedFeatureLevels[1] = { D3D_FEATURE_LEVEL_11_1 };
	HRESULT hr = D3D11CreateDevice(NULL,
		D3D_DRIVER_TYPE_HARDWARE,
		NULL,
		createDeviceFlags,
		supportedFeatureLevels,
		1,
		D3D11_SDK_VERSION,
		&g_render.Device,
		&featureLevel,
		&g_render.DeviceContext
	);

	if (FAILED(hr))
		return false;

	return true;
}

bool Render_Init(const RenderInitParams& params)
{
	if (!CreateDeviceAndContext(params.DebugEnabled))
		return false;

	g_render.MainRootSig = CreateRootSignature(params.RootSigDesc);

	return true;
}

bool Render_Initialised()
{
	return g_render.Device != nullptr;
}

void Render_BeginFrame()
{
	DynamicBuffers_NewFrame();
}

void Render_BeginRenderFrame()
{

}

void Render_EndFrame()
{
	DynamicBuffers_EndFrame();
}

void Render_ShutDown()
{
	g_render.DeviceContext = nullptr;

	g_render.Device = nullptr;	

	// RHI TODO: release all device resources if we shutdown the renderer and dont immediately close the program.
}

void Render_PushDebugWarningDisable(RenderDebugWarnings warning)
{
	if (g_render.DebugMode)
	{
		ComPtr<ID3D11Debug> d3dDebug;
		if (SUCCEEDED(g_render.Device.As(&d3dDebug)))
		{
			ComPtr<ID3D11InfoQueue> d3dInfoQueue;
			if (SUCCEEDED(d3dDebug.As(&d3dInfoQueue)))
			{
				D3D11_MESSAGE_ID disable;
				switch (warning)
				{
				case RenderDebugWarnings::RENDER_TARGET_NOT_SET:
					disable = D3D11_MESSAGE_ID_DEVICE_DRAW_RENDERTARGETVIEW_NOT_SET;
					break;
				default:
					return;
				}

				UINT hh = d3dInfoQueue->GetStorageFilterStackSize();

				D3D11_INFO_QUEUE_FILTER filter;
				memset(&filter, 0, sizeof(filter));
				filter.DenyList.NumIDs = 1;
				filter.DenyList.pIDList = &disable;
				d3dInfoQueue->PushStorageFilter(&filter);
			}
		}
	}
}

void Render_PopDebugWarningDisable()
{
	if (g_render.DebugMode)
	{
		ComPtr<ID3D11Debug> d3dDebug;
		if (SUCCEEDED(g_render.Device.As(&d3dDebug)))
		{
			ComPtr<ID3D11InfoQueue> d3dInfoQueue;
			if (SUCCEEDED(d3dDebug.As(&d3dInfoQueue)))
			{
				UINT hh = d3dInfoQueue->GetStorageFilterStackSize();
				d3dInfoQueue->PopStorageFilter();
			}
		}
	}
}

bool Render_IsBindless()
{
	return false;
}

bool Render_IsThreadSafe()
{
	// TODO
	return false;
}

bool Render_SupportsMeshShaders()
{
	return false;
}

bool Render_SupportsRaytracing()
{
	return false;
}

const char* Render_ApiId()
{
	return "RAPI_DX11";
}

}
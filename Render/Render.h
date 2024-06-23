#pragma once

#define RENDER_MAJOR_VERSION 1
#define RENDER_MINOR_VERSION 0

#include "Binding.h"
#include "Buffers.h"
#include "CommandList.h"
#include "PipelineState.h"
#include "RenderTypes.h"
#include "RootSignature.h"
#include "Samplers.h"
#include "Shaders.h"
#include "Textures.h"
#include "View.h"

namespace tpr
{

struct RenderInitParams
{
	bool DebugEnabled = false;

	std::vector<RenderDebugWarnings> DisabledWarnings;

	RootSignatureDesc RootSigDesc;
};

bool Render_Init(const RenderInitParams& params);
bool Render_Initialised();

// Reset systems access by the frame update
void Render_BeginFrame();

// Begin rendering
void Render_BeginRenderFrame();

void Render_EndFrame();

void Render_ShutDown();

void Render_PushDebugWarningDisable(RenderDebugWarnings warning);
void Render_PopDebugWarningDisable();

bool Render_BindlessMode();
const char* Render_ApiId();

}
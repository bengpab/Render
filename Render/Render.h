#pragma once

// Temporary define so I can test the RHI as I implement it, when fully moved over this will go away.
// This define acts as a kind of TODO until then.
#define USE_RHI 1

#if USE_RHI

#include "Binding.h"
#include "Buffers.h"
#include "CommandList.h"
#include "PipelineState.h"
#include "RenderTypes.h"
#include "Samplers.h"
#include "Shaders.h"
#include "Textures.h"
#include "View.h"

#define RENDER_DEBUG 1
bool Render_Init();
bool Render_Initialised();
void Render_NewFrame();
void Render_ShutDown();

void Render_PushDebugWarningDisable(RenderDebugWarnings warning);
void Render_PopDebugWarningDisable();

#endif
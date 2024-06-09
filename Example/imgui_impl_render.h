// dear imgui: Renderer
// This needs to be used along with a Platform Binding (e.g. Win32)

#include "imgui/imgui.h"

#pragma once

struct CommandList;
struct ImRenderFrameData;

IMGUI_IMPL_API bool					ImGui_ImplRender_Init(RenderFormat backbufferFormat, const char* shaderPath);
IMGUI_IMPL_API void					ImGui_ImplRender_Shutdown();
IMGUI_IMPL_API void					ImGui_ImplRender_NewFrame();
IMGUI_IMPL_API ImRenderFrameData*   ImGui_ImplRender_PrepareFrameData(ImDrawData* draw_data);
IMGUI_IMPL_API void					ImGui_ImplRender_RenderDrawData(ImRenderFrameData* frame_data, ImDrawData* draw_data, CommandList* cl);
IMGUI_IMPL_API void					ImGui_ImplRender_ReleaseFrameData(ImRenderFrameData* frame_data);

// Use if you want to reset your rendering device without losing ImGui state.
IMGUI_IMPL_API void					ImGui_ImplRender_InvalidateDeviceObjects();
IMGUI_IMPL_API bool					ImGui_ImplRender_CreateDeviceObjects();

IMGUI_IMPL_API RootSignature_t		ImGui_ImplRender_GetRootSignature();
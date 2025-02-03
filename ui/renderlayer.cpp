/*
Copyright 2020-2024 Vector 35 Inc.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

#include "renderlayer.h"
#include "debuggerapi.h"

using namespace BinaryNinja;
using namespace BinaryNinjaDebuggerAPI;

PCRenderLayer::PCRenderLayer(): RenderLayer("Debugger PC")
{

}


void PCRenderLayer::ApplyToBlock(Ref<BasicBlock> block, std::vector<DisassemblyTextLine>& lines)
{
	Ref<BinaryView> bv = block->GetFunction()->GetView();
	DbgRef<DebuggerController> controller = DebuggerController::GetController(bv);
	if (!controller)
		return;

	uint64_t ipAddr = controller->IP();

	for (auto& line : lines)
	{
		if (line.addr == ipAddr)
		{
			InstructionTextToken pcIndicator(BNInstructionTextTokenType::TextToken, "=> ");
			line.tokens.insert(line.tokens.begin(), pcIndicator);

			line.highlight.style = StandardHighlightColor;
			line.highlight.color = BlueHighlightColor;
			line.highlight.mixColor = NoHighlightColor;
			line.highlight.mix = 0;
			line.highlight.r = 0;
			line.highlight.g = 0;
			line.highlight.b = 0;
			line.highlight.alpha = 255;
		}
	}
}


BreakpointRenderLayer::BreakpointRenderLayer(): RenderLayer("Debugger Breakpoints")
{

}


void BreakpointRenderLayer::ApplyToBlock(Ref<BasicBlock> block, std::vector<DisassemblyTextLine>& lines)
{
	Ref<BinaryView> bv = block->GetFunction()->GetView();
	DbgRef<DebuggerController> controller = DebuggerController::GetController(bv);
	if (!controller)
		return;

	for (auto& line : lines)
	{
		if (controller->ContainsBreakpoint(line.addr))
		{
			InstructionTextToken breakpointToken(BNInstructionTextTokenType::TagToken, "🛑");
			line.tokens.insert(line.tokens.begin(), breakpointToken);

			line.highlight.style = StandardHighlightColor;
			line.highlight.color = RedHighlightColor;
			line.highlight.mixColor = NoHighlightColor;
			line.highlight.mix = 0;
			line.highlight.r = 0;
			line.highlight.g = 0;
			line.highlight.b = 0;
			line.highlight.alpha = 255;
		}
	}
}


void RegisterRenderLayers()
{
	static BreakpointRenderLayer* g_breakpointRenderLayer = new BreakpointRenderLayer();
	static PCRenderLayer* g_pcRenderLayer = new PCRenderLayer();

	RenderLayer::Register(g_breakpointRenderLayer, BNRenderLayerDefaultEnableState::AlwaysEnabledRenderLayerDefaultEnableState);
	RenderLayer::Register(g_pcRenderLayer, BNRenderLayerDefaultEnableState::AlwaysEnabledRenderLayerDefaultEnableState);
}

/*
Copyright(c) 2016-2018 Panos Karabelas

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and / or sell
copies of the Software, and to permit persons to whom the Software is furnished
to do so, subject to the following conditions :

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE AUTHORS OR
COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

//= INCLUDES ===========================
#include "GBuffer.h"
#include "../../RHI/RHI_Device.h"
#include "../../RHI/RHI_PipelineState.h"
#include "../../RHI/RHI_RenderTexture.h"
//======================================

//= NAMESPACES ================
using namespace std;
using namespace Directus::Math;
//=============================

namespace Directus
{
	GBuffer::GBuffer(const shared_ptr<RHI_Device>& rhiDevice, int width, int height)
	{
		m_renderTargets[GBuffer_Target_Albedo]		= make_shared<RHI_RenderTexture>(rhiDevice, width, height, false,	Texture_Format_R8G8B8A8_UNORM);
		m_renderTargets[GBuffer_Target_Normal]		= make_shared<RHI_RenderTexture>(rhiDevice, width, height, false,	Texture_Format_R8G8B8A8_UNORM);
		m_renderTargets[GBuffer_Target_Specular]	= make_shared<RHI_RenderTexture>(rhiDevice, width, height, false,	Texture_Format_R8G8B8A8_UNORM);
		m_renderTargets[GBuffer_Target_Depth]		= make_shared<RHI_RenderTexture>(rhiDevice, width, height, true,	Texture_Format_R32G32_FLOAT);

		m_renderTargetViews.emplace_back(m_renderTargets[GBuffer_Target_Albedo]->GetRenderTarget());
		m_renderTargetViews.emplace_back(m_renderTargets[GBuffer_Target_Normal]->GetRenderTarget());
		m_renderTargetViews.emplace_back(m_renderTargets[GBuffer_Target_Specular]->GetRenderTarget());
		m_renderTargetViews.emplace_back(m_renderTargets[GBuffer_Target_Depth]->GetRenderTarget());
	}

	GBuffer::~GBuffer()
	{
		m_renderTargets.clear();
	}

	bool GBuffer::SetAsRenderTarget(const std::shared_ptr<RHI_PipelineState>& pipelineState)
	{
		pipelineState->SetRenderTargets(m_renderTargetViews, m_renderTargets[GBuffer_Target_Depth]->GetDepthStencil());
		// Grab the viewport from one of the render targets and set it
		pipelineState->SetViewport(m_renderTargets[GBuffer_Target_Albedo]->GetViewport());

		return true;
	}

	bool GBuffer::Clear(const shared_ptr<RHI_Device>& rhiDevice)
	{
		// Clear the render target buffers.
		for (auto& renderTarget : m_renderTargets)
		{
			if (!renderTarget.second->GetDepthEnabled())
			{
				// Color buffer
				rhiDevice->ClearRenderTarget(renderTarget.second->GetRenderTarget(), Vector4(0, 0, 0, 1));		
			}
			else
			{
				// Clear the depth buffer.
				float maxDepth	= renderTarget.second->GetViewport().GetMaxDepth();
				uint8_t stencil = 0;
				rhiDevice->ClearDepthStencil(renderTarget.second->GetDepthStencil(), Clear_Depth | Clear_Stencil, maxDepth, stencil);
			}
		}
		return true;
	}

	const shared_ptr<RHI_RenderTexture>& GBuffer::GetTexture(GBuffer_Texture_Type type)
	{
		return m_renderTargets[type];
	}
}
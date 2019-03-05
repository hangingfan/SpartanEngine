/*
Copyright(c) 2016-2019 Panos Karabelas

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

#pragma once

//= INCLUDES ==================
#include "RHI_Definition.h"
#include "../Core/EngineDefs.h"
#include <memory>
#include <string>
#include <vector>
//=============================

namespace Directus
{
	namespace Math
	{ 
		class Vector4;
		class Rectangle;
	}

	struct DisplayMode
	{
		DisplayMode() = default;
		DisplayMode(const unsigned int width, const unsigned int height, const unsigned int refresh_rate_numerator, const unsigned int refresh_rate_denominator)
		{
			this->width						= width;
			this->height					= height;
			this->refreshRateNumerator		= refresh_rate_numerator;
			this->refreshRateDenominator	= refresh_rate_denominator;
			this->refreshRate				= static_cast<float>(refresh_rate_numerator) / static_cast<float>(refresh_rate_denominator);
		}

		unsigned int width					= 0;
		unsigned int height					= 0;
		unsigned int refreshRateNumerator	= 0;
		unsigned int refreshRateDenominator = 0;
		float refreshRate					= 0;
	};

	struct DisplayAdapter
	{
		DisplayAdapter(const std::string& name, const unsigned int memory, const unsigned int vendor_id, void* data)
		{
			this->name		= name;
			this->memory	= memory;
			this->vendorID	= vendor_id;
			this->data		= data;
		}

		std::string name		= "Unknown";
		unsigned int vendorID	= 0;
		unsigned int memory		= 0;
		void* data				= nullptr;

		//Nvidia: 0x10DE
		//AMD	: 0x1002, 0x1022
		//Intel : 0x163C, 0x8086, 0x8087
	};

	class ENGINE_CLASS RHI_Device
	{
	public:
		RHI_Device();
		~RHI_Device();

		//= DRAW/PRESENT =======================================================================================
		bool Draw(unsigned int vertex_count) const;
		bool DrawIndexed(unsigned int index_count, unsigned int index_offset, unsigned int vertex_offset) const;
		//======================================================================================================

		//= CLEAR ===================================================================================================
		bool ClearRenderTarget(void* render_target, const Math::Vector4& color) const;
		bool ClearDepthStencil(void* depth_stencil, unsigned int flags, float depth, unsigned int stencil = 0) const;
		//===========================================================================================================

		//= SET ================================================================================================================
		bool SetVertexBuffer(const std::shared_ptr<RHI_VertexBuffer>& buffer) const;
		bool SetIndexBuffer(const std::shared_ptr<RHI_IndexBuffer>& buffer) const;
		bool SetVertexShader(const std::shared_ptr<RHI_Shader>& shader) const;
		bool SetPixelShader(const std::shared_ptr<RHI_Shader>& shader) const;
		bool SetDepthStencilState(const std::shared_ptr<RHI_DepthStencilState>& depth_stencil_state) const;
		bool SetRasterizerState(const std::shared_ptr<RHI_RasterizerState>& rasterizer_state) const;
		bool SetBlendState(const std::shared_ptr<RHI_BlendState>& blend_state) const;
		bool SetInputLayout(const std::shared_ptr<RHI_InputLayout>& input_layout) const;	
		bool SetPrimitiveTopology(RHI_PrimitiveTopology_Mode primitive_topology) const;
		bool SetConstantBuffers(unsigned int start_slot, unsigned int buffer_count, void* buffer, RHI_Buffer_Scope scope) const;
		bool SetSamplers(unsigned int start_slot, unsigned int sampler_count, void* samplers) const;
		bool SetTextures(unsigned int start_slot, unsigned int resource_count, void* shader_resources) const;	
		bool SetRenderTargets(unsigned int render_target_count, void* render_targets, void* depth_stencil) const;	
		bool SetViewport(const RHI_Viewport& viewport) const;
		bool SetScissorRectangle(const Math::Rectangle& rectangle) const;
		//======================================================================================================================

		//= EVENTS =====================================
		static void EventBegin(const std::string& name);
		static void EventEnd();
		//==============================================

		//= PROFILING =============================================================================
		bool ProfilingCreateQuery(void** query, RHI_Query_Type type) const;
		bool ProfilingQueryStart(void* query_object) const;
		bool ProfilingQueryEnd(void* query_object) const;
		bool ProfilingGetTimeStamp(void* query_disjoint) const;
		float ProfilingGetDuration(void* query_disjoint, void* query_start, void* query_end) const;
		//=========================================================================================

		//= ADAPTERS ============================================================================================================================		
		void AddDisplayMode(unsigned int width, unsigned int height, unsigned int refresh_rate_numerator, unsigned int refresh_rate_denominator);
		bool GetDidsplayModeFastest(DisplayMode* display_mode);
		void AddAdapter(const std::string& name, unsigned int memory, unsigned int vendor_id, void* adapter);
		void SetPrimaryAdapter(const DisplayAdapter* primary_adapter);
		const std::vector<DisplayAdapter>& GetAdapters() const	{ return m_displayAdapters; }
		const DisplayAdapter* GetPrimaryAdapter()				{ return m_primaryAdapter; }
		RHI_Format GetBackBufferFormat()						{ return m_back_buffer_format; }
		//=======================================================================================================================================

		//= API ACCESS ===================================================================
		bool IsInitialized() const { return m_initialized; }
		#if defined(API_GRAPHICS_D3D11)
		template <typename T>
		constexpr T* GetDevicePhysical()	{ return static_cast<T*>(m_device_physical); }
		template <typename T>
		constexpr T* GetDevice()			{ return static_cast<T*>(m_device); }
		#elif defined(API_GRAPHICS_VULKAN)
		template <typename T>
		constexpr T GetDevicePhysical()	{ return static_cast<T>(m_device_physical); }
		template <typename T>
		constexpr T GetDevice()			{ return static_cast<T>(m_device); }
		template <typename T>
		constexpr T GetInstance()		{ return static_cast<T>(m_instance); }
		#endif
		//================================================================================

	private:
		std::vector<DisplayMode> m_displayModes;
		std::vector<DisplayAdapter> m_displayAdapters;
		const DisplayAdapter* m_primaryAdapter = nullptr;
		RHI_Format m_back_buffer_format = Format_R8G8B8A8_UNORM;

		bool m_initialized		= false;
		void* m_device_physical	= nullptr;
		void* m_device			= nullptr;

		// Low-level (used by Vulkan)
		void* m_instance		= nullptr;
		void* m_present_queue	= nullptr;
	};
}
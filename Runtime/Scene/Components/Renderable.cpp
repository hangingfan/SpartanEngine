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

//= INCLUDES ===============================
#include "Renderable.h"
#include "Transform.h"
#include "../../Rendering/GeometryUtility.h"
#include "../../IO/FileStream.h"
#include "../../Resource/ResourceManager.h"
//==========================================

//= NAMESPACES ================
using namespace std;
using namespace Directus::Math;
//=============================

namespace Directus
{
	namespace DefaultRenderables
	{
		inline void Build(GeometryType type, Renderable* renderable)
		{	
			Model* model = new Model(renderable->GetContext());
			vector<RHI_Vertex_PosUVTBN> vertices;
			vector<unsigned int> indices;

			// Construct geometry
			if (type == Geometry_Default_Cube)
			{
				GeometryUtility::CreateCube(&vertices, &indices);		
				model->SetResourceName("Default_Cube");
			}
			else if (type == Geometry_Default_Quad)
			{
				GeometryUtility::CreateQuad(&vertices, &indices);
				model->SetResourceName("Default_Cube");
			}
			else if (type == Geometry_Default_Sphere)
			{
				GeometryUtility::CreateSphere(&vertices, &indices);
				model->SetResourceName("Default_Cube");
			}
			else if (type == Geometry_Default_Cylinder)
			{
				GeometryUtility::CreateCylinder(&vertices, &indices);
				model->SetResourceName("Default_Cube");
			}
			else if (type == Geometry_Default_Cone)
			{
				GeometryUtility::CreateCone(&vertices, &indices);
				model->SetResourceName("Default_Cube");
			}

			if (vertices.empty() || indices.empty())
				return;

			model->Geometry_Append(indices, vertices, nullptr, nullptr);
			model->Geometry_Update();

			renderable->Geometry_Set(
				"Default_Geometry",
				0,
				(unsigned int)indices.size(),
				0,
				(unsigned int)vertices.size(),
				BoundingBox(vertices),
				model
			);
		}
	}

	Renderable::Renderable(Context* context, Actor* actor, Transform* transform) : IComponent(context, actor, transform)
	{
		m_geometryType			= Geometry_Custom;	
		m_geometryIndexOffset	= 0;
		m_geometryIndexCount	= 0;
		m_geometryVertexOffset	= 0;
		m_geometryVertexCount	= 0;
		m_materialDefault		= false;
		m_materialRef			= nullptr;
		m_castShadows			= true;
		m_receiveShadows		= true;
	}

	Renderable::~Renderable()
	{
		if (m_geometryType != Geometry_Custom)
		{
			delete m_model;
		}
	}

	//= ICOMPONENT ===============================================================
	void Renderable::Serialize(FileStream* stream)
	{
		// Mesh
		stream->Write((int)m_geometryType);
		stream->Write(m_geometryIndexOffset);
		stream->Write(m_geometryIndexCount);
		stream->Write(m_geometryVertexOffset);
		stream->Write(m_geometryVertexCount);
		stream->Write(m_geometryAABB);
		stream->Write(m_model ? m_model->GetResourceName() : NOT_ASSIGNED);

		// Material
		stream->Write(m_castShadows);
		stream->Write(m_receiveShadows);
		stream->Write(m_materialDefault);
		if (!m_materialDefault)
		{
			stream->Write(!m_materialRefWeak.expired() ? m_materialRefWeak.lock()->GetResourceName() : NOT_ASSIGNED);
		}
	}

	void Renderable::Deserialize(FileStream* stream)
	{
		// Geometry
		m_geometryType			= (GeometryType)stream->ReadInt();
		m_geometryIndexOffset	= stream->ReadUInt();
		m_geometryIndexCount	= stream->ReadUInt();	
		m_geometryVertexOffset	= stream->ReadUInt();
		m_geometryVertexCount	= stream->ReadUInt();
		stream->Read(&m_geometryAABB);
		string modelName;
		stream->Read(&modelName);
		m_model = m_context->GetSubsystem<ResourceManager>()->GetResourceByName<Model>(modelName).lock().get();

		// If it was a default mesh, we have to reconstruct it
		if (m_geometryType != Geometry_Custom) 
		{
			Geometry_Set(m_geometryType);
		}

		// Material
		stream->Read(&m_castShadows);
		stream->Read(&m_receiveShadows);
		stream->Read(&m_materialDefault);
		if (m_materialDefault)
		{
			Material_UseDefault();		
		}
		else
		{
			string materialName;
			stream->Read(&materialName);
			m_materialRefWeak	= m_context->GetSubsystem<ResourceManager>()->GetResourceByName<Material>(materialName);
			m_materialRef		= m_materialRefWeak.lock().get();
		}
	}
	//==============================================================================

	//= GEOMETRY =====================================================================================
	void Renderable::Geometry_Set(const string& name, unsigned int indexOffset, unsigned int indexCount, unsigned int vertexOffset, unsigned int vertexCount, const BoundingBox& AABB, Model* model)
	{	
		m_geometryName			= name;
		m_geometryIndexOffset	= indexOffset;
		m_geometryIndexCount	= indexCount;
		m_geometryVertexOffset	= vertexOffset;
		m_geometryVertexCount	= vertexCount;
		m_geometryAABB			= AABB;
		m_model					= model;
	}

	void Renderable::Geometry_Set(GeometryType type)
	{
		m_geometryType = type;

		if (type != Geometry_Custom)
		{
			DefaultRenderables::Build(type, this);
		}
	}

	void Renderable::Geometry_Get(vector<unsigned int>* indices, vector<RHI_Vertex_PosUVTBN>* vertices)
	{
		if (!m_model)
		{
			LOG_ERROR("Renderable::Geometry_Get: Invalid model");
			return;
		}

		m_model->Geometry_Get(m_geometryIndexOffset, m_geometryIndexCount, m_geometryVertexOffset, m_geometryVertexCount, indices, vertices);
	}

	BoundingBox Renderable::Geometry_BB()
	{
		return m_geometryAABB.Transformed(GetTransform()->GetWorldTransform());
	}
	//==============================================================================

	//= MATERIAL ===================================================================
	// All functions (set/load) resolve to this
	void Renderable::Material_Set(const weak_ptr<Material>& materialWeak, bool autoCache /* true */)
	{
		// Validate material
		auto material = materialWeak.lock();
		if (!material)
		{
			LOG_WARNING("Renderable::SetMaterialFromMemory(): Provided material is null, can't execute function");
			return;
		}

		if (autoCache) // Cache it
		{
			if (auto cachedMat = material->Cache<Material>().lock())
			{
				m_materialRefWeak = cachedMat;
				m_materialRef = m_materialRefWeak.lock().get();
				if (cachedMat->HasFilePath())
				{
					m_materialRef->SaveToFile(material->GetResourceFilePath());
					m_materialDefault = false;
				}
			}
		}
		else
		{
			m_materialRefWeak = material;
			m_materialRef = m_materialRefWeak.lock().get();
		}
	}

	weak_ptr<Material> Renderable::Material_Set(const string& filePath)
	{
		// Load the material
		auto material = make_shared<Material>(GetContext());
		if (!material->LoadFromFile(filePath))
		{
			LOGF_WARNING("Renderable::SetMaterialFromFile(): Failed to load material from \"%s\"", filePath.c_str());
			return weak_ptr<Material>();
		}

		// Set it as the current material
		Material_Set(material);

		// Return it
		return Material_RefWeak();
	}

	void Renderable::Material_UseDefault()
	{
		m_materialDefault = true;

		auto projectStandardAssetDir = GetContext()->GetSubsystem<ResourceManager>()->GetProjectStandardAssetsDirectory();
		FileSystem::CreateDirectory_(projectStandardAssetDir);
		auto materialStandard = make_shared<Material>(GetContext());
		materialStandard->SetResourceName("Standard");
		materialStandard->SetCullMode(Cull_Back);
		materialStandard->SetColorAlbedo(Vector4(0.6f, 0.6f, 0.6f, 1.0f));
		materialStandard->SetIsEditable(false);		
		Material_Set(materialStandard->Cache<Material>(), false);
	}

	string Renderable::Material_Name()
	{
		return !Material_RefWeak().expired() ? Material_RefWeak().lock()->GetResourceName() : NOT_ASSIGNED;
	}
	//==============================================================================
}
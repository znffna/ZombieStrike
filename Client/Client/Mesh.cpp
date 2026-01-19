///////////////////////////////////////////////////////////////////////////////
// Date: 2025-01-03
// Mesh.cpp : CMesh 클래스의 구현 파일
// Version : 0.1
///////////////////////////////////////////////////////////////////////////////
#include "Mesh.h"

#include "GameObject.h"

CMesh::CMesh(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList)
{
	static int nMeshIndex = 0;
	// 메쉬의 이름 초기화
	m_strMeshName = "Mesh" + std::to_string(nMeshIndex++);

	// 포지션 버퍼 초기화
	ZeroMemory(&m_d3dPositionBufferView, sizeof(D3D12_VERTEX_BUFFER_VIEW));

	m_nOffset = 0;
	m_nSlot = 0;
	m_nStride = sizeof(XMFLOAT3);
}

CMesh::~CMesh()
{
	m_pxmf3Positions.clear();
	m_pd3dPositionBuffer.Reset();
	m_pd3dPositionUploadBuffer.Reset();

	m_ppnSubSetIndices.clear();
	m_ppd3dSubSetIndexBuffers.clear();
	m_ppd3dSubSetIndexUploadBuffers.clear();
	m_ppd3dSubSetIndexBuffers.clear();
}

void CMesh::ReleaseUploadBuffers()
{
	// Release Upload Buffers
	m_pd3dPositionUploadBuffer.Reset();
	for (auto& pd3dSubSetIndexUploadBuffer : m_ppd3dSubSetIndexUploadBuffers)
	{
		pd3dSubSetIndexUploadBuffer.Reset();
	}

}

void CMesh::OnPreRender(ID3D12GraphicsCommandList* pd3dCommandList, void* pContext)
{
	// Pre-Render Process
}

void CMesh::Render(ID3D12GraphicsCommandList* pd3dCommandList, int nSubSet)
{
	int nSubMeshes = (int)m_ppnSubSetIndices.size();
	// Render Process
	pd3dCommandList->IASetPrimitiveTopology(m_d3dPrimitiveTopology);

	pd3dCommandList->IASetVertexBuffers(m_nSlot, 1, &m_d3dPositionBufferView);

	if ((nSubMeshes > 0) && (nSubSet < nSubMeshes))
	{
		pd3dCommandList->IASetIndexBuffer(&(m_pd3dSubSetIndexBufferViews[nSubSet]));
		pd3dCommandList->DrawIndexedInstanced((UINT)m_ppnSubSetIndices[nSubSet].size(), 1, 0, 0, 0);
	}
	else
	{
		pd3dCommandList->DrawInstanced(m_nVertices, 1, m_nOffset, 0);
	}
}

void CMesh::OnPostRender(int nPipelineState)
{
}

BoundingBox CMesh::GetBoundingBox()
{
	return BoundingBox(m_xmf3AABBCenter, m_xmf3AABBExtents);
}

BoundingSphere CMesh::GetBoundingSphere()
{
	return BoundingSphere(m_xmf3AABBCenter, Vector3::Length(m_xmf3AABBExtents));
}

BoundingOrientedBox CMesh::GetBoundingOrientedBox(const XMFLOAT4X4& xmf4x4WorldMatrix)
{
	XMFLOAT4 quaternion;
	XMStoreFloat4(&quaternion, XMQuaternionRotationMatrix(XMLoadFloat4x4(&xmf4x4WorldMatrix)));
	return GetBoundingOrientedBox(quaternion);
}

BoundingOrientedBox CMesh::GetBoundingOrientedBox(const XMFLOAT4& xmf4x4Quaternion)
{
	return BoundingOrientedBox(m_xmf3AABBCenter, m_xmf3AABBExtents, xmf4x4Quaternion);
}

void CMesh::SetSubMeshCount(int nSubMeshes)
{
	m_nSubMeshes = nSubMeshes;
	m_ppnSubSetIndices.resize(nSubMeshes);
	m_ppd3dSubSetIndexBuffers.resize(nSubMeshes);
	m_ppd3dSubSetIndexUploadBuffers.resize(nSubMeshes);
	m_pd3dSubSetIndexBufferViews.resize(nSubMeshes);
}

BoundingBox CMesh::GetBoundingBox(const XMFLOAT4X4& xmf4x4WorldMatrix)
{
	BoundingBox boundingBox;
	m_xmBoundingBox.Transform(boundingBox, XMLoadFloat4x4(&xmf4x4WorldMatrix));
	return boundingBox;
}

///////////////////////////////////////////////////////////////////////////////
//

CStandardMesh::CStandardMesh(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList)
	: CMesh(pd3dDevice, pd3dCommandList)
{
	// 버텍스 정보 버퍼 초기화
	ZeroMemory(&m_d3dTextureCoord0BufferView, sizeof(D3D12_VERTEX_BUFFER_VIEW));
	ZeroMemory(&m_d3dTextureCoord1BufferView, sizeof(D3D12_VERTEX_BUFFER_VIEW));
	ZeroMemory(&m_d3dNormalBufferView, sizeof(D3D12_VERTEX_BUFFER_VIEW));
	ZeroMemory(&m_d3dTangentBufferView, sizeof(D3D12_VERTEX_BUFFER_VIEW));
	ZeroMemory(&m_d3dBiTangentBufferView, sizeof(D3D12_VERTEX_BUFFER_VIEW));
}

CStandardMesh::~CStandardMesh()
{
	m_pxmf4Colors.clear();
	m_pxmf2TextureCoords0.clear();
	m_pxmf2TextureCoords1.clear();
	m_pxmf3Normals.clear();
	m_pxmf3Tangents.clear();
	m_pxmf3BiTangents.clear();

	m_pd3dTextureCoord0Buffer.Reset();
	m_pd3dTextureCoord0UploadBuffer.Reset();

	m_pd3dTextureCoord1Buffer.Reset();
	m_pd3dTextureCoord1UploadBuffer.Reset();

	m_pd3dNormalBuffer.Reset();
	m_pd3dNormalUploadBuffer.Reset();

	m_pd3dTangentBuffer.Reset();
	m_pd3dTangentUploadBuffer.Reset();

	m_pd3dBiTangentBuffer.Reset();
	m_pd3dBiTangentUploadBuffer.Reset();
}

void CStandardMesh::LoadMeshFromFile(std::ifstream& File)
{
	char pstrToken[64] = { '\0' };
	int nPositions = 0, nColors = 0, nNormals = 0, nTangents = 0, nBiTangents = 0, nTextureCoords = 0, nIndices = 0, nSubMeshes = 0, nSubIndices = 0;

	//UINT nReads;
	File.read((char*) & m_nVertices, sizeof(int) * 1);

	::ReadStringFromFile(File, m_strMeshName);

	for (; ; )
	{
		::ReadStringFromFile(File, pstrToken);
		if (!strcmp(pstrToken, "<Bounds>:"))
		{
			File.read((char*)&m_xmf3AABBCenter, sizeof(XMFLOAT3));
			File.read((char*)&m_xmf3AABBExtents, sizeof(XMFLOAT3));
			m_xmBoundingBox = BoundingBox(m_xmf3AABBCenter, m_xmf3AABBExtents);
		}
		else if (!strcmp(pstrToken, "<Positions>:"))
		{
			File.read((char*)&nPositions, sizeof(int));
			if (nPositions > 0)
			{
				m_nType |= VERTEXT_POSITION;
				m_pxmf3Positions.resize(nPositions);
				File.read((char*)m_pxmf3Positions.data(), sizeof(XMFLOAT3) * nPositions);
			}
		}
		else if (!strcmp(pstrToken, "<Colors>:"))
		{
			File.read((char*)&nColors, sizeof(int));
			if (nColors > 0)
			{
				m_nType |= VERTEXT_COLOR;
				m_pxmf4Colors.resize(nColors);
				File.read((char*)m_pxmf4Colors.data(), sizeof(XMFLOAT4) * nColors);
			}
		}
		else if (!strcmp(pstrToken, "<TextureCoords0>:"))
		{
			nTextureCoords = ReadIntegerFromFile(File);
			if (nTextureCoords > 0)
			{
				m_nType |= VERTEXT_TEXTURE_COORD0;
				m_pxmf2TextureCoords0.resize(nTextureCoords);
				File.read((char*)m_pxmf2TextureCoords0.data(), sizeof(XMFLOAT2) * nTextureCoords);
			}
		}
		else if (!strcmp(pstrToken, "<TextureCoords1>:"))
		{
			nTextureCoords = ReadIntegerFromFile(File);
			if (nTextureCoords > 0)
			{
				m_nType |= VERTEXT_TEXTURE_COORD1;
				m_pxmf2TextureCoords1.resize(nTextureCoords);
				File.read((char*)m_pxmf2TextureCoords1.data(), sizeof(XMFLOAT2) * nTextureCoords);
			}
		}
		else if (!strcmp(pstrToken, "<Normals>:"))
		{
			nNormals = ReadIntegerFromFile(File);
			if (nNormals > 0)
			{
				m_nType |= VERTEXT_NORMAL;
				m_pxmf3Normals.resize(nNormals);
				File.read((char*)m_pxmf3Normals.data(), sizeof(XMFLOAT3) * nNormals);
			}
		}
		else if (!strcmp(pstrToken, "<Tangents>:"))
		{
			nTangents = ReadIntegerFromFile(File);
			if (nTangents > 0)
			{
				m_nType |= VERTEXT_TANGENT;
				m_pxmf3Tangents.resize(nTangents);
				File.read((char*)m_pxmf3Tangents.data(), sizeof(XMFLOAT3) * nTangents);
			}
		}
		else if (!strcmp(pstrToken, "<BiTangents>:"))
		{
			nBiTangents = ReadIntegerFromFile(File);
			if (nBiTangents > 0)
			{
				m_pxmf3BiTangents.resize(nBiTangents);
				File.read((char*)m_pxmf3BiTangents.data(), sizeof(XMFLOAT3) * nBiTangents);
			}
		}
		else if (!strcmp(pstrToken, "<SubMeshes>:"))
		{
			m_nSubMeshes = ReadIntegerFromFile(File);
			if (m_nSubMeshes > 0)
			{
				SetSubMeshCount(m_nSubMeshes);
				for (UINT i = 0; i < m_nSubMeshes; i++)
				{
					::ReadStringFromFile(File, pstrToken);
					if (!strcmp(pstrToken, "<SubMesh>:"))
					{
						int nIndex = ReadIntegerFromFile(File); // i

						UINT nIndices;
						nIndices = ReadIntegerFromFile(File);
						if (nIndices > 0)
						{
							m_ppnSubSetIndices[i].resize(nIndices);
							File.read((char*)m_ppnSubSetIndices[i].data(), sizeof(UINT) * nIndices);
						}
					}
				}
			}
		}
		else if (!strcmp(pstrToken, "</Mesh>"))
		{
			break;
		}
	}

	CResourceManager::Instance().RegisterMeshUpload(this);
}

void CStandardMesh::CreateShaderVariables(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList)
{
	CMesh::CreateShaderVariables(pd3dDevice, pd3dCommandList);

	// Create TextureCoord0 Buffer
	if (!m_pxmf2TextureCoords0.empty())
	{
		m_pd3dTextureCoord0Buffer = ::CreateBufferResource(pd3dDevice, pd3dCommandList, m_pxmf2TextureCoords0.data(), sizeof(XMFLOAT2) * m_nVertices, D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, &m_pd3dTextureCoord0UploadBuffer);

		std::wstring wstrName = to_wstring(GetName()) + L" : m_pd3dTextureCoord0Buffer";
		m_pd3dTextureCoord0Buffer->SetName(wstrName.c_str());

		m_d3dTextureCoord0BufferView.BufferLocation = m_pd3dTextureCoord0Buffer->GetGPUVirtualAddress();
		m_d3dTextureCoord0BufferView.StrideInBytes = sizeof(XMFLOAT2);
		m_d3dTextureCoord0BufferView.SizeInBytes = sizeof(XMFLOAT2) * m_nVertices;
	}

	// Create TextureCoord1 Buffer
	if (!m_pxmf2TextureCoords1.empty())
	{
		m_pd3dTextureCoord1Buffer = ::CreateBufferResource(pd3dDevice, pd3dCommandList, m_pxmf2TextureCoords1.data(), sizeof(XMFLOAT2) * m_nVertices, D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, &m_pd3dTextureCoord1UploadBuffer);

		std::wstring wstrName = to_wstring(GetName()) + L" : m_pd3dTextureCoord1Buffer";
		m_pd3dTextureCoord1Buffer->SetName(wstrName.c_str());

		m_d3dTextureCoord1BufferView.BufferLocation = m_pd3dTextureCoord1Buffer->GetGPUVirtualAddress();
		m_d3dTextureCoord1BufferView.StrideInBytes = sizeof(XMFLOAT2);
		m_d3dTextureCoord1BufferView.SizeInBytes = sizeof(XMFLOAT2) * m_nVertices;
	}
	
	// Create Normal Buffer
	if (!m_pxmf3Normals.empty())
	{
		m_pd3dNormalBuffer = ::CreateBufferResource(pd3dDevice, pd3dCommandList, m_pxmf3Normals.data(), sizeof(XMFLOAT3) * m_nVertices, D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, &m_pd3dNormalUploadBuffer);

		std::wstring wstrName = to_wstring(GetName()) + L" : m_pd3dNormalBuffer";
		m_pd3dNormalBuffer->SetName(wstrName.c_str());

		m_d3dNormalBufferView.BufferLocation = m_pd3dNormalBuffer->GetGPUVirtualAddress();
		m_d3dNormalBufferView.StrideInBytes = sizeof(XMFLOAT3);
		m_d3dNormalBufferView.SizeInBytes = sizeof(XMFLOAT3) * m_nVertices;
	}

	// Create Tangent Buffer
	if (!m_pxmf3Tangents.empty())
	{
		m_pd3dTangentBuffer = ::CreateBufferResource(pd3dDevice, pd3dCommandList, m_pxmf3Tangents.data(), sizeof(XMFLOAT3) * m_nVertices, D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, &m_pd3dTangentUploadBuffer);

		std::wstring wstrName = to_wstring(GetName()) + L" : m_pd3dTangentBuffer";
		m_pd3dTangentBuffer->SetName(wstrName.c_str());

		m_d3dTangentBufferView.BufferLocation = m_pd3dTangentBuffer->GetGPUVirtualAddress();
		m_d3dTangentBufferView.StrideInBytes = sizeof(XMFLOAT3);
		m_d3dTangentBufferView.SizeInBytes = sizeof(XMFLOAT3) * m_nVertices;
	}

	// Create BiTangent Buffer
	if (!m_pxmf3BiTangents.empty())
	{
		m_pd3dBiTangentBuffer = ::CreateBufferResource(pd3dDevice, pd3dCommandList, m_pxmf3BiTangents.data(), sizeof(XMFLOAT3) * m_nVertices, D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, &m_pd3dBiTangentUploadBuffer);

		std::wstring wstrName = to_wstring(GetName()) + L" : m_pxmf3BiTangents";
		m_pd3dBiTangentBuffer->SetName(wstrName.c_str());

		m_d3dBiTangentBufferView.BufferLocation = m_pd3dBiTangentBuffer->GetGPUVirtualAddress();
		m_d3dBiTangentBufferView.StrideInBytes = sizeof(XMFLOAT3);
		m_d3dBiTangentBufferView.SizeInBytes = sizeof(XMFLOAT3) * m_nVertices;
	}

}

void CStandardMesh::ReleaseUploadBuffers()
{
	CMesh::ReleaseUploadBuffers();

	m_pd3dTextureCoord0UploadBuffer.Reset();
	m_pd3dTextureCoord1UploadBuffer.Reset();

	m_pd3dNormalUploadBuffer.Reset();
	m_pd3dTangentUploadBuffer.Reset();
	m_pd3dBiTangentUploadBuffer.Reset();
}

void CStandardMesh::OnPreRender(ID3D12GraphicsCommandList* pd3dCommandList, void* pContext)
{
	D3D12_VERTEX_BUFFER_VIEW pVertexBufferViews[5] = { m_d3dPositionBufferView, m_d3dTextureCoord0BufferView, m_d3dNormalBufferView, m_d3dTangentBufferView, m_d3dBiTangentBufferView };
	pd3dCommandList->IASetVertexBuffers(m_nSlot, 5, pVertexBufferViews);
}

void CStandardMesh::Render(ID3D12GraphicsCommandList* pd3dCommandList, int nSubSet)
{
	int nSubMeshes = (int)m_nSubMeshes;

	// Render Process
	UpdateShaderVariables(pd3dCommandList);

	OnPreRender(pd3dCommandList, nullptr);

	pd3dCommandList->IASetPrimitiveTopology(m_d3dPrimitiveTopology);

	if ((nSubMeshes > 0) && (nSubSet < nSubMeshes))
	{
		pd3dCommandList->IASetIndexBuffer(&(m_pd3dSubSetIndexBufferViews[nSubSet]));
		UINT nSubSetIndex = (UINT)m_ppnSubSetIndices[nSubSet].size();
		pd3dCommandList->DrawIndexedInstanced((UINT)m_ppnSubSetIndices[nSubSet].size(), 1, 0, 0, 0);
	}
	else
	{
		pd3dCommandList->DrawInstanced(m_nVertices, 1, m_nOffset, 0);
	}
}

///////////////////////////////////////////////////////////////////////////////
//

CQuadMesh::CQuadMesh(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, float fWidth, float fHeight)
	: CStandardMesh(pd3dDevice, pd3dCommandList)
{
	// position
	m_nVertices = 4;
	m_d3dPrimitiveTopology = D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP;

	float halfWidth = fWidth * 0.5f;
	float halfHeight = fHeight * 0.5f;
	std::vector<XMFLOAT3> xmf3Positions = {
		{-halfWidth, -halfHeight, 0.0f},
		{ halfWidth, -halfHeight, 0.0f},
		{-halfWidth,  halfHeight, 0.0f},
		{ halfWidth,  halfHeight, 0.0f}
	};
	m_pxmf3Positions = std::move(xmf3Positions);

	// index buffer
	m_ppnSubSetIndices.resize(1);
	m_ppnSubSetIndices[0] = { 0, 1, 2, 3 };

	// normal
	std::vector<XMFLOAT3> pxmf3Normals = {
		{0.0f, 0.0f, -1.0f},
		{0.0f, 0.0f, -1.0f},
		{0.0f, 0.0f, -1.0f},
		{0.0f, 0.0f, -1.0f}
	};
	m_pxmf3Normals = std::move(pxmf3Normals);

	// texture coord
	std::vector<XMFLOAT2> pxmf2TextureCoords0 = {
		{0.0f, 1.0f},
		{1.0f, 1.0f},
		{0.0f, 0.0f},
		{1.0f, 0.0f}
	};

	m_pxmf2TextureCoords0 = std::move(pxmf2TextureCoords0);

	// tangent
	std::vector<XMFLOAT3> pxmf3Tangents = {
		{1.0f, 0.0f, 0.0f},
		{1.0f, 0.0f, 0.0f},
		{1.0f, 0.0f, 0.0f},
		{1.0f, 0.0f, 0.0f}
	};

	m_pxmf3Tangents = std::move(pxmf3Tangents);

	// bitangent
	std::vector<XMFLOAT3> pxmf3BiTangents = {
		{0.0f, 1.0f, 0.0f},
		{0.0f, 1.0f, 0.0f},
		{0.0f, 1.0f, 0.0f},
		{0.0f, 1.0f, 0.0f}
	};

	m_pxmf3BiTangents = std::move(pxmf3BiTangents);
}

CQuadMesh::~CQuadMesh()
{
}

///////////////////////////////////////////////////////////////////////////////
//

CCubeMesh::CCubeMesh(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, float fWidth, float fHeight, float fDepth)
	: CStandardMesh(pd3dDevice, pd3dCommandList)
{
	static int nCubeIndex = 0;
	std::string strName = "Cube_" + std::to_string(nCubeIndex++);
	SetName(strName);

	// position
	m_nVertices = 36;

	m_d3dPrimitiveTopology = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

	float halfWidth = fWidth * 0.5f;
	float halfHeight = fHeight * 0.5f;
	float halfDepth = fDepth * 0.5f;

	std::vector<XMFLOAT3> xmf3Positions = {
		// Front face
		{-halfWidth, -halfHeight,  halfDepth},  // 0
		{ halfWidth, -halfHeight,  halfDepth},  // 1
		{ halfWidth,  halfHeight,  halfDepth},  // 2
		{-halfWidth, -halfHeight,  halfDepth},  // 0
		{ halfWidth,  halfHeight,  halfDepth},  // 2
		{-halfWidth,  halfHeight,  halfDepth},  // 3

		// Back face
		{ halfWidth, -halfHeight, -halfDepth},  // 4
		{-halfWidth, -halfHeight, -halfDepth},  // 5
		{-halfWidth,  halfHeight, -halfDepth},  // 6
		{ halfWidth, -halfHeight, -halfDepth},  // 4
		{-halfWidth,  halfHeight, -halfDepth},  // 6
		{ halfWidth,  halfHeight, -halfDepth},  // 7

		// Top face
		{-halfWidth,  halfHeight, -halfDepth},  // 6
		{-halfWidth,  halfHeight,  halfDepth},  // 3
		{ halfWidth,  halfHeight,  halfDepth},  // 2
		{-halfWidth,  halfHeight, -halfDepth},  // 6
		{ halfWidth,  halfHeight,  halfDepth},  // 2
		{ halfWidth,  halfHeight, -halfDepth},  // 7

		// Bottom face
		{-halfWidth, -halfHeight, -halfDepth},  // 5
		{ halfWidth, -halfHeight, -halfDepth},  // 4
		{ halfWidth, -halfHeight,  halfDepth},  // 1
		{-halfWidth, -halfHeight, -halfDepth},  // 5
		{ halfWidth, -halfHeight,  halfDepth},  // 1
		{-halfWidth, -halfHeight,  halfDepth},  // 0

		// Right face
		{ halfWidth, -halfHeight, -halfDepth},  // 4
		{ halfWidth,  halfHeight, -halfDepth},  // 7
		{ halfWidth,  halfHeight,  halfDepth},  // 2
		{ halfWidth, -halfHeight, -halfDepth},  // 4
		{ halfWidth,  halfHeight,  halfDepth},  // 2
		{ halfWidth, -halfHeight,  halfDepth},  // 1

		// Left face
		{-halfWidth, -halfHeight, -halfDepth},  // 5
		{-halfWidth, -halfHeight,  halfDepth},  // 0
		{-halfWidth,  halfHeight,  halfDepth},  // 3
		{-halfWidth, -halfHeight, -halfDepth},  // 5
		{-halfWidth,  halfHeight,  halfDepth},  // 3
		{-halfWidth,  halfHeight, -halfDepth}   // 6
	};

	m_pxmf3Positions = std::move(xmf3Positions);

	/*m_pd3dPositionBuffer = CreateBufferResource(pd3dDevice, pd3dCommandList, m_pxmf3Positions.data(), sizeof(XMFLOAT3) * m_nVertices, D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, m_pd3dPositionUploadBuffer.GetAddressOf());

	std::wstring debugName = L"Cube_" + std::to_wstring(nCubeIndex - 1);
	{
		std::wstring name = debugName + L" : m_pd3dPositionBuffer";
		m_pd3dPositionBuffer->SetName(name.c_str());
	}

	m_d3dPositionBufferView.BufferLocation = m_pd3dPositionBuffer->GetGPUVirtualAddress();
	m_d3dPositionBufferView.StrideInBytes = sizeof(XMFLOAT3);
	m_d3dPositionBufferView.SizeInBytes = sizeof(XMFLOAT3) * m_nVertices;*/
	
	// normal
	std::vector<XMFLOAT3> pxmf3Normals = {
		// Front face
		{0.0f, 0.0f, 1.0f},
		{0.0f, 0.0f, 1.0f},
		{0.0f, 0.0f, 1.0f},
		{0.0f, 0.0f, 1.0f},
		{0.0f, 0.0f, 1.0f},
		{0.0f, 0.0f, 1.0f},

		// Back face
		{0.0f, 0.0f, -1.0f},
		{0.0f, 0.0f, -1.0f},
		{0.0f, 0.0f, -1.0f},
		{0.0f, 0.0f, -1.0f},
		{0.0f, 0.0f, -1.0f},
		{0.0f, 0.0f, -1.0f},

		// Top face
		{0.0f, 1.0f, 0.0f},
		{0.0f, 1.0f, 0.0f},
		{0.0f, 1.0f, 0.0f},
		{0.0f, 1.0f, 0.0f},
		{0.0f, 1.0f, 0.0f},
		{0.0f, 1.0f, 0.0f},

		// Bottom face
		{0.0f, -1.0f, 0.0f},
		{0.0f, -1.0f, 0.0f},
		{0.0f, -1.0f, 0.0f},
		{0.0f, -1.0f, 0.0f},
		{0.0f, -1.0f, 0.0f},
		{0.0f, -1.0f, 0.0f},

		// Right face
		{1.0f, 0.0f, 0.0f},
		{1.0f, 0.0f, 0.0f},
		{1.0f, 0.0f, 0.0f},
		{1.0f, 0.0f, 0.0f},
		{1.0f, 0.0f, 0.0f},
		{1.0f, 0.0f, 0.0f},

		// Left face
		{-1.0f, 0.0f, 0.0f},
		{-1.0f, 0.0f, 0.0f},
		{-1.0f, 0.0f, 0.0f},
		{-1.0f, 0.0f, 0.0f},
		{-1.0f, 0.0f, 0.0f},
		{-1.0f, 0.0f, 0.0f}
	};

	m_pxmf3Normals = std::move(pxmf3Normals);

	/*m_pd3dNormalBuffer = CreateBufferResource(pd3dDevice, pd3dCommandList, m_pxmf3Normals.data(), sizeof(XMFLOAT3) * m_nVertices, D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, m_pd3dNormalUploadBuffer.GetAddressOf());
	
	{
		std::wstring name = debugName + L" : m_pd3dNormalBuffer";
		m_pd3dNormalBuffer->SetName(name.c_str());
	}

	m_d3dNormalBufferView.BufferLocation = m_pd3dNormalBuffer->GetGPUVirtualAddress();
	m_d3dNormalBufferView.StrideInBytes = sizeof(XMFLOAT3);
	m_d3dNormalBufferView.SizeInBytes = sizeof(XMFLOAT3) * static_cast<UINT>(m_pxmf3Normals.size());*/

	// texture
	std::vector<XMFLOAT2> xmf2Texture0Coords = {
		// Front face (+Z)
		{0.0f, 1.0f}, {1.0f, 1.0f}, {1.0f, 0.0f},
		{0.0f, 1.0f}, {1.0f, 0.0f}, {0.0f, 0.0f},

		// Back face (-Z)
		{1.0f, 1.0f}, {0.0f, 1.0f}, {0.0f, 0.0f},
		{1.0f, 1.0f}, {0.0f, 0.0f}, {1.0f, 0.0f},

		// Top face (+Y)
		{0.0f, 1.0f}, {0.0f, 0.0f}, {1.0f, 0.0f},
		{0.0f, 1.0f}, {1.0f, 0.0f}, {1.0f, 1.0f},

		// Bottom face (-Y)
		{0.0f, 0.0f}, {1.0f, 0.0f}, {1.0f, 1.0f},
		{0.0f, 0.0f}, {1.0f, 1.0f}, {0.0f, 1.0f},

		// Right face (+X)
		{1.0f, 0.0f}, {1.0f, 1.0f}, {0.0f, 1.0f},
		{1.0f, 0.0f}, {1.0f, 0.0f}, {0.0f, 1.0f},

		// Left face (-X)
		{0.0f, 0.0f}, {0.0f, 1.0f}, {1.0f, 1.0f},
		{0.0f, 0.0f}, {1.0f, 1.0f}, {1.0f, 0.0f}
	};

	m_pxmf2TextureCoords0 = std::move(xmf2Texture0Coords);

	//m_pd3dTextureCoord0Buffer = CreateBufferResource(pd3dDevice, pd3dCommandList, m_pxmf2TextureCoords0.data(), sizeof(XMFLOAT2) * m_nVertices, D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, m_pd3dTextureCoord0UploadBuffer.GetAddressOf());

	//{
	//	std::wstring name = debugName + L" : m_pd3dTextureCoord0Buffer";
	//	m_pd3dTextureCoord0Buffer->SetName(name.c_str());
	//}

	//m_d3dTextureCoord0BufferView.BufferLocation = m_pd3dTextureCoord0Buffer->GetGPUVirtualAddress();
	//m_d3dTextureCoord0BufferView.StrideInBytes = sizeof(XMFLOAT2);
	//m_d3dTextureCoord0BufferView.SizeInBytes = sizeof(XMFLOAT2) * static_cast<UINT>(m_pxmf2TextureCoords0.size());

	// Standard Shader를 사용하기위한 더미 데이터 생성
	// --------------------------------------------
	// Tangent Dummy
	std::vector<XMFLOAT3> xmf3Tangents(m_nVertices, XMFLOAT3(0.0f, 0.0f, 0.0f));

	m_pxmf3Tangents = std::move(xmf3Tangents);

	/*m_pd3dTangentBuffer = CreateBufferResource(pd3dDevice, pd3dCommandList, m_pxmf3Tangents.data(), sizeof(XMFLOAT3) * m_nVertices, D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, m_pd3dTangentUploadBuffer.GetAddressOf());

	{
		std::wstring name = debugName + L" : m_pd3dTangentBuffer";
		m_pd3dTangentBuffer->SetName(name.c_str());
	}

	m_d3dTangentBufferView.BufferLocation = m_pd3dTangentBuffer->GetGPUVirtualAddress();
	m_d3dTangentBufferView.StrideInBytes = sizeof(XMFLOAT3);
	m_d3dTangentBufferView.SizeInBytes = sizeof(XMFLOAT3) * static_cast<UINT>(m_pxmf3Tangents.size());*/

	// BiTangent Dummy

	std::vector<XMFLOAT3> xmf3BiTangents(m_nVertices, XMFLOAT3(0.0f, 0.0f, 0.0f));

	m_pxmf3BiTangents = std::move(xmf3BiTangents);

	/*m_pd3dBiTangentBuffer = CreateBufferResource(pd3dDevice, pd3dCommandList, m_pxmf3BiTangents.data(), sizeof(XMFLOAT3) * m_nVertices, D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, m_pd3dBiTangentUploadBuffer.GetAddressOf());
	{
		std::wstring name = debugName + L" : m_pd3dBiTangentBuffer";
		m_pd3dBiTangentBuffer->SetName(name.c_str());
	}
	m_d3dBiTangentBufferView.BufferLocation = m_pd3dBiTangentBuffer->GetGPUVirtualAddress();
	m_d3dBiTangentBufferView.StrideInBytes = sizeof(XMFLOAT3);
	m_d3dBiTangentBufferView.SizeInBytes = sizeof(XMFLOAT3) * static_cast<UINT>(m_pxmf3BiTangents.size());*/
}

CCubeMesh::~CCubeMesh()
{

}

///////////////////////////////////////////////////////////////////////////////
//

CSkyBoxMesh::CSkyBoxMesh(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, float fWidth, float fHeight, float fDepth)
	: CMesh(pd3dDevice, pd3dCommandList)
{
	static int nSkyBoxIndex = 0;
	std::string strName = "SkyBox_" + std::to_string(nSkyBoxIndex++);
	SetName(strName);

	// position
	float halfWidth = fWidth * 0.5f;
	float halfHeight = fHeight * 0.5f;
	float halfDepth = fDepth * 0.5f;

	m_nVertices = 36;
	m_d3dPrimitiveTopology = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

	m_pxmf3Positions.resize(m_nVertices);

	// Front Quad (quads point inward)
	m_pxmf3Positions[0] = XMFLOAT3(-halfWidth, +halfHeight, +halfDepth);
	m_pxmf3Positions[1] = XMFLOAT3(+halfWidth, +halfHeight, +halfDepth);
	m_pxmf3Positions[2] = XMFLOAT3(-halfWidth, -halfHeight, +halfDepth);
	m_pxmf3Positions[3] = XMFLOAT3(-halfWidth, -halfHeight, +halfDepth);
	m_pxmf3Positions[4] = XMFLOAT3(+halfWidth, +halfHeight, +halfDepth);
	m_pxmf3Positions[5] = XMFLOAT3(+halfWidth, -halfHeight, +halfDepth);
	// Back Quad										
	m_pxmf3Positions[6] = XMFLOAT3(+halfWidth, +halfHeight, -halfDepth);
	m_pxmf3Positions[7] = XMFLOAT3(-halfWidth, +halfHeight, -halfDepth);
	m_pxmf3Positions[8] = XMFLOAT3(+halfWidth, -halfHeight, -halfDepth);
	m_pxmf3Positions[9] = XMFLOAT3(+halfWidth, -halfHeight, -halfDepth);
	m_pxmf3Positions[10] = XMFLOAT3(-halfWidth, +halfHeight, -halfDepth);
	m_pxmf3Positions[11] = XMFLOAT3(-halfWidth, -halfHeight, -halfDepth);
	// Left Quad										
	m_pxmf3Positions[12] = XMFLOAT3(-halfWidth, +halfHeight, -halfDepth);
	m_pxmf3Positions[13] = XMFLOAT3(-halfWidth, +halfHeight, +halfDepth);
	m_pxmf3Positions[14] = XMFLOAT3(-halfWidth, -halfHeight, -halfDepth);
	m_pxmf3Positions[15] = XMFLOAT3(-halfWidth, -halfHeight, -halfDepth);
	m_pxmf3Positions[16] = XMFLOAT3(-halfWidth, +halfHeight, +halfDepth);
	m_pxmf3Positions[17] = XMFLOAT3(-halfWidth, -halfHeight, +halfDepth);
	// Right Quad										
	m_pxmf3Positions[18] = XMFLOAT3(+halfWidth, +halfHeight, +halfDepth);
	m_pxmf3Positions[19] = XMFLOAT3(+halfWidth, +halfHeight, -halfDepth);
	m_pxmf3Positions[20] = XMFLOAT3(+halfWidth, -halfHeight, +halfDepth);
	m_pxmf3Positions[21] = XMFLOAT3(+halfWidth, -halfHeight, +halfDepth);
	m_pxmf3Positions[22] = XMFLOAT3(+halfWidth, +halfHeight, -halfDepth);
	m_pxmf3Positions[23] = XMFLOAT3(+halfWidth, -halfHeight, -halfDepth);
	// Top Quad											
	m_pxmf3Positions[24] = XMFLOAT3(-halfWidth, +halfHeight, -halfDepth);
	m_pxmf3Positions[25] = XMFLOAT3(+halfWidth, +halfHeight, -halfDepth);
	m_pxmf3Positions[26] = XMFLOAT3(-halfWidth, +halfHeight, +halfDepth);
	m_pxmf3Positions[27] = XMFLOAT3(-halfWidth, +halfHeight, +halfDepth);
	m_pxmf3Positions[28] = XMFLOAT3(+halfWidth, +halfHeight, -halfDepth);
	m_pxmf3Positions[29] = XMFLOAT3(+halfWidth, +halfHeight, +halfDepth);
	// Bottom Quad										
	m_pxmf3Positions[30] = XMFLOAT3(-halfWidth, -halfHeight, +halfDepth);
	m_pxmf3Positions[31] = XMFLOAT3(+halfWidth, -halfHeight, +halfDepth);
	m_pxmf3Positions[32] = XMFLOAT3(-halfWidth, -halfHeight, -halfDepth);
	m_pxmf3Positions[33] = XMFLOAT3(-halfWidth, -halfHeight, -halfDepth);
	m_pxmf3Positions[34] = XMFLOAT3(+halfWidth, -halfHeight, +halfDepth);
	m_pxmf3Positions[35] = XMFLOAT3(+halfWidth, -halfHeight, -halfDepth);

	m_pd3dPositionBuffer = ::CreateBufferResource(pd3dDevice, pd3dCommandList, m_pxmf3Positions.data(), sizeof(XMFLOAT3) * m_nVertices, D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, &m_pd3dPositionUploadBuffer);

	m_d3dPositionBufferView.BufferLocation = m_pd3dPositionBuffer->GetGPUVirtualAddress();
	m_d3dPositionBufferView.StrideInBytes = sizeof(XMFLOAT3);
	m_d3dPositionBufferView.SizeInBytes = sizeof(XMFLOAT3) * m_nVertices;
	
}

CSkyBoxMesh::~CSkyBoxMesh()
{
}

///////////////////////////////////////////////////////////////////////////////
//

CHeightMapGridMesh::CHeightMapGridMesh(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, int xStart, int zStart, int nWidth, int nLength, XMFLOAT3 xmf3Scale, XMFLOAT4 xmf4Color, void* pContext)
	: CMesh(pd3dDevice, pd3dCommandList)
{
#ifdef _WITH_TERRAIN_TESSELATION
	m_d3dPrimitiveTopology = D3D_PRIMITIVE_TOPOLOGY_25_CONTROL_POINT_PATCHLIST;
	m_nVertices = 25;
#else
	m_d3dPrimitiveTopology = D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP;
	m_nVertices = nWidth * nLength;
#endif

	//격자의 교점(정점)의 개수는 (nWidth * nLength)이다. 
	//m_nVertices = nWidth * nLength;
	m_nStride = sizeof(CTerrainVertex);

	//격자는 삼각형 스트립으로 구성한다. 
	//m_d3dPrimitiveTopology = D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP;
	m_nWidth = nWidth;
	m_nLength = nLength;
	m_xmf3Scale = xmf3Scale;

	CTerrainVertex* pVertices = new CTerrainVertex[m_nVertices];

	/*xStart와 zStart는 격자의 시작 위치(x-좌표와 z-좌표)를 나타낸다.
	커다란 지형은 격자들의 이차원 배열로 만들 필요가 있기 때문에 전체 지형에서 각 격자의 시작 위치를 나타내는 정보가 필요하다.*/
	float fHeight = 0.0f, fMinHeight = +FLT_MAX, fMaxHeight = -FLT_MAX;
	CHeightMapImage* pHeightMapImage = (CHeightMapImage*)pContext;
	int cxHeightMap = pHeightMapImage->GetHeightMapWidth();
	int czHeightMap = pHeightMapImage->GetHeightMapLength();

#ifdef _WITH_TERRAIN_TESSELATION
#ifdef _WITH_TERRAIN_PARTITION
	int nIncrease = 3; //(Block Size == 9) ? 2, (Block Size == 13) ? 3
	for (int i = 0, z = (zStart + nLength - 1); z >= zStart; z -= nIncrease)
	{
		for (int x = xStart; x < (xStart + nWidth); x += nIncrease, i++)
		{
			float xPosition = x * m_xmf3Scale.x, zPosition = z * m_xmf3Scale.z;
			fHeight = pHeightMapImage->GetHeight(xPosition, zPosition, m_xmf3Scale);
			pVertices[i].m_xmf3Position = XMFLOAT3(xPosition, fHeight, zPosition);
			pVertices[i].m_xmf4Diffuse = Vector4::Add(OnGetColor(int(x), int(z), pContext), xmf4Color);
			pVertices[i].m_xmf3Normal = pHeightMapImage->GetHeightMapNormal(x, z);
			pVertices[i].m_xmf2TexCoord0 = XMFLOAT2(float(x) / float(cxHeightMap - 1), float(czHeightMap - 1 - z) / float(czHeightMap - 1));
			pVertices[i].m_xmf2TexCoord1 = XMFLOAT2(float(x) / float(m_xmf3Scale.x * 0.5f), float(z) / float(m_xmf3Scale.z * 0.5f));
			if (fHeight < fMinHeight) fMinHeight = fHeight;
			if (fHeight > fMaxHeight) fMaxHeight = fHeight;

		}
	}

#else
	int cxQuadsPerPatch = 5 - 1;
	int czQuadsPerPatch = 5 - 1;

	long cxPatches = (nWidth - 1) / cxQuadsPerPatch;
	long czPatches = (nLength - 1) / czQuadsPerPatch;
	for (int i = 0, z = 0, zStart = 0; z < czPatches; z++)
	{
		for (int x = 0, xStart = 0; x < cxPatches; x++)
		{
			xStart = x * (5 - 1);
			zStart = z * (5 - 1);
			float xPosition = x * m_xmf3Scale.x, zPosition = z * m_xmf3Scale.z;
			fHeight = pHeightMapImage->GetHeight(xPosition, zPosition, m_xmf3Scale);
			pVertices[i].m_xmf3Position = XMFLOAT3(xPosition, fHeight, zPosition);
			pVertices[i].m_xmf4Diffuse = Vector4::Add(OnGetColor(int(x), int(z), pContext), xmf4Color);
			pVertices[i].m_xmf2TexCoord0 = XMFLOAT2(float(x) / float(cxHeightMap - 1), float(czHeightMap - 1 - z) / float(czHeightMap - 1));
			pVertices[i].m_xmf2TexCoord1 = XMFLOAT2(float(x) / float(m_xmf3Scale.x * 0.5f), float(z) / float(m_xmf3Scale.z * 0.5f));
			if (fHeight < fMinHeight) fMinHeight = fHeight;
			if (fHeight > fMaxHeight) fMaxHeight = fHeight;
		}
	}

	for (int z = 0, zStart = 0; z < czBlocks; z++)
	{
		for (int x = 0, xStart = 0; x < cxBlocks; x++)
		{
			xStart = x * (nBlockWidth - 1);
			zStart = z * (nBlockLength - 1);
			pHeightMapGridMesh = new CHeightMapGridMesh(pd3dDevice, pd3dCommandList, xStart, zStart, nBlockWidth, nBlockLength, xmf3Scale, xmf4Color, m_pHeightMapImage);
			SetMesh(x + (z * cxBlocks), pHeightMapGridMesh);
		}
	}
	//
	for (int i = 0, z = (zStart + nLength - 1); z >= zStart; z -= nIncrease)
	{
		for (int x = xStart; x < (xStart + nWidth); x += nIncrease, i++)
		{
			for (int i = 0, z = (zStart + nLength - 1); z >= zStart; z -= nIncrease)
			{
				for (int j = 0; j < 5; j++, i++)
				{
					x += j;
					float xPosition = x * m_xmf3Scale.x, zPosition = z * m_xmf3Scale.z;
					fHeight = pHeightMapImage->GetHeight(xPosition, zPosition, m_xmf3Scale);
					pVertices[i].m_xmf3Position = XMFLOAT3(xPosition, fHeight, zPosition);
					pVertices[i].m_xmf4Diffuse = Vector4::Add(OnGetColor(int(x), int(z), pContext), xmf4Color);
					pVertices[i].m_xmf2TexCoord0 = XMFLOAT2(float(x) / float(cxHeightMap - 1), float(czHeightMap - 1 - z) / float(czHeightMap - 1));
					pVertices[i].m_xmf2TexCoord1 = XMFLOAT2(float(x) / float(m_xmf3Scale.x * 0.5f), float(z) / float(m_xmf3Scale.z * 0.5f));
					if (fHeight < fMinHeight) fMinHeight = fHeight;
					if (fHeight > fMaxHeight) fMaxHeight = fHeight;
				}
			}
		}
	}
#endif
#else
	for (int i = 0, z = zStart; z < (zStart + nLength); z++)
	{
		for (int x = xStart; x < (xStart + nWidth); x++, i++)
		{
			//정점의 높이와 색상을 높이 맵으로부터 구한다. 
			XMFLOAT3 xmf3Position = XMFLOAT3((x * m_xmf3Scale.x), OnGetHeight(x, z, pContext), (z * m_xmf3Scale.z));

			XMFLOAT4 xmf3Color = Vector4::Add(OnGetColor(x, z, pContext), xmf4Color);
			XMFLOAT3 xmf3Normal = pHeightMapImage->GetHeightMapNormal(x, z);
			XMFLOAT2 xmf2UV0 = XMFLOAT2{ (float)x / m_nWidth,(float)z / m_nLength };
			//XMFLOAT2 xmf2UV1 = XMFLOAT2{ (float)x / m_nWidth,(float)z / m_nLength };
			XMFLOAT2 xmf2UV1 = XMFLOAT2{ (float)x,(float)z };

			pVertices[i] = CTerrainVertex(xmf3Position, xmf3Color, xmf3Normal, xmf2UV0, xmf2UV1);
			if (fHeight < fMinHeight) fMinHeight = fHeight;
			if (fHeight > fMaxHeight) fMaxHeight = fHeight;
		}
	}
#endif

	//다음 그림은 격자의 교점(정점)을 나열하는 순서를 보여준다.
	m_pd3dPositionBuffer = ::CreateBufferResource(pd3dDevice, pd3dCommandList, pVertices,
		m_nStride * m_nVertices, D3D12_HEAP_TYPE_DEFAULT,
		D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, &m_pd3dPositionUploadBuffer);

	m_d3dPositionBufferView.BufferLocation = m_pd3dPositionBuffer->GetGPUVirtualAddress();
	m_d3dPositionBufferView.StrideInBytes = m_nStride;
	m_d3dPositionBufferView.SizeInBytes = m_nStride * m_nVertices;

	delete[] pVertices;


#ifndef _WITH_TERRAIN_TESSELATION
	int nIndices = ((nWidth * 2) * (nLength - 1)) + ((nLength - 1) - 1);
	std::vector<UINT> pnIndices(nIndices);
	for (int j = 0, z = 0; z < nLength - 1; z++)
	{
		if ((z % 2) == 0)
		{
			//홀수 번째 줄이므로(z = 0, 2, 4, ...) 인덱스의 나열 순서는 왼쪽에서 오른쪽 방향이다. 
			for (int x = 0; x < nWidth; x++)
			{
				//첫 번째 줄을 제외하고 줄이 바뀔 때마다(x == 0) 첫 번째 인덱스를 추가한다. 
				if ((x == 0) && (z > 0)) pnIndices[j++] = (UINT)(x + (z * nWidth));

				//아래(x, z), 위(x, z+1)의 순서로 인덱스를 추가한다. 
				pnIndices[j++] = (UINT)(x + (z * nWidth));
				pnIndices[j++] = (UINT)((x + (z * nWidth)) + nWidth);
			}
		}
		else
		{
			//짝수 번째 줄이므로(z = 1, 3, 5, ...) 인덱스의 나열 순서는 오른쪽에서 왼쪽 방향이다. 
			for (int x = nWidth - 1; x >= 0; x--)
			{
				//줄이 바뀔 때마다(x == (nWidth-1)) 첫 번째 인덱스를 추가한다.
				if (x == (nWidth - 1)) pnIndices[j++] = (UINT)(x + (z * nWidth));
				//아래(x, z), 위(x, z+1)의 순서로 인덱스를 추가한다. 
				pnIndices[j++] = (UINT)(x + (z * nWidth));
				pnIndices[j++] = (UINT)((x + (z * nWidth)) + nWidth);
			}
		}
	}

	SetSubSetAmount(1);

	m_ppnSubSetIndices[0].resize(nIndices);

	m_ppd3dSubSetIndexBuffers[0] = ::CreateBufferResource(pd3dDevice, pd3dCommandList, pnIndices.data(),
		sizeof(UINT) * nIndices, D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_INDEX_BUFFER,
		&m_ppd3dSubSetIndexUploadBuffers[0]);

	m_pd3dSubSetIndexBufferViews[0].BufferLocation = m_ppd3dSubSetIndexBuffers[0]->GetGPUVirtualAddress();
	m_pd3dSubSetIndexBufferViews[0].Format = DXGI_FORMAT_R32_UINT;
	m_pd3dSubSetIndexBufferViews[0].SizeInBytes = sizeof(UINT) * nIndices;

#endif
}

CHeightMapGridMesh::CHeightMapGridMesh(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, std::vector<CTerrainVertex>& pVertices, std::vector<UINT>& pIndices)
	: CMesh(pd3dDevice, pd3dCommandList)
{
	m_nVertices = (UINT)pVertices.size();

	m_d3dPrimitiveTopology = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	m_nStride = sizeof(CTerrainVertex);

	//격자는 삼각형 스트립으로 구성한다. 
	//m_d3dPrimitiveTopology = D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP;
	m_nWidth = 0;
	m_nLength = 0;
	m_pd3dPositionBuffer = ::CreateBufferResource(pd3dDevice, pd3dCommandList, pVertices.data(),
		sizeof(CTerrainVertex) * m_nVertices, D3D12_HEAP_TYPE_DEFAULT,
		D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, &m_pd3dPositionUploadBuffer);
	m_d3dPositionBufferView.BufferLocation = m_pd3dPositionBuffer->GetGPUVirtualAddress();
	m_d3dPositionBufferView.StrideInBytes = m_nStride;
	m_d3dPositionBufferView.SizeInBytes = sizeof(CTerrainVertex) * m_nVertices;

	SetSubSetAmount(1);
	m_ppnSubSetIndices[0] = pIndices;

	m_ppd3dSubSetIndexBuffers[0] = ::CreateBufferResource(pd3dDevice, pd3dCommandList, m_ppnSubSetIndices[0].data(),
		sizeof(UINT) * pIndices.size(), D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_INDEX_BUFFER,
		&m_ppd3dSubSetIndexUploadBuffers[0]);

	m_pd3dSubSetIndexBufferViews[0].BufferLocation = m_ppd3dSubSetIndexBuffers[0]->GetGPUVirtualAddress();
	m_pd3dSubSetIndexBufferViews[0].Format = DXGI_FORMAT_R32_UINT;
	m_pd3dSubSetIndexBufferViews[0].SizeInBytes = sizeof(UINT) * pIndices.size();
}

CHeightMapGridMesh::~CHeightMapGridMesh()
{
}

float CHeightMapGridMesh::OnGetHeight(int x, int z, void* pContext)
{
	CHeightMapImage* pHeightMapImage = (CHeightMapImage*)pContext;
	std::vector<SHORT> pHeightMapPixels = pHeightMapImage->GetHeightMapPixels();
	XMFLOAT3 xmf3Scale = pHeightMapImage->GetScale();

	int nWidth = pHeightMapImage->GetHeightMapWidth();
	float fHeight = pHeightMapPixels[x + (z * nWidth)] * xmf3Scale.y;

	return(fHeight);
}

XMFLOAT4 CHeightMapGridMesh::OnGetColor(int x, int z, void* pContext)
{
	//조명의 방향 벡터(정점에서 조명까지의 벡터)이다. 
	XMFLOAT3 xmf3LightDirection = XMFLOAT3(-1.0f, 1.0f, 1.0f);
	xmf3LightDirection = Vector3::Normalize(xmf3LightDirection);
	CHeightMapImage* pHeightMapImage = (CHeightMapImage*)pContext;
	XMFLOAT3 xmf3Scale = pHeightMapImage->GetScale();
	//조명의 색상(세기, 밝기)이다. 
	XMFLOAT4 xmf4IncidentLightColor(0.9f, 0.8f, 0.0f, 1.0f);
	/*정점 (x, z)에서 조명이 반사되는 양(비율)은 정점 (x, z)의 법선 벡터와 조명의 방향 벡터의 내적(cos)과
	인접한 3개의 정점 (x+1, z), (x, z+1), (x+1, z+1)의 법선 벡터와 조명의 방향 벡터의 내적을 평균하여 구한다.
	정점 (x, z)의 색상은 조명 색상(세기)과 반사되는 양(비율)을 곱한 값이다.*/
	float fScale = Vector3::DotProduct(pHeightMapImage->GetHeightMapNormal(x, z),
		xmf3LightDirection);
	fScale += Vector3::DotProduct(pHeightMapImage->GetHeightMapNormal(x + 1, z),
		xmf3LightDirection);
	fScale += Vector3::DotProduct(pHeightMapImage->GetHeightMapNormal(x + 1, z + 1),
		xmf3LightDirection);
	fScale += Vector3::DotProduct(pHeightMapImage->GetHeightMapNormal(x, z + 1),
		xmf3LightDirection);
	fScale = (fScale / 4.0f) + 0.05f;
	if (fScale > 1.0f) fScale = 1.0f;
	if (fScale < 0.25f) fScale = 0.25f;

	//fScale은 조명 색상(밝기)이 반사되는 비율이다. 
	XMFLOAT4 xmf4Color = Vector4::Multiply(fScale, xmf4IncidentLightColor);
	return(xmf4Color);
}

XMFLOAT2 CHeightMapGridMesh::OnGetUVs(int x, int z, void* pContext)
{
	//높이 맵 이미지의 픽셀 좌표를 UV 좌표로 변환한다.
	XMFLOAT2 UV = XMFLOAT2((float)x / m_nWidth, (float)z / m_nLength);

	return UV;
}

///////////////////////////////////////////////////////////////////////////////
//

CSkinnedMesh::CSkinnedMesh(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList)
	: CStandardMesh(pd3dDevice, pd3dCommandList)
{
}

CSkinnedMesh::~CSkinnedMesh()
{
}

void CSkinnedMesh::PrepareSkinning(std::shared_ptr<CGameObject> pModelRootObject)
{
	for (int j = 0; j < m_nSkinningBones; j++)
	{
		m_ppSkinningBoneFrameCaches[j] = pModelRootObject->FindFrame(m_ppstrSkinningBoneNames[j]);
	}
}

void CSkinnedMesh::LoadSkinInfoFromFile(std::ifstream& pInFile)
{
	char pstrToken[64] = { '\0' };
	UINT nReads = 0;

	::ReadStringFromFile(pInFile, m_strMeshName);

	for (; ; )
	{
		::ReadStringFromFile(pInFile, pstrToken);
		if (!strcmp(pstrToken, "<BonesPerVertex>:"))
		{
			m_nBonesPerVertex = ::ReadIntegerFromFile(pInFile);
		}
		else if (!strcmp(pstrToken, "<Bounds>:"))
		{
			pInFile.read((char*)&m_xmf3AABBCenter, sizeof(XMFLOAT3));
			pInFile.read((char*)&m_xmf3AABBExtents, sizeof(XMFLOAT3));
		}
		else if (!strcmp(pstrToken, "<BoneNames>:"))
		{
			m_nSkinningBones = ::ReadIntegerFromFile(pInFile);
			if (m_nSkinningBones > 0)
			{
				m_ppstrSkinningBoneNames.resize(m_nSkinningBones);
				m_ppSkinningBoneFrameCaches.resize(m_nSkinningBones);
				for (int i = 0; i < m_nSkinningBones; i++)
				{
					::ReadStringFromFile(pInFile, m_ppstrSkinningBoneNames[i]);
					m_ppSkinningBoneFrameCaches[i] = NULL;
				}
			}
		}
		else if (!strcmp(pstrToken, "<BoneOffsets>:"))
		{
			m_nSkinningBones = ::ReadIntegerFromFile(pInFile);
			if (m_nSkinningBones > 0)
			{
				m_pxmf4x4BindPoseBoneOffsets.resize(m_nSkinningBones);
				pInFile.read((char*)m_pxmf4x4BindPoseBoneOffsets.data(), sizeof(XMFLOAT4X4) * m_nSkinningBones);
			}
		}
		else if (!strcmp(pstrToken, "<BoneIndices>:"))
		{
			m_nType |= VERTEXT_BONE_INDEX_WEIGHT;

			m_nVertices = ::ReadIntegerFromFile(pInFile);
			if (m_nVertices > 0)
			{
				m_pxmn4BoneIndices.resize(m_nVertices);
				pInFile.read((char*)m_pxmn4BoneIndices.data(), sizeof(XMINT4) * m_nVertices);
			}
		}
		else if (!strcmp(pstrToken, "<BoneWeights>:"))
		{
			m_nType |= VERTEXT_BONE_INDEX_WEIGHT;

			m_nVertices = ::ReadIntegerFromFile(pInFile);
			if (m_nVertices > 0)
			{
				m_pxmf4BoneWeights.resize(m_nVertices);
				pInFile.read((char*)m_pxmf4BoneWeights.data(), sizeof(XMFLOAT4) * m_nVertices);
			}
		}
		else if (!strcmp(pstrToken, "</SkinningInfo>"))
		{
			break;
		}
	}
}

void CSkinnedMesh::CreateShaderVariables(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList)
{
	// Create Standard Mesh Shader Variables
	CStandardMesh::CreateShaderVariables(pd3dDevice, pd3dCommandList);

	// Create Skinning Bone Offset Buffer
	UINT ncbElementBytes = (((sizeof(XMFLOAT4X4) * SKINNED_ANIMATION_BONES) + 255) & ~255); //256의 배수
	m_pd3dcbBindPoseBoneOffsets = ::CreateBufferResource(pd3dDevice, pd3dCommandList, NULL, ncbElementBytes, D3D12_HEAP_TYPE_UPLOAD, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, NULL);
	m_pd3dcbBindPoseBoneOffsets->Map(0, NULL, (void**)&m_pcbxmf4x4MappedBindPoseBoneOffsets);

	for (int i = 0; i < m_nSkinningBones; i++)
	{
		XMStoreFloat4x4(&m_pcbxmf4x4MappedBindPoseBoneOffsets[i], XMMatrixTranspose(XMLoadFloat4x4(&m_pxmf4x4BindPoseBoneOffsets[i])));
	}

	// Create Bone Index Buffer
	m_pd3dBoneIndexBuffer = ::CreateBufferResource(pd3dDevice, pd3dCommandList, m_pxmn4BoneIndices.data(), sizeof(XMINT4) * m_nVertices, D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, &m_pd3dBoneIndexUploadBuffer);

	m_d3dBoneIndexBufferView.BufferLocation = m_pd3dBoneIndexBuffer->GetGPUVirtualAddress();
	m_d3dBoneIndexBufferView.StrideInBytes = sizeof(XMINT4);
	m_d3dBoneIndexBufferView.SizeInBytes = sizeof(XMINT4) * m_nVertices;

	// Create Bone Weight Buffer
	m_pd3dBoneWeightBuffer = ::CreateBufferResource(pd3dDevice, pd3dCommandList, m_pxmf4BoneWeights.data(), sizeof(XMFLOAT4) * m_nVertices, D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, &m_pd3dBoneWeightUploadBuffer);

	m_d3dBoneWeightBufferView.BufferLocation = m_pd3dBoneWeightBuffer->GetGPUVirtualAddress();
	m_d3dBoneWeightBufferView.StrideInBytes = sizeof(XMFLOAT4);
	m_d3dBoneWeightBufferView.SizeInBytes = sizeof(XMFLOAT4) * m_nVertices;
}

void CSkinnedMesh::UpdateShaderVariables(ID3D12GraphicsCommandList* pd3dCommandList)
{
	if (m_pd3dcbBindPoseBoneOffsets)
	{
		D3D12_GPU_VIRTUAL_ADDRESS d3dcbBoneOffsetsGpuVirtualAddress = m_pd3dcbBindPoseBoneOffsets->GetGPUVirtualAddress();
		pd3dCommandList->SetGraphicsRootConstantBufferView(ROOT_PARAMETER_SKINNED_BONE_OFFSETS, d3dcbBoneOffsetsGpuVirtualAddress); //Skinned Bone Offsets
	}

	if (m_pd3dcbSkinningBoneTransforms)
	{
		D3D12_GPU_VIRTUAL_ADDRESS d3dcbBoneTransformsGpuVirtualAddress = m_pd3dcbSkinningBoneTransforms->GetGPUVirtualAddress();
		pd3dCommandList->SetGraphicsRootConstantBufferView(ROOT_PARAMETER_SKINNED_BONE_TRANSFORM, d3dcbBoneTransformsGpuVirtualAddress); //Skinned Bone Transforms

		// UpdateSkinningBoneTransforms(m_pcbxmf4x4MappedSkinningBoneTransforms);
		pd3dCommandList->SetGraphicsRoot32BitConstants(ROOT_PARAMETER_OBJECT, 1, &m_nSkinningBoneTransformsOffset, 20);
	}
}

void CSkinnedMesh::ReleaseShaderVariables()
{
}

void CSkinnedMesh::ReleaseUploadBuffers()
{
	CStandardMesh::ReleaseUploadBuffers();

	if (m_pd3dBoneIndexUploadBuffer) m_pd3dBoneIndexUploadBuffer.Reset();
	if (m_pd3dBoneWeightUploadBuffer) m_pd3dBoneWeightUploadBuffer.Reset();
}

void CSkinnedMesh::OnPreRender(ID3D12GraphicsCommandList* pd3dCommandList, void* pContext)
{
	D3D12_VERTEX_BUFFER_VIEW pVertexBufferViews[7] = { m_d3dPositionBufferView, m_d3dTextureCoord0BufferView, m_d3dNormalBufferView, m_d3dTangentBufferView, m_d3dBiTangentBufferView, m_d3dBoneIndexBufferView, m_d3dBoneWeightBufferView };
	pd3dCommandList->IASetVertexBuffers(m_nSlot, 7, pVertexBufferViews);
}

void CSkinnedMesh::UpdateSkinningBoneTransforms(XMFLOAT4X4* pcbxmf4x4MappedSkinningBoneTransforms)
{
	for (int j = 0; j < m_nSkinningBones; j++)
	{
		XMFLOAT4X4 WorldMatrix = m_ppSkinningBoneFrameCaches[j]->GetWorldMatrix();
		XMStoreFloat4x4(&pcbxmf4x4MappedSkinningBoneTransforms[j], XMMatrixTranspose(XMLoadFloat4x4(&WorldMatrix)));
	}
}

void CSkinnedMesh::UpdateSkinningBoneTransforms(std::vector<XMFLOAT4X4>& pxmf4x4MappedSkinningBoneTransforms)
{
	for (int j = 0; j < m_nSkinningBones; j++)
	{
		XMFLOAT4X4 WorldMatrix = m_ppSkinningBoneFrameCaches[j]->GetWorldMatrix();
		XMStoreFloat4x4(&pxmf4x4MappedSkinningBoneTransforms[j], XMMatrixTranspose(XMLoadFloat4x4(&WorldMatrix)));
	}
}

///////////////////////////////////////////////////////////////////////////////////
//

CSphereMesh::CSphereMesh(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, float fRadius, int nSlices, int nStacks)
	: CStandardMesh(pd3dDevice, pd3dCommandList)
{
	// 최소 값 보정
	if (nSlices < 3) nSlices = 3;
	if (nStacks < 2) nStacks = 2;

	m_d3dPrimitiveTopology = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

	// 로컬 버퍼들
	std::vector<XMFLOAT3> xmf3Positions;
	std::vector<XMFLOAT3> xmf3Normals;
	std::vector<XMFLOAT3> xmf3Tangents;
	std::vector<XMFLOAT3> xmf3BiTangents;
	std::vector<XMFLOAT2> xmf2TextureCoords;
	std::vector<UINT> pnIndices;

	float phiStep = XM_PI / nStacks;					// 위도 (0..PI)
	float thetaStep = 2.0f * XM_PI / nSlices;		// 경도 (0..2PI)

	// 정점 생성: (nStacks + 1) x (nSlices + 1) (wrap을 위해 +1)
	int nVerticesPerRow = nSlices + 1;
	int nVertexCount = (nStacks + 1) * nVerticesPerRow;
	xmf3Positions.reserve(nVertexCount);
	xmf3Normals.reserve(nVertexCount);
	xmf3Tangents.reserve(nVertexCount);
	xmf3BiTangents.reserve(nVertexCount);
	xmf2TextureCoords.reserve(nVertexCount);

	for (int i = 0; i <= nStacks; ++i)
	{
		float phi = i * phiStep;					// 0..PI
		float cosPhi = cosf(phi);
		float sinPhi = sinf(phi);

		for (int j = 0; j <= nSlices; ++j)
		{
			float theta = j * thetaStep;			// 0..2PI
			float cosTheta = cosf(theta);
			float sinTheta = sinf(theta);

			// position
			float x = fRadius * sinPhi * cosTheta;
			float y = fRadius * cosPhi;
			float z = fRadius * sinPhi * sinTheta;
			XMFLOAT3 position = XMFLOAT3(x, y, z);
			xmf3Positions.push_back(position);

			// normal (정규화된 위치 벡터)
			XMVECTOR vPos = XMVectorSet(x, y, z, 0.0f);
			XMVECTOR vNormal = XMVector3Normalize(vPos);
			XMFLOAT3 normal;
			XMStoreFloat3(&normal, vNormal);
			xmf3Normals.push_back(normal);

			// tangent: ∂P/∂θ = r * sin(phi) * (-sinθ, 0, cosθ)
			XMVECTOR vT = XMVectorSet(-fRadius * sinPhi * sinTheta, 0.0f, fRadius * sinPhi * cosTheta, 0.0f);
			float tlen = XMVectorGetX(XMVector3Length(vT));
			if (tlen > 1e-6f)
			{
				vT = XMVector3Normalize(vT);
			}
			else
			{
				// 극지방(폴)에서의 fallback tangent
				vT = XMVectorSet(1.0f, 0.0f, 0.0f, 0.0f);
			}
			XMFLOAT3 tangent;
			XMStoreFloat3(&tangent, vT);
			xmf3Tangents.push_back(tangent);

			// bitangent = cross(normal, tangent) (정규화)
			XMVECTOR vB = XMVector3Cross(vNormal, vT);
			vB = XMVector3Normalize(vB);
			XMFLOAT3 bitangent;
			XMStoreFloat3(&bitangent, vB);
			xmf3BiTangents.push_back(bitangent);

			// UV0, UV1 (UV1을 동일하게 복사)
			float u = (float)j / (float)nSlices;		// 0..1
			float v = 1.0f - (float)i / (float)nStacks; // 위/아래 텍스처 정렬을 기존 코드 스타일에 맞춰 뒤집음
			xmf2TextureCoords.push_back(XMFLOAT2(u, v));
		}
	}

	// 인덱스 생성 (triangle list)
	for (int i = 0; i < nStacks; ++i)
	{
		for (int j = 0; j < nSlices; ++j)
		{
			UINT first = (UINT)(i * nVerticesPerRow + j);
			UINT second = (UINT)(first + nVerticesPerRow);

			// triangle 1
			pnIndices.push_back(first);
			pnIndices.push_back(second);
			pnIndices.push_back(first + 1);

			// triangle 2
			pnIndices.push_back(first + 1);
			pnIndices.push_back(second);
			pnIndices.push_back(second + 1);
		}
	}

	// 멤버에 이동(생성한 데이터 연결)
	m_nVertices = (UINT)xmf3Positions.size();
	m_pxmf3Positions = std::move(xmf3Positions);
	m_pxmf3Normals = std::move(xmf3Normals);
	m_pxmf3Tangents = std::move(xmf3Tangents);
	m_pxmf3BiTangents = std::move(xmf3BiTangents);
	m_pxmf2TextureCoords0 = std::move(xmf2TextureCoords);
	m_pxmf2TextureCoords1 = m_pxmf2TextureCoords0; // UV1을 동일하게 사용

	// subset / index 설정

	SetSubMeshCount(1);
	m_ppnSubSetIndices[0] = std::move(pnIndices);

	// AABB / BoundingBox 설정
	m_xmf3AABBCenter = XMFLOAT3(0.0f, 0.0f, 0.0f);
	m_xmf3AABBExtents = XMFLOAT3(fRadius, fRadius, fRadius);
	m_xmBoundingBox = BoundingBox(m_xmf3AABBCenter, m_xmf3AABBExtents);
}

CSphereMesh::~CSphereMesh()
{
}

////////////////////////////////////////////////////////////////////////////////////
//

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
CBulletMesh::CBulletMesh(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, XMFLOAT3 xmf3Position, XMFLOAT3 xmf3Look, float fLifetime, XMFLOAT3 xmf3Acceleration, XMFLOAT3 xmf3Color, XMFLOAT2 xmf2Size, UINT nMaxParticles) : CMesh(pd3dDevice, pd3dCommandList)
{
	CreateVertexBuffer(pd3dDevice, pd3dCommandList, xmf3Position, xmf3Look, fLifetime, xmf3Acceleration, xmf3Color, xmf2Size);
	CreateStreamOutputBuffer(pd3dDevice, pd3dCommandList, nMaxParticles);
}

void CBulletMesh::CreateVertexBuffer(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, XMFLOAT3 xmf3Position, XMFLOAT3 xmf3Look, float fLifetime, XMFLOAT3 xmf3Acceleration, XMFLOAT3 xmf3Color, XMFLOAT2 xmf2Size)
{
	m_nVertices = 1;
	m_nStride = sizeof(CBulletVertex);
	m_d3dPrimitiveTopology = D3D_PRIMITIVE_TOPOLOGY_POINTLIST;

	CBulletVertex pVertices[1];

	pVertices[0].m_xmf3Destination = XMFLOAT3{ 0,0,0 };
	pVertices[0].m_xmf3Position = XMFLOAT3{ 0,0,0 };
	pVertices[0].m_xmf3Velocity = XMFLOAT3{ 0,0,0 };
	pVertices[0].m_fLifetime = 0.0f;
	pVertices[0].m_nBulletType = BULLET_TYPE_MAINTAIN;
	pVertices[0].m_nHitObjectType = HIT_TYPE_ENVIRONMENT;

	m_pd3dPositionBuffer = ::CreateBufferResource(pd3dDevice, pd3dCommandList, pVertices, m_nStride * m_nVertices, D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, &m_pd3dPositionUploadBuffer);

	m_d3dPositionBufferView.BufferLocation = m_pd3dPositionBuffer->GetGPUVirtualAddress();
	m_d3dPositionBufferView.StrideInBytes = m_nStride;
	m_d3dPositionBufferView.SizeInBytes = m_nStride * m_nVertices;
}

void CBulletMesh::CreateStreamOutputBuffer(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, UINT nMaxParticles)
{
	m_nMaxBullets = nMaxParticles;

	m_pd3dStreamOutputBuffer = ::CreateBufferResource(pd3dDevice, pd3dCommandList, NULL, (m_nStride * m_nMaxBullets), D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_STREAM_OUT, NULL);
	m_pd3dDrawBuffer = ::CreateBufferResource(pd3dDevice, pd3dCommandList, NULL, (m_nStride * m_nMaxBullets), D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, NULL);

	// DrawBuffer에 대한 Upload 버퍼를 생성한다.
	m_pd3dUploadDrawBuffer = ::CreateBufferResource(pd3dDevice, pd3dCommandList, NULL, m_nStride, D3D12_HEAP_TYPE_UPLOAD, D3D12_RESOURCE_STATE_GENERIC_READ, NULL);

	UINT64 nBufferFilledSize = 0;
	m_pd3dDefaultBufferFilledSize = ::CreateBufferResource(pd3dDevice, pd3dCommandList, &nBufferFilledSize, sizeof(UINT64), D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_STREAM_OUT, NULL);

	m_pd3dUploadBufferFilledSize = ::CreateBufferResource(pd3dDevice, pd3dCommandList, NULL, sizeof(UINT64), D3D12_HEAP_TYPE_UPLOAD, D3D12_RESOURCE_STATE_GENERIC_READ, NULL);
	m_pd3dUploadBufferFilledSize->Map(0, NULL, (void**)&m_pnUploadBufferFilledSize);

#ifdef _WITH_QUERY_DATA_SO_STATISTICS
	D3D12_QUERY_HEAP_DESC d3dQueryHeapDesc = { };
	d3dQueryHeapDesc.Type = D3D12_QUERY_HEAP_TYPE_SO_STATISTICS;
	d3dQueryHeapDesc.Count = 1;
	d3dQueryHeapDesc.NodeMask = 0;
	pd3dDevice->CreateQueryHeap(&d3dQueryHeapDesc, __uuidof(ID3D12QueryHeap), (void**)&m_pd3dSOQueryHeap);

	m_pd3dSOQueryBuffer = ::CreateBufferResource(pd3dDevice, pd3dCommandList, NULL, sizeof(D3D12_QUERY_DATA_SO_STATISTICS), D3D12_HEAP_TYPE_READBACK, D3D12_RESOURCE_STATE_COPY_DEST, NULL);
#else
	m_pd3dReadBackBufferFilledSize = ::CreateBufferResource(pd3dDevice, pd3dCommandList, NULL, sizeof(UINT64), D3D12_HEAP_TYPE_READBACK, D3D12_RESOURCE_STATE_COPY_DEST, NULL);
#endif
}

CBulletMesh::~CBulletMesh()
{
	if (m_pd3dStreamOutputBuffer) m_pd3dStreamOutputBuffer.Reset();
	if (m_pd3dDrawBuffer) m_pd3dDrawBuffer.Reset();
	if (m_pd3dDefaultBufferFilledSize) m_pd3dDefaultBufferFilledSize.Reset();
	if (m_pd3dUploadBufferFilledSize) m_pd3dUploadBufferFilledSize.Reset();

#ifdef _WITH_QUERY_DATA_SO_STATISTICS
	if (m_pd3dSOQueryBuffer) m_pd3dSOQueryBuffer.Reset();
	if (m_pd3dSOQueryHeap) m_pd3dSOQueryHeap.Reset();
#else
	if (m_pd3dReadBackBufferFilledSize) m_pd3dReadBackBufferFilledSize.Reset();
#endif
}

void CBulletMesh::PreRender(ID3D12GraphicsCommandList* pd3dCommandList, int nPipelineState)
{
	if (nPipelineState == 0)
	{
		if (m_bStart)
		{
			m_bStart = false;

			m_nVertices = 1;

			m_d3dPositionBufferView.BufferLocation = m_pd3dPositionBuffer->GetGPUVirtualAddress();
			m_d3dPositionBufferView.StrideInBytes = m_nStride;
			m_d3dPositionBufferView.SizeInBytes = m_nStride * m_nVertices;
		}
		else
		{
			m_d3dPositionBufferView.BufferLocation = m_pd3dDrawBuffer->GetGPUVirtualAddress();
			m_d3dPositionBufferView.StrideInBytes = m_nStride;
			m_d3dPositionBufferView.SizeInBytes = m_nStride * m_nVertices;
		}
		m_d3dStreamOutputBufferView.BufferLocation = m_pd3dStreamOutputBuffer->GetGPUVirtualAddress();
		m_d3dStreamOutputBufferView.SizeInBytes = m_nStride * m_nMaxBullets;
		m_d3dStreamOutputBufferView.BufferFilledSizeLocation = m_pd3dDefaultBufferFilledSize->GetGPUVirtualAddress();

		// *m_pnUploadBufferFilledSize = m_nStride * m_nVertices;
		*m_pnUploadBufferFilledSize = 0;

		::SynchronizeResourceTransition(pd3dCommandList, m_pd3dDefaultBufferFilledSize.Get(), D3D12_RESOURCE_STATE_STREAM_OUT, D3D12_RESOURCE_STATE_COPY_DEST);
		pd3dCommandList->CopyResource(m_pd3dDefaultBufferFilledSize.Get(), m_pd3dUploadBufferFilledSize.Get());
		::SynchronizeResourceTransition(pd3dCommandList, m_pd3dDefaultBufferFilledSize.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_STREAM_OUT);
	}
	else if (nPipelineState == 1)
	{
		::SynchronizeResourceTransition(pd3dCommandList, m_pd3dStreamOutputBuffer.Get(), D3D12_RESOURCE_STATE_STREAM_OUT, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
		::SynchronizeResourceTransition(pd3dCommandList, m_pd3dDrawBuffer.Get(), D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, D3D12_RESOURCE_STATE_STREAM_OUT);

		::SwapResourcePointer(m_pd3dDrawBuffer, m_pd3dStreamOutputBuffer);

		m_d3dPositionBufferView.BufferLocation = m_pd3dDrawBuffer->GetGPUVirtualAddress();
		m_d3dPositionBufferView.StrideInBytes = m_nStride;
		m_d3dPositionBufferView.SizeInBytes = m_nStride * m_nVertices;
	}
}

void CBulletMesh::Render(ID3D12GraphicsCommandList* pd3dCommandList, int nPipelineState)
{
	if (nPipelineState == 0)
	{
		D3D12_STREAM_OUTPUT_BUFFER_VIEW pStreamOutputBufferViews[1] = { m_d3dStreamOutputBufferView };
		pd3dCommandList->SOSetTargets(0, 1, pStreamOutputBufferViews);

#ifdef _WITH_QUERY_DATA_SO_STATISTICS
		pd3dCommandList->BeginQuery(m_pd3dSOQueryHeap, D3D12_QUERY_TYPE_SO_STATISTICS_STREAM0, 0);
		CMesh::Render(pd3dCommandList);
		pd3dCommandList->EndQuery(m_pd3dSOQueryHeap, D3D12_QUERY_TYPE_SO_STATISTICS_STREAM0, 0);

		pd3dCommandList->ResolveQueryData(m_pd3dSOQueryHeap, D3D12_QUERY_TYPE_SO_STATISTICS_STREAM0, 0, 1, m_pd3dSOQueryBuffer, 0);
#else
		CMesh::Render(pd3dCommandList); //Stream Output to m_pd3dStreamOutputBuffer

		::SynchronizeResourceTransition(pd3dCommandList, m_pd3dDefaultBufferFilledSize.Get(), D3D12_RESOURCE_STATE_STREAM_OUT, D3D12_RESOURCE_STATE_COPY_SOURCE);
		pd3dCommandList->CopyResource(m_pd3dReadBackBufferFilledSize.Get(), m_pd3dDefaultBufferFilledSize.Get());
		::SynchronizeResourceTransition(pd3dCommandList, m_pd3dDefaultBufferFilledSize.Get(), D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_STREAM_OUT);
#endif
	}
	else if (nPipelineState == 1)
	{
		pd3dCommandList->SOSetTargets(0, 1, NULL);

		CMesh::Render(pd3dCommandList); //Render m_pd3dDrawBuffer 
	}
}

void CBulletMesh::PostRender(ID3D12GraphicsCommandList* pd3dCommandList, int nPipelineState)
{
}

#define _WITH_DEBUG_STREAM_OUTPUT_VERTICES

void CBulletMesh::OnPostRender(int nPipelineState)
{
	if (nPipelineState == 0)
	{
#ifdef _WITH_QUERY_DATA_SO_STATISTICS
		D3D12_RANGE d3dReadRange = { 0, 0 };
		UINT8* pBufferDataBegin = NULL;
		m_pd3dSOQueryBuffer->Map(0, &d3dReadRange, (void**)&m_pd3dSOQueryDataStatistics);
		if (m_pd3dSOQueryDataStatistics) m_nVertices = (UINT)m_pd3dSOQueryDataStatistics->NumPrimitivesWritten;
		m_pd3dSOQueryBuffer->Unmap(0, NULL);
#else
		UINT64* pnReadBackBufferFilledSize = NULL;
		m_pd3dReadBackBufferFilledSize->Map(0, NULL, (void**)&pnReadBackBufferFilledSize);
		m_nVertices = UINT(*pnReadBackBufferFilledSize) / m_nStride;
		m_pd3dReadBackBufferFilledSize->Unmap(0, NULL);
#endif

		::gnCurrentBullets = m_nVertices;
#ifdef _WITH_DEBUG_STREAM_OUTPUT_VERTICES
		TCHAR pstrDebug[256] = { 0 };
		_stprintf_s(pstrDebug, 256, _T("Stream Output Vertices = %d\n"), m_nVertices);
		OutputDebugString(pstrDebug);
#endif
		if ((m_nVertices == 0) || (m_nVertices >= MAX_BULLETS)) m_bStart = true;
	}
}

#include "GameFramework.h"

void CBulletMesh::AddBullet(const CBulletVertex& Bullet)
{
	// TODO : 나중에 한번에 업로드 하는 방식으로 변경
	if(false){
		auto& uploadContext = CUploadContext::Instance();
		ID3D12GraphicsCommandList* m_pd3dCommandList = uploadContext.m_pd3dGraphicCommandList;

		m_pd3dUploadDrawBuffer->Map(0, NULL, (void**)&m_pBullets);

		// 새로운 입자를 업로드 버퍼에 추가한다.
		memcpy(m_pBullets, &Bullet, sizeof(CBulletVertex));

		// 디폴트 버퍼를 복사 상태로 전환한다.
		::SynchronizeResourceTransition(m_pd3dCommandList, m_pd3dDrawBuffer.Get(), D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, D3D12_RESOURCE_STATE_COPY_DEST);

		// 업로드 버퍼 내용을 디폴트 버퍼로 복사한다.
		m_pd3dCommandList->CopyBufferRegion(m_pd3dDrawBuffer.Get(), m_nStride * m_nVertices, m_pd3dUploadDrawBuffer.Get(), 0, m_nStride);

		// 디폴트 버퍼 상태 복원
		::SynchronizeResourceTransition(m_pd3dCommandList, m_pd3dDrawBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);

		m_pd3dUploadDrawBuffer->Unmap(0, NULL);

		uploadContext.ExecuteAndReset();

		// 정점 갯수를 증가시킨다.
		::gnCurrentBullets = ++m_nVertices;
	}
}

void CBulletMesh::AddBullets(const std::vector<CBulletVertex>& Bullets)
{
	if(false){
		auto& uploadContext = CUploadContext::Instance();
		ID3D12GraphicsCommandList* m_pd3dCommandList = uploadContext.m_pd3dGraphicCommandList;

		m_pd3dUploadDrawBuffer->Map(0, NULL, (void**)&m_pBullets);

		// 새로운 입자를 업로드 버퍼에 추가한다.
		memcpy(m_pBullets, Bullets.data(), sizeof(CBulletVertex) * Bullets.size());

		// 디폴트 버퍼를 복사 상태로 전환한다.
		::SynchronizeResourceTransition(m_pd3dCommandList, m_pd3dDrawBuffer.Get(), D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, D3D12_RESOURCE_STATE_COPY_DEST);

		// 업로드 버퍼 내용을 디폴트 버퍼로 복사한다.
		m_pd3dCommandList->CopyBufferRegion(m_pd3dDrawBuffer.Get(), m_nStride * m_nVertices, m_pd3dUploadDrawBuffer.Get(), 0, m_nStride);

		// 디폴트 버퍼 상태 복원
		::SynchronizeResourceTransition(m_pd3dCommandList, m_pd3dDrawBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);

		m_pd3dUploadDrawBuffer->Unmap(0, NULL);

		uploadContext.ExecuteAndReset();
	}

	// 정점 갯수를 증가시킨다.
	::gnCurrentBullets = m_nVertices = (m_nVertices + Bullets.size());
}


///////////////////////////////////////////////////////////////////////////////
// Date: 2025-01-03
// Mesh.cpp : CMesh 클래스의 구현 파일
// Version : 0.1
///////////////////////////////////////////////////////////////////////////////
#include "Mesh.h"

#include "GameObject.h"

CMesh::CMesh(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList)
{
	// 메쉬의 이름 초기화
	m_strMeshName = "Mesh";

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

void CMesh::OnPostRender(ID3D12GraphicsCommandList* pd3dCommandList, void* pContext)
{
	// Post-Render Process
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
	m_ppnSubSetIndices.resize(nSubMeshes);
	m_ppd3dSubSetIndexBuffers.resize(nSubMeshes);
	m_ppd3dSubSetIndexUploadBuffers.resize(nSubMeshes);
	m_pd3dSubSetIndexBufferViews.resize(nSubMeshes);
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

void CStandardMesh::LoadMeshFromFile(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, std::ifstream& File)
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
		}
		else if (!strcmp(pstrToken, "<Positions>:"))
		{
			File.read((char*)&nPositions, sizeof(int));
			if (nPositions > 0)
			{
				m_nType |= VERTEXT_POSITION;
				m_pxmf3Positions.resize(nPositions);
				File.read((char*)m_pxmf3Positions.data(), sizeof(XMFLOAT3) * nPositions);

				m_pd3dPositionBuffer = ::CreateBufferResource(pd3dDevice, pd3dCommandList, m_pxmf3Positions.data(), sizeof(XMFLOAT3) * m_nVertices, D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, &m_pd3dPositionUploadBuffer);

				m_d3dPositionBufferView.BufferLocation = m_pd3dPositionBuffer->GetGPUVirtualAddress();
				m_d3dPositionBufferView.StrideInBytes = sizeof(XMFLOAT3);
				m_d3dPositionBufferView.SizeInBytes = sizeof(XMFLOAT3) * m_nVertices;
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

				m_pd3dTextureCoord0Buffer = ::CreateBufferResource(pd3dDevice, pd3dCommandList, m_pxmf2TextureCoords0.data(), sizeof(XMFLOAT2) * m_nVertices, D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, &m_pd3dTextureCoord0UploadBuffer);

				m_d3dTextureCoord0BufferView.BufferLocation = m_pd3dTextureCoord0Buffer->GetGPUVirtualAddress();
				m_d3dTextureCoord0BufferView.StrideInBytes = sizeof(XMFLOAT2);
				m_d3dTextureCoord0BufferView.SizeInBytes = sizeof(XMFLOAT2) * m_nVertices;
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

				m_pd3dTextureCoord1Buffer = ::CreateBufferResource(pd3dDevice, pd3dCommandList, m_pxmf2TextureCoords1.data(), sizeof(XMFLOAT2) * m_nVertices, D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, &m_pd3dTextureCoord1UploadBuffer);

				m_d3dTextureCoord1BufferView.BufferLocation = m_pd3dTextureCoord1Buffer->GetGPUVirtualAddress();
				m_d3dTextureCoord1BufferView.StrideInBytes = sizeof(XMFLOAT2);
				m_d3dTextureCoord1BufferView.SizeInBytes = sizeof(XMFLOAT2) * m_nVertices;
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

				m_pd3dNormalBuffer = ::CreateBufferResource(pd3dDevice, pd3dCommandList, m_pxmf3Normals.data(), sizeof(XMFLOAT3) * m_nVertices, D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, &m_pd3dNormalUploadBuffer);

				m_d3dNormalBufferView.BufferLocation = m_pd3dNormalBuffer->GetGPUVirtualAddress();
				m_d3dNormalBufferView.StrideInBytes = sizeof(XMFLOAT3);
				m_d3dNormalBufferView.SizeInBytes = sizeof(XMFLOAT3) * m_nVertices;
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

				m_pd3dTangentBuffer = ::CreateBufferResource(pd3dDevice, pd3dCommandList, m_pxmf3Tangents.data(), sizeof(XMFLOAT3) * m_nVertices, D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, &m_pd3dTangentUploadBuffer);

				m_d3dTangentBufferView.BufferLocation = m_pd3dTangentBuffer->GetGPUVirtualAddress();
				m_d3dTangentBufferView.StrideInBytes = sizeof(XMFLOAT3);
				m_d3dTangentBufferView.SizeInBytes = sizeof(XMFLOAT3) * m_nVertices;
			}
		}
		else if (!strcmp(pstrToken, "<BiTangents>:"))
		{
			nBiTangents = ReadIntegerFromFile(File);
			if (nBiTangents > 0)
			{
				m_pxmf3BiTangents.resize(nBiTangents);
				File.read((char*)m_pxmf3BiTangents.data(), sizeof(XMFLOAT3) * nBiTangents);

				m_pd3dBiTangentBuffer = ::CreateBufferResource(pd3dDevice, pd3dCommandList, m_pxmf3BiTangents.data(), sizeof(XMFLOAT3) * m_nVertices, D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, &m_pd3dBiTangentUploadBuffer);

				m_d3dBiTangentBufferView.BufferLocation = m_pd3dBiTangentBuffer->GetGPUVirtualAddress();
				m_d3dBiTangentBufferView.StrideInBytes = sizeof(XMFLOAT3);
				m_d3dBiTangentBufferView.SizeInBytes = sizeof(XMFLOAT3) * m_nVertices;
			}
		}
		else if (!strcmp(pstrToken, "<SubMeshes>:"))
		{
			m_nSubMeshes = ReadIntegerFromFile(File);
			if (m_nSubMeshes > 0)
			{
				SetSubMeshCount(m_nSubMeshes);
				//m_pnSubSetIndices = new int[m_nSubMeshes];
				//m_ppnSubSetIndices = new UINT * [m_nSubMeshes];

				//m_ppd3dSubSetIndexBuffers = new ID3D12Resource * [m_nSubMeshes];
				//m_ppd3dSubSetIndexUploadBuffers = new ID3D12Resource * [m_nSubMeshes];
				//m_pd3dSubSetIndexBufferViews = new D3D12_INDEX_BUFFER_VIEW[m_nSubMeshes];

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

							m_ppd3dSubSetIndexBuffers[i] = ::CreateBufferResource(pd3dDevice, pd3dCommandList, m_ppnSubSetIndices[i].data(), sizeof(UINT) * (UINT)m_ppnSubSetIndices[i].size(), D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_INDEX_BUFFER, &m_ppd3dSubSetIndexUploadBuffers[i]);

							m_pd3dSubSetIndexBufferViews[i].BufferLocation = m_ppd3dSubSetIndexBuffers[i]->GetGPUVirtualAddress();
							m_pd3dSubSetIndexBufferViews[i].Format = DXGI_FORMAT_R32_UINT;
							m_pd3dSubSetIndexBufferViews[i].SizeInBytes = (UINT) (sizeof(UINT) * m_ppnSubSetIndices[i].size());
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

	m_pd3dPositionBuffer = CreateBufferResource(pd3dDevice, pd3dCommandList, m_pxmf3Positions.data(), sizeof(XMFLOAT3) * m_nVertices, D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, m_pd3dPositionUploadBuffer.GetAddressOf());

	std::wstring debugName = L"Cube_" + std::to_wstring(nCubeIndex - 1);
	{
		std::wstring name = debugName + L" : m_pd3dPositionBuffer";
		m_pd3dPositionBuffer->SetName(name.c_str());
	}

	m_d3dPositionBufferView.BufferLocation = m_pd3dPositionBuffer->GetGPUVirtualAddress();
	m_d3dPositionBufferView.StrideInBytes = sizeof(XMFLOAT3);
	m_d3dPositionBufferView.SizeInBytes = sizeof(XMFLOAT3) * m_nVertices;
	
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

	m_pd3dNormalBuffer = CreateBufferResource(pd3dDevice, pd3dCommandList, m_pxmf3Normals.data(), sizeof(XMFLOAT3) * m_nVertices, D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, m_pd3dNormalUploadBuffer.GetAddressOf());
	
	{
		std::wstring name = debugName + L" : m_pd3dNormalBuffer";
		m_pd3dNormalBuffer->SetName(name.c_str());
	}

	m_d3dNormalBufferView.BufferLocation = m_pd3dNormalBuffer->GetGPUVirtualAddress();
	m_d3dNormalBufferView.StrideInBytes = sizeof(XMFLOAT3);
	m_d3dNormalBufferView.SizeInBytes = sizeof(XMFLOAT3) * static_cast<UINT>(m_pxmf3Normals.size());

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

	m_pd3dTextureCoord0Buffer = CreateBufferResource(pd3dDevice, pd3dCommandList, m_pxmf2TextureCoords0.data(), sizeof(XMFLOAT2) * m_nVertices, D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, m_pd3dTextureCoord0UploadBuffer.GetAddressOf());

	{
		std::wstring name = debugName + L" : m_pd3dTextureCoord0Buffer";
		m_pd3dTextureCoord0Buffer->SetName(name.c_str());
	}

	m_d3dTextureCoord0BufferView.BufferLocation = m_pd3dTextureCoord0Buffer->GetGPUVirtualAddress();
	m_d3dTextureCoord0BufferView.StrideInBytes = sizeof(XMFLOAT2);
	m_d3dTextureCoord0BufferView.SizeInBytes = sizeof(XMFLOAT2) * static_cast<UINT>(m_pxmf2TextureCoords0.size());

	// Standard Shader를 사용하기위한 더미 데이터 생성
	// --------------------------------------------
	// Tangent Dummy
	std::vector<XMFLOAT3> xmf3Tangents(m_nVertices, XMFLOAT3(0.0f, 0.0f, 0.0f));

	m_pxmf3Tangents = std::move(xmf3Tangents);

	m_pd3dTangentBuffer = CreateBufferResource(pd3dDevice, pd3dCommandList, m_pxmf3Tangents.data(), sizeof(XMFLOAT3) * m_nVertices, D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, m_pd3dTangentUploadBuffer.GetAddressOf());

	{
		std::wstring name = debugName + L" : m_pd3dTangentBuffer";
		m_pd3dTangentBuffer->SetName(name.c_str());
	}

	m_d3dTangentBufferView.BufferLocation = m_pd3dTangentBuffer->GetGPUVirtualAddress();
	m_d3dTangentBufferView.StrideInBytes = sizeof(XMFLOAT3);
	m_d3dTangentBufferView.SizeInBytes = sizeof(XMFLOAT3) * static_cast<UINT>(m_pxmf3Tangents.size());

	// BiTangent Dummy

	std::vector<XMFLOAT3> xmf3BiTangents(m_nVertices, XMFLOAT3(0.0f, 0.0f, 0.0f));

	m_pxmf3BiTangents = std::move(xmf3BiTangents);

	m_pd3dBiTangentBuffer = CreateBufferResource(pd3dDevice, pd3dCommandList, m_pxmf3BiTangents.data(), sizeof(XMFLOAT3) * m_nVertices, D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, m_pd3dBiTangentUploadBuffer.GetAddressOf());
	{
		std::wstring name = debugName + L" : m_pd3dBiTangentBuffer";
		m_pd3dBiTangentBuffer->SetName(name.c_str());
	}
	m_d3dBiTangentBufferView.BufferLocation = m_pd3dBiTangentBuffer->GetGPUVirtualAddress();
	m_d3dBiTangentBufferView.StrideInBytes = sizeof(XMFLOAT3);
	m_d3dBiTangentBufferView.SizeInBytes = sizeof(XMFLOAT3) * static_cast<UINT>(m_pxmf3BiTangents.size());
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

CHeightMapGridMesh::~CHeightMapGridMesh()
{
}

float CHeightMapGridMesh::OnGetHeight(int x, int z, void* pContext)
{
	CHeightMapImage* pHeightMapImage = (CHeightMapImage*)pContext;
	std::vector<BYTE> pHeightMapPixels = pHeightMapImage->GetHeightMapPixels();
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

void CSkinnedMesh::LoadSkinInfoFromFile(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, std::ifstream& pInFile)
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
				//m_ppstrSkinningBoneNames = new char[m_nSkinningBones][64];
				m_ppSkinningBoneFrameCaches.resize(m_nSkinningBones);
				//m_ppSkinningBoneFrameCaches = new CGameObject * [m_nSkinningBones];
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

				UINT ncbElementBytes = (((sizeof(XMFLOAT4X4) * SKINNED_ANIMATION_BONES) + 255) & ~255); //256의 배수
				m_pd3dcbBindPoseBoneOffsets = ::CreateBufferResource(pd3dDevice, pd3dCommandList, NULL, ncbElementBytes, D3D12_HEAP_TYPE_UPLOAD, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, NULL);
				m_pd3dcbBindPoseBoneOffsets->Map(0, NULL, (void**)&m_pcbxmf4x4MappedBindPoseBoneOffsets);

				for (int i = 0; i < m_nSkinningBones; i++)
				{
					XMStoreFloat4x4(&m_pcbxmf4x4MappedBindPoseBoneOffsets[i], XMMatrixTranspose(XMLoadFloat4x4(&m_pxmf4x4BindPoseBoneOffsets[i])));
				}
			}
		}
		else if (!strcmp(pstrToken, "<BoneIndices>:"))
		{
			m_nType |= VERTEXT_BONE_INDEX_WEIGHT;

			m_nVertices = ::ReadIntegerFromFile(pInFile);
			if (m_nVertices > 0)
			{
				m_pxmn4BoneIndices.resize(m_nVertices);
				//m_pxmn4BoneIndices = new XMINT4[m_nVertices];

				pInFile.read((char*)m_pxmn4BoneIndices.data(), sizeof(XMINT4) * m_nVertices);
				m_pd3dBoneIndexBuffer = ::CreateBufferResource(pd3dDevice, pd3dCommandList, m_pxmn4BoneIndices.data(), sizeof(XMINT4) * m_nVertices, D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, &m_pd3dBoneIndexUploadBuffer);

				m_d3dBoneIndexBufferView.BufferLocation = m_pd3dBoneIndexBuffer->GetGPUVirtualAddress();
				m_d3dBoneIndexBufferView.StrideInBytes = sizeof(XMINT4);
				m_d3dBoneIndexBufferView.SizeInBytes = sizeof(XMINT4) * m_nVertices;
			}
		}
		else if (!strcmp(pstrToken, "<BoneWeights>:"))
		{
			m_nType |= VERTEXT_BONE_INDEX_WEIGHT;

			m_nVertices = ::ReadIntegerFromFile(pInFile);
			if (m_nVertices > 0)
			{
				m_pxmf4BoneWeights.resize(m_nVertices);
				//m_pxmf4BoneWeights = new XMFLOAT4[m_nVertices];

				pInFile.read((char*)m_pxmf4BoneWeights.data(), sizeof(XMFLOAT4) * m_nVertices);
				m_pd3dBoneWeightBuffer = ::CreateBufferResource(pd3dDevice, pd3dCommandList, m_pxmf4BoneWeights.data(), sizeof(XMFLOAT4) * m_nVertices, D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, &m_pd3dBoneWeightUploadBuffer);

				m_d3dBoneWeightBufferView.BufferLocation = m_pd3dBoneWeightBuffer->GetGPUVirtualAddress();
				m_d3dBoneWeightBufferView.StrideInBytes = sizeof(XMFLOAT4);
				m_d3dBoneWeightBufferView.SizeInBytes = sizeof(XMFLOAT4) * m_nVertices;
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

		for (int j = 0; j < m_nSkinningBones; j++)
		{
			XMFLOAT4X4 WorldMatrix = m_ppSkinningBoneFrameCaches[j]->GetWorldMatrix();
			XMStoreFloat4x4(&m_pcbxmf4x4MappedSkinningBoneTransforms[j], XMMatrixTranspose(XMLoadFloat4x4(&WorldMatrix)));
		}
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

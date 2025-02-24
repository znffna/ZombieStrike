///////////////////////////////////////////////////////////////////////////////
// Date: 2025-01-03
// Mesh.cpp : CMesh Ŭ������ ���� ����
// Version : 0.1
///////////////////////////////////////////////////////////////////////////////
#include "Mesh.h"

CMesh::CMesh(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList)
{
	// �޽��� �̸� �ʱ�ȭ
	m_strName = "Mesh";

	// ������ ���� �ʱ�ȭ
	ZeroMemory(&m_d3dPositionBufferView, sizeof(D3D12_VERTEX_BUFFER_VIEW));

	m_nOffset = 0;
	m_nSlot = 0;
	m_nStride = sizeof(XMFLOAT3);
}

CMesh::~CMesh()
{
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
	// ���� ���� �ʱ�ȭ
	m_xmf4Color = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);

	// ���ؽ� ���� ���� �ʱ�ȭ
	ZeroMemory(&m_d3dTextureCoord0BufferView, sizeof(D3D12_VERTEX_BUFFER_VIEW));
	ZeroMemory(&m_d3dTextureCoord1BufferView, sizeof(D3D12_VERTEX_BUFFER_VIEW));
	ZeroMemory(&m_d3dNormalBufferView, sizeof(D3D12_VERTEX_BUFFER_VIEW));
	ZeroMemory(&m_d3dTangentBufferView, sizeof(D3D12_VERTEX_BUFFER_VIEW));
	ZeroMemory(&m_d3dBiTangentBufferView, sizeof(D3D12_VERTEX_BUFFER_VIEW));
}

CStandardMesh::~CStandardMesh()
{
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
	CMesh::OnPreRender(pd3dCommandList, pContext);
}

void CStandardMesh::Render(ID3D12GraphicsCommandList* pd3dCommandList, int nSubSet)
{
	int nSubMeshes = (int)m_ppnSubSetIndices.size();
	// Render Process
	pd3dCommandList->IASetPrimitiveTopology(m_d3dPrimitiveTopology);

	D3D12_VERTEX_BUFFER_VIEW pVertexBufferViews[5] = { m_d3dPositionBufferView, m_d3dTextureCoord0BufferView, m_d3dNormalBufferView, m_d3dTangentBufferView, m_d3dBiTangentBufferView };
	pd3dCommandList->IASetVertexBuffers(m_nSlot, 5, pVertexBufferViews);

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



///////////////////////////////////////////////////////////////////////////////
//

CCubeMesh::CCubeMesh(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, float fWidth, float fHeight, float fDepth)
	: CStandardMesh(pd3dDevice, pd3dCommandList)
{
	static int nCubeIndex = 0;
	std::string strName = "Cube_" + std::to_string(nCubeIndex++);
	SetName(strName);

	// ������ ������ ����
	m_xmf4Color = XMFLOAT4(1.0f, 0.0f, 1.0f, 1.0f);

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

	m_pxmf2Texture0Coords = std::move(xmf2Texture0Coords);

	m_pd3dTextureCoord0Buffer = CreateBufferResource(pd3dDevice, pd3dCommandList, m_pxmf2Texture0Coords.data(), sizeof(XMFLOAT2) * m_nVertices, D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, m_pd3dTextureCoord0UploadBuffer.GetAddressOf());

	m_d3dTextureCoord0BufferView.BufferLocation = m_pd3dTextureCoord0Buffer->GetGPUVirtualAddress();
	m_d3dTextureCoord0BufferView.StrideInBytes = sizeof(XMFLOAT2);
	m_d3dTextureCoord0BufferView.SizeInBytes = sizeof(XMFLOAT2) * static_cast<UINT>(m_pxmf2Texture0Coords.size());

	// Standard Shader�� ����ϱ����� ���� ������ ����
	// --------------------------------------------
	// Tangent Dummy
	std::vector<XMFLOAT3> xmf3Tangents(m_nVertices, XMFLOAT3(0.0f, 0.0f, 0.0f));

	m_pxmf3Tangents = std::move(xmf3Tangents);

	m_pd3dTangentBuffer = CreateBufferResource(pd3dDevice, pd3dCommandList, m_pxmf3Tangents.data(), sizeof(XMFLOAT3) * m_nVertices, D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, m_pd3dTangentUploadBuffer.GetAddressOf());

	m_d3dTangentBufferView.BufferLocation = m_pd3dTangentBuffer->GetGPUVirtualAddress();
	m_d3dTangentBufferView.StrideInBytes = sizeof(XMFLOAT3);
	m_d3dTangentBufferView.SizeInBytes = sizeof(XMFLOAT3) * static_cast<UINT>(m_pxmf3Tangents.size());

	// BiTangent Dummy

	std::vector<XMFLOAT3> xmf3BiTangents(m_nVertices, XMFLOAT3(0.0f, 0.0f, 0.0f));

	m_pxmf3BiTangents = std::move(xmf3BiTangents);

	m_pd3dBiTangentBuffer = CreateBufferResource(pd3dDevice, pd3dCommandList, m_pxmf3BiTangents.data(), sizeof(XMFLOAT3) * m_nVertices, D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, m_pd3dBiTangentUploadBuffer.GetAddressOf());

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

	//������ ����(����)�� ������ (nWidth * nLength)�̴�. 
	//m_nVertices = nWidth * nLength;
	m_nStride = sizeof(CTerrainVertex);

	//���ڴ� �ﰢ�� ��Ʈ������ �����Ѵ�. 
	//m_d3dPrimitiveTopology = D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP;
	m_nWidth = nWidth;
	m_nLength = nLength;
	m_xmf3Scale = xmf3Scale;

	CTerrainVertex* pVertices = new CTerrainVertex[m_nVertices];

	/*xStart�� zStart�� ������ ���� ��ġ(x-��ǥ�� z-��ǥ)�� ��Ÿ����.
	Ŀ�ٶ� ������ ���ڵ��� ������ �迭�� ���� �ʿ䰡 �ֱ� ������ ��ü �������� �� ������ ���� ��ġ�� ��Ÿ���� ������ �ʿ��ϴ�.*/
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
			//������ ���̿� ������ ���� �����κ��� ���Ѵ�. 
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

	//���� �׸��� ������ ����(����)�� �����ϴ� ������ �����ش�.
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
			//Ȧ�� ��° ���̹Ƿ�(z = 0, 2, 4, ...) �ε����� ���� ������ ���ʿ��� ������ �����̴�. 
			for (int x = 0; x < nWidth; x++)
			{
				//ù ��° ���� �����ϰ� ���� �ٲ� ������(x == 0) ù ��° �ε����� �߰��Ѵ�. 
				if ((x == 0) && (z > 0)) pnIndices[j++] = (UINT)(x + (z * nWidth));

				//�Ʒ�(x, z), ��(x, z+1)�� ������ �ε����� �߰��Ѵ�. 
				pnIndices[j++] = (UINT)(x + (z * nWidth));
				pnIndices[j++] = (UINT)((x + (z * nWidth)) + nWidth);
			}
		}
		else
		{
			//¦�� ��° ���̹Ƿ�(z = 1, 3, 5, ...) �ε����� ���� ������ �����ʿ��� ���� �����̴�. 
			for (int x = nWidth - 1; x >= 0; x--)
			{
				//���� �ٲ� ������(x == (nWidth-1)) ù ��° �ε����� �߰��Ѵ�.
				if (x == (nWidth - 1)) pnIndices[j++] = (UINT)(x + (z * nWidth));
				//�Ʒ�(x, z), ��(x, z+1)�� ������ �ε����� �߰��Ѵ�. 
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
	//������ ���� ����(�������� ��������� ����)�̴�. 
	XMFLOAT3 xmf3LightDirection = XMFLOAT3(-1.0f, 1.0f, 1.0f);
	xmf3LightDirection = Vector3::Normalize(xmf3LightDirection);
	CHeightMapImage* pHeightMapImage = (CHeightMapImage*)pContext;
	XMFLOAT3 xmf3Scale = pHeightMapImage->GetScale();
	//������ ����(����, ���)�̴�. 
	XMFLOAT4 xmf4IncidentLightColor(0.9f, 0.8f, 0.0f, 1.0f);
	/*���� (x, z)���� ������ �ݻ�Ǵ� ��(����)�� ���� (x, z)�� ���� ���Ϳ� ������ ���� ������ ����(cos)��
	������ 3���� ���� (x+1, z), (x, z+1), (x+1, z+1)�� ���� ���Ϳ� ������ ���� ������ ������ ����Ͽ� ���Ѵ�.
	���� (x, z)�� ������ ���� ����(����)�� �ݻ�Ǵ� ��(����)�� ���� ���̴�.*/
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

	//fScale�� ���� ����(���)�� �ݻ�Ǵ� �����̴�. 
	XMFLOAT4 xmf4Color = Vector4::Multiply(fScale, xmf4IncidentLightColor);
	return(xmf4Color);
}

XMFLOAT2 CHeightMapGridMesh::OnGetUVs(int x, int z, void* pContext)
{
	//���� �� �̹����� �ȼ� ��ǥ�� UV ��ǥ�� ��ȯ�Ѵ�.
	XMFLOAT2 UV = XMFLOAT2((float)x / m_nWidth, (float)z / m_nLength);

	return UV;
}

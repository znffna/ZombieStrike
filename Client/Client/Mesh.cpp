///////////////////////////////////////////////////////////////////////////////
// Date: 2025-01-03
// Mesh.cpp : CMesh Å¬·¡½ºÀÇ ±¸Çö ÆÄÀÏ
// Version : 0.1
///////////////////////////////////////////////////////////////////////////////
#include "Mesh.h"

CMesh::CMesh()
{
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
	// Render Process
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

CStandardMesh::CStandardMesh()
{
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



///////////////////////////////////////////////////////////////////////////////
//

CCubeMesh::CCubeMesh(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, float fWidth, float fHeight, float fDepth)
{
	static int nCubeIndex = 0;
	std::string strName = "Cube_" + std::to_string(nCubeIndex++);
	SetName(strName);

	// ÀÓÀÇÀÇ °ªÀ¸·Î ¼³Á¤
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
		{-halfWidth,  halfHeight,  halfDepth},  // 3

		// Back face
		{ halfWidth, -halfHeight, -halfDepth},  // 4
		{-halfWidth, -halfHeight, -halfDepth},  // 5
		{-halfWidth,  halfHeight, -halfDepth},  // 6
		{ halfWidth,  halfHeight, -halfDepth},  // 7

		// Top face
		{-halfWidth,  halfHeight, -halfDepth},  // 6
		{-halfWidth,  halfHeight,  halfDepth},  // 3
		{ halfWidth,  halfHeight,  halfDepth},  // 2
		{ halfWidth,  halfHeight, -halfDepth},  // 7

		// Bottom face
		{-halfWidth, -halfHeight, -halfDepth},  // 5
		{ halfWidth, -halfHeight, -halfDepth},  // 4
		{ halfWidth, -halfHeight,  halfDepth},  // 1
		{-halfWidth, -halfHeight,  halfDepth},  // 0

		// Right face
		{ halfWidth, -halfHeight, -halfDepth},  // 4
		{ halfWidth,  halfHeight, -halfDepth},  // 7
		{ halfWidth,  halfHeight,  halfDepth},  // 2
		{ halfWidth, -halfHeight,  halfDepth},  // 1

		// Left face
		{-halfWidth, -halfHeight, -halfDepth},  // 5
		{-halfWidth, -halfHeight,  halfDepth},  // 0
		{-halfWidth,  halfHeight,  halfDepth},  // 3
		{-halfWidth,  halfHeight, -halfDepth}   // 6
	};

	m_pxmf3Positions = std::move(xmf3Positions);

	m_pd3dPositionBuffer = CreateBufferResource(pd3dDevice, pd3dCommandList, m_pxmf3Positions.data(), sizeof(XMFLOAT3) * m_nVertices, D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, m_pd3dPositionUploadBuffer.GetAddressOf());

	m_d3dPositionBufferView.BufferLocation = m_pd3dPositionBuffer->GetGPUVirtualAddress();
	m_d3dPositionBufferView.StrideInBytes = sizeof(XMFLOAT3);
	m_d3dPositionBufferView.SizeInBytes = sizeof(XMFLOAT3) * m_nVertices;

	// index

	std::vector<UINT> pnIndices = {
		// Front face
		0, 1, 2, 0, 2, 3,

		// Back face
		4, 5, 6, 4, 6, 7,

		// Top face
		8, 9, 10, 8, 10, 11,

		// Bottom face
		12, 13, 14, 12, 14, 15,

		// Right face
		16, 17, 18, 16, 18, 19,

		// Left face
		20, 21, 22, 20, 22, 23
	};

	// ¼­ºê¼Â °¹¼ö ¼³Á¤
	SetSubMeshCount(1);

	m_ppnSubSetIndices[0] = std::move(pnIndices);

	m_ppd3dSubSetIndexBuffers[0] = CreateBufferResource(pd3dDevice, pd3dCommandList, m_ppnSubSetIndices[0].data(), sizeof(UINT) * static_cast<int>(m_ppnSubSetIndices[0].size()), D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, m_ppd3dSubSetIndexUploadBuffers[0].GetAddressOf());
	m_pd3dSubSetIndexBufferViews[0].BufferLocation = m_ppd3dSubSetIndexBuffers[0]->GetGPUVirtualAddress();
	m_pd3dSubSetIndexBufferViews[0].Format = DXGI_FORMAT_R32_UINT;
	m_pd3dSubSetIndexBufferViews[0].SizeInBytes = sizeof(UINT) * static_cast<UINT>(m_ppnSubSetIndices[0].size());

	// normal
	std::vector<XMFLOAT3> pxmf3Normals = {
		// Front face
		{0.0f, 0.0f, 1.0f},  // 0
		{0.0f, 0.0f, 1.0f},  // 1
		{0.0f, 0.0f, 1.0f},  // 2
		{0.0f, 0.0f, 1.0f},  // 3

		// Back face
		{0.0f, 0.0f, -1.0f},  // 4
		{0.0f, 0.0f, -1.0f},  // 5
		{0.0f, 0.0f, -1.0f},  // 6
		{0.0f, 0.0f, -1.0f},  // 7

		// Top face
		{0.0f, 1.0f, 0.0f},  // 6
		{0.0f, 1.0f, 0.0f},  // 3
		{0.0f, 1.0f, 0.0f},  // 2
		{0.0f, 1.0f, 0.0f},  // 7

		// Bottom face
		{0.0f, -1.0f, 0.0f},  // 5
		{0.0f, -1.0f, 0.0f},  // 4
		{0.0f, -1.0f, 0.0f},  // 1
		{0.0f, -1.0f, 0.0f},  // 0

		// Right face
		{1.0f, 0.0f, 0.0f},  // 4
		{1.0f, 0.0f, 0.0f},  // 7
		{1.0f, 0.0f, 0.0f},  // 2
		{1.0f, 0.0f, 0.0f},  // 1

		// Left face
		{-1.0f, 0.0f, 0.0f},  // 5
		{-1.0f, 0.0f, 0.0f},  // 0
		{-1.0f, 0.0f, 0.0f},  // 3
		{-1.0f, 0.0f, 0.0f}   // 6
	};

	m_pxmf3Normals = std::move(pxmf3Normals);

	m_pd3dNormalBuffer = CreateBufferResource(pd3dDevice, pd3dCommandList, m_pxmf3Normals.data(), sizeof(XMFLOAT3) * m_nVertices, D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, m_pd3dNormalUploadBuffer.GetAddressOf());

	m_d3dNormalBufferView.BufferLocation = m_pd3dNormalBuffer->GetGPUVirtualAddress();
	m_d3dNormalBufferView.StrideInBytes = sizeof(XMFLOAT3);
	m_d3dNormalBufferView.SizeInBytes = sizeof(XMFLOAT3) * static_cast<UINT>(m_pxmf3Normals.size());

	// texture
	std::vector<XMFLOAT2> xmf2Texture0Coords = {
		// +X ¸é
	   {1.0f, 0.0f}, {1.0f, 1.0f}, {0.0f, 1.0f}, // »ï°¢Çü 1
	   {1.0f, 0.0f}, {0.0f, 1.0f}, {0.0f, 0.0f}, // »ï°¢Çü 2

	   // -X ¸é
	   {0.0f, 0.0f}, {0.0f, 1.0f}, {1.0f, 1.0f}, // »ï°¢Çü 1
	   {0.0f, 0.0f}, {1.0f, 1.0f}, {1.0f, 0.0f}, // »ï°¢Çü 2

	   // +Y ¸é
	   {0.0f, 0.0f}, {1.0f, 0.0f}, {1.0f, 1.0f}, // »ï°¢Çü 1
	   {0.0f, 0.0f}, {1.0f, 1.0f}, {0.0f, 1.0f}, // »ï°¢Çü 2

	   // -Y ¸é
	   {0.0f, 1.0f}, {1.0f, 1.0f}, {1.0f, 0.0f}, // »ï°¢Çü 1
	   {0.0f, 1.0f}, {1.0f, 0.0f}, {0.0f, 0.0f}, // »ï°¢Çü 2

	   // +Z ¸é
	   {0.0f, 0.0f}, {1.0f, 0.0f}, {1.0f, 1.0f}, // »ï°¢Çü 1
	   {0.0f, 0.0f}, {1.0f, 1.0f}, {0.0f, 1.0f}, // »ï°¢Çü 2

	   // -Z ¸é
	   {1.0f, 0.0f}, {0.0f, 0.0f}, {0.0f, 1.0f}, // »ï°¢Çü 1
	   {1.0f, 0.0f}, {0.0f, 1.0f}, {1.0f, 1.0f}  // »ï°¢Çü 2
	};

	m_pxmf2Texture0Coords = std::move(xmf2Texture0Coords);

	m_pd3dTextureCoord0Buffer = CreateBufferResource(pd3dDevice, pd3dCommandList, m_pxmf2Texture0Coords.data(), sizeof(XMFLOAT2) * m_nVertices, D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, m_pd3dTextureCoord0UploadBuffer.GetAddressOf());

	m_d3dTextureCoord0BufferView.BufferLocation = m_pd3dTextureCoord0Buffer->GetGPUVirtualAddress();
	m_d3dTextureCoord0BufferView.StrideInBytes = sizeof(XMFLOAT2);
	m_d3dTextureCoord0BufferView.SizeInBytes = sizeof(XMFLOAT2) * static_cast<UINT>(m_pxmf2Texture0Coords.size());
}

CCubeMesh::~CCubeMesh()
{
}
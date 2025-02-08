///////////////////////////////////////////////////////////////////////////////
// Date: 2025-01-03
// Mesh.cpp : CMesh 클래스의 구현 파일
// Version : 0.1
///////////////////////////////////////////////////////////////////////////////
#include "Mesh.h"

CMesh::CMesh(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList)
{
	// 메쉬의 이름 초기화
	m_strName = "Mesh";

	// 포지션 버퍼 초기화
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
	// 색상 정보 초기화
	m_xmf4Color = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);

	// 버텍스 정보 버퍼 초기화
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

	// 임의의 값으로 설정
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

	// Standard Shader를 사용하기위한 더미 데이터 생성
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
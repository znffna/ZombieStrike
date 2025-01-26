///////////////////////////////////////////////////////////////////////////////
// Date: 2025-01-03
// Mesh.h : CMesh 클래스의 헤더 파일
// Version : 0.1
///////////////////////////////////////////////////////////////////////////////
#pragma once

#include "stdafx.h"

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//

class CMesh
{
public:
	CMesh();
	~CMesh();

	// setter, getter
	UINT GetType() const { return(m_nType); }

	void SetName(const std::string& strName) { m_strName = strName; }
	std::string GetName() const { return(m_strName); }

	// method
	virtual void CreateShaderVariables(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList) { }
	virtual void UpdateShaderVariables(ID3D12GraphicsCommandList* pd3dCommandList) { }
	virtual void ReleaseShaderVariables() { }

	virtual void ReleaseUploadBuffers();

	virtual void OnPreRender(ID3D12GraphicsCommandList* pd3dCommandList, void* pContext);
	virtual void Render(ID3D12GraphicsCommandList* pd3dCommandList, int nSubSet);
	virtual void OnPostRender(ID3D12GraphicsCommandList* pd3dCommandList, void* pContext);

private:
	std::string m_strName; // 메쉬의 이름

protected:
	UINT m_nType = 0x00; // 메쉬의 종류

	// 포지션 버퍼
	int	m_nVertices = 0;								// 버텍스의 개수
	std::vector<XMFLOAT3> m_pxmf3Positions;				// CPU에 저장된 버텍스 데이터
	ComPtr<ID3D12Resource> m_pd3dPositionBuffer;		// GPU에 저장된 버텍스 데이터
	ComPtr<ID3D12Resource> m_pd3dPositionUploadBuffer;	// CPU에 저장된 버텍스 데이터를 GPU에 업로드하기 위한 버퍼
	D3D12_VERTEX_BUFFER_VIEW m_d3dPositionBufferView;	// 버텍스 버퍼 뷰

	// 서브 메쉬(Index Buffer)
	std::vector<std::vector<UINT>> m_ppnSubSetIndices;  // 서브셋의 인덱스 데이터
	// m_ppnSubSetIndices.size() = 서브셋의 개수 (서브 셋을 사용하는 이유 : mesh의 primitive마다 Meterial이 다르기 적용하기 위함)
	// m_ppnSubSetIndices[i].size() = i번째 서브셋의 인덱스 개수

	std::vector<ComPtr<ID3D12Resource>> m_ppd3dSubSetIndexBuffers;
	std::vector<ComPtr<ID3D12Resource>> m_ppd3dSubSetIndexUploadBuffers;
	std::vector<D3D12_INDEX_BUFFER_VIEW> m_pd3dSubSetIndexBufferViews;

	// 프리미티브 탑로지 [ Primitive Topology ]
	D3D12_PRIMITIVE_TOPOLOGY		m_d3dPrimitiveTopology = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	UINT m_nSlot;		// 슬롯 번호
	UINT m_nStride;		// 한 정점의 크기
	UINT m_nOffset;		// 시작 오프셋

protected:
	// Index 데이터의 갯수에 따라서 인덱스 버퍼를 생성하는 함수
	void SetSubMeshCount(int nSubMeshes);
};

///////////////////////////////////////////////////////////////////////////////
//

class CStandardMesh : public CMesh
{
public:
	CStandardMesh();
	virtual ~CStandardMesh();

	virtual void ReleaseUploadBuffers() override;

	virtual void OnPreRender(ID3D12GraphicsCommandList* pd3dCommandList, void* pContext) override;

protected:
	// 버텍스 정보 버퍼
	XMFLOAT4 m_xmf4Color;						// Material Diffuse Color
	std::vector<XMFLOAT3> m_pxmf3Tangents;		// CPU에 저장된 접선 벡터 데이터
	std::vector<XMFLOAT3> m_pxmf3BiTangents;	// CPU에 저장된 이접선 벡터 데이터
	std::vector<XMFLOAT3> m_pxmf3Normals;		// CPU에 저장된 법선 벡터 데이터

	std::vector<XMFLOAT2> m_pxmf2Texture0Coords;	// CPU에 저장된 텍스처 좌표 데이터
	std::vector<XMFLOAT2> m_pxmf2Texture1Coords;	// CPU에 저장된 텍스처 좌표 데이터

	ComPtr<ID3D12Resource> m_pd3dTextureCoord0Buffer;		// GPU에 저장된 텍스처 좌표 데이터
	ComPtr<ID3D12Resource> m_pd3dTextureCoord0UploadBuffer;	// CPU에 저장된 텍스처 좌표 데이터를 GPU에 업로드하기 위한 버퍼
	D3D12_VERTEX_BUFFER_VIEW m_d3dTextureCoord0BufferView;	// 텍스처 좌표 버퍼 뷰

	ComPtr<ID3D12Resource> m_pd3dTextureCoord1Buffer;		// GPU에 저장된 텍스처 좌표 데이터
	ComPtr<ID3D12Resource> m_pd3dTextureCoord1UploadBuffer;	// CPU에 저장된 텍스처 좌표 데이터를 GPU에 업로드하기 위한 버퍼
	D3D12_VERTEX_BUFFER_VIEW m_d3dTextureCoord1BufferView;	// 텍스처 좌표 버퍼 뷰

	ComPtr<ID3D12Resource> m_pd3dNormalBuffer;		// GPU에 저장된 법선 벡터 데이터
	ComPtr<ID3D12Resource> m_pd3dNormalUploadBuffer;	// CPU에 저장된 법선 벡터 데이터를 GPU에 업로드하기 위한 버퍼
	D3D12_VERTEX_BUFFER_VIEW m_d3dNormalBufferView;	// 법선 버퍼 뷰

	ComPtr<ID3D12Resource> m_pd3dTangentBuffer;		// GPU에 저장된 접선 벡터 데이터
	ComPtr<ID3D12Resource> m_pd3dTangentUploadBuffer;	// CPU에 저장된 접선 벡터 데이터를 GPU에 업로드하기 위한 버퍼
	D3D12_VERTEX_BUFFER_VIEW m_d3dTangentBufferView;	// 접선 버퍼 뷰

	ComPtr<ID3D12Resource> m_pd3dBiTangentBuffer;		// GPU에 저장된 이접선 벡터 데이터
	ComPtr<ID3D12Resource> m_pd3dBiTangentUploadBuffer;	// CPU에 저장된 이접선 벡터 데이터를 GPU에 업로드하기 위한 버퍼
	D3D12_VERTEX_BUFFER_VIEW m_d3dBiTangentBufferView;	// 이접선 버퍼 뷰
};

///////////////////////////////////////////////////////////////////////////////
//

class CCubeMesh : public CStandardMesh
{
public:
	CCubeMesh(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, float fWidth = 2.0f, float fHeight = 2.0f, float fDepth = 2.0f);
	virtual ~CCubeMesh();
};


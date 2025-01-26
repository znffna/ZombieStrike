///////////////////////////////////////////////////////////////////////////////
// Date: 2025-01-03
// Mesh.h : CMesh Ŭ������ ��� ����
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
	std::string m_strName; // �޽��� �̸�

protected:
	UINT m_nType = 0x00; // �޽��� ����

	// ������ ����
	int	m_nVertices = 0;								// ���ؽ��� ����
	std::vector<XMFLOAT3> m_pxmf3Positions;				// CPU�� ����� ���ؽ� ������
	ComPtr<ID3D12Resource> m_pd3dPositionBuffer;		// GPU�� ����� ���ؽ� ������
	ComPtr<ID3D12Resource> m_pd3dPositionUploadBuffer;	// CPU�� ����� ���ؽ� �����͸� GPU�� ���ε��ϱ� ���� ����
	D3D12_VERTEX_BUFFER_VIEW m_d3dPositionBufferView;	// ���ؽ� ���� ��

	// ���� �޽�(Index Buffer)
	std::vector<std::vector<UINT>> m_ppnSubSetIndices;  // ������� �ε��� ������
	// m_ppnSubSetIndices.size() = ������� ���� (���� ���� ����ϴ� ���� : mesh�� primitive���� Meterial�� �ٸ��� �����ϱ� ����)
	// m_ppnSubSetIndices[i].size() = i��° ������� �ε��� ����

	std::vector<ComPtr<ID3D12Resource>> m_ppd3dSubSetIndexBuffers;
	std::vector<ComPtr<ID3D12Resource>> m_ppd3dSubSetIndexUploadBuffers;
	std::vector<D3D12_INDEX_BUFFER_VIEW> m_pd3dSubSetIndexBufferViews;

	// ������Ƽ�� ž���� [ Primitive Topology ]
	D3D12_PRIMITIVE_TOPOLOGY		m_d3dPrimitiveTopology = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	UINT m_nSlot;		// ���� ��ȣ
	UINT m_nStride;		// �� ������ ũ��
	UINT m_nOffset;		// ���� ������

protected:
	// Index �������� ������ ���� �ε��� ���۸� �����ϴ� �Լ�
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
	// ���ؽ� ���� ����
	XMFLOAT4 m_xmf4Color;						// Material Diffuse Color
	std::vector<XMFLOAT3> m_pxmf3Tangents;		// CPU�� ����� ���� ���� ������
	std::vector<XMFLOAT3> m_pxmf3BiTangents;	// CPU�� ����� ������ ���� ������
	std::vector<XMFLOAT3> m_pxmf3Normals;		// CPU�� ����� ���� ���� ������

	std::vector<XMFLOAT2> m_pxmf2Texture0Coords;	// CPU�� ����� �ؽ�ó ��ǥ ������
	std::vector<XMFLOAT2> m_pxmf2Texture1Coords;	// CPU�� ����� �ؽ�ó ��ǥ ������

	ComPtr<ID3D12Resource> m_pd3dTextureCoord0Buffer;		// GPU�� ����� �ؽ�ó ��ǥ ������
	ComPtr<ID3D12Resource> m_pd3dTextureCoord0UploadBuffer;	// CPU�� ����� �ؽ�ó ��ǥ �����͸� GPU�� ���ε��ϱ� ���� ����
	D3D12_VERTEX_BUFFER_VIEW m_d3dTextureCoord0BufferView;	// �ؽ�ó ��ǥ ���� ��

	ComPtr<ID3D12Resource> m_pd3dTextureCoord1Buffer;		// GPU�� ����� �ؽ�ó ��ǥ ������
	ComPtr<ID3D12Resource> m_pd3dTextureCoord1UploadBuffer;	// CPU�� ����� �ؽ�ó ��ǥ �����͸� GPU�� ���ε��ϱ� ���� ����
	D3D12_VERTEX_BUFFER_VIEW m_d3dTextureCoord1BufferView;	// �ؽ�ó ��ǥ ���� ��

	ComPtr<ID3D12Resource> m_pd3dNormalBuffer;		// GPU�� ����� ���� ���� ������
	ComPtr<ID3D12Resource> m_pd3dNormalUploadBuffer;	// CPU�� ����� ���� ���� �����͸� GPU�� ���ε��ϱ� ���� ����
	D3D12_VERTEX_BUFFER_VIEW m_d3dNormalBufferView;	// ���� ���� ��

	ComPtr<ID3D12Resource> m_pd3dTangentBuffer;		// GPU�� ����� ���� ���� ������
	ComPtr<ID3D12Resource> m_pd3dTangentUploadBuffer;	// CPU�� ����� ���� ���� �����͸� GPU�� ���ε��ϱ� ���� ����
	D3D12_VERTEX_BUFFER_VIEW m_d3dTangentBufferView;	// ���� ���� ��

	ComPtr<ID3D12Resource> m_pd3dBiTangentBuffer;		// GPU�� ����� ������ ���� ������
	ComPtr<ID3D12Resource> m_pd3dBiTangentUploadBuffer;	// CPU�� ����� ������ ���� �����͸� GPU�� ���ε��ϱ� ���� ����
	D3D12_VERTEX_BUFFER_VIEW m_d3dBiTangentBufferView;	// ������ ���� ��
};

///////////////////////////////////////////////////////////////////////////////
//

class CCubeMesh : public CStandardMesh
{
public:
	CCubeMesh(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, float fWidth = 2.0f, float fHeight = 2.0f, float fDepth = 2.0f);
	virtual ~CCubeMesh();
};


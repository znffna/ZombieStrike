///////////////////////////////////////////////////////////////////////////////
// Date: 2025-01-03
// Mesh.h : CMesh Ŭ������ ��� ����
// Version : 0.1
///////////////////////////////////////////////////////////////////////////////
#pragma once

#include "stdafx.h"

///////////////////////////////////////////////////////////////////////////////
// Vertex ����ü
///////////////////////////////////////////////////////////////////////////////

//������ ǥ���ϱ� ���� Ŭ������ �����Ѵ�.
class CVertex
{
public:
	//������ ��ġ �����̴�(��� ������ �ּ��� ��ġ ���͸� ������ �Ѵ�).
	XMFLOAT3 m_xmf3Position;
public:
	CVertex() { m_xmf3Position = XMFLOAT3(0.0f, 0.0f, 0.0f); }
	CVertex(XMFLOAT3 xmf3Position) { m_xmf3Position = xmf3Position; }
	~CVertex() { }
};

///////////////////////////////////////////////////////////////////////////////
//

class CDiffusedVertex : public CVertex
{
public:
	//������ �����̴�.
	XMFLOAT4 m_xmf4Diffuse;
public:
	CDiffusedVertex() {
		m_xmf3Position = XMFLOAT3(0.0f, 0.0f, 0.0f);
		m_xmf4Diffuse = XMFLOAT4(0.0f, 0.0f, 0.0f, 0.0f);
	}
	CDiffusedVertex(float x, float y, float z, XMFLOAT4 xmf4Diffuse) {
		m_xmf3Position = XMFLOAT3(x, y, z);
		m_xmf4Diffuse = xmf4Diffuse;
	}
	CDiffusedVertex(XMFLOAT3 xmf3Position, XMFLOAT4 xmf4Diffuse) {
		m_xmf3Position = xmf3Position;
		m_xmf4Diffuse = xmf4Diffuse;
	}
	~CDiffusedVertex() { }
};

///////////////////////////////////////////////////////////////////////////////
//

class CTerrainVertex : public CDiffusedVertex
{
public:
	XMFLOAT3						m_xmf3Normal;
	XMFLOAT2						m_xmf2TexCoord0;
	XMFLOAT2						m_xmf2TexCoord1;

public:
	CTerrainVertex() {
		m_xmf3Normal = XMFLOAT3(0.0f, 1.0f, 0.0f);
		m_xmf2TexCoord0 = XMFLOAT2(0.0f, 0.0f);
		m_xmf2TexCoord1 = XMFLOAT2(0.0f, 0.0f);
	}

	CTerrainVertex(float x, float y, float z, XMFLOAT4 xmf4Diffuse, XMFLOAT3 xmf3Normal, XMFLOAT2 xmf2TexCoord0, XMFLOAT2 xmf2TexCoord1)
		: CDiffusedVertex(x, y, z, xmf4Diffuse)
	{
		m_xmf3Normal = xmf3Normal;
		m_xmf2TexCoord0 = xmf2TexCoord0;
		m_xmf2TexCoord1 = xmf2TexCoord1;
	}

	CTerrainVertex(XMFLOAT3 xmf3Position, XMFLOAT4 xmf4Diffuse = XMFLOAT4(0.0f, 0.0f, 0.0f, 0.0f), XMFLOAT3 xmf3Normal = XMFLOAT3(0.0f, 1.0f, 0.0f), XMFLOAT2 xmf2TexCoord0 = XMFLOAT2(0.0f, 0.0f), XMFLOAT2 xmf2TexCoord1 = XMFLOAT2(0.0f, 0.0f))
		: CDiffusedVertex(xmf3Position, xmf4Diffuse)
	{
		m_xmf3Normal = xmf3Normal;
		m_xmf2TexCoord0 = xmf2TexCoord0;
		m_xmf2TexCoord1 = xmf2TexCoord1;
	}
	~CTerrainVertex() { }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Mesh Ŭ����
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class CMesh
{
public:
	CMesh(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList);
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

	// Initiallize helper
	void SetSubSetAmount(int nSubSet) 
	{
		m_ppnSubSetIndices.resize(nSubSet); 

		m_ppd3dSubSetIndexBuffers.resize(nSubSet);
		m_ppd3dSubSetIndexUploadBuffers.resize(nSubSet);
		m_pd3dSubSetIndexBufferViews.resize(nSubSet);
	};
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
	CStandardMesh(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList);
	virtual ~CStandardMesh();

	virtual void ReleaseUploadBuffers() override;

	virtual void OnPreRender(ID3D12GraphicsCommandList* pd3dCommandList, void* pContext) override;

	virtual void Render(ID3D12GraphicsCommandList* pd3dCommandList, int nSubSet) override;

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

///////////////////////////////////////////////////////////////////////////////
//

class CSkyBoxMesh : public CMesh
{
public:
	CSkyBoxMesh(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, float fWidth = 20.0f, float fHeight = 20.0f, float fDepth = 20.0f);
	virtual ~CSkyBoxMesh();
};

///////////////////////////////////////////////////////////////////////////////
//


class CHeightMapImage
{
private:
	//���� �� �̹��� �ȼ�(8-��Ʈ)���� ������ �迭�̴�. �� �ȼ��� 0~255�� ���� ���´�. 
	std::vector<BYTE> m_pHeightMapPixels;

	//���� �� �̹����� ���ο� ���� ũ���̴�. 
	int m_nWidth;
	int m_nLength;

	//���� �� �̹����� ������ �� �� Ȯ���Ͽ� ����� ���ΰ��� ��Ÿ���� ������ �����̴�. 
	XMFLOAT3 m_xmf3Scale;
public:
	CHeightMapImage(LPCTSTR pFileName, int nWidth, int nLength, XMFLOAT3 xmf3Scale)
	{
		m_nWidth = nWidth;
		m_nLength = nLength;
		m_xmf3Scale = xmf3Scale;

		// �̹��� ũ�⸸ŭ �޸� Ȯ��
		std::vector<BYTE> pHeightMapPixels(m_nWidth * m_nLength);

		// �̹����� ���Ͽ��� �о �޸𸮿� ����
		FILE* pFile = NULL;
		HANDLE hFile = ::CreateFile(pFileName, GENERIC_READ, 0, NULL, OPEN_EXISTING,
			FILE_ATTRIBUTE_NORMAL | FILE_ATTRIBUTE_READONLY, NULL);
		DWORD dwBytesRead;
		::ReadFile(hFile, pHeightMapPixels.data(), (m_nWidth * m_nLength), &dwBytesRead, NULL);
		::CloseHandle(hFile);

		m_pHeightMapPixels.resize(m_nWidth * m_nLength);

		for (int y = 0; y < m_nLength; y++)
		{
			for (int x = 0; x < m_nWidth; x++)
			{
				m_pHeightMapPixels[x + ((m_nLength - 1 - y) * m_nWidth)] =
					pHeightMapPixels[x + (y * m_nWidth)];
			}
		}
	};
	~CHeightMapImage() {};

	//���� �� �̹������� (x, z) ��ġ�� �ȼ� ���� ����� ������ ���̸� ��ȯ�Ѵ�. 
	float GetHeight(float fx, float fz, XMFLOAT3 xmf3Scale = XMFLOAT3(1.0f, 1.0f, 1.0f))
	{
#ifdef _WITH_TERRAIN_TESSELATION
		fx /= xmf3Scale.x;
		fz /= xmf3Scale.z;

		int x = (int)fx;
		int z = (int)fz;
		float xFractional = fx - x;
		float zFractional = fz - z;

		bool bReverseQuad = ((z % 2) != 0);
		float fHeight = GetInterpolatedHeight(x, z, xFractional, zFractional, bReverseQuad);

		return(fHeight * xmf3Scale.y);
#else

		/*������ ��ǥ (fx, fz)�� �̹��� ��ǥ���̴�. ���� ���� x-��ǥ�� z-��ǥ�� ���� ���� ������ ����� ������ ���̴�
		0�̴�.*/
		if ((fx < 0.0f) || (fz < 0.0f) || (fx >= m_nWidth) || (fz >= m_nLength)) return(0.0f);

		//���� ���� ��ǥ�� ���� �κа� �Ҽ� �κ��� ����Ѵ�. 
		int x = (int)fx;
		int z = (int)fz;
		float fxPercent = fx - x;
		float fzPercent = fz - z;
		float fBottomLeft = (float)m_pHeightMapPixels[x + (z * m_nWidth)];
		float fBottomRight = (float)m_pHeightMapPixels[(x + 1) + (z * m_nWidth)];
		float fTopLeft = (float)m_pHeightMapPixels[x + ((z + 1) * m_nWidth)];
		float fTopRight = (float)m_pHeightMapPixels[(x + 1) + ((z + 1) * m_nWidth)];

#ifdef _WITH_APPROXIMATE_OPPOSITE_CORNER
		//z-��ǥ�� 1, 3, 5, ...�� ��� �ε����� �����ʿ��� �������� �����ȴ�. 
		bool bRightToLeft = ((z % 2) != 0);
		if (bRightToLeft)
		{
			/*������ �ﰢ������ �����ʿ��� ���� �������� �����Ǵ� ����̴�.
			���� �׸��� �������� (fzPercent < fxPercent)�� ����̴�.
			�� ��� TopLeft�� �ȼ� ���� (fTopLeft = fTopRight + (fBottomLeft - fBottomRight))�� �ٻ��Ѵ�.
			���� �׸��� ������ (fzPercent �� fxPercent)�� ����̴�.
			�� ��� BottomRight�� �ȼ� ���� (fBottomRight = fBottomLeft + (fTopRight - fTopLeft))�� �ٻ��Ѵ�.*/
			if (fzPercent >= fxPercent)
				fBottomRight = fBottomLeft + (fTopRight - fTopLeft);
			else
				fTopLeft = fTopRight + (fBottomLeft - fBottomRight);
		}
		else
		{
			/*������ �ﰢ������ ���ʿ��� ������ �������� �����Ǵ� ����̴�.
			���� �׸��� ������ (fzPercent < (1.0f - fxPercent))�� ����̴�.
			�� ��� TopRight�� �ȼ� ���� (fTopRight = fTopLeft + (fBottomRight - fBottomLeft))�� �ٻ��Ѵ�.
			���� �׸��� �������� (fzPercent �� (1.0f - fxPercent))�� ����̴�.
			�� ��� BottomLeft�� �ȼ� ���� (fBottomLeft = fTopLeft + (fBottomRight - fTopRight))�� �ٻ��Ѵ�.*/
			if (fzPercent < (1.0f - fxPercent))
				fTopRight = fTopLeft + (fBottomRight - fBottomLeft);
			else
				fBottomLeft = fTopLeft + (fBottomRight - fTopRight);
		}
#endif
		//�簢���� �� ���� �����Ͽ� ����(�ȼ� ��)�� ����Ѵ�. 
		float fTopHeight = fTopLeft * (1 - fxPercent) + fTopRight * fxPercent;
		float fBottomHeight = fBottomLeft * (1 - fxPercent) + fBottomRight * fxPercent;
		float fHeight = fBottomHeight * (1 - fzPercent) + fTopHeight * fzPercent;

		return(fHeight);
#endif
	};
	//���� �� �̹������� (x, z) ��ġ�� ���� ���͸� ��ȯ�Ѵ�. 
	XMFLOAT3 GetHeightMapNormal(int x, int z)
	{
		//x-��ǥ�� z-��ǥ�� ���� ���� ������ ����� ������ ���� ���ʹ� y-�� ���� �����̴�. 
		if ((x < 0.0f) || (z < 0.0f) || (x >= m_nWidth) || (z >= m_nLength))
			return(XMFLOAT3(0.0f, 1.0f, 0.0f));

		/*���� �ʿ��� (x, z) ��ǥ�� �ȼ� ���� ������ �� ���� �� (x+1, z), (z, z+1)�� ���� �ȼ� ���� ����Ͽ� ���� ���͸�
		����Ѵ�.*/
		int nHeightMapIndex = x + (z * m_nWidth);
		int xHeightMapAdd = (x < (m_nWidth - 1)) ? 1 : -1;
		int zHeightMapAdd = (z < (m_nLength - 1)) ? m_nWidth : -m_nWidth;

		//(x, z), (x+1, z), (z, z+1)�� �ȼ����� ������ ���̸� ���Ѵ�. 
		float y1 = (float)m_pHeightMapPixels[nHeightMapIndex] * m_xmf3Scale.y;
		float y2 = (float)m_pHeightMapPixels[nHeightMapIndex + xHeightMapAdd] * m_xmf3Scale.y;
		float y3 = (float)m_pHeightMapPixels[nHeightMapIndex + zHeightMapAdd] * m_xmf3Scale.y;

		//xmf3Edge1�� (0, y3, m_xmf3Scale.z) - (0, y1, 0) �����̴�. 
		XMFLOAT3 xmf3Edge1 = XMFLOAT3(0.0f, y3 - y1, m_xmf3Scale.z);
		//xmf3Edge2�� (m_xmf3Scale.x, y2, 0) - (0, y1, 0) �����̴�. 
		XMFLOAT3 xmf3Edge2 = XMFLOAT3(m_xmf3Scale.x, y2 - y1, 0.0f);
		//���� ���ʹ� xmf3Edge1�� xmf3Edge2�� ������ ����ȭ�ϸ� �ȴ�.
		XMFLOAT3 xmf3Normal = Vector3::CrossProduct(xmf3Edge1, xmf3Edge2, true);

		return(xmf3Normal);
	};

	XMFLOAT3 GetScale() { return(m_xmf3Scale); }
	std::vector<BYTE> GetHeightMapPixels() { return(m_pHeightMapPixels); }
	int GetHeightMapWidth() { return(m_nWidth); }
	int GetHeightMapLength() { return(m_nLength); }

	float GetInterpolatedHeight(int x, int z, float xFractional, float zFractional, bool bReverseQuad)
	{
		if ((x < 0) || (z < 0) || (x >= m_nWidth) || (z >= m_nLength)) return(0.0f);

		float fBottomLeft = (float)m_pHeightMapPixels[x + (z * m_nWidth)];
		float fBottomRight = (float)m_pHeightMapPixels[(x + 1) + (z * m_nWidth)];
		float fTopLeft = (float)m_pHeightMapPixels[x + ((z + 1) * m_nWidth)];
		float fTopRight = (float)m_pHeightMapPixels[(x + 1) + ((z + 1) * m_nWidth)];
#ifdef _WITH_APPROXIMATE_OPPOSITE_CORNER
		if (bReverseQuad)
		{
			if (zFractional >= xFractional)
				fBottomRight = fBottomLeft + (fTopRight - fTopLeft);
			else
				fTopLeft = fTopRight + (fBottomLeft - fBottomRight);
		}
		else
		{
			if (zFractional < (1.0f - xFractional))
				fTopRight = fTopLeft + (fBottomRight - fBottomLeft);
			else
				fBottomLeft = fTopLeft + (fBottomRight - fTopRight);
		}
#endif
		float fTopHeight = fTopLeft * (1 - xFractional) + fTopRight * xFractional;
		float fBottomHeight = fBottomLeft * (1 - xFractional) + fBottomRight * xFractional;
		float fHeight = fBottomHeight * (1 - zFractional) + fTopHeight * zFractional;

		return(fHeight);
	};
};

class CHeightMapGridMesh : public CMesh
{
protected:
	//������ ũ��(����: x-����, ����: z-����)�̴�. 
	int m_nWidth;
	int m_nLength;
	/*������ ������(����: x-����, ����: z-����, ����: y-����) �����̴�.
	���� ���� �޽��� �� ������ x-��ǥ, y-��ǥ, z-��ǥ�� ������ ������ x-��ǥ, y-��ǥ, z-��ǥ�� ���� ���� ���´�.
	��, ���� ������ x-�� ������ ������ 1�� �ƴ϶� ������ ������ x-��ǥ�� �ȴ�.
	�̷��� �ϸ� ���� ����(���� ����)�� ����ϴ��� ū ũ���� ����(����)�� �����Ҽ� �ִ�.*/
	XMFLOAT3 m_xmf3Scale;
public:
	CHeightMapGridMesh(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList
		* pd3dCommandList, int xStart, int zStart, int nWidth, int nLength, XMFLOAT3 xmf3Scale =
		XMFLOAT3(1.0f, 1.0f, 1.0f), XMFLOAT4 xmf4Color = XMFLOAT4(1.0f, 1.0f, 0.0f, 0.0f), void
		* pContext = NULL);
	virtual ~CHeightMapGridMesh();

	XMFLOAT3 GetScale() { return(m_xmf3Scale); }
	int GetWidth() { return(m_nWidth); }
	int GetLength() { return(m_nLength); }

	//������ ��ǥ�� (x, z)�� �� ����(����)�� ���̸� ��ȯ�ϴ� �Լ��̴�. 
	virtual float OnGetHeight(int x, int z, void* pContext);
	//������ ��ǥ�� (x, z)�� �� ����(����)�� ������ ��ȯ�ϴ� �Լ��̴�. 
	virtual XMFLOAT4 OnGetColor(int x, int z, void* pContext);

	//������ ��ǥ�� (x, z)�� �� ����(����)�� �ؽ��� ��ǥ�� ��ȯ�ϴ� �Լ��̴�.
	virtual XMFLOAT2 OnGetUVs(int x, int z, void* pContext);
};

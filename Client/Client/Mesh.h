///////////////////////////////////////////////////////////////////////////////
// Date: 2025-01-03
// Mesh.h : CMesh 클래스의 헤더 파일
// Version : 0.1
///////////////////////////////////////////////////////////////////////////////
#pragma once

#include "stdafx.h"

///////////////////////////////////////////////////////////////////////////////
// Vertex 구조체
///////////////////////////////////////////////////////////////////////////////

//정점을 표현하기 위한 클래스를 선언한다.
class CVertex
{
public:
	//정점의 위치 벡터이다(모든 정점은 최소한 위치 벡터를 가져야 한다).
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
	//정점의 색상이다.
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
// Mesh 클래스
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#define VERTEXT_POSITION				0x0001
#define VERTEXT_COLOR					0x0002
#define VERTEXT_NORMAL					0x0004
#define VERTEXT_TANGENT					0x0008
#define VERTEXT_TEXTURE_COORD0			0x0010
#define VERTEXT_TEXTURE_COORD1			0x0020

#define VERTEXT_BONE_INDEX_WEIGHT		0x1000

#define VERTEXT_TEXTURE					(VERTEXT_POSITION | VERTEXT_TEXTURE_COORD0)
#define VERTEXT_DETAIL					(VERTEXT_POSITION | VERTEXT_TEXTURE_COORD0 | VERTEXT_TEXTURE_COORD1)
#define VERTEXT_NORMAL_TEXTURE			(VERTEXT_POSITION | VERTEXT_NORMAL | VERTEXT_TEXTURE_COORD0)
#define VERTEXT_NORMAL_TANGENT_TEXTURE	(VERTEXT_POSITION | VERTEXT_NORMAL | VERTEXT_TANGENT | VERTEXT_TEXTURE_COORD0)
#define VERTEXT_NORMAL_DETAIL			(VERTEXT_POSITION | VERTEXT_NORMAL | VERTEXT_TEXTURE_COORD0 | VERTEXT_TEXTURE_COORD1)
#define VERTEXT_NORMAL_TANGENT__DETAIL	(VERTEXT_POSITION | VERTEXT_NORMAL | VERTEXT_TANGENT | VERTEXT_TEXTURE_COORD0 | VERTEXT_TEXTURE_COORD1)

class CMesh
{
public:
	CMesh(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList);
	~CMesh();

	// setter, getter
	UINT GetType() const { return(m_nType); }

	void SetName(const std::string& strName) { m_strMeshName = strName; }
	std::string GetName() const { return(m_strMeshName); }

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
protected:
	std::string m_strMeshName; // 메쉬의 이름

protected:
	UINT m_nType = 0x00; // 메쉬의 종류

	// 포지션 버퍼
	int	m_nVertices = 0;								// 버텍스의 개수
	std::vector<XMFLOAT3> m_pxmf3Positions;				// CPU에 저장된 버텍스 데이터
	ComPtr<ID3D12Resource> m_pd3dPositionBuffer;		// GPU에 저장된 버텍스 데이터
	ComPtr<ID3D12Resource> m_pd3dPositionUploadBuffer;	// CPU에 저장된 버텍스 데이터를 GPU에 업로드하기 위한 버퍼
	D3D12_VERTEX_BUFFER_VIEW m_d3dPositionBufferView;	// 버텍스 버퍼 뷰

	// 서브 메쉬(Index Buffer)
	UINT m_nSubMeshes;	// 서브 메쉬의 개수
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

protected:
	XMFLOAT3 m_xmf3AABBCenter;	// AABB의 중심
	XMFLOAT3 m_xmf3AABBExtents;	// AABB의 반지름
};

///////////////////////////////////////////////////////////////////////////////
//

class CStandardMesh : public CMesh
{
public:
	CStandardMesh(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList);
	virtual ~CStandardMesh();

	void LoadMeshFromFile(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, std::ifstream& File);

	virtual void ReleaseUploadBuffers() override;

	virtual void OnPreRender(ID3D12GraphicsCommandList* pd3dCommandList, void* pContext) override;

	virtual void Render(ID3D12GraphicsCommandList* pd3dCommandList, int nSubSet) override;

protected:
	// 버텍스 정보 버퍼
	std::vector<XMFLOAT4> m_pxmf4Colors;						// Material Diffuse Color
	std::vector<XMFLOAT3> m_pxmf3Tangents;		// CPU에 저장된 접선 벡터 데이터
	std::vector<XMFLOAT3> m_pxmf3BiTangents;	// CPU에 저장된 이접선 벡터 데이터
	std::vector<XMFLOAT3> m_pxmf3Normals;		// CPU에 저장된 법선 벡터 데이터

	std::vector<XMFLOAT2> m_pxmf2TextureCoords0;	// CPU에 저장된 텍스처 좌표 데이터
	std::vector<XMFLOAT2> m_pxmf2TextureCoords1;	// CPU에 저장된 텍스처 좌표 데이터

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
class CGameObject;
#define SKINNED_ANIMATION_BONES		256

class CSkinnedMesh : public CStandardMesh
{
public:
	CSkinnedMesh(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList);
	virtual ~CSkinnedMesh();

protected:
	ComPtr<ID3D12Resource> m_pd3dBoneIndexBuffer;
	ComPtr<ID3D12Resource> m_pd3dBoneIndexUploadBuffer;
	D3D12_VERTEX_BUFFER_VIEW		m_d3dBoneIndexBufferView;

	ComPtr<ID3D12Resource> m_pd3dBoneWeightBuffer;
	ComPtr<ID3D12Resource> m_pd3dBoneWeightUploadBuffer;
	D3D12_VERTEX_BUFFER_VIEW		m_d3dBoneWeightBufferView;

protected:
	int								m_nBonesPerVertex = 4;

	std::vector<XMINT4> m_pxmn4BoneIndices;
	std::vector<XMFLOAT4> m_pxmf4BoneWeights;

public:
	int		m_nSkinningBones = 0;

	std::vector<std::string> m_ppstrSkinningBoneNames; //[m_nSkinningBones]
	std::vector<std::shared_ptr<CGameObject>> m_ppSkinningBoneFrameCaches; //[m_nSkinningBones]
	std::vector<XMFLOAT4X4> m_pxmf4x4BindPoseBoneOffsets; //[m_nSkinningBones], Transposed

	ComPtr<ID3D12Resource> m_pd3dcbBindPoseBoneOffsets; //[m_nSkinningBones]
	XMFLOAT4X4* m_pcbxmf4x4MappedBindPoseBoneOffsets; //[m_nSkinningBones]

	ComPtr<ID3D12Resource> m_pd3dcbSkinningBoneTransforms; //[m_nSkinningBones], Pointer Only
	XMFLOAT4X4* m_pcbxmf4x4MappedSkinningBoneTransforms; //[m_nSkinningBones]

public:
	void PrepareSkinning(std::shared_ptr<CGameObject> pModelRootObject);
	void LoadSkinInfoFromFile(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, std::ifstream& pInFile);

	virtual void CreateShaderVariables(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList);
	virtual void UpdateShaderVariables(ID3D12GraphicsCommandList* pd3dCommandList);
	virtual void ReleaseShaderVariables();

	virtual void ReleaseUploadBuffers();

	virtual void OnPreRender(ID3D12GraphicsCommandList* pd3dCommandList, void* pContext) override;
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
	//높이 맵 이미지 픽셀(8-비트)들의 이차원 배열이다. 각 픽셀은 0~255의 값을 갖는다. 
	std::vector<BYTE> m_pHeightMapPixels;

	//높이 맵 이미지의 가로와 세로 크기이다. 
	int m_nWidth;
	int m_nLength;

	//높이 맵 이미지를 실제로 몇 배 확대하여 사용할 것인가를 나타내는 스케일 벡터이다. 
	XMFLOAT3 m_xmf3Scale;
public:
	CHeightMapImage(LPCTSTR pFileName, int nWidth, int nLength, XMFLOAT3 xmf3Scale)
	{
		m_nWidth = nWidth;
		m_nLength = nLength;
		m_xmf3Scale = xmf3Scale;

		// 이미지 크기만큼 메모리 확보
		std::vector<BYTE> pHeightMapPixels(m_nWidth * m_nLength);

		// 이미지를 파일에서 읽어서 메모리에 저장
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

	//높이 맵 이미지에서 (x, z) 위치의 픽셀 값에 기반한 지형의 높이를 반환한다. 
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

		/*지형의 좌표 (fx, fz)는 이미지 좌표계이다. 높이 맵의 x-좌표와 z-좌표가 높이 맵의 범위를 벗어나면 지형의 높이는
		0이다.*/
		if ((fx < 0.0f) || (fz < 0.0f) || (fx >= m_nWidth) || (fz >= m_nLength)) return(0.0f);

		//높이 맵의 좌표의 정수 부분과 소수 부분을 계산한다. 
		int x = (int)fx;
		int z = (int)fz;
		float fxPercent = fx - x;
		float fzPercent = fz - z;
		float fBottomLeft = (float)m_pHeightMapPixels[x + (z * m_nWidth)];
		float fBottomRight = (float)m_pHeightMapPixels[(x + 1) + (z * m_nWidth)];
		float fTopLeft = (float)m_pHeightMapPixels[x + ((z + 1) * m_nWidth)];
		float fTopRight = (float)m_pHeightMapPixels[(x + 1) + ((z + 1) * m_nWidth)];

#ifdef _WITH_APPROXIMATE_OPPOSITE_CORNER
		//z-좌표가 1, 3, 5, ...인 경우 인덱스가 오른쪽에서 왼쪽으로 나열된다. 
		bool bRightToLeft = ((z % 2) != 0);
		if (bRightToLeft)
		{
			/*지형의 삼각형들이 오른쪽에서 왼쪽 방향으로 나열되는 경우이다.
			다음 그림의 오른쪽은 (fzPercent < fxPercent)인 경우이다.
			이 경우 TopLeft의 픽셀 값은 (fTopLeft = fTopRight + (fBottomLeft - fBottomRight))로 근사한다.
			다음 그림의 왼쪽은 (fzPercent ≥ fxPercent)인 경우이다.
			이 경우 BottomRight의 픽셀 값은 (fBottomRight = fBottomLeft + (fTopRight - fTopLeft))로 근사한다.*/
			if (fzPercent >= fxPercent)
				fBottomRight = fBottomLeft + (fTopRight - fTopLeft);
			else
				fTopLeft = fTopRight + (fBottomLeft - fBottomRight);
		}
		else
		{
			/*지형의 삼각형들이 왼쪽에서 오른쪽 방향으로 나열되는 경우이다.
			다음 그림의 왼쪽은 (fzPercent < (1.0f - fxPercent))인 경우이다.
			이 경우 TopRight의 픽셀 값은 (fTopRight = fTopLeft + (fBottomRight - fBottomLeft))로 근사한다.
			다음 그림의 오른쪽은 (fzPercent ≥ (1.0f - fxPercent))인 경우이다.
			이 경우 BottomLeft의 픽셀 값은 (fBottomLeft = fTopLeft + (fBottomRight - fTopRight))로 근사한다.*/
			if (fzPercent < (1.0f - fxPercent))
				fTopRight = fTopLeft + (fBottomRight - fBottomLeft);
			else
				fBottomLeft = fTopLeft + (fBottomRight - fTopRight);
		}
#endif
		//사각형의 네 점을 보간하여 높이(픽셀 값)를 계산한다. 
		float fTopHeight = fTopLeft * (1 - fxPercent) + fTopRight * fxPercent;
		float fBottomHeight = fBottomLeft * (1 - fxPercent) + fBottomRight * fxPercent;
		float fHeight = fBottomHeight * (1 - fzPercent) + fTopHeight * fzPercent;

		return(fHeight);
#endif
	};
	//높이 맵 이미지에서 (x, z) 위치의 법선 벡터를 반환한다. 
	XMFLOAT3 GetHeightMapNormal(int x, int z)
	{
		//x-좌표와 z-좌표가 높이 맵의 범위를 벗어나면 지형의 법선 벡터는 y-축 방향 벡터이다. 
		if ((x < 0.0f) || (z < 0.0f) || (x >= m_nWidth) || (z >= m_nLength))
			return(XMFLOAT3(0.0f, 1.0f, 0.0f));

		/*높이 맵에서 (x, z) 좌표의 픽셀 값과 인접한 두 개의 점 (x+1, z), (z, z+1)에 대한 픽셀 값을 사용하여 법선 벡터를
		계산한다.*/
		int nHeightMapIndex = x + (z * m_nWidth);
		int xHeightMapAdd = (x < (m_nWidth - 1)) ? 1 : -1;
		int zHeightMapAdd = (z < (m_nLength - 1)) ? m_nWidth : -m_nWidth;

		//(x, z), (x+1, z), (z, z+1)의 픽셀에서 지형의 높이를 구한다. 
		float y1 = (float)m_pHeightMapPixels[nHeightMapIndex] * m_xmf3Scale.y;
		float y2 = (float)m_pHeightMapPixels[nHeightMapIndex + xHeightMapAdd] * m_xmf3Scale.y;
		float y3 = (float)m_pHeightMapPixels[nHeightMapIndex + zHeightMapAdd] * m_xmf3Scale.y;

		//xmf3Edge1은 (0, y3, m_xmf3Scale.z) - (0, y1, 0) 벡터이다. 
		XMFLOAT3 xmf3Edge1 = XMFLOAT3(0.0f, y3 - y1, m_xmf3Scale.z);
		//xmf3Edge2는 (m_xmf3Scale.x, y2, 0) - (0, y1, 0) 벡터이다. 
		XMFLOAT3 xmf3Edge2 = XMFLOAT3(m_xmf3Scale.x, y2 - y1, 0.0f);
		//법선 벡터는 xmf3Edge1과 xmf3Edge2의 외적을 정규화하면 된다.
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
	//격자의 크기(가로: x-방향, 세로: z-방향)이다. 
	int m_nWidth;
	int m_nLength;
	/*격자의 스케일(가로: x-방향, 세로: z-방향, 높이: y-방향) 벡터이다.
	실제 격자 메쉬의 각 정점의 x-좌표, y-좌표, z-좌표는 스케일 벡터의 x-좌표, y-좌표, z-좌표로 곱한 값을 갖는다.
	즉, 실제 격자의 x-축 방향의 간격은 1이 아니라 스케일 벡터의 x-좌표가 된다.
	이렇게 하면 작은 격자(적은 정점)를 사용하더라도 큰 크기의 격자(지형)를 생성할수 있다.*/
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

	//격자의 좌표가 (x, z)일 때 교점(정점)의 높이를 반환하는 함수이다. 
	virtual float OnGetHeight(int x, int z, void* pContext);
	//격자의 좌표가 (x, z)일 때 교점(정점)의 색상을 반환하는 함수이다. 
	virtual XMFLOAT4 OnGetColor(int x, int z, void* pContext);

	//격자의 좌표가 (x, z)일 때 교점(정점)의 텍스쳐 좌표를 반환하는 함수이다.
	virtual XMFLOAT2 OnGetUVs(int x, int z, void* pContext);
};

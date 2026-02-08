#pragma once
#include "GameObject.h"

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//

struct CHeightMapTerrainDesc
{
	std::wstring wstrHeightMapFilePath = L"null";
	std::wstring wstrMeshFilePath = L"null";
	int nWidth = 0;
	int nLength = 0;
	int nBlockWidth = 0;
	int nBlockLength = 0;
	XMFLOAT3 xmf3Scale = XMFLOAT3(1.0f, 1.0f, 1.0f);
	XMFLOAT4 xmf4Color = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
};

class CHeightMapTerrain : public CGameObject
{
public:
	CHeightMapTerrain();
	virtual ~CHeightMapTerrain();

	virtual GAMEOBJECT_LAYER GetLayer() override { return GAMEOBJECT_LAYER::LAYER_TERRAIN; }

	// Object Initialization
	virtual void Initialize(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, ID3D12RootSignature* pd3dGraphicsRootSignature,
		LPCTSTR pFileName, int nWidth, int nLength, int nBlockWidth, int nBlockLength,
		XMFLOAT3 xmf3Scale, XMFLOAT4 xmf4Color);

	static std::shared_ptr<CHeightMapTerrain> InitializeByBinary(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, ID3D12RootSignature* pd3dGraphicsRootSignature,
		LPCTSTR pBinFileName, LPCTSTR pFileName, int nWidth, int nLength, int nBlockWidth, int nBlockLength,
		XMFLOAT3 xmf3Scale, XMFLOAT4 xmf4Color);

	virtual std::string GetDefaultName() override { return "CHeightMapTerrain"; }

	static std::shared_ptr<CHeightMapTerrain> Create(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, ID3D12RootSignature* pd3dGraphicsRootSignature,
		LPCTSTR pFileName, int nWidth, int nLength, int nBlockWidth, int nBlockLength,
		XMFLOAT3 xmf3Scale, XMFLOAT4 xmf4Color);

	void Initialize(CHeightMapTerrainDesc desc);

	void Initialize(std::wstring wstrHeightMapFilePath, std::wstring wstrMeshFilePath,
		int nWidth, int nLength, int nBlockWidth, int nBlockLength,
		XMFLOAT3 xmf3Scale, XMFLOAT4 xmf4Color);

	void CreateGridMeshFromHeightMap(int nBlockWidth, int nBlockLength, XMFLOAT4 xmf4Color = XMFLOAT4{ 1.0f, 1.0f, 1.0f, 1.0f });
	void CreateGridMeshFromFile(std::wstring& wstrMeshFilePath);

	virtual BoundingBox GetMergedMeshBound(BoundingBox* pVolume = nullptr) override;


	// Object Render
	virtual void Render(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera, bool bDepthWrite = false) override;

	//지형의 높이를 계산하는 함수이다(월드 좌표계). 높이 맵의 높이에 스케일의 y를 곱한 값이다. 
	float GetHeight(float x, float z) {
		return (m_pHeightMapImage->GetHeight(x / m_xmf3Scale.x, z / m_xmf3Scale.z) * m_xmf3Scale.y);
	}

	//지형의 법선 벡터를 계산하는 함수이다(월드 좌표계). 높이 맵의 법선 벡터를 사용한다. 
	XMFLOAT3 GetNormal(float x, float z) {
		return(m_pHeightMapImage->GetHeightMapNormal(int(x / m_xmf3Scale.x),
			int(z / m_xmf3Scale.z)));
	}

	int GetHeightMapWidth() { return(m_pHeightMapImage->GetHeightMapWidth()); }
	int GetHeightMapLength() { return(m_pHeightMapImage->GetHeightMapLength()); }

	XMFLOAT3 GetScale() { return(m_xmf3Scale); }

	//지형의 크기(가로/세로)를 반환한다. 높이 맵의 크기에 스케일을 곱한 값이다. 
	float GetWidth() { return(m_nWidth * m_xmf3Scale.x); }
	float GetLength() { return(m_nLength * m_xmf3Scale.z); }

private:
	//지형의 높이 맵으로 사용할 이미지이다. 
	std::shared_ptr<CHeightMapImage> m_pHeightMapImage;

	//높이 맵의 가로와 세로 크기이다. 
	int m_nWidth;
	int m_nLength;

	//지형을 실제로 몇 배 확대할 것인가를 나타내는 스케일 벡터이다. 
	XMFLOAT3 m_xmf3Scale;

	std::vector<CTerrainVertex> m_pVertices;
	std::vector<UINT> m_pIndices;

	BoundingBox m_TerrainBoundingBox; // 지형의 Bounding Box

}; // CHeightMapTerrain


#include "HeightMapTerrain.h"

///////////////////////////////////////////////////////////////////////////////
//

CHeightMapTerrain::CHeightMapTerrain()
{
	SetLayer(LAYER_ENVIRONMENT);
}

CHeightMapTerrain::~CHeightMapTerrain()
{
}

std::shared_ptr<CHeightMapTerrain> CHeightMapTerrain::Create(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, ID3D12RootSignature* pd3dGraphicsRootSignature, LPCTSTR pFileName, int nWidth, int nLength, int nBlockWidth, int nBlockLength, XMFLOAT3 xmf3Scale, XMFLOAT4 xmf4Color)
{
	std::shared_ptr<CHeightMapTerrain> pHeightMapTerrain = std::make_shared<CHeightMapTerrain>();
	pHeightMapTerrain->Initialize(pd3dDevice, pd3dCommandList, pd3dGraphicsRootSignature, pFileName, nWidth, nLength, nBlockWidth, nBlockLength, xmf3Scale, xmf4Color);

	return pHeightMapTerrain;
}

void CHeightMapTerrain::Initialize(CHeightMapTerrainDesc desc)
{
	Initialize(desc.wstrHeightMapFilePath, desc.wstrMeshFilePath, desc.nWidth, desc.nLength, desc.nBlockWidth, desc.nBlockLength, desc.xmf3Scale, desc.xmf4Color);
}

void CHeightMapTerrain::Initialize(std::wstring wstrHeightMapFilePath, std::wstring wstrMeshFilePath, int nWidth, int nLength, int nBlockWidth, int nBlockLength, XMFLOAT3 xmf3Scale, XMFLOAT4 xmf4Color)
{
	// assert(wstrHeightMapFilePath != L"null");

	// 지형에 사용할 높이 맵을 생성한다.
	m_pHeightMapImage = std::make_shared<CHeightMapImage>(wstrHeightMapFilePath, nWidth, nLength, xmf3Scale);

	//지형에 사용할 높이 맵의 가로, 세로의 크기이다. 
	m_nWidth = nWidth;
	m_nLength = nLength;

	//xmf3Scale는 지형을 실제로 몇 배 확대할 것인가를 나타낸다. 
	m_xmf3Scale = xmf3Scale;

	// Mesh 
	if (wstrMeshFilePath != L"null") CreateGridMeshFromFile(wstrMeshFilePath);
	else CreateGridMeshFromHeightMap(nBlockWidth, nBlockLength, xmf4Color);


	// Material
	m_ppMaterials.resize(1);
	m_ppMaterials[0] = std::make_shared<CMaterial>();

	// Set Shader
	std::shared_ptr<CShader> pShader = CResourceManager::Instance().GetOrCreate<CTerrainShader>();
	m_ppMaterials[0]->SetShader(pShader);

	// Set Texture
	TextureRecipe terrainTextureRecipe;
	terrainTextureRecipe.source = TEXTURE_SOURCE_FILE;
	terrainTextureRecipe.name = L"TerrainTexture_0";
	terrainTextureRecipe.filePath = L"Image/Grass.jpg";
	terrainTextureRecipe.type = RESOURCE_TEXTURE2D;
	terrainTextureRecipe.rootparameterindex = ROOT_PARAMETER_TERRAIN0;

	auto pTexture = std::make_shared<CTexture>(terrainTextureRecipe);
	m_ppMaterials[0]->AddTexture(pTexture);

	terrainTextureRecipe.name = L"TerrainTexture_1";
	terrainTextureRecipe.filePath = L"Image/Stone01.jpg";
	terrainTextureRecipe.rootparameterindex = ROOT_PARAMETER_TERRAIN1;

	pTexture = std::make_shared<CTexture>(terrainTextureRecipe);
	m_ppMaterials[0]->AddTexture(pTexture);

	// Set MeshBondingBox
	// 지형은 [0,0,0] 부터 [m_nWidth * m_xmf3Scale.x, 1 * m_xmf3Scale.y, m_nLength * m_xmf3Scale.z] 까지 생성되므로 BoundingBox의 Center와 Extents를 다음과 같이 설정한다.
	m_TerrainBoundingBox.Center = XMFLOAT3((m_nWidth * m_xmf3Scale.x) / 2.0f, m_xmf3Scale.y / 2.0f, (m_nLength * m_xmf3Scale.z) / 2.0f);
	m_TerrainBoundingBox.Extents = XMFLOAT3((m_nWidth * m_xmf3Scale.x) / 2.0f, m_xmf3Scale.y / 2.0f, (m_nLength * m_xmf3Scale.z) / 2.0f);
}

void CHeightMapTerrain::CreateGridMeshFromHeightMap(int nBlockWidth, int nBlockLength, XMFLOAT4 xmf4Color)
{
	/*지형 객체는 격자 메쉬들의 배열로 만들 것이다.
	nBlockWidth, nBlockLength는 격자 메쉬 하나의 가로, 세로 크기이다.
	cxQuadsPerBlock, czQuadsPerBlock은 격자 메쉬의 가로 방향과 세로 방향 사각형의 개수이다.*/
	int cxQuadsPerBlock = nBlockWidth - 1;
	int czQuadsPerBlock = nBlockLength - 1;

	long cxBlocks = (m_nWidth - 1) / cxQuadsPerBlock;
	long czBlocks = (m_nLength - 1) / czQuadsPerBlock;

	if (m_nWidth == nBlockWidth && m_nLength == nBlockLength) {
		std::shared_ptr<CMesh> pHeightMapGridMesh;
		pHeightMapGridMesh = std::make_shared<CHeightMapGridMesh>(0, 0, nBlockWidth, nBlockLength, m_xmf3Scale, m_xmf4Color, m_pHeightMapImage.get());
		SetMesh(pHeightMapGridMesh);
	}
	else
	{
		std::shared_ptr<CMesh> pHeightMapGridMesh;

		m_pChilds.reserve(cxBlocks * czBlocks); //지형을 표현하기 위한 격자 메쉬의 개수이다.
		int index = 0;
		for (int z = 0, zStart = 0; z < czBlocks; z++)
		{
			for (int x = 0, xStart = 0; x < cxBlocks; x++)
			{
				std::string name = "HeightMapSub" + std::to_string(index++);
				auto pHeightMapGameObject = std::make_unique<CGameObject>(name);
				pHeightMapGameObject->SetMaterialSize(0);
				xStart = x * (nBlockWidth - 1);
				zStart = z * (nBlockLength - 1);
				pHeightMapGridMesh = std::make_shared<CHeightMapGridMesh>(xStart, zStart, nBlockWidth, nBlockLength, m_xmf3Scale, xmf4Color, m_pHeightMapImage.get());
				pHeightMapGridMesh->SetName(name + "_Mesh");
				pHeightMapGameObject->SetMesh(pHeightMapGridMesh);
				SetChild(std::move(pHeightMapGameObject));
			}
		}
	}
}

void CHeightMapTerrain::CreateGridMeshFromFile(std::wstring& wstrMeshFilePath)
{
	std::ifstream in(wstrMeshFilePath, std::ios::binary);
	if (!in.is_open())
	{
		std::wcout << L"Failed to open mesh file: " << wstrMeshFilePath << L"\n";
		return;
	}

	UINT vertexCount = 0;
	UINT indexCount = 0;

	in.read(reinterpret_cast<char*>(&vertexCount), sizeof(UINT));
	in.read(reinterpret_cast<char*>(&indexCount), sizeof(UINT));

	std::vector<CTerrainVertex> outVertices;
	std::vector<UINT> outIndices;

	outVertices.resize(vertexCount);
	outIndices.resize(indexCount);

	in.read(reinterpret_cast<char*>(outVertices.data()), vertexCount * sizeof(CTerrainVertex));
	in.read(reinterpret_cast<char*>(outIndices.data()), indexCount * sizeof(UINT));

	in.close();

	std::cout << "Import successful.\n";
	std::cout << "Vertices: " << vertexCount << ", Indices: " << indexCount << "\n";

	std::shared_ptr<CMesh> pHeightMapGridMesh;
	pHeightMapGridMesh = std::make_shared<CHeightMapGridMesh>(outVertices, outIndices);
	SetMesh(pHeightMapGridMesh);

	m_pVertices = outVertices;
	m_pIndices = outIndices;

}

BoundingBox CHeightMapTerrain::GetMergedMeshBound(BoundingBox* pVolume)
{
	return m_TerrainBoundingBox;
}

void CHeightMapTerrain::Initialize(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, ID3D12RootSignature* pd3dGraphicsRootSignature, LPCTSTR pFileName, int nWidth, int nLength, int nBlockWidth, int nBlockLength, XMFLOAT3 xmf3Scale, XMFLOAT4 xmf4Color)
{
	//지형에 사용할 높이 맵의 가로, 세로의 크기이다. 
	m_nWidth = nWidth;
	m_nLength = nLength;

	/*지형 객체는 격자 메쉬들의 배열로 만들 것이다.
	nBlockWidth, nBlockLength는 격자 메쉬 하나의 가로, 세로 크기이다.
	cxQuadsPerBlock, czQuadsPerBlock은 격자 메쉬의 가로 방향과 세로 방향 사각형의 개수이다.*/
	int cxQuadsPerBlock = nBlockWidth - 1;
	int czQuadsPerBlock = nBlockLength - 1;

	long cxBlocks = (nWidth - 1) / cxQuadsPerBlock;
	long czBlocks = (nLength - 1) / czQuadsPerBlock;

	//xmf3Scale는 지형을 실제로 몇 배 확대할 것인가를 나타낸다. 
	m_xmf3Scale = xmf3Scale;

	//지형에 사용할 높이 맵을 생성한다. 
	m_pHeightMapImage = std::make_shared<CHeightMapImage>(pFileName, nWidth, nLength, xmf3Scale);

	if (nWidth == nBlockWidth && nLength == nBlockLength) {
		std::shared_ptr<CMesh> pHeightMapGridMesh;
		pHeightMapGridMesh = std::make_shared<CHeightMapGridMesh>(pd3dDevice, pd3dCommandList, 0, 0, nBlockWidth, nBlockLength, xmf3Scale, xmf4Color, m_pHeightMapImage.get());
		SetMesh(pHeightMapGridMesh);
	}
	else
	{
		std::shared_ptr<CMesh> pHeightMapGridMesh;

		m_pChilds.reserve(cxBlocks * czBlocks); //지형을 표현하기 위한 격자 메쉬의 개수이다.

		for (int z = 0, zStart = 0; z < czBlocks; z++)
		{
			for (int x = 0, xStart = 0; x < cxBlocks; x++)
			{
				auto pHeightMapGameObject = std::make_unique<CGameObject>("HeightMapSub");
				pHeightMapGameObject->SetMaterialSize(0);
				xStart = x * (nBlockWidth - 1);
				zStart = z * (nBlockLength - 1);
				pHeightMapGridMesh = std::make_shared<CHeightMapGridMesh>(pd3dDevice, pd3dCommandList, xStart, zStart, nBlockWidth, nBlockLength, xmf3Scale, xmf4Color, m_pHeightMapImage.get());
				pHeightMapGameObject->SetMesh(pHeightMapGridMesh);
				SetChild(std::move(pHeightMapGameObject));
			}
		}
	}
	//{

	//	//지형 전체를 표현하기 위한 격자 메쉬에 대한 포인터 배열을 생성한다. 
	//	CHeightMapGridMesh* pHeightMapGridMesh = NULL;
	//	//지형의 일부분을 나타내는 격자 메쉬를 생성하여 지형 메쉬에 저장한다. 
	//	pHeightMapGridMesh = new CHeightMapGridMesh(pd3dDevice, pd3dCommandList, 0,
	//		0, nBlockWidth, nBlockLength, xmf3Scale, xmf4Color, m_pHeightMapImage);

	//	SetMesh(pHeightMapGridMesh);
	//}

	//지형을 렌더링하기 위한 셰이더를 생성한다. 

#ifdef _WITH_TERRAIN_TESSELATION
	CTerrainTessellationShader* pShader = new CTerrainTessellationShader();
#else
	std::shared_ptr<CShader> pShader = CResourceManager::Instance().GetOrCreate<CTerrainShader>();
#endif
	pShader->CreateShader(pd3dDevice, pd3dCommandList, pd3dGraphicsRootSignature);
	pShader->CreateShaderVariables(pd3dDevice, pd3dCommandList);
	SetShader(pShader);

	auto pMaterial = std::make_shared<CMaterial>();
	m_ppMaterials.push_back(pMaterial);

	TextureRecipe terrainTextureRecipe;
	terrainTextureRecipe.source = TEXTURE_SOURCE_FILE;
	terrainTextureRecipe.name = L"TerrainTexture_0";
	terrainTextureRecipe.filePath = L"Image/Stone01.jpg";
	terrainTextureRecipe.type = RESOURCE_TEXTURE2D;
	terrainTextureRecipe.rootparameterindex = ROOT_PARAMETER_ALBEDO_TEXTURE;

	auto pTexture = std::make_shared<CTexture>(terrainTextureRecipe);
	pMaterial->AddTexture(pTexture);

	terrainTextureRecipe.name = L"TerrainTexture_1";
	terrainTextureRecipe.filePath = L"Image/Grass.jpg";
	terrainTextureRecipe.rootparameterindex++;

	pTexture = std::make_shared<CTexture>(terrainTextureRecipe);
	pMaterial->AddTexture(pTexture);

	//std::shared_ptr<CTexture> pTexture = std::make_shared<CTexture>(2, RESOURCE_TEXTURE2D, 2);
	//pTexture->LoadTextureFromWICFile(pd3dDevice, pd3dCommandList, L"Image/Stone01.jpg", RESOURCE_TEXTURE2D, 0);
	//pTexture->LoadTextureFromWICFile(pd3dDevice, pd3dCommandList, L"Image/Grass.jpg", RESOURCE_TEXTURE2D, 1);
	//CResourceManager::Instance().CreateShaderResourceViews(pd3dDevice, pTexture.get(), 0, ROOT_PARAMETER_STANDARD_TEXTURES);


}

std::shared_ptr<CHeightMapTerrain> CHeightMapTerrain::InitializeByBinary(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, ID3D12RootSignature* pd3dGraphicsRootSignature, LPCTSTR pBinFileName, LPCTSTR pFileName, int nWidth, int nLength, int nBlockWidth, int nBlockLength, XMFLOAT3 xmf3Scale, XMFLOAT4 xmf4Color)
{
	std::shared_ptr<CHeightMapTerrain> pHeightMapTerrain = std::make_shared<CHeightMapTerrain>();

	//지형에 사용할 높이 맵의 가로, 세로의 크기이다. 
	pHeightMapTerrain->m_nWidth = nWidth;
	pHeightMapTerrain->m_nLength = nLength;

	/*지형 객체는 격자 메쉬들의 배열로 만들 것이다.
	nBlockWidth, nBlockLength는 격자 메쉬 하나의 가로, 세로 크기이다.
	cxQuadsPerBlock, czQuadsPerBlock은 격자 메쉬의 가로 방향과 세로 방향 사각형의 개수이다.*/
	int cxQuadsPerBlock = nBlockWidth - 1;
	int czQuadsPerBlock = nBlockLength - 1;

	long cxBlocks = (nWidth - 1) / cxQuadsPerBlock;
	long czBlocks = (nLength - 1) / czQuadsPerBlock;

	//xmf3Scale는 지형을 실제로 몇 배 확대할 것인가를 나타낸다. 
	pHeightMapTerrain->m_xmf3Scale = xmf3Scale;

	//지형에 사용할 높이 맵을 생성한다. 
	pHeightMapTerrain->m_pHeightMapImage = std::make_shared<CHeightMapImage>(pFileName, nWidth, nLength, xmf3Scale);

	{
		std::ifstream in(pBinFileName, std::ios::binary);
		if (!in.is_open()) return nullptr;

		UINT vertexCount = 0;
		UINT indexCount = 0;

		in.read(reinterpret_cast<char*>(&vertexCount), sizeof(UINT));
		in.read(reinterpret_cast<char*>(&indexCount), sizeof(UINT));

		std::vector<CTerrainVertex> outVertices;
		std::vector<UINT> outIndices;

		outVertices.resize(vertexCount);
		outIndices.resize(indexCount);

		in.read(reinterpret_cast<char*>(outVertices.data()), vertexCount * sizeof(CTerrainVertex));
		in.read(reinterpret_cast<char*>(outIndices.data()), indexCount * sizeof(UINT));

		in.close();

		std::cout << "Import successful.\n";
		std::cout << "Vertices: " << vertexCount << ", Indices: " << indexCount << "\n";

		std::shared_ptr<CMesh> pHeightMapGridMesh;
		pHeightMapGridMesh = std::make_shared<CHeightMapGridMesh>(pd3dDevice, pd3dCommandList, outVertices, outIndices);
		pHeightMapTerrain->SetMesh(pHeightMapGridMesh);

		pHeightMapTerrain->m_pVertices = outVertices;
		pHeightMapTerrain->m_pIndices = outIndices;
	}


#ifdef _WITH_TERRAIN_TESSELATION
	CTerrainTessellationShader* pShader = new CTerrainTessellationShader();
#else
	std::shared_ptr<CShader> pShader = CResourceManager::Instance().GetOrCreate<CTerrainShader>();
#endif
	pShader->CreateShader(pd3dDevice, pd3dCommandList, pd3dGraphicsRootSignature);
	pShader->CreateShaderVariables(pd3dDevice, pd3dCommandList);
	pHeightMapTerrain->SetShader(pShader);

	// 이 define 은 stdafx.h 에서 정의되어 있다.

	pHeightMapTerrain->m_ppMaterials.resize(1);

	TextureRecipe terrainTextureRecipe;
	terrainTextureRecipe.source = TEXTURE_SOURCE_FILE;
	terrainTextureRecipe.name = L"TerrainTexture_0";
	terrainTextureRecipe.filePath = L"Image/Stone01.jpg";
	terrainTextureRecipe.type = RESOURCE_TEXTURE2D;
	terrainTextureRecipe.rootparameterindex = ROOT_PARAMETER_TERRAIN0;

	auto pTexture = std::make_shared<CTexture>(terrainTextureRecipe);
	pHeightMapTerrain->m_ppMaterials[0]->AddTexture(pTexture);

	terrainTextureRecipe.name = L"TerrainTexture_1";
	terrainTextureRecipe.filePath = L"Image/Grass.jpg";
	terrainTextureRecipe.rootparameterindex = ROOT_PARAMETER_TERRAIN1;

	pTexture = std::make_shared<CTexture>(terrainTextureRecipe);
	pHeightMapTerrain->m_ppMaterials[0]->AddTexture(pTexture);

	//std::shared_ptr<CTexture> pTexture = std::make_shared<CTexture>(2, RESOURCE_TEXTURE2D, 2);
	//pTexture->LoadTextureFromWICFile(pd3dDevice, pd3dCommandList, L"Image/Grass.jpg", RESOURCE_TEXTURE2D, 0);
	//pTexture->LoadTextureFromWICFile(pd3dDevice, pd3dCommandList, L"Image/Stone01.jpg", RESOURCE_TEXTURE2D, 1);

	//CResourceManager::Instance().CreateShaderResourceViews(pd3dDevice, pTexture.get(), 0, ROOT_PARAMETER_STANDARD_TEXTURES);
	return pHeightMapTerrain;
}

void CHeightMapTerrain::Render(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera, bool bDepthWrite)
{
	UpdateTransform();

	CGameObject::Render(pd3dCommandList, pCamera, bDepthWrite);
}

void CalculateNormal(std::vector<CTerrainVertex>& vertices) {
	constexpr int WIDTH = 257;
	constexpr int HEIGHT = 257;
	constexpr float HEIGHT_SCALE = 50.0f;
	constexpr float CELL_SIZE = 1.0f;

	for (int z = 1; z < HEIGHT - 1; ++z) {
		for (int x = 1; x < WIDTH - 1; ++x) {
			int idx = z * WIDTH + x;

			XMFLOAT3 left = vertices[z * WIDTH + (x - 1)].m_xmf3Position;
			XMFLOAT3 right = vertices[z * WIDTH + (x + 1)].m_xmf3Position;
			XMFLOAT3 down = vertices[(z - 1) * WIDTH + x].m_xmf3Position;
			XMFLOAT3 up = vertices[(z + 1) * WIDTH + x].m_xmf3Position;

			XMVECTOR dx = XMVectorSubtract(XMLoadFloat3(&right), XMLoadFloat3(&left));
			XMVECTOR dz = XMVectorSubtract(XMLoadFloat3(&up), XMLoadFloat3(&down));
			XMVECTOR normal = XMVector3Normalize(XMVector3Cross(dz, dx));

			XMStoreFloat3(&vertices[idx].m_xmf3Normal, normal);
		}
	}
}

void ExportTerrain(const char* rawFile, const char* outFile) {
	constexpr int WIDTH = 257;
	constexpr int HEIGHT = 257;
	constexpr float HEIGHT_SCALE = 50.0f;
	constexpr float CELL_SIZE = 1.0f;

	std::ifstream in(rawFile, std::ios::binary);
	if (!in.is_open()) {
		std::cerr << "Failed to open raw file.\n";
		return;
	}

	std::vector<HEIGHTMAPDEPTH> heightMap(WIDTH * HEIGHT);
	in.read(reinterpret_cast<char*>(heightMap.data()), heightMap.size() * sizeof(HEIGHTMAPDEPTH));
	in.close();

	std::vector<CTerrainVertex> vertices(WIDTH * HEIGHT);

	for (int z = 0; z < HEIGHT; ++z) {
		for (int x = 0; x < WIDTH; ++x) {
			int idx = z * WIDTH + x;
			float height = static_cast<float>(heightMap[idx]) / std::numeric_limits<HEIGHTMAPDEPTH>::max() * HEIGHT_SCALE;

			vertices[idx].m_xmf3Position = XMFLOAT3(x * CELL_SIZE, height, z * CELL_SIZE);
			vertices[idx].m_xmf4Diffuse = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f); // white
			vertices[idx].m_xmf2TexCoord0 = XMFLOAT2(static_cast<float>(x), static_cast<float>(z));
			vertices[idx].m_xmf2TexCoord1 = XMFLOAT2{ vertices[idx].m_xmf2TexCoord0.x / 2, vertices[idx].m_xmf2TexCoord0.y / 2 };
			vertices[idx].m_xmf3Normal = XMFLOAT3(0, 1, 0); // 임시
		}
	}

	CalculateNormal(vertices);

	std::vector<unsigned int> indices;
	for (int z = 0; z < HEIGHT - 1; ++z) {
		for (int x = 0; x < WIDTH - 1; ++x) {
			int topLeft = z * WIDTH + x;
			int topRight = topLeft + 1;
			int bottomLeft = (z + 1) * WIDTH + x;
			int bottomRight = bottomLeft + 1;

			indices.push_back(topLeft);
			indices.push_back(bottomLeft);
			indices.push_back(topRight);

			indices.push_back(topRight);
			indices.push_back(bottomLeft);
			indices.push_back(bottomRight);
		}
	}

	std::ofstream out(outFile, std::ios::binary);
	unsigned int vertexCount = static_cast<unsigned int>(vertices.size());
	unsigned int indexCount = static_cast<unsigned int>(indices.size());

	out.write(reinterpret_cast<const char*>(&vertexCount), sizeof(unsigned int));
	out.write(reinterpret_cast<const char*>(&indexCount), sizeof(unsigned int));
	out.write(reinterpret_cast<const char*>(vertices.data()), vertexCount * sizeof(CTerrainVertex));
	out.write(reinterpret_cast<const char*>(indices.data()), indexCount * sizeof(unsigned int));
	out.close();

	std::cout << "Export complete: " << outFile << "\n";
}

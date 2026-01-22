#include "ResourceManager.h"


/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//

void CResourceManager::Initialize(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommnadList, ID3D12RootSignature* rootsignature) {
	m_d3dGraphicRootSignature = rootsignature;

	CreateDescriptorHeap(pd3dDevice);

	CreateDefaultMesh(pd3dDevice, pd3dCommnadList);
	LoadModelList(pd3dDevice, pd3dCommnadList);
}

void CResourceManager::CreateDefaultMesh(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommnadList)
{
	CreateDefualtQuad(pd3dDevice, pd3dCommnadList);
	CreateDefaultCube(pd3dDevice, pd3dCommnadList);
	CreateDefualtSphere(pd3dDevice, pd3dCommnadList);
}

void CResourceManager::CreateDefualtQuad(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommnadList)
{
	// Quad Mesh 생성
	auto pMesh = std::make_shared<CQuadMesh>(pd3dDevice, pd3dCommnadList);
	pMesh->SetName("Quad");
	SetMesh("Quad", pMesh);
	CResourceManager::Instance().RegisterMeshUpload(pMesh.get());
}

void CResourceManager::CreateDefaultCube(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommnadList)
{
	// Cube Mesh 생성
	auto pMesh = std::make_shared<CCubeMesh>(pd3dDevice, pd3dCommnadList);
	pMesh->SetName("Cube");
	SetMesh("Cube", pMesh);
	CResourceManager::Instance().RegisterMeshUpload(pMesh.get());
}

void CResourceManager::CreateDefualtSphere(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommnadList)
{
	auto pMesh = std::make_shared<CSphereMesh>(pd3dDevice, pd3dCommnadList);
	pMesh->SetName("Sphere");
	SetMesh("Sphere", pMesh);
	CResourceManager::Instance().RegisterMeshUpload(pMesh.get());
}

// 모든 리소스 해제
void CResourceManager::ReleaseResources() {
	ModelInfos.clear();
	TextureInfos.clear();
	MeshInfos.clear();

	// Shader Resource Release
	ShaderInfos.clear();
}


// ----------------------------------------
// 서술자 힙 (Descriptor Heap) 관련
// ----------------------------------------
void CResourceManager::CreateDescriptorHeap(ID3D12Device* pd3dDevice)
{
	CreateCbvSrvDescriptorHeaps(pd3dDevice, 100, 3000);
}

void CResourceManager::CreateCbvSrvDescriptorHeaps(ID3D12Device * pd3dDevice, int nConstantBufferViews, int nShaderResourceViews)
{
	if (m_pDescriptorHeap) return;

	D3D12_DESCRIPTOR_HEAP_DESC d3dDescriptorHeapDesc;
	d3dDescriptorHeapDesc.NumDescriptors = nConstantBufferViews + nShaderResourceViews; //CBVs + SRVs 
	d3dDescriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	d3dDescriptorHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	d3dDescriptorHeapDesc.NodeMask = 0;

	m_pDescriptorHeap = std::make_unique<CDescirptorHeap>();

	pd3dDevice->CreateDescriptorHeap(&d3dDescriptorHeapDesc, __uuidof(ID3D12DescriptorHeap), (void**)&m_pDescriptorHeap->m_pd3dCbvSrvDescriptorHeap);

	m_pDescriptorHeap->m_pd3dCbvSrvDescriptorHeap->SetName(L"CResourceManager::m_pd3dCbvSrvDescriptorHeap");

	m_pDescriptorHeap->m_d3dCbvCPUDescriptorStartHandle = m_pDescriptorHeap->m_pd3dCbvSrvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
	m_pDescriptorHeap->m_d3dCbvGPUDescriptorStartHandle = m_pDescriptorHeap->m_pd3dCbvSrvDescriptorHeap->GetGPUDescriptorHandleForHeapStart();
	m_pDescriptorHeap->m_d3dSrvCPUDescriptorStartHandle.ptr = m_pDescriptorHeap->m_d3dCbvCPUDescriptorStartHandle.ptr + (::gnCbvSrvDescriptorIncrementSize * nConstantBufferViews);
	m_pDescriptorHeap->m_d3dSrvGPUDescriptorStartHandle.ptr = m_pDescriptorHeap->m_d3dCbvGPUDescriptorStartHandle.ptr + (::gnCbvSrvDescriptorIncrementSize * nConstantBufferViews);

	m_pDescriptorHeap->m_d3dCbvCPUDescriptorNextHandle = m_pDescriptorHeap->m_d3dCbvCPUDescriptorStartHandle;
	m_pDescriptorHeap->m_d3dCbvGPUDescriptorNextHandle = m_pDescriptorHeap->m_d3dCbvGPUDescriptorStartHandle;
	m_pDescriptorHeap->m_d3dSrvCPUDescriptorNextHandle = m_pDescriptorHeap->m_d3dSrvCPUDescriptorStartHandle;
	m_pDescriptorHeap->m_d3dSrvGPUDescriptorNextHandle = m_pDescriptorHeap->m_d3dSrvGPUDescriptorStartHandle;
}

void CResourceManager::CreateConstantBufferViews(ID3D12Device* pd3dDevice, int nConstantBufferViews, ID3D12Resource* pd3dConstantBuffers, UINT nStride)
{
	D3D12_GPU_VIRTUAL_ADDRESS d3dGpuVirtualAddress = pd3dConstantBuffers->GetGPUVirtualAddress();
	D3D12_CONSTANT_BUFFER_VIEW_DESC d3dCBVDesc;
	d3dCBVDesc.SizeInBytes = nStride;
	for (int j = 0; j < nConstantBufferViews; j++)
	{
		d3dCBVDesc.BufferLocation = d3dGpuVirtualAddress + (nStride * j);
		pd3dDevice->CreateConstantBufferView(&d3dCBVDesc, m_pDescriptorHeap->m_d3dCbvCPUDescriptorNextHandle);
		m_pDescriptorHeap->m_d3dCbvCPUDescriptorNextHandle.ptr += ::gnCbvSrvDescriptorIncrementSize;
		m_pDescriptorHeap->m_d3dCbvGPUDescriptorNextHandle.ptr += ::gnCbvSrvDescriptorIncrementSize;
	}
}

D3D12_GPU_DESCRIPTOR_HANDLE CResourceManager::CreateConstantBufferView(ID3D12Device* pd3dDevice, ID3D12Resource* pd3dConstantBuffer, UINT nStride)
{
	D3D12_CONSTANT_BUFFER_VIEW_DESC d3dCBVDesc;
	d3dCBVDesc.SizeInBytes = nStride;
	d3dCBVDesc.BufferLocation = pd3dConstantBuffer->GetGPUVirtualAddress();
	pd3dDevice->CreateConstantBufferView(&d3dCBVDesc, m_pDescriptorHeap->m_d3dCbvCPUDescriptorNextHandle);
	D3D12_GPU_DESCRIPTOR_HANDLE d3dCbvGPUDescriptorHandle = m_pDescriptorHeap->m_d3dCbvGPUDescriptorNextHandle;
	m_pDescriptorHeap->m_d3dCbvCPUDescriptorNextHandle.ptr += ::gnCbvSrvDescriptorIncrementSize;
	m_pDescriptorHeap->m_d3dCbvGPUDescriptorNextHandle.ptr += ::gnCbvSrvDescriptorIncrementSize;

	return(d3dCbvGPUDescriptorHandle);
}

D3D12_GPU_DESCRIPTOR_HANDLE CResourceManager::CreateConstantBufferView(ID3D12Device* pd3dDevice, D3D12_GPU_VIRTUAL_ADDRESS d3dGpuVirtualAddress, UINT nStride)
{
	D3D12_CONSTANT_BUFFER_VIEW_DESC d3dCBVDesc;
	d3dCBVDesc.SizeInBytes = nStride;
	d3dCBVDesc.BufferLocation = d3dGpuVirtualAddress;
	pd3dDevice->CreateConstantBufferView(&d3dCBVDesc, m_pDescriptorHeap->m_d3dCbvCPUDescriptorNextHandle);
	D3D12_GPU_DESCRIPTOR_HANDLE d3dCbvGPUDescriptorHandle = m_pDescriptorHeap->m_d3dCbvGPUDescriptorNextHandle;
	m_pDescriptorHeap->m_d3dCbvCPUDescriptorNextHandle.ptr += ::gnCbvSrvDescriptorIncrementSize;
	m_pDescriptorHeap->m_d3dCbvGPUDescriptorNextHandle.ptr += ::gnCbvSrvDescriptorIncrementSize;

	return(d3dCbvGPUDescriptorHandle);
}

void CResourceManager::CreateShaderResourceViews(ID3D12Device* pd3dDevice, CTexture* pTexture, UINT nDescriptorHeapIndex, UINT nRootParameterStartIndex)
{
	m_pDescriptorHeap->m_d3dSrvCPUDescriptorNextHandle.ptr += (::gnCbvSrvDescriptorIncrementSize * nDescriptorHeapIndex);
	m_pDescriptorHeap->m_d3dSrvGPUDescriptorNextHandle.ptr += (::gnCbvSrvDescriptorIncrementSize * nDescriptorHeapIndex);

	int nTextures = pTexture->GetTextures();
	for (int i = 0; i < nTextures; i++)
	{
		ID3D12Resource* pShaderResource = pTexture->GetResource(i);
		D3D12_SHADER_RESOURCE_VIEW_DESC d3dShaderResourceViewDesc = pTexture->GetShaderResourceViewDesc(i);
		pd3dDevice->CreateShaderResourceView(pShaderResource, &d3dShaderResourceViewDesc, m_pDescriptorHeap->m_d3dSrvCPUDescriptorNextHandle);
		m_pDescriptorHeap->m_d3dSrvCPUDescriptorNextHandle.ptr += ::gnCbvSrvDescriptorIncrementSize;

		pTexture->SetGpuDescriptorHandle(i, m_pDescriptorHeap->m_d3dSrvGPUDescriptorNextHandle);
		m_pDescriptorHeap->m_d3dSrvGPUDescriptorNextHandle.ptr += ::gnCbvSrvDescriptorIncrementSize;

	}
	int nRootParameters = pTexture->GetRootParameters();
	for (int i = 0; i < nRootParameters; i++) pTexture->SetRootParameterIndex(i, nRootParameterStartIndex + i);
}

void CResourceManager::CreateShaderResourceView(ID3D12Device* pd3dDevice, CTexture* pTexture, int nIndex, UINT nRootParameterStartIndex)
{
	ID3D12Resource* pShaderResource = pTexture->GetResource(nIndex);
	D3D12_GPU_DESCRIPTOR_HANDLE d3dGpuDescriptorHandle = pTexture->GetGpuDescriptorHandle(nIndex);
	if (pShaderResource && !d3dGpuDescriptorHandle.ptr)
	{
		D3D12_SHADER_RESOURCE_VIEW_DESC d3dShaderResourceViewDesc = pTexture->GetShaderResourceViewDesc(nIndex);
		pd3dDevice->CreateShaderResourceView(pShaderResource, &d3dShaderResourceViewDesc, m_pDescriptorHeap->m_d3dSrvCPUDescriptorNextHandle);
		m_pDescriptorHeap->m_d3dSrvCPUDescriptorNextHandle.ptr += ::gnCbvSrvDescriptorIncrementSize;

		pTexture->SetGpuDescriptorHandle(nIndex, m_pDescriptorHeap->m_d3dSrvGPUDescriptorNextHandle);
		m_pDescriptorHeap->m_d3dSrvGPUDescriptorNextHandle.ptr += ::gnCbvSrvDescriptorIncrementSize;

		pTexture->SetRootParameterIndex(nIndex, nRootParameterStartIndex + nIndex);
	}

}

void CResourceManager::CreateShaderResourceView(ID3D12Device* pd3dDevice, CTexture* pTexture, int nIndex)
{
	static int calledcount = 0;
	++calledcount;

	ID3D12Resource* pShaderResource = pTexture->GetResource(nIndex);
	D3D12_GPU_DESCRIPTOR_HANDLE d3dGpuDescriptorHandle = pTexture->GetGpuDescriptorHandle(nIndex);
	if (pShaderResource && !d3dGpuDescriptorHandle.ptr)
	{
		D3D12_SHADER_RESOURCE_VIEW_DESC d3dShaderResourceViewDesc = pTexture->GetShaderResourceViewDesc(nIndex);
		pd3dDevice->CreateShaderResourceView(pShaderResource, &d3dShaderResourceViewDesc, m_pDescriptorHeap->m_d3dSrvCPUDescriptorNextHandle);
		m_pDescriptorHeap->m_d3dSrvCPUDescriptorNextHandle.ptr += ::gnCbvSrvDescriptorIncrementSize;

		pTexture->SetGpuDescriptorHandle(nIndex, m_pDescriptorHeap->m_d3dSrvGPUDescriptorNextHandle);
		m_pDescriptorHeap->m_d3dSrvGPUDescriptorNextHandle.ptr += ::gnCbvSrvDescriptorIncrementSize;
	}
}

void CResourceManager::PrepareRender(ID3D12GraphicsCommandList* pd3dCommnadList)
{
	ID3D12DescriptorHeap* ppHeaps[] = { m_pDescriptorHeap->m_pd3dCbvSrvDescriptorHeap.Get() };
	pd3dCommnadList->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);
}

// ----------------------------------------
// 텍스쳐 정보를 저장
// ----------------------------------------
void CResourceManager::SetTexture(const std::wstring& path, std::shared_ptr<CTexture> texture) {
	if (texture == nullptr) return;
	TextureInfos[path] = texture;
}

void CResourceManager::SetTexture(const std::wstring& path, TextureInfo texture)
{
	if (texture.texture == nullptr) return;
	Textures[path] = texture;
}

std::shared_ptr<CTexture> CResourceManager::LoadOrCreateTexture(const std::wstring& path) {
	if (TextureInfos.find(path) != TextureInfos.end()) {
		// 이미 로드된 모델이 있는 경우
		return TextureInfos[path];
	}
	return nullptr;
}

void CResourceManager::LoadModelList(std::string filepath) {
	std::ifstream file(filepath);
	std::string modelname;
	while (file >> modelname) {
		GetModelInfo(modelname);
	}
}

void CResourceManager::LoadModelList(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommnadList, std::string filepath) {
	std::ifstream file(filepath);
	std::string modelname;
	while (file >> modelname) {
		GetModelInfo(pd3dDevice, pd3dCommnadList, modelname);
	}
}

void CResourceManager::SetSkinInfo(const std::string& name, std::shared_ptr<CLoadedModelInfo> modelInfo) {
	ModelInfos[name] = modelInfo;
}

CLoadedModelInfo* CResourceManager::GetModelInfo(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommnadList, const std::string& name)
{
	if (ModelInfos.find(name) != ModelInfos.end()) {
		// 이미 로드된 모델이 있는 경우
		if (ModelInfos[name]) return ModelInfos[name].get();
		else return nullptr;
	}

	ModelInfos[name] = nullptr;

	// 없는경우 바로 불러와서 저장하고 return 한다.
	std::string filepath = "Model/" + name + ".bin";

	auto pModelInfo = CGameObject::LoadGeometryAndAnimationFromFile(pd3dDevice, pd3dCommnadList, m_d3dGraphicRootSignature, filepath.c_str(), nullptr);
	if (pModelInfo) {
		SetSkinInfo(name, pModelInfo);
		std::string strDebug = "Success to load model: " + name + "\n";
		OutputDebugStringA(strDebug.c_str());
		return pModelInfo.get();
	}
	else {
		// 로드 실패
		std::string strDebug = "Failed to load model: " + name + "\n";
		OutputDebugStringA(strDebug.c_str());
	}

	return nullptr;
}

inline CLoadedModelInfo* CResourceManager::GetModelInfo(const std::string& name)
{
	// 이 함수는 이미 Model이 Load된 상태에서만 호출되어야 한다.
	CLoadedModelInfo* ret = nullptr;
	auto uploadcontext = CUploadContext::Instance();
	ret = GetModelInfo(uploadcontext.m_pd3dDevice, uploadcontext.m_pd3dGraphicCommandList, name);
	return ret;
}

// ----------------------------------------
// 메쉬 정보를 저장
// ----------------------------------------
void CResourceManager::SetMesh(const std::string& name, std::shared_ptr<CMesh> pMesh) {
	MeshInfos[name] = pMesh;
}

std::shared_ptr<CMesh> CResourceManager::GetMesh(const std::string& name) {
	if (MeshInfos.find(name) != MeshInfos.end()) {
		return MeshInfos[name];
	}
	return nullptr;
}

void CResourceManager::RegisterGameObjectResources(CGameObject* pGameObject)
{
	std::lock_guard<std::mutex> lock(m_RegisterGameObjectMutex);
	m_GameObjectResourceRegisterList.push_back(pGameObject);
	{
		std::string debugname = "Registered GameObject for Resource Collection: " + pGameObject->GetName() + "\n";
		OutputDebugStringA(debugname.c_str());
	}
}

void CResourceManager::CollectGameObjectRequest(int maxcount)
{
	std::lock_guard<std::mutex> lock(m_RegisterGameObjectMutex);
	int count = 0;
	while (!m_GameObjectResourceRegisterList.empty() && count < maxcount)
	{
		CGameObject* pGameObject = m_GameObjectResourceRegisterList.back();
		m_GameObjectResourceRegisterList.pop_back();

		pGameObject->CollectShaderVariables();
		count++;
	}

	m_GameObjectResourceRegisterList.clear();
}

// ----------------------------------------
// 셰이더 정보를 저장
// ----------------------------------------

// ----------------------------------------
// Mesh Upload 처리
// ----------------------------------------
void CResourceManager::RegisterMeshUpload(CMesh* pMesh)
{
	// 등록 갯수 증가
	m_nRegisterMeshCount.fetch_add(1);

	std::lock_guard<std::mutex> lock(m_UploadMutex);
	m_MeshRegisterBuffer.push_back(pMesh);
}

void CResourceManager::CollectMeshRegister(int maxcount)
{
	{
		int count{};
		while (false == m_MeshRegisterBuffer.empty() && count < maxcount)
		{
			m_MeshUploadList.push_back(m_MeshRegisterBuffer.front());
			m_MeshRegisterBuffer.pop_front();
			++count;
		}

		m_nUploadMeshCount.fetch_add(count);
	}
}

void CResourceManager::ProcessMeshUpload(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList)
{
	for (auto pMesh : m_MeshUploadList)
	{
		{
			std::string debugname = "Processing Mesh Upload: " + pMesh->GetName() + "\n";
			OutputDebugStringA(debugname.c_str());
		}

		pMesh->CreateShaderVariables(pd3dDevice, pd3dCommandList);
	}
}

void CResourceManager::ReleaseMeshUploadBuffers()
{
	// Release Mesh Upload Buffers
	for (auto& pMesh : m_MeshUploadList)
	{
		pMesh->ReleaseUploadBuffers();
	}
	m_MeshUploadList.clear();
}

// ----------------------------------------
// Material Upload 처리
// ----------------------------------------
void CResourceManager::RegisterMaterialUpload(CMaterial* pMaterial)
{
	// 등록 갯수 증가
	m_nRegisterMaterialCount.fetch_add(1);

	std::lock_guard<std::mutex> lock(m_UploadMutex);
	m_MaterialRegisterBuffer.push_back(pMaterial);
}

void CResourceManager::CollectMaterialRegister(int maxcount)
{
	{
		int count{};
		while (false == m_MaterialRegisterBuffer.empty() && count < maxcount)
		{
			m_MaterialUploadList.push_back(m_MaterialRegisterBuffer.front());
			m_MaterialRegisterBuffer.pop_front();
			++count;
		}
		m_nUploadMaterialCount.fetch_add(count);
	}
}

void CResourceManager::ProcessMaterialUpload(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList)
{
	for (auto& pMaterial : m_MaterialUploadList)
	{
		{
			std::string debugname = "Processing Material Upload: " + pMaterial->GetName() + "\n";
			OutputDebugStringA(debugname.c_str());
		}
		pMaterial->CreateShaderVariables(pd3dDevice, pd3dCommandList);
	}
}

void CResourceManager::ReleaseMaterialUploadBuffers()
{
	// Release Material Upload Buffers
	for (auto pMaterial : m_MaterialUploadList)
	{
		pMaterial->ReleaseUploadBuffers();
	}
	m_MaterialUploadList.clear();
}

// ----------------------------------------
// Texture Upload 처리
// ----------------------------------------
void CResourceManager::RegisterTextureUpload(CTexture* pTexture)
{
	// 등록 갯수 증가
	m_nRegisterTextureCount.fetch_add(1);

	std::lock_guard<std::mutex> lock(m_UploadMutex);
	m_TextureRegisterBuffer.push_back(pTexture);
}

void CResourceManager::CollectTextureRegister(int maxcount)
{
	{
		int count{};
		while (false == m_TextureRegisterBuffer.empty() && count < maxcount)
		{
			m_TextureUploadList.push_back(m_TextureRegisterBuffer.front());
			m_TextureRegisterBuffer.pop_front();
			++count;
		}
		m_nUploadTextureCount.fetch_add(count);
	}
}

void CResourceManager::ProcessTextureUpload(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList)
{
	for (auto& pTexture : m_TextureUploadList)
	{
		/*{
			std::string debugname = "Processing Texture Upload: " + pTexture->GetName() + "\n";
			OutputDebugStringA(debugname.c_str());
		}*/
		pTexture->CreateShaderVariables(pd3dDevice, pd3dCommandList);
	}
}

void CResourceManager::ReleaseTextureUploadBuffers()
{
	// Release Texture Upload Buffers
	for (auto pTexture : m_TextureUploadList)
	{
		pTexture->ReleaseUploadBuffers();
	}
	m_TextureUploadList.clear();
}

// ----------------------------------------
// Shader Upload 처리
// ----------------------------------------
void CResourceManager::RegisterShaderUpload(CShader* pShader)
{
	std::lock_guard<std::mutex> lock(m_UploadMutex);
	m_ShaderRegisterBuffer.push_back(pShader);
}

void CResourceManager::CollectShaderRegister(int maxcount)
{
	{
		int count{};
		while (false == m_ShaderRegisterBuffer.empty() && count < maxcount)
		{
			m_ShaderToCreateList.push_back(m_ShaderRegisterBuffer.front());
			m_ShaderRegisterBuffer.pop_front();
			++count;
		}
	}
}

void CResourceManager::ProcessShaderCreate(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList)
{
	for (auto& pShader : m_ShaderToCreateList)
	{
		{
			std::string debugname = "Processing Shader Upload: " + to_string(pShader->GetShaderName()) + "\n";
			OutputDebugStringA(debugname.c_str());
		}
		pShader->CreateShader(pd3dDevice, m_d3dGraphicRootSignature);
	}

	m_ShaderToCreateList.clear();
}

// ----------------------------------------
// Create Shader Variables For Camera
// ----------------------------------------

void CResourceManager::RegisterCamera(CCamera* pCamera)
{
	std::lock_guard<std::mutex> lock(m_UploadMutex);
	m_CameraRegisterBuffer.push_back(pCamera);
}


void CResourceManager::CollectCameraRegister(int maxcount)
{
	{
		int count{};
		while (false == m_CameraRegisterBuffer.empty() && count < maxcount)
		{
			m_CameraToCreateList.push_back(m_CameraRegisterBuffer.front());
			m_CameraRegisterBuffer.pop_front();
			++count;
		}
	}
}

void CResourceManager::ProcessCameraCreate(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList)
{
	for (auto& pCamera : m_CameraToCreateList)
	{
		pCamera->CreateShaderVariables(pd3dDevice, pd3dCommandList);
	}

	m_ShaderToCreateList.clear();
}


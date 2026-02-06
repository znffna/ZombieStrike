#pragma once
#include "GameObject.h"

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
struct FIRE_INFO {
	XMFLOAT3 xmf3Position;
	XMFLOAT3 xmf3Look;
	XMFLOAT3 xmf3MuzzlePosition; // 총구 위치(렌더링 파티클 점 생성시 사용)
	int nBulletType = 0; // 총알 타입(0: 일반, 1: 산탄총 등)
	float fRange = 0.0f;
	float fspeed = 0.0f; // 총알 속도
};

class CBulletParticleObject : public CGameObject
{
	// TODO : Bullet을 전부 관리하는 Object로 변경할 예정
	// 현황 : GPU상에서 모든 Bullet을 파티클처럼 관리 하는 중(즉, 생성만 직접하고 소멸은 GPU에서 SO를 통해 출력시 discard하는 방식)
	// 목표 : 사격 즉시 피격위치 확정 및 GPU에 파티클 출력.
	//      : 이떄 총알은 GPU상에서 전진되며, GPU에 파티클 생성시에 주어진 거리 비례 LifeTime을 소유.
	//      : 즉, 총알이 날아가는 듯한 느낌만 주기 위함이며, 실제 피격효과로 인한 출력은 HitResult에 의해
	//      : 별도 파티클 생성으로 이루어 진다.(즉, Trail과 혈흔 표현을 별도로 구현 예정)
public:
	CBulletParticleObject(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, ID3D12RootSignature* pd3dGraphicsRootSignature, XMFLOAT3 xmf3Position, XMFLOAT3 xmf3Look, float fLifetime, XMFLOAT3 xmf3Acceleration, XMFLOAT3 xmf3Color, XMFLOAT2 xmf2Size, UINT nMaxParticles);
	virtual ~CBulletParticleObject();

	virtual GAMEOBJECT_LAYER GetLayer() override { return GAMEOBJECT_LAYER::LAYER_BULLET; }

	virtual void Render(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera, bool bDepthWrite = false);
	virtual void OnPostRender();

	// method
	void AddBullet(const XMFLOAT3& pOrigin, const XMFLOAT3& xmf3Look, float fRange);
	void AddBullet(const CBulletVertex& pBulletVertex);

private:
	std::vector<FIRE_INFO> m_pFireInfos;

	std::shared_ptr<CTexture> m_pRandowmValueTexture;
	std::shared_ptr<CTexture> m_pRandowmValueOnSphereTexture;

public:
	void AddFireInfo(const FIRE_INFO& fireInfo) {
		m_pFireInfos.push_back(fireInfo);
	}

	std::vector<FIRE_INFO> GetFireInfos() const {
		return m_pFireInfos;
	}

	void UpdateBulletVertices(const std::vector<CBulletVertex>& pBulletVertices) {
		//std::dynamic_pointer_cast<CBulletMesh>(m_pMesh)->AddBullets(pBulletVertices);
	}

	void ClearFireInfos() {
		m_pFireInfos.clear();
	}

}; // CBulletParticleObject



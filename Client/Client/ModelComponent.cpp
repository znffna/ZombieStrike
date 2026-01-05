#include "ModelComponent.h"

#include "GameObject.h"
#include "AnimationController.h"

void CModelComponent::SetModel(CLoadedModelInfo* pModel)
{
	m_pModel = pModel; 

	// Prepare Skinning 
	if (auto panimationController = gameObject->GetComponent<CAnimationController>())
	{
		panimationController->SettingByModel(m_pModel);
	}
}

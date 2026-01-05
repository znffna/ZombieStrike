#include "TextComponent.h"

#include "GameObject.h"
#include "Scene.h"

void CTextComponent::Initialize()
{
	// textblock의 등록
	if (auto pScene = gameObject->GetScene()) pScene->RegisterText(&m_TextBlock);
}

void CTextComponent::OnDestroy()
{
	// textblock의 등록 해제
	if(auto pScene = gameObject->GetScene()) pScene->UnregisterText(&m_TextBlock);
}

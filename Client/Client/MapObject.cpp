#include "MapObject.h"

////////////////////////////////////////////////////////////////////////////////
//

void CMapObject::Initialize(std::wstring wstrMapFilePath)
{
	LoadGeometryAndAnimationFromFile(wstrMapFilePath);

	CollectMeshBound();
}

void CMapObject::LoadGeometryAndAnimationFromFile(std::wstring wstrMapFilePath)
{
	// 파일 열기 (실패시 바로 종료)
	std::ifstream pInFile(wstrMapFilePath, std::ios::binary);
	if (!pInFile.is_open()) return;

	SetName(to_string(wstrMapFilePath));

	char pstrToken[500] = { '\0' };

	int pnSkinnedMeshes = 0;

	for (; ; )
	{
		if (::ReadStringFromFile(pInFile, pstrToken))
		{
			if (!strcmp(pstrToken, "<Hierarchy>:"))
			{
				// ::ReadStringFromFile(pInFile, pstrToken); //"</Hierarchy>"
			}
			else if (!strcmp(pstrToken, "<Frame>:"))
			{
				int nFrame = ::ReadIntegerFromFile(pInFile);
				int nTextures = ::ReadIntegerFromFile(pInFile);

				std::string strFrameName;
				::ReadStringFromFile(pInFile, strFrameName);
			}
			else if (!strcmp(pstrToken, "<Tag>:")) {

				std::string strTag;
				::ReadStringFromFile(pInFile, strTag);
			}
			else if (!strcmp(pstrToken, "<Transform>:"))
			{
				XMFLOAT3 xmf3Position, xmf3Rotation, xmf3Scale;
				XMFLOAT4 xmf4Rotation;
				pInFile.read((char*)&xmf3Position, sizeof(float) * 3);
				pInFile.read((char*)&xmf3Rotation, sizeof(float) * 3); //Euler Angle
				pInFile.read((char*)&xmf3Scale, sizeof(float) * 3);
				pInFile.read((char*)&xmf4Rotation, sizeof(float) * 4); //Quaternion
			}
			else if (!strcmp(pstrToken, "<TransformMatrix>:"))
			{
				XMFLOAT4X4 xmf4x4Matrix;
				pInFile.read((char*)&xmf4x4Matrix, sizeof(float) * 16);
				SetLocalMatrix(xmf4x4Matrix);
			}
			else if (!strcmp(pstrToken, "<Children>:"))
			{
				int nChilds = ::ReadIntegerFromFile(pInFile);
				if (nChilds > 0)
				{
					m_pChilds.reserve(nChilds);
					for (int i = 0; i < nChilds; i++)
					{
						auto pChild = CGameObject::LoadFrameHierarchyFromFile(this, pInFile, nullptr, &pnSkinnedMeshes, 0);
						if (pChild) SetChild(std::move(pChild));

#ifdef _WITH_DEBUG_FRAME_HIERARCHY
						std::string strDebug = "(Frame: " + pChild->GetName() + ") (Parent: " + pGameObject->GetName() + ")\n";
						for (auto& pComponent : pChild->m_pComponents)
						{
							std::string name = typeid(*pComponent.get()).name();
							strDebug += "\tComponent: " + name + "\n";
						}
						OutputDebugStringA(strDebug.c_str());
#endif
					}
				}
			}
			else if (!strcmp(pstrToken, "<ModelName>"))
			{
				std::string strModelName;
				::ReadStringFromFile(pInFile, strModelName);

				DeepCopyFromModel(strModelName, this);
			}
			else if (!strcmp(pstrToken, "</Hierarchy>"))
			{
				break;
			}
		}
		else
		{
			break;
		}
	}
}

void CMapObject::CollectMeshBound()
{

}

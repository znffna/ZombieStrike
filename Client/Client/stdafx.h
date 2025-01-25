///////////////////////////////////////////////////////////////////
// Date: 2024-12-28
// stdafx.h : �̸� �����ϵ� ��� �����Դϴ�. �������� ����� ���⿡ �����մϴ�.
// Version : 0.1
///////////////////////////////////////////////////////////////////

#pragma once

#include "targetver.h"
#define WIN32_LEAN_AND_MEAN             // ���� ������ �ʴ� ������ Windows ������� �����մϴ�.

// Windows ��� ����
#include <windows.h>
// C ��Ÿ�� ��� �����Դϴ�.
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <tchar.h>

// C++ ��Ÿ�� ��� ����
#include <iostream>
#include <fstream>

// C++ STL Containers
#include <array>
#include <vector>
#include <list>
#include <map>
#include <set>
#include <queue>
#include <stack>
#include <deque>
#include <unordered_map>
#include <unordered_set>
#include <string>


// C++ STL Algorithms
#include <algorithm>

// DirectX 12
#include <d3d12.h>
#include <dxgi1_4.h>
#include <D3Dcompiler.h>
#include <DirectXMath.h>
#include <DirectXColors.h>
#include <DirectXCollision.h>
#include <DirectXPackedVector.h>
#include <wrl.h>
#include <shellapi.h>

#ifdef _DEBUG
#include <dxgidebug.h>
#endif

using namespace DirectX;
using namespace DirectX::PackedVector;
using Microsoft::WRL::ComPtr;

#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")

#pragma comment(lib, "dxguid.lib")

// DirectXTK
//#include <WICTextureLoader.h>
//#include <DDSTextureLoader.h>


// Functions

extern UINT gnCbvSrvDescriptorIncrementSize;
extern UINT	gnRtvDescriptorIncrementSize;
extern UINT gnDsvDescriptorIncrementSize;

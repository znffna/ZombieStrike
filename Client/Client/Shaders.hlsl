///////////////////////////////////////////////////////////////////////////////
// Date: 2025-02-02
// Shaders.hlsl : Shader 정의 파일
// Version : 0.1
///////////////////////////////////////////////////////////////////////////////
struct MATERIAL
{
    float4 m_cAmbient;
    float4 m_cDiffuse;
    float4 m_cSpecular; //a = power
    float4 m_cEmissive;
};

#define _USE_OBJECT_MATERIAL_CBV

cbuffer cbGameObjectInfo : register(b0)
{
    matrix gmtxGameObject : packoffset(c0);
};

cbuffer cbMaterialInfo : register(b1)
{
    MATERIAL gMaterial : packoffset(c0);
    uint gnTexturesMask : packoffset(c4.x);
};

cbuffer cbCameraInfo : register(b2)
{
    matrix gmtxView : packoffset(c0);
    matrix gmtxInvView : packoffset(c4);
    matrix gmtxProjection : packoffset(c8);
    matrix gmtxInvProjection : packoffset(c12);
};

cbuffer cbFrameworkInfo : register(b3)
{
    float gfCurrentTime : packoffset(c0.x);
    float gfElapsedTime : packoffset(c0.y);
    uint gnRenderMode : packoffset(c0.z);
};

#include "Light.hlsl"

// Render Config
#include "RenderConfig.h"

////////////////////////////////////////////////////////////////////////////////
//

#define MATERIAL_ALBEDO_MAP			0x01
#define MATERIAL_SPECULAR_MAP		0x02
#define MATERIAL_NORMAL_MAP			0x04
#define MATERIAL_METALLIC_MAP		0x08
#define MATERIAL_EMISSION_MAP		0x10
#define MATERIAL_DETAIL_ALBEDO_MAP	0x20
#define MATERIAL_DETAIL_NORMAL_MAP	0x40

#ifdef _WITH_STANDARD_TEXTURE_MULTIPLE_PARAMETERS
Texture2D gtxtAlbedoTexture : register(t6);
Texture2D gtxtSpecularTexture : register(t7);
Texture2D gtxtNormalTexture : register(t8);
Texture2D gtxtMetallicTexture : register(t9);
Texture2D gtxtEmissionTexture : register(t10);
Texture2D gtxtDetailAlbedoTexture : register(t11);
Texture2D gtxtDetailNormalTexture : register(t12);
#else
Texture2D gtxtStandardTextures[7] : register(t6); // t6 ~ t12 : Albedo, Specular, Normal, Metallic, Emission, Detail Albedo, Detail Normal
#endif

TextureCube gtxtSkyCubeTexture : register(t13);

SamplerState gssWrap : register(s0);

////////////////////////////////////////////////////////////////////////////////
//

struct VS_STANDARD_INPUT
{
    float3 position : POSITION;
    float2 uv : TEXCOORD;
    float3 normal : NORMAL;
    float3 tangent : TANGENT;
    float3 bitangent : BITANGENT;
};

struct VS_STANDARD_OUTPUT
{
    float4 position : SV_POSITION;
    float3 positionW : POSITION;
    float3 normalW : NORMAL;
    float3 tangentW : TANGENT;
    float3 bitangentW : BITANGENT;
    float2 uv : TEXCOORD;
};

VS_STANDARD_OUTPUT VSStandard(VS_STANDARD_INPUT input)
{
    VS_STANDARD_OUTPUT output;

    output.positionW = mul(float4(input.position, 1.0f), gmtxGameObject).xyz;
    output.normalW = mul(input.normal, (float3x3) gmtxGameObject);
    output.tangentW = mul(input.tangent, (float3x3) gmtxGameObject);
    output.bitangentW = mul(input.bitangent, (float3x3) gmtxGameObject);
    output.position = mul(mul(float4(output.positionW, 1.0f), gmtxView), gmtxProjection);
    output.uv = input.uv;

    return (output);
}

float4 PSStandard(VS_STANDARD_OUTPUT input) : SV_TARGET
{    
    float4 cAlbedoColor = float4(1.0f, 0.0f, 0.0f, 1.0f);
    float4 cSpecularColor = float4(0.0f, 0.0f, 0.0f, 1.0f);
    float4 cNormalColor = float4(0.0f, 0.0f, 0.0f, 1.0f);
    float4 cMetallicColor = float4(0.0f, 0.0f, 0.0f, 1.0f);
    float4 cEmissionColor = float4(0.0f, 0.0f, 0.0f, 1.0f);
    
#ifdef _WITH_STANDARD_TEXTURE_MULTIPLE_PARAMETERS
    if (gnTexturesMask & MATERIAL_ALBEDO_MAP) cAlbedoColor = gtxtAlbedoTexture.Sample(gssWrap, input.uv);
    if (gnTexturesMask & MATERIAL_SPECULAR_MAP) cSpecularColor = gtxtSpecularTexture.Sample(gssWrap, input.uv);
    if (gnTexturesMask & MATERIAL_NORMAL_MAP) cNormalColor = gtxtNormalTexture.Sample(gssWrap, input.uv);
    if (gnTexturesMask & MATERIAL_METALLIC_MAP) cMetallicColor = gtxtMetallicTexture.Sample(gssWrap, input.uv);
    if (gnTexturesMask & MATERIAL_EMISSION_MAP) cEmissionColor = gtxtEmissionTexture.Sample(gssWrap, input.uv);
#else
    if (gnTexturesMask & MATERIAL_ALBEDO_MAP) cAlbedoColor = gtxtStandardTextures[0].Sample(gssWrap, input.uv);
    if (gnTexturesMask & MATERIAL_SPECULAR_MAP) cSpecularColor = gtxtStandardTextures[1].Sample(gssWrap, input.uv);
    if (gnTexturesMask & MATERIAL_NORMAL_MAP) cNormalColor = gtxtStandardTextures[2].Sample(gssWrap, input.uv);
    if (gnTexturesMask & MATERIAL_METALLIC_MAP) cMetallicColor = gtxtStandardTextures[3].Sample(gssWrap, input.uv);
    if (gnTexturesMask & MATERIAL_EMISSION_MAP) cEmissionColor = gtxtStandardTextures[4].Sample(gssWrap, input.uv);
#endif
    
    float4 cIllumination = float4(1.0f, 1.0f, 1.0f, 1.0f);
    float4 cColor = cAlbedoColor + cSpecularColor + cEmissionColor;
//    float4 cColor = cAlbedoColor;
    float3 normalW = input.normalW;
    
    if (gnTexturesMask & MATERIAL_NORMAL_MAP)
    {
        float3x3 TBN = float3x3(normalize(input.tangentW), normalize(input.bitangentW), normalize(input.normalW));
        float3 vNormal = normalize(cNormalColor.rgb * 2.0f - 1.0f); //[0, 1] → [-1, 1]
        normalW = normalize(mul(vNormal, TBN));
    }
    cIllumination = Lighting(input.positionW, normalW);
  
//    cColor = cColor + cIllumination;
//    cColor = cColor * cIllumination;
//    cColor = cColor / (1 + cColor);
//    cColor = lerp(cColor, cIllumination, 0.5f);
    
        return (cColor);
    }

////////////////////////////////////////////////////////////////////////////////
//

struct VS_SKYBOX_INPUT
{
    float3 position : POSITION;
};

struct VS_SKYBOX_OUTPUT
{
    float4 position : SV_POSITION;
    float3 uv : TEXCOORD;
};

VS_SKYBOX_OUTPUT VSSkyBox(VS_SKYBOX_INPUT input)
{
    VS_SKYBOX_OUTPUT output;

    output.position = mul(mul(float4(input.position, 1.0f), gmtxGameObject), gmtxView).xyzw;
    output.uv = input.position;
    output.position = mul(output.position, gmtxProjection).xyww;

    return (output);
}

float4 PSSkyBox(VS_SKYBOX_OUTPUT input) : SV_TARGET
{
    return (gtxtSkyCubeTexture.Sample(gssWrap, input.uv));
}

////////////////////////////////////////////////////////////////////////////////
//

//정점 셰이더의 입력을 위한 구조체를 선언한다.
struct VS_TERRAIN_INPUT
{
    float3 position : POSITION;
    float4 color : COLOR;
    float3 normal : NORMAL;
    float2 uv0 : TEXCOORD0;
    float2 uv1 : TEXCOORD1;
};

//정점 셰이더의 출력(픽셀 셰이더의 입력)을 위한 구조체를 선언한다.
struct VS_TERRAIN_OUTPUT
{
    float4 position : SV_POSITION;
    float4 color : COLOR;
    float3 positionW : POSITION;
    float3 normalW : NORMAL;
    float2 uv0 : TEXCOORD0;
    float2 uv1 : TEXCOORD1;
};

//정점 셰이더를 정의한다.
VS_TERRAIN_OUTPUT VSTerrain(VS_TERRAIN_INPUT input)
{
    VS_TERRAIN_OUTPUT output;
	
    output.position = mul(mul(mul(float4(input.position, 1.0f), gmtxGameObject), gmtxView), gmtxProjection);
    output.color = input.color;
    output.positionW = mul(float4(input.position, 1.0f), gmtxGameObject).xyz;
    output.normalW = mul(input.normal, (float3x3) gmtxGameObject);
    //output.position = float4(input.position, 1.0f);
    output.uv0 = input.uv0;
    output.uv1 = input.uv1;
    
    return (output);
}


//픽셀 셰이더를 정의한다.
float4 PSTerrain(VS_TERRAIN_OUTPUT input) : SV_TARGET
{
    float4 baseColor = input.color;
    
#ifdef _WITH_STANDARD_TEXTURE_MULTIPLE_PARAMETERS
    float4 texColor = gtxtAlbedoTexture.Sample(gssWrap, input.uv0);
    float4 detailTexColor = gtxtSpecularTexture.Sample(gssWrap, input.uv1);
#else
    float4 texColor = gtxtStandardTextures[0].Sample(gssWrap, input.uv0);
    float4 detailTexColor = gtxtStandardTextures[1].Sample(gssWrap, input.uv1);
#endif
    
    float4 cIllumination = Lighting(input.positionW, input.normalW);
    //float4 cColor = texColor * 0.5f + cIllumination * 0.5f;
    float4 cColor = (baseColor * 0.4f + texColor * 0.4f + detailTexColor * 0.2f);
    return lerp(cColor, cIllumination, 0.7f);
    
	
    //return (baseColor);
}

////////////////////////////////////////////////////////////////////////////////
//

#define MAX_VERTEX_INFLUENCES			4
#define SKINNED_ANIMATION_BONES			256

cbuffer cbBoneOffsets : register(b7)
{
    float4x4 gpmtxBoneOffsets[SKINNED_ANIMATION_BONES];
};

cbuffer cbBoneTransforms : register(b8)
{
    float4x4 gpmtxBoneTransforms[SKINNED_ANIMATION_BONES];
};

struct VS_SKINNED_STANDARD_INPUT
{
    float3 position : POSITION;
    float2 uv : TEXCOORD;
    float3 normal : NORMAL;
    float3 tangent : TANGENT;
    float3 bitangent : BITANGENT;
    int4 indices : BONEINDEX;
    float4 weights : BONEWEIGHT;
};

VS_STANDARD_OUTPUT VSSkinnedAnimationStandard(VS_SKINNED_STANDARD_INPUT input)
{
    VS_STANDARD_OUTPUT output;

	//output.positionW = float3(0.0f, 0.0f, 0.0f);
	//output.normalW = float3(0.0f, 0.0f, 0.0f);
	//output.tangentW = float3(0.0f, 0.0f, 0.0f);
	//output.bitangentW = float3(0.0f, 0.0f, 0.0f);
	//matrix mtxVertexToBoneWorld;
	//for (int i = 0; i < MAX_VERTEX_INFLUENCES; i++)
	//{
	//	mtxVertexToBoneWorld = mul(gpmtxBoneOffsets[input.indices[i]], gpmtxBoneTransforms[input.indices[i]]);
	//	output.positionW += input.weights[i] * mul(float4(input.position, 1.0f), mtxVertexToBoneWorld).xyz;
	//	output.normalW += input.weights[i] * mul(input.normal, (float3x3)mtxVertexToBoneWorld);
	//	output.tangentW += input.weights[i] * mul(input.tangent, (float3x3)mtxVertexToBoneWorld);
	//	output.bitangentW += input.weights[i] * mul(input.bitangent, (float3x3)mtxVertexToBoneWorld);
	//}
    float4x4 mtxVertexToBoneWorld = (float4x4) 0.0f;
    for (int i = 0; i < MAX_VERTEX_INFLUENCES; i++)
    {
//		mtxVertexToBoneWorld += input.weights[i] * gpmtxBoneTransforms[input.indices[i]];
        mtxVertexToBoneWorld += input.weights[i] * mul(gpmtxBoneOffsets[input.indices[i]], gpmtxBoneTransforms[input.indices[i]]);
    }
    output.positionW = mul(float4(input.position, 1.0f), mtxVertexToBoneWorld).xyz;
    output.normalW = mul(input.normal, (float3x3) mtxVertexToBoneWorld).xyz;
    output.tangentW = mul(input.tangent, (float3x3) mtxVertexToBoneWorld).xyz;
    output.bitangentW = mul(input.bitangent, (float3x3) mtxVertexToBoneWorld).xyz;

//	output.positionW = mul(float4(input.position, 1.0f), gmtxGameObject).xyz;

    output.position = mul(mul(float4(output.positionW, 1.0f), gmtxView), gmtxProjection);
    output.uv = input.uv;

    return (output);
}

////////////////////////////////////////////////////////////////////////////////
//

struct VS_COLLIDER_INPUT
{
    float3 position : POSITION;
};

struct VS_COLLIDER_OUTPUT
{
    float4 position : SV_POSITION;
};

VS_COLLIDER_OUTPUT VSCollider(VS_COLLIDER_INPUT input)
{
    VS_COLLIDER_OUTPUT output;
    output.position = mul(mul(mul(float4(input.position, 1.0f), gmtxGameObject), gmtxView), gmtxProjection);
    return (output);
}

float4 PSCollider(VS_COLLIDER_OUTPUT input) : SV_TARGET
{
    float4 cColor = float4(0.0f, 1.0f, 0.0f, 1.0f);
    
    return cColor;
}
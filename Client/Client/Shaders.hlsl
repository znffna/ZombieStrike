///////////////////////////////////////////////////////////////////////////////
// Date: 2025-02-02
// Shaders.hlsl : Shader 정의 파일
// Version : 0.1
///////////////////////////////////////////////////////////////////////////////


#define _USE_OBJECT_MATERIAL_CBV

cbuffer cbGameObjectInfo : register(b0)
{
    matrix gmtxGameObject : packoffset(c0);
    float4 gf4ObjectColor : packoffset(c4);
    uint gnBoneOffset : packoffset(c5.x);
};

cbuffer cbCameraInfo : register(b2)
{
    matrix gmtxView : packoffset(c0);
    matrix gmtxInvView : packoffset(c4);
    matrix gmtxProjection : packoffset(c8);
    matrix gmtxInvProjection : packoffset(c12);
    float3 gCameraPosition : packoffset(c16);
    float padding : packoffset(c16.w);
};

cbuffer cbFrameworkInfo : register(b3)
{
    float gfCurrentTime : packoffset(c0.x);
    float gfElapsedTime : packoffset(c0.y);
    uint gnRenderMode : packoffset(c0.z);
    float gfBias : packoffset(c0.w);
    int gnShadowIndex : packoffset(c1.x);
};

#include "Light.hlsl"

// Render Config
#define _WITH_STANDARD_TEXTURE_MULTIPLE_PARAMETERS

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

Buffer<float4> gRandomBuffer : register(t30);
Buffer<float4> gRandomSphereBuffer : register(t31);

SamplerState gssWrap : register(s0);

////////////////////////////////////////////////////////////////////////////////
//
struct VS_TEXTURED_INPUT
{
    float3 position : POSITION;
    float2 uv : TEXCOORD;
};

struct VS_TEXTURED_OUTPUT
{
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD;
};

VS_TEXTURED_OUTPUT VSBillboard(VS_TEXTURED_INPUT input)
{
    VS_TEXTURED_OUTPUT output;

    output.position = mul(mul(mul(float4(input.position, 1.0f), gmtxGameObject), gmtxView), gmtxProjection);
    output.uv = input.uv;

    return (output);
}

float4 PSBillboard(VS_TEXTURED_OUTPUT input) : SV_TARGET
{
//	float4 cColor = gtxtTexture.SampleLevel(gWrapSamplerState, input.uv, 0);
    float4 cColor = gtxtAlbedoTexture.Sample(gssWrap, input.uv);

    return (cColor);
}

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
    float depthV : VIEWDEPTH;
};

VS_STANDARD_OUTPUT VSStandard(VS_STANDARD_INPUT input)
{
    VS_STANDARD_OUTPUT output;

    float4 positionW = mul(float4(input.position, 1.0f), gmtxGameObject);
    
    output.positionW = positionW.xyz;
    output.normalW = mul(input.normal, (float3x3) gmtxGameObject);
    output.tangentW = mul(input.tangent, (float3x3) gmtxGameObject);
    output.bitangentW = mul(input.bitangent, (float3x3) gmtxGameObject);
    output.position = mul(mul(float4(positionW), gmtxView), gmtxProjection);
    output.uv = input.uv;
    output.depthV = mul(float4(positionW), gmtxView).z;
    
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
    
    cIllumination = Lighting(input.positionW, normalW, input.depthV, true);
  
    return lerp(cColor, cColor * cIllumination, 0.7);
    //return (1.0 - 2.0 * cIllumination) * cColor * cColor + 2.0 * cIllumination * cColor;
    
    //return (cColor);
    //return (cIllumination);
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
    float3 positionW : POSITION;
    float3 normalW : NORMAL;
    float4 color : COLOR;
    float2 uv0 : TEXCOORD0;
    float2 uv1 : TEXCOORD1;
    float depthV : VIEWDEPTH;  
};

//정점 셰이더를 정의한다.
VS_TERRAIN_OUTPUT VSTerrain(VS_TERRAIN_INPUT input)
{
    VS_TERRAIN_OUTPUT output;
	
    float4 positionW = mul(float4(input.position, 1.0f), gmtxGameObject);
    output.positionW = positionW.xyz;
    output.position = mul(mul(mul(float4(input.position, 1.0f), gmtxGameObject), gmtxView), gmtxProjection);
    output.color = input.color;
    output.normalW = mul(input.normal, (float3x3) gmtxGameObject);
    //output.position = float4(input.position, 1.0f);
    output.uv0 = input.uv0;
    output.uv1 = input.uv1;
    output.depthV = mul(positionW, gmtxView).z;
    
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
    
    float4 cIllumination = Lighting(input.positionW, input.normalW, input.depthV, true);
    //float4 cColor = texColor * 0.5f + cIllumination * 0.5f;
    float4 cColor = (texColor * 0.8f + detailTexColor * 0.2f);
    return lerp(cColor, cIllumination, 0.5f);
    
	
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

//cbuffer cbBoneTransforms : register(b8)
//{
//    float4x4 gpmtxBoneTransforms[SKINNED_ANIMATION_BONES];
//};

StructuredBuffer<float4x4> gpmtxBoneTransforms : register(t100);

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
        mtxVertexToBoneWorld += input.weights[i] * mul(gpmtxBoneOffsets[input.indices[i]], gpmtxBoneTransforms[input.indices[i] + gnBoneOffset]);
    }
    
    float4 positionW = mul(float4(input.position, 1.0f), mtxVertexToBoneWorld);
    output.positionW = positionW.xyz;
    output.normalW = mul(input.normal, (float3x3) mtxVertexToBoneWorld).xyz;
    output.tangentW = mul(input.tangent, (float3x3) mtxVertexToBoneWorld).xyz;
    output.bitangentW = mul(input.bitangent, (float3x3) mtxVertexToBoneWorld).xyz;

//	output.positionW = mul(float4(input.position, 1.0f), gmtxGameObject).xyz;

    output.position = mul(mul(positionW, gmtxView), gmtxProjection);
    output.uv = input.uv;
    output.depthV = mul(positionW, gmtxView).z;

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

//////////////////////////////////////////////////////////////////////////////////
//

#define BULLET_MAINTAIN -1 // uint 가 type이기에 최대값을 가진다.
#define BULLET_TYPE_EMIT_ASSAULT 0
#define BULLET_TYPE_EMIT_SHOTGUN 1
#define BULLET_TYPE_TRAIL 2
#define BULLET_TYPE_MUZZLE_SPARK 3
#define BULLET_TYPE_FRAGMENT 4
#define BULLET_TYPE_BLOOD 5

#define TARGET_ENVIRONMENT 0
#define TARGET_ENEMY 1

float gfSparkLifetime = 0.2f; // 총구 스파크의 생명주기

float4 RandomDirection(float fOffset)
{
    int u = uint(gfCurrentTime + fOffset + frac(gfCurrentTime) * 1000.0f) % 1024;
    return (normalize(gRandomBuffer.Load(u)));
}

float4 RandomDirectionOnSphere(float fOffset)
{
    int u = uint(gfCurrentTime + fOffset + frac(gfCurrentTime) * 1000.0f) % 256;
    return (normalize(gRandomSphereBuffer.Load(u)));
}


struct VS_BULLET_INPUT
{
    float3 position : POSITION;
    float3 endposition : LASTPOSITION;
    float3 velocity : VELOCITY;
    float lifetime : LIFETIME;
    int type : TYPE;
    int target : TARGET; // 총알이 맞은 대상의 TYPE(적, 환경 등)
    uint id : SV_InstanceID; // 렌더링시에 사용되는 인스턴스 ID, 랜덤성처럼 보이기 위한 장치로 사용
};

VS_BULLET_INPUT VSBulletStreamOutput(VS_BULLET_INPUT input)
{
    return (input);
}

void GenerateSparkParticles(VS_BULLET_INPUT input, inout PointStream<VS_BULLET_INPUT> output)
{
    VS_BULLET_INPUT p = input;
    
    // 총구 스파크 중 4개를 생성
    p.type = BULLET_TYPE_MUZZLE_SPARK;
    p.lifetime = gfSparkLifetime;
    
    float3 fourDir[4];
    for (int i = 0; i < 4; i++)
    {
        p.velocity = cross(p.velocity, float3(0.0f, 0.0f, 1.0f)); // 스파크의 방향을 랜덤하게 설정
        fourDir[i] = p.velocity;
    }
    // 8방향 스파크 생성
    for (int k = 0; k < 4; k++)
    {
        p.velocity = fourDir[k];
        output.Append(p);
    }
    for (int j = 0; j < 4; j++)
    {
        p.velocity = fourDir[j] + fourDir[(j + 1) % 4];
        output.Append(p);
    }
    
    // 랜덤 스파크 생성
    for(int l= 0; l < 30; l++)
    {
        p.velocity = RandomDirectionOnSphere(input.id + l);
        output.Append(p);
    }
}

void GenerateImpactDustParticles(VS_BULLET_INPUT input, inout PointStream<VS_BULLET_INPUT> output)
{
    VS_BULLET_INPUT p = input;
    if (input.target == TARGET_ENVIRONMENT)
    {
        p.type = BULLET_TYPE_FRAGMENT;
    }
    else if (input.target == TARGET_ENEMY)
    {
        p.type = BULLET_TYPE_BLOOD;
    }
    else
        return; // 잘못된 대상 타입이면 아무것도 하지 않는다.
    
    p.position = p.endposition; // 충돌 위치로 설정
    // 랜덤 스파크 생성
    for (int l = 0; l < 30; l++)
    {
        p.velocity = RandomDirection(input.id + l);
        p.lifetime = 0.5f + (float) l * 0.01f; // 생명주기를 조금씩 증가시켜서 먼지의 크기를 다르게 한다.
        output.Append(p);
    }
}

// 라이플 총알 생성
void EmmitAssaultBullet(VS_BULLET_INPUT input, inout PointStream<VS_BULLET_INPUT> output)
{
    // 총구 머즐 스파크
    GenerateSparkParticles(input, output);
        
    input.type = BULLET_TYPE_TRAIL;
    output.Append(input);
    
    GenerateImpactDustParticles(input, output);
}

// 샷건 총알 생성
void EmmitShotgunBullet(VS_BULLET_INPUT input, inout PointStream<VS_BULLET_INPUT> output)
{
    // 총구 머즐 스파크
    GenerateSparkParticles(input, output);
        
    input.type = BULLET_TYPE_TRAIL;
    output.Append(input);
    
    GenerateImpactDustParticles(input, output);
}


// 총알 궤적 생성
void OutputTrailParticles(VS_BULLET_INPUT input, inout PointStream<VS_BULLET_INPUT> output)
{
    const float3 toEnd = input.endposition - input.position;
    const float distLeft = length(toEnd);
    const float moveLen = length(input.velocity) * gfElapsedTime;
    
    float adv = min(moveLen, distLeft); 
    input.position += normalize(input.velocity) * adv;
    
    const bool reachedEnd = (distLeft <= moveLen + 1e-6); 
    
    float fBeforeTime = input.lifetime;
    input.lifetime -= gfElapsedTime;
    
    if (!reachedEnd && fBeforeTime > 0.0f)
    {
        output.Append(input);
    }
}

// 총구 스파크 갱신
void OutputSparkParticles(VS_BULLET_INPUT input, inout PointStream<VS_BULLET_INPUT> output)
{
    input.lifetime -= gfElapsedTime;
    input.position += normalize(input.velocity) * gfElapsedTime;
    if (input.lifetime > 0.0f)
    {
        output.Append(input);
    }
}

[maxvertexcount(64)]
void GSBulletStreamOutput(point VS_BULLET_INPUT input[1], inout PointStream<VS_BULLET_INPUT> output)
{
    VS_BULLET_INPUT particle = input[0];
    float3 gf3Gravity = float3(0.0f, -9.81f, 0.0f); // 중력 벡터
    
    if (particle.type == BULLET_MAINTAIN) output.Append(particle);
    else if (particle.type == BULLET_TYPE_EMIT_ASSAULT) EmmitAssaultBullet(particle, output);
    else if (particle.type == BULLET_TYPE_EMIT_SHOTGUN) EmmitShotgunBullet(particle, output);
    else if (particle.type == BULLET_TYPE_TRAIL) OutputTrailParticles(particle, output);
    else if (particle.type == BULLET_TYPE_MUZZLE_SPARK) OutputSparkParticles(particle, output);
    else if (particle.type == BULLET_TYPE_FRAGMENT || particle.type == BULLET_TYPE_BLOOD)
    {
        particle.position += particle.velocity * gfElapsedTime;
        particle.velocity += gf3Gravity * gfElapsedTime;
        particle.lifetime -= gfElapsedTime;
        if (particle.lifetime > 0.0f)
        {
            output.Append(particle);
        }
    }

}

//////////////////////////////////////////////////////////////////////////////////
//

struct VS_BULLET_DRAW_OUTPUT
{
    float3 position : POSITION;
    float3 endposition : LASTPOSITION;
    float3 velocity : VELOCITY;
    float lifetime : LIFETIME;
    int type : TYPE;
    float4 color : COLOR;
};

struct GS_BULLET_DRAW_OUTPUT
{
    float4 position : SV_Position;
    float4 color : COLOR;
    float2 uv : TEXTURE;
    int type : TYPE;
};

VS_BULLET_DRAW_OUTPUT VSBulletDraw(VS_BULLET_INPUT input)
{
    VS_BULLET_DRAW_OUTPUT output = (VS_BULLET_DRAW_OUTPUT) 0;

    output.position = input.position;
    output.endposition = input.endposition;
    output.color = float4(1.0f, 1.0f, 1.0f, 1.0f);
    output.velocity = input.velocity;
    output.lifetime = input.lifetime;
    output.type = input.type;
    
    return (output);
}

static float3 gf3Positions[4] = { float3(-1.0f, +1.0f, 0.5f), float3(+1.0f, +1.0f, 0.5f), float3(-1.0f, -1.0f, 0.5f), float3(+1.0f, -1.0f, 0.5f) };
static float2 gf2QuadUVs[4] = { float2(0.0f, 0.0f), float2(1.0f, 0.0f), float2(0.0f, 1.0f), float2(1.0f, 1.0f) };

[maxvertexcount(4)]
void GSBulletDraw(point VS_BULLET_DRAW_OUTPUT input[1], inout TriangleStream<GS_BULLET_DRAW_OUTPUT> outputStream)
{
    GS_BULLET_DRAW_OUTPUT output = (GS_BULLET_DRAW_OUTPUT) 0;
    output.type = input[0].type;
    
    if (input[0].type == BULLET_MAINTAIN)
    {
        output.position = float4(input[0].position, 1.0f);
        output.color = input[0].color;
        output.uv = float2(0.5f, 0.5f);
        outputStream.Append(output);
    }
    else if (input[0].type == BULLET_TYPE_TRAIL)
    {
        float3 cameraPos = GetCameraPosition();
    
        float3 start = input[0].position;
        float3 end = input[0].endposition;
    
        float3 halfPos = (end + start) * 0.5f;
    
        float3 dir = normalize(halfPos - cameraPos);
        dir = cross(dir, normalize(input[0].velocity));
    
        float height = 5.0f;
        
        float3 gf3RectPositions[4] = { float3(start + dir * height), float3(end + dir * height), float3(start - dir * height), float3(end - dir * height) };
    
        for (int i = 0; i < 4; i++)
        {
            //float3 positionW = mul(gf3Positions[i], (float3x3) gmtxInvView) + input[0].position;
            float3 positionW = gf3RectPositions[i];
            output.position = mul(mul(float4(positionW, 1.0f), gmtxView), gmtxProjection);
            output.uv = gf2QuadUVs[i];
            output.color = input[0].color;

            outputStream.Append(output);
        }
    }
    else if (input[0].type == BULLET_TYPE_MUZZLE_SPARK)
    {
        float3 gf3RectPositions[4] = gf3Positions;
        for (int i = 0; i < 4; i++)
        {
            //gf3RectPositions[i] *= input[0].lifetime;
            float3 positionW = mul(gf3Positions[i] * input[0].lifetime * 0.2f, (float3x3) gmtxInvView) + input[0].position;
            //float3 positionW = gf3RectPositions[i] * input[0].lifetime;;
            output.position = mul(mul(float4(positionW, 1.0f), gmtxView), gmtxProjection);
            output.uv = gf2QuadUVs[i];
            output.color = input[0].color;

            outputStream.Append(output);
        }
    }
    else if (input[0].type == BULLET_TYPE_FRAGMENT || input[0].type == BULLET_TYPE_BLOOD)
    {
        float3 gf3RectPositions[4] = gf3Positions;
        for (int i = 0; i < 4; i++)
        {
            //gf3RectPositions[i] *= input[0].lifetime;
            float3 positionW = mul(gf3Positions[i] * input[0].lifetime * 0.2f, (float3x3) gmtxInvView) + input[0].position;
            //float3 positionW = gf3RectPositions[i] * input[0].lifetime;;
            output.position = mul(mul(float4(positionW, 1.0f), gmtxView), gmtxProjection);
            output.uv = gf2QuadUVs[i];
            output.color = input[0].color;

            outputStream.Append(output);
        }
    }
    
//#define BULLET_MAINTAIN -1 // uint 가 type이기에 최대값을 가진다.
//#define BULLET_TYPE_EMIT_ASSAULT 0
//#define BULLET_TYPE_EMIT_SHOTGUN 1
//#define BULLET_TYPE_TRAIL 2
//#define BULLET_TYPE_MUZZLE_SPARK 3
//#define BULLET_TYPE_FRAGMENT 4
//#define BULLET_TYPE_BLOOD 5
}

float4 PSBulletDraw(GS_BULLET_DRAW_OUTPUT input) : SV_TARGET
{
    float4 cColor = float4(0,0,0,0);
    if (input.type == BULLET_TYPE_TRAIL)
        cColor = gtxtAlbedoTexture.Sample(gssWrap, input.uv);
    else if (input.type == BULLET_TYPE_MUZZLE_SPARK)
        cColor = gtxtSpecularTexture.Sample(gssWrap, input.uv);
    else if (input.type == BULLET_TYPE_FRAGMENT)
        cColor = gtxtNormalTexture.Sample(gssWrap, input.uv);
    else if (input.type == BULLET_TYPE_BLOOD)
        cColor = gtxtMetallicTexture.Sample(gssWrap, input.uv);
    else
        discard;
    
    // 색상 별도 적용(기본은 흰색으로 텍스쳐 색상만 적용)
    cColor *= input.color;
    
    if (cColor.a < 0.1f)
        discard;

    return (cColor);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
struct VS_LIGHTING_INPUT
{
    float3 position : POSITION;
    float3 normal : NORMAL;
};

struct VS_LIGHTING_OUTPUT
{
    float4 position : SV_POSITION;
    float3 positionW : POSITION;
    float3 normalW : NORMAL;
    float depthV : VIEWDEPTH;
};

VS_LIGHTING_OUTPUT VSLighting(VS_LIGHTING_INPUT input)
{
    VS_LIGHTING_OUTPUT output;

    output.normalW = mul(input.normal, (float3x3) gmtxGameObject);
    output.positionW = (float3) mul(float4(input.position, 1.0f), gmtxGameObject);
    output.position = mul(mul(float4(output.positionW, 1.0f), gmtxView), gmtxProjection);
    output.depthV = mul(float4(output.positionW, 1.0f), gmtxView).z;

    return (output);
}

float4 PSLighting(VS_LIGHTING_OUTPUT input) : SV_TARGET
{
    input.normalW = normalize(input.normalW);
    float4 cIllumination = Lighting(input.positionW, input.normalW, input.depthV, false);

//	return(cIllumination);
    return (float4(input.normalW * 0.5f + 0.5f, 1.0f));
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
struct PS_DEPTH_OUTPUT
{
    float fzPosition : SV_Target;
    float fDepth : SV_Depth;
};

PS_DEPTH_OUTPUT PSDepthWriteShader(float4 input : SV_Position)
{
    PS_DEPTH_OUTPUT output;

    output.fzPosition = input.z;
    output.fDepth = input.z;

    return (output);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
struct VS_SHADOW_MAP_OUTPUT
{
    float4 position : SV_POSITION;
    float3 positionW : POSITION;
    float3 normalW : NORMAL;
    float depthV : VIEWDEPTH;
};

VS_SHADOW_MAP_OUTPUT VSShadowMapShadow(VS_STANDARD_INPUT input)
{
    VS_SHADOW_MAP_OUTPUT output = (VS_SHADOW_MAP_OUTPUT) 0;

    float4 positionW = mul(float4(input.position, 1.0f), gmtxGameObject);
    output.positionW = positionW.xyz;
    output.position = mul(mul(positionW, gmtxView), gmtxProjection);
    output.normalW = mul(float4(input.normal, 0.0f), gmtxGameObject).xyz;
    output.depthV = mul(positionW, gmtxView).z;

    return (output);
}

float4 PSShadowMapShadow(VS_SHADOW_MAP_OUTPUT input) : SV_TARGET
{
    float4 cIllumination = Lighting(input.positionW, normalize(input.normalW), input.depthV, true);
    return (cIllumination);
}

///////////////////////////////////////////////////////////////////////////////
//

VS_TEXTURED_OUTPUT VSTextureToViewport(uint nVertexID : SV_VertexID)
{
    VS_TEXTURED_OUTPUT output = (VS_TEXTURED_OUTPUT) 0;

    if (nVertexID == 0)
    {
        output.position = float4(-1.0f, +1.0f, 0.0f, 1.0f);
        output.uv = float2(0.0f, 0.0f);
    }
    if (nVertexID == 1)
    {
        output.position = float4(+1.0f, +1.0f, 0.0f, 1.0f);
        output.uv = float2(1.0f, 0.0f);
    }
    if (nVertexID == 2)
    {
        output.position = float4(+1.0f, -1.0f, 0.0f, 1.0f);
        output.uv = float2(1.0f, 1.0f);
    }
    if (nVertexID == 3)
    {
        output.position = float4(-1.0f, +1.0f, 0.0f, 1.0f);
        output.uv = float2(0.0f, 0.0f);
    }
    if (nVertexID == 4)
    {
        output.position = float4(+1.0f, -1.0f, 0.0f, 1.0f);
        output.uv = float2(1.0f, 1.0f);
    }
    if (nVertexID == 5)
    {
        output.position = float4(-1.0f, -1.0f, 0.0f, 1.0f);
        output.uv = float2(0.0f, 1.0f);
    }
    
    output.position = mul(output.position, gmtxGameObject);

    return (output);
}

float4 GetColorFromDepth(float fDepth)
{
    float4 cColor = float4(0.0f, 0.0f, 0.0f, 1.0f);

    if (fDepth < 0.00625f)
        cColor = float4(1.0f, 0.0f, 0.0f, 1.0f);
    else if (fDepth < 0.0125f)
        cColor = float4(0.0f, 1.0f, 0.0f, 1.0f);
    else if (fDepth < 0.025f)
        cColor = float4(0.0f, 0.0f, 1.0f, 1.0f);
    else if (fDepth < 0.05f)
        cColor = float4(1.0f, 1.0f, 0.0f, 1.0f);
    else if (fDepth < 0.075f)
        cColor = float4(0.0f, 1.0f, 1.0f, 1.0f);
    else if (fDepth < 0.1f)
        cColor = float4(1.0f, 0.5f, 0.5f, 1.0f);
    else if (fDepth < 0.4f)
        cColor = float4(0.5f, 1.0f, 1.0f, 1.0f);
    else if (fDepth < 0.6f)
        cColor = float4(1.0f, 0.0f, 1.0f, 1.0f);
    else if (fDepth < 0.8f)
        cColor = float4(0.5f, 0.5f, 1.0f, 1.0f);
    else if (fDepth < 0.9f)
        cColor = float4(0.5f, 1.0f, 0.5f, 1.0f);
    else if (fDepth < 0.95f)
        cColor = float4(0.5f, 0.0f, 0.5f, 1.0f);
    else if (fDepth < 0.99f)
        cColor = float4(1.0f, 1.0f, 1.0f, 1.0f);
    else if (fDepth < 0.999f)
        cColor = float4(1.0f, 0.0f, 1.0f, 1.0f);
    else if (fDepth == 1.0f)
        cColor = float4(0.5f, 0.5f, 0.5f, 1.0f);
    else if (fDepth > 1.0f)
        cColor = float4(0.0f, 0.0f, 0.5f, 1.0f);
    else
        cColor = float4(0.0f, 0.0f, 0.0f, 1.0f);

    return (cColor);
}

SamplerState gssBorder : register(s3);

float4 PSTextureToViewport(VS_TEXTURED_OUTPUT input) : SV_Target
{
//    float fDepthFromLight0 = gtxtDepthTextures[0].SampleLevel(gssBorder, input.uv, 0).r;
    float4 fTextureColor = gtxtAlbedoTexture.SampleLevel(gssBorder, input.uv, 0);
    fTextureColor *= gf4ObjectColor;
//    return ((float4) (fDepthFromLight0));
    if(fTextureColor.a < 0.1f)
    {
        discard;
    }
   
    return (fTextureColor);
}

float4 PSShadowToViewport(VS_TEXTURED_OUTPUT input) : SV_Target
{
    float fDepthFromLight0 = gtxtDepthTextures[gnShadowIndex].SampleLevel(gssBorder, input.uv, 0).r;
    
    return ((float4) (fDepthFromLight0));
}
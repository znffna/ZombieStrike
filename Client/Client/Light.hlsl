///////////////////////////////////////////////////////////////////////////////
// Date: 2025-02-02
// Light.hlsl : Light 상수 버퍼 및 조명 계산을 위한 함수
// Version : 0.1
///////////////////////////////////////////////////////////////////////////////
//--------------------------------------------------------------------------------------
#define MAX_LIGHTS			16 

#define POINT_LIGHT			1
#define SPOT_LIGHT			2
#define DIRECTIONAL_LIGHT	3

#define _WITH_LOCAL_VIEWER_HIGHLIGHTING
#define _WITH_THETA_PHI_CONES
//#define _WITH_REFLECT

struct MATERIAL
{
    float4 m_cAmbient;
    float4 m_cDiffuse;
    float4 m_cSpecular; //a = power
    float4 m_cEmissive;
};

struct LIGHT
{
    float4 m_cAmbient;
    float4 m_cDiffuse;
    float4 m_cSpecular;
    float3 m_vPosition;
    float m_fFalloff;
    float3 m_vDirection;
    float m_fTheta; //cos(m_fTheta)
    float3 m_vAttenuation;
    float m_fPhi; //cos(m_fPhi)
    bool m_bEnable;
    int m_nType;
    float m_fRange;
    //float padding;
    uint m_nShadowStartIndex;
};

cbuffer cbMaterialInfo : register(b1)
{
    MATERIAL gMaterial : packoffset(c0);
    uint gnTexturesMask : packoffset(c4.x);
};

cbuffer cbLights : register(b4)
{
    LIGHT gLights[MAX_LIGHTS];
    float4 gcGlobalAmbientLight;
    int gnLights;
};

struct CB_TO_LIGHT_SPACE
{
    matrix mtxToTextureSpace;
    float4 f4Position;
};

cbuffer cbToLightSpace : register(b5)
{
    CB_TO_LIGHT_SPACE gcbToLightSpaces[MAX_LIGHTS];
};

struct ShadowMapUVs
{
    float4 UVs[MAX_LIGHTS];
};

ShadowMapUVs CalculateShadowMapUVs(float4 positionW)
{
    ShadowMapUVs result;
    for (int i = 0; i < MAX_LIGHTS; i++)
    {
        if (gcbToLightSpaces[i].f4Position.w != 0.0f)
        {
            result.UVs[i] = mul(positionW, gcbToLightSpaces[i].mtxToTextureSpace);
            result.UVs[i].xyz /= result.UVs[i].w; // Perspective divide
            //result.UVs[i] /= result.UVs[i].w; // Perspective divide
        }
    }
    return result;
}


// return : Camera World Position
float3 GetCameraPosition()
{
    //float3 vCameraPosition = float3(-gmtxView._14, -gmtxView._24, -gmtxView._34);
    //float3 vCameraPosition = float3(-gmtxView._41, -gmtxView._42, -gmtxView._43);
    float3 vCameraPosition = gCameraPosition;
    
    return (vCameraPosition);
}

#define FRAME_BUFFER_WIDTH		1280
#define FRAME_BUFFER_HEIGHT		720

#define _DEPTH_BUFFER_WIDTH		(FRAME_BUFFER_WIDTH * 4)
#define _DEPTH_BUFFER_HEIGHT	(FRAME_BUFFER_HEIGHT * 4)

#define DELTA_X					(1.0f / _DEPTH_BUFFER_WIDTH)
#define DELTA_Y					(1.0f / _DEPTH_BUFFER_HEIGHT)

#define MAX_DEPTH_TEXTURES		MAX_LIGHTS

#define _WITH_PCF_FILTERING

Texture2D<float> gtxtDepthTextures[] : register(t0, space1);
SamplerComparisonState gssComparisonPCFShadow : register(s2);

float Compute3x3ShadowFactor(float2 uv, float fDepth, uint nIndex)
{
    float fPercentLit = gtxtDepthTextures[nIndex].SampleCmpLevelZero(gssComparisonPCFShadow, uv, fDepth).r;
    fPercentLit += gtxtDepthTextures[nIndex].SampleCmpLevelZero(gssComparisonPCFShadow, uv + float2(-DELTA_X, 0.0f), fDepth).r;
    fPercentLit += gtxtDepthTextures[nIndex].SampleCmpLevelZero(gssComparisonPCFShadow, uv + float2(+DELTA_X, 0.0f), fDepth).r;
    fPercentLit += gtxtDepthTextures[nIndex].SampleCmpLevelZero(gssComparisonPCFShadow, uv + float2(0.0f, -DELTA_Y), fDepth).r;
    fPercentLit += gtxtDepthTextures[nIndex].SampleCmpLevelZero(gssComparisonPCFShadow, uv + float2(0.0f, +DELTA_Y), fDepth).r;
    fPercentLit += gtxtDepthTextures[nIndex].SampleCmpLevelZero(gssComparisonPCFShadow, uv + float2(-DELTA_X, -DELTA_Y), fDepth).r;
    fPercentLit += gtxtDepthTextures[nIndex].SampleCmpLevelZero(gssComparisonPCFShadow, uv + float2(-DELTA_X, +DELTA_Y), fDepth).r;
    fPercentLit += gtxtDepthTextures[nIndex].SampleCmpLevelZero(gssComparisonPCFShadow, uv + float2(+DELTA_X, -DELTA_Y), fDepth).r;
    fPercentLit += gtxtDepthTextures[nIndex].SampleCmpLevelZero(gssComparisonPCFShadow, uv + float2(+DELTA_X, +DELTA_Y), fDepth).r;

    return (fPercentLit / 9.0f);
}

float4 DirectionalLight(int nIndex, float3 vNormal, float3 vToCamera)
{
    float3 vToLight = -gLights[nIndex].m_vDirection;
    float fDiffuseFactor = dot(vToLight, vNormal);
    float fSpecularFactor = 0.0f;
    if (fDiffuseFactor > 0.0f)
    {
        if (gMaterial.m_cSpecular.a != 0.0f)
        {
#ifdef _WITH_REFLECT
			float3 vReflect = reflect(-vToLight, vNormal);
			fSpecularFactor = pow(max(dot(vReflect, vToCamera), 0.0f), gMaterial.m_cSpecular.a);
#else
#ifdef _WITH_LOCAL_VIEWER_HIGHLIGHTING
            float3 vHalf = normalize(vToCamera + vToLight);
#else
			float3 vHalf = float3(0.0f, 1.0f, 0.0f);
#endif
            fSpecularFactor = pow(max(dot(vHalf, vNormal), 0.0f), gMaterial.m_cSpecular.a);
#endif
        }
    }

    return ((gLights[nIndex].m_cAmbient * gMaterial.m_cAmbient) + (gLights[nIndex].m_cDiffuse * fDiffuseFactor * gMaterial.m_cDiffuse) + (gLights[nIndex].m_cSpecular * fSpecularFactor * gMaterial.m_cSpecular));
}

float4 PointLight(int nIndex, float3 vPosition, float3 vNormal, float3 vToCamera)
{
    float3 vToLight = gLights[nIndex].m_vPosition - vPosition;
    float fDistance = length(vToLight);
    if (fDistance <= gLights[nIndex].m_fRange)
    {
        float fSpecularFactor = 0.0f;
        vToLight /= fDistance;
        float fDiffuseFactor = dot(vToLight, vNormal);
        if (fDiffuseFactor > 0.0f)
        {
            if (gMaterial.m_cSpecular.a != 0.0f)
            {
#ifdef _WITH_REFLECT
				float3 vReflect = reflect(-vToLight, vNormal);
				fSpecularFactor = pow(max(dot(vReflect, vToCamera), 0.0f), gMaterial.m_cSpecular.a);
#else
#ifdef _WITH_LOCAL_VIEWER_HIGHLIGHTING
                float3 vHalf = normalize(vToCamera + vToLight);
#else
				float3 vHalf = float3(0.0f, 1.0f, 0.0f);
#endif
                fSpecularFactor = pow(max(dot(vHalf, vNormal), 0.0f), gMaterial.m_cSpecular.a);
#endif
            }
        }
        float fAttenuationFactor = 1.0f / dot(gLights[nIndex].m_vAttenuation, float3(1.0f, fDistance, fDistance * fDistance));

        return (((gLights[nIndex].m_cAmbient * gMaterial.m_cAmbient) + (gLights[nIndex].m_cDiffuse * fDiffuseFactor * gMaterial.m_cDiffuse) + (gLights[nIndex].m_cSpecular * fSpecularFactor * gMaterial.m_cSpecular)) * fAttenuationFactor);
    }
    return (float4(0.0f, 0.0f, 0.0f, 0.0f));
}

float4 SpotLight(int nIndex, float3 vPosition, float3 vNormal, float3 vToCamera)
{
    float3 vToLight = gLights[nIndex].m_vPosition - vPosition;
    float fDistance = length(vToLight);
    if (fDistance <= gLights[nIndex].m_fRange)
    {
        float fSpecularFactor = 0.0f;
        vToLight /= fDistance;
        float fDiffuseFactor = dot(vToLight, vNormal);
        if (fDiffuseFactor > 0.0f)
        {
            if (gMaterial.m_cSpecular.a != 0.0f)
            {
#ifdef _WITH_REFLECT
				float3 vReflect = reflect(-vToLight, vNormal);
				fSpecularFactor = pow(max(dot(vReflect, vToCamera), 0.0f), gMaterial.m_cSpecular.a);
#else
#ifdef _WITH_LOCAL_VIEWER_HIGHLIGHTING
                float3 vHalf = normalize(vToCamera + vToLight);
#else
				float3 vHalf = float3(0.0f, 1.0f, 0.0f);
#endif
                fSpecularFactor = pow(max(dot(vHalf, vNormal), 0.0f), gMaterial.m_cSpecular.a);
#endif
            }
        }
#ifdef _WITH_THETA_PHI_CONES
        float fAlpha = max(dot(-vToLight, gLights[nIndex].m_vDirection), 0.0f);
        float fSpotFactor = pow(max(((fAlpha - gLights[nIndex].m_fPhi) / (gLights[nIndex].m_fTheta - gLights[nIndex].m_fPhi)), 0.0f), gLights[nIndex].m_fFalloff);
#else
		float fSpotFactor = pow(max(dot(-vToLight, gLights[i].m_vDirection), 0.0f), gLights[i].m_fFalloff);
#endif
        float fAttenuationFactor = 1.0f / dot(gLights[nIndex].m_vAttenuation, float3(1.0f, fDistance, fDistance * fDistance));

        return (((gLights[nIndex].m_cAmbient * gMaterial.m_cAmbient) + (gLights[nIndex].m_cDiffuse * fDiffuseFactor * gMaterial.m_cDiffuse) + (gLights[nIndex].m_cSpecular * fSpecularFactor * gMaterial.m_cSpecular)) * fAttenuationFactor * fSpotFactor);
    }
    return (float4(0.0f, 0.0f, 0.0f, 0.0f));
}

//float4 Lighting(float3 vPosition, float3 vNormal)
//{
//    float3 vCameraPosition = GetCameraPosition();
//    float3 vToCamera = normalize(vCameraPosition - vPosition);

//    float4 cColor = float4(0.0f, 0.0f, 0.0f, 0.0f);
//	[unroll(MAX_LIGHTS)]
//    for (int i = 0; i < gnLights; i++)
//    {
//        if (gLights[i].m_bEnable)
//        {
//            if (gLights[i].m_nType == DIRECTIONAL_LIGHT)
//            {
//                cColor += DirectionalLight(i, vNormal, vToCamera);
//            }
//            else if (gLights[i].m_nType == POINT_LIGHT)
//            {
//                cColor += PointLight(i, vPosition, vNormal, vToCamera);
//            }
//            else if (gLights[i].m_nType == SPOT_LIGHT)
//            {
//                cColor += SpotLight(i, vPosition, vNormal, vToCamera);
//            }
//        }
//    }
//    cColor += (gcGlobalAmbientLight * gMaterial.m_cAmbient);
//    cColor.a = gMaterial.m_cDiffuse.a;

//    return (cColor);
//}

float4 Lighting(float3 vPosition, float3 vNormal, float fdepthV, bool bShadow)
{
    float3 vCameraPosition = GetCameraPosition();
    float3 vToCamera = normalize(vCameraPosition - vPosition);

    float4 cColor = float4(0.0f, 0.0f, 0.0f, 0.0f);
    
    float4 shadowMapUVs[MAX_LIGHTS];
    shadowMapUVs = CalculateShadowMapUVs(float4(vPosition, 1.0f)).UVs;
    
	[unroll(MAX_LIGHTS)]
    for (int i = 0; i < gnLights; i++)
    {
		float fShadowFactor = 1.0f;
        float fBias = gfBias; // Bias to prevent shadow acne
#ifdef _WITH_PCF_FILTERING
		if (bShadow) fShadowFactor = Compute3x3ShadowFactor(shadowMapUVs[gLights[i].m_nShadowStartIndex].xy, shadowMapUVs[gLights[i].m_nShadowStartIndex].z + fBias, gLights[i].m_nShadowStartIndex);
#else
        if (bShadow) fShadowFactor = gtxtDepthTextures[gLights[i].m_nShadowStartIndex].SampleCmpLevelZero(gssComparisonPCFShadow, shadowMapUVs[gLights[i].m_nShadowStartIndex].xy, shadowMapUVs[gLights[i].m_nShadowStartIndex].z).r;
#endif
        if (gLights[i].m_bEnable)
        {
            if (gLights[i].m_nType == DIRECTIONAL_LIGHT)
            {
                cColor += DirectionalLight(i, vNormal, vToCamera) * fShadowFactor;
            }
            else if (gLights[i].m_nType == POINT_LIGHT)
            {
                cColor += PointLight(i, vPosition, vNormal, vToCamera) * fShadowFactor;
            }
            else if (gLights[i].m_nType == SPOT_LIGHT)
            {
                cColor += SpotLight(i, vPosition, vNormal, vToCamera) * fShadowFactor;
            }
            cColor += (gLights[i].m_cAmbient * gMaterial.m_cAmbient);
        }
    }
    cColor += (gcGlobalAmbientLight * gMaterial.m_cAmbient);
    cColor.a = gMaterial.m_cDiffuse.a;

    return (cColor);
}



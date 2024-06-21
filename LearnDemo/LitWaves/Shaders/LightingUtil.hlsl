//***************************************************************************************
// LightingUtil.hlsl by Frank Luna (C) 2015 All Rights Reserved.
//
// Contains API for shader lighting.
//***************************************************************************************

#define MaxLights 16

struct Light
{
    float3 Strength;
    float FalloffStart; // point/spot light only
    float3 Direction;   // directional/spot light only
    float FalloffEnd;   // point/spot light only
    float3 Position;    // point light only
    float SpotPower;    // spot light only
};

struct Material
{
    float4 DiffuseAlbedo;
    float3 FresnelR0;
    float Shininess;
};

float CalcAttenuation(float d, float falloffStart, float falloffEnd)
{
    // Linear falloff.
    return saturate((falloffEnd-d) / (falloffEnd - falloffStart));
}

// Schlick gives an approximation to Fresnel reflectance (see pg. 233 "Real-Time Rendering 3rd Ed.").
// R0 = ( (n-1)/(n+1) )^2, where n is the index of refraction.
float3 SchlickFresnel(float3 R0, float3 normal, float3 lightVec)
{
    float cosIncidentAngle = saturate(dot(normal, lightVec));

    float f0 = 1.0f - cosIncidentAngle;
    float3 reflectPercent = R0 + (1.0f - R0)*(f0*f0*f0*f0*f0);

    return reflectPercent;
}

float3 BlinnPhong(float3 lightStrength, float3 lightVec, float3 normal, float3 toEye, Material mat)
{
    const float m = mat.Shininess * 256.0f;
    float3 halfVec = normalize(toEye + lightVec);

    float roughnessFactor = (m + 8.0f)*pow(max(dot(halfVec, normal), 0.0f), m) / 8.0f;
    float3 fresnelFactor = SchlickFresnel(mat.FresnelR0, halfVec, lightVec);

    float3 specAlbedo = fresnelFactor*roughnessFactor;

    // Our spec formula goes outside [0,1] range, but we are 
    // doing LDR rendering.  So scale it down a bit.
    specAlbedo = specAlbedo / (specAlbedo + 1.0f);

    return (mat.DiffuseAlbedo.rgb + specAlbedo) * lightStrength;
}

//---------------------------------------------------------------------------------------
// Evaluates the lighting equation for directional lights.
//---------------------------------------------------------------------------------------
float3 ComputeDirectionalLight(Light L, Material mat, float3 normal, float3 toEye)
{
    // The light vector aims opposite the direction the light rays travel.
    float3 lightVec = -L.Direction;

    // Scale light down by Lambert's cosine law.
    float ndotl = max(dot(lightVec, normal), 0.0f);
    float3 lightStrength = L.Strength * ndotl;

    return BlinnPhong(lightStrength, lightVec, normal, toEye, mat);
}

//---------------------------------------------------------------------------------------
// Evaluates the lighting equation for point lights.
//---------------------------------------------------------------------------------------
float3 ComputePointLight(Light L, Material mat, float3 pos, float3 normal, float3 toEye)
{
    // The vector from the surface to the light.
    float3 lightVec = L.Position - pos;

    // The distance from surface to light.
    float d = length(lightVec);

    // Range test.
    if(d > L.FalloffEnd)
        return 0.0f;

    // Normalize the light vector.
    lightVec /= d;

    // Scale light down by Lambert's cosine law.
    float ndotl = max(dot(lightVec, normal), 0.0f);
    float3 lightStrength = L.Strength * ndotl;

    // Attenuate light by distance.
    float att = CalcAttenuation(d, L.FalloffStart, L.FalloffEnd);
    lightStrength *= att;

    return BlinnPhong(lightStrength, lightVec, normal, toEye, mat);
}

//---------------------------------------------------------------------------------------
// Evaluates the lighting equation for spot lights.
//---------------------------------------------------------------------------------------
float3 ComputeSpotLight(Light L, Material mat, float3 pos, float3 normal, float3 toEye)
{
    // The vector from the surface to the light.
    float3 lightVec = L.Position - pos;

    // The distance from surface to light.
    float d = length(lightVec);

    // Range test.
    if(d > L.FalloffEnd)
        return 0.0f;

    // Normalize the light vector.
    lightVec /= d;

    // Scale light down by Lambert's cosine law.
    float ndotl = max(dot(lightVec, normal), 0.0f);
    float3 lightStrength = L.Strength * ndotl;

    // Attenuate light by distance.
    float att = CalcAttenuation(d, L.FalloffStart, L.FalloffEnd);
    lightStrength *= att;

    // Scale by spotlight
    float spotFactor = pow(max(dot(-lightVec, L.Direction), 0.0f), L.SpotPower);
    lightStrength *= spotFactor;

    return BlinnPhong(lightStrength, lightVec, normal, toEye, mat);
}

float4 ComputeLighting(Light gLights[MaxLights], Material mat,
                       float3 pos, float3 normal, float3 toEye,
                       float3 shadowFactor)
{
    float3 result = 0.0f;

    int i = 0;

#if (NUM_DIR_LIGHTS > 0)
    for(i = 0; i < NUM_DIR_LIGHTS; ++i)
    {
        result += shadowFactor[i] * ComputeDirectionalLight(gLights[i], mat, normal, toEye);
    }
#endif

#if (NUM_POINT_LIGHTS > 0)
    for(i = NUM_DIR_LIGHTS; i < NUM_DIR_LIGHTS+NUM_POINT_LIGHTS; ++i)
    {
        result += ComputePointLight(gLights[i], mat, pos, normal, toEye);
    }
#endif

#if (NUM_SPOT_LIGHTS > 0)
    for(i = NUM_DIR_LIGHTS + NUM_POINT_LIGHTS; i < NUM_DIR_LIGHTS + NUM_POINT_LIGHTS + NUM_SPOT_LIGHTS; ++i)
    {
        result += ComputeSpotLight(gLights[i], mat, pos, normal, toEye);
    }
#endif 

    return float4(result, 0.0f);
}


/*
    �������Ƕ���͵Ľ���ѽ����˹淶��������˱�����������������ƽ��ֵ�����ٳ���4��
    ע�⣬Ϊ�˵õ���Ϊ��׼�Ľ�������ǻ����Բ��ø��Ӹ��ӵ���ƽ��ֵ����������˵�����ݶ���ε������ȷ��Ȩ��
    ���������Ķ���ε�Ȩ��Ҫ�������С�Ķ���Σ�������ȡ��Ȩƽ��ֵ��
    ����α����չʾ������������������Ķ����б�������б����������ȡ��Ӧ�ķ���ƽ��ֵ��

    // ����
    // 1. һ���������飨mVertices����ÿ�����㶼��һ��λ�÷�����pos����һ�����߷�����normal��
    // 2. һ���������飨mIndices��
    // ���������е�ÿ����������˵
    for(UINT i = 0; i < mNumTriangles; ++i)
    {
        // ��i�������ε�����
        UINT i0 = mIndices[i*3+0];
        UINT i1 = mIndices[i*3+1];
        UINT i2 = mIndices[i*3+2];

        // ��i�������εĶ���
        Vertex v0 = mVertices[i0];
        Vertex v1 = mVertices[i1];
        Vertex v2 = mVertices[i2];

        // ����ƽ�淨��
        Vector3 e0 = v1.pos - v0.pos;
        Vector3 e1 = v2.pos - v0.pos;
        Vector3 faceNormal = Cross(e0, e1);

        // �������ι���������3�����㣬���Խ���ƽ�淨������Щ���㷨���������ƽ��ֵ
        mVertices[i0].normal += faceNormal;
        mVertices[i1].normal += faceNormal;
        mVertices[i2].normal += faceNormal;
    }

    // ����ÿ������v��˵�����������Ѿ������й�����v�������ε�ƽ�淨�߽�����ͣ��������ڽ�����С���// �淶��������
    for(UINT i = 0; i < mNumVertices; ++i)
        mVertices[i].normal = Normalize(&mVertices[i].normal));


    ���������������ı任:
    1.���������(n)��������(u)������
    2.������(n)����(A)�仯�� == n*A����������(u)����������
    3.������(u)��Ҫ������ת�þ���仯(��(u*(A^-1)^T))������仯��ķ�����(n*A)����

    ���ԣ�������������Ե������ǣ�������һ�����ڱ任�����������Ƿ��ߣ��ı任����[A]��
    ����ܹ��������һ���任����[B]��ͨ�������任��������
    ʹ������[A]�任����������뷨�����ع������Ĺ�ϵ����[uA��nB=0]����
    Ϊ�ˣ��������ȴ���֪����Ϣ���֣����������[n]������������[u]�����У�

    u��n = 0                     �����������뷨����
    u*n^T = 0                   �������дΪ����˷�
    u*(A*A^-1)*n^T = 0          ���뵥λ����I=A*A^-1
    (u*A)*(A^-1*n^T) = 0        ���ݾ���˷�����Ľ����
    (u*A)*((A^-1*n^T)^T)^T = 0  ����ת�þ��������(A^T)^T=A
    (u*A)*(n*(A^-1)^T)^T = 0    ����ת�þ��������(A*B)^T=B^T*A^T
    u*A��n*(A^-1)^T = 0          ������˷���дΪ�����ʽ
    u*A��n*B = 0                 �任���������������任��ķ�����
    ==> B = (A^-1)^T

    ��ˣ�ͨ��[��ͼ]������[��ͼ]����ת�þ��󣩶Է��������б任��
    ����ʹ����ֱ�ھ�����[��ͼ]�任���������[��ͼ]��ע�⣬�������[��ͼ]���������󣨼�����[��ͼ]����
    ��ô[��ͼ]��Ҳ����˵����������������������ټ���������ת�þ���
    ��Ϊ������������[��ͼ]������ʵ����һ�任��
    �ܶ���֮����������Ҫ�Ծ����ǵȱȱ任����б任��shear transformation��Ҳ�������б�ת��ȣ�
    ��ķ��������б任ʱ�����ʹ����ת�þ���
    ������ͷ�ļ�MathHelper.h��Ϊ������ת�þ���ʵ����һ������������

    static XMMATRIX InverseTranspose(CXMMATRIX M)
    {
      XMMATRIX A = M;
      A.r[3] = XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f);

      XMVECTOR det = XMMatrixDeterminant(A);
      return XMMatrixTranspose(XMMatrixInverse(&det, A));
    }


*/


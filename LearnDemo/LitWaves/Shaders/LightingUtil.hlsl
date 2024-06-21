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
    由于我们对求和的结果已进行了规范化处理，因此便无需像往常求算术平均值那样再除以4。
    注意，为了得到更为精准的结果，我们还可以采用更加复杂的求平均值方法，比如说，根据多边形的面积来确定权重
    （如面积大的多边形的权重要大于面积小的多边形），以求取加权平均值。
    下列伪代码展示了若给定三角形网格的顶点列表和索引列表，该如何来求取相应的法线平均值。

    // 输入
    // 1. 一个顶点数组（mVertices）。每个顶点都有一个位置分量（pos）和一个法线分量（normal）
    // 2. 一个索引数组（mIndices）
    // 对于网格中的每个三角形来说
    for(UINT i = 0; i < mNumTriangles; ++i)
    {
        // 第i个三角形的索引
        UINT i0 = mIndices[i*3+0];
        UINT i1 = mIndices[i*3+1];
        UINT i2 = mIndices[i*3+2];

        // 第i个三角形的顶点
        Vertex v0 = mVertices[i0];
        Vertex v1 = mVertices[i1];
        Vertex v2 = mVertices[i2];

        // 计算平面法线
        Vector3 e0 = v1.pos - v0.pos;
        Vector3 e1 = v2.pos - v0.pos;
        Vector3 faceNormal = Cross(e0, e1);

        // 该三角形共享了下面3个顶点，所以将此平面法线与这些顶点法线相加以求平均值
        mVertices[i0].normal += faceNormal;
        mVertices[i1].normal += faceNormal;
        mVertices[i2].normal += faceNormal;
    }

    // 对于每个顶点v来说，由于我们已经对所有共享顶点v的三角形的平面法线进行求和，所以现在仅需进行　　// 规范化处理即可
    for(UINT i = 0; i < mNumVertices; ++i)
        mVertices[i].normal = Normalize(&mVertices[i].normal));


    法向量与切向量的变换:
    1.最初法向量(n)和切向量(u)正交，
    2.法向量(n)经过(A)变化后 == n*A，与切向量(u)不再正交。
    3.切向量(u)需要经过逆转置矩阵变化(即(u*(A^-1)^T))才能与变化后的法向量(n*A)正交

    所以，我们现在所面对的问题是，若给定一个用于变换点与向量（非法线）的变换矩阵[A]，
    如何能够求出这样一个变换矩阵[B]：通过它来变换法向量，
    使经矩阵[A]变换后的切向量与法向量重归正交的关系（即[uA·nB=0]）。
    为此，我们首先从已知的信息着手，如果法向量[n]正交于切向量[u]，则有：

    u·n = 0                     切向量正交与法向量
    u*n^T = 0                   将点积改写为矩阵乘法
    u*(A*A^-1)*n^T = 0          插入单位矩阵I=A*A^-1
    (u*A)*(A^-1*n^T) = 0        根据矩阵乘法运算的结合律
    (u*A)*((A^-1*n^T)^T)^T = 0  根据转置矩阵的性质(A^T)^T=A
    (u*A)*(n*(A^-1)^T)^T = 0    根据转置矩阵的性质(A*B)^T=B^T*A^T
    u*A·n*(A^-1)^T = 0          将矩阵乘法改写为点积形式
    u*A·n*B = 0                 变换后的切向量正交与变换后的法向量
    ==> B = (A^-1)^T

    因此，通过[插图]（矩阵[插图]的逆转置矩阵）对法向量进行变换后，
    即可使它垂直于经矩阵[插图]变换后的切向量[插图]。注意，如果矩阵[插图]是正交矩阵（即满足[插图]），
    那么[插图]。也就是说，在这种情况下我们无需再计算它的逆转置矩阵，
    因为利用正交矩阵[插图]自身即可实现这一变换。
    总而言之，当我们需要对经过非等比变换或剪切变换（shear transformation，也有译作切变转变等）
    后的法向量进行变换时，则可使用逆转置矩阵。
    我们在头文件MathHelper.h中为计算逆转置矩阵实现了一个辅助函数：

    static XMMATRIX InverseTranspose(CXMMATRIX M)
    {
      XMMATRIX A = M;
      A.r[3] = XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f);

      XMVECTOR det = XMMatrixDeterminant(A);
      return XMMatrixTranspose(XMMatrixInverse(&det, A));
    }


*/


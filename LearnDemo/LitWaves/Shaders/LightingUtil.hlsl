//***************************************************************************************
// LightingUtil.hlsl by Frank Luna (C) 2015 All Rights Reserved.
//
// Contains API for shader lighting.
//***************************************************************************************

#define MaxLights 16

struct Light
{
    float3 Strength;    //                              ��Դ����ɫ
    float FalloffStart; // point/spot light only        �������Դ/�۹�ƹ�Դʹ��
    float3 Direction;   // directional/spot light only  ���������Դ/�۹�ƹ�Դʹ��
    float FalloffEnd;   // point/spot light only        �������Դ/�۹�ƹ�Դʹ��
    float3 Position;    // point light only             �������Դ/�۹�ƹ�Դʹ��
    float SpotPower;    // spot light only              �����۹�ƹ�Դʹ��
};

struct Material
{
    float4 DiffuseAlbedo;
    float3 FresnelR0;
    float Shininess;
};

// ʵ����һ������˥�����ӵļ��㷽�����ɽ���Ӧ���ڵ��Դ��۹�ƹ�Դ��
float CalcAttenuation(float d, float falloffStart, float falloffEnd)
{
    // Linear falloff.
    // ����˥��
    return saturate((falloffEnd-d) / (falloffEnd - falloffStart));
}

// ������������̵�ʯ��˽��ơ��˺������ڹ�����[L]����淨��[n]֮��ļнǣ������ݷ�����ЧӦ���Ƶؼ������[n]Ϊ���ߵı����������İٷֱȡ�
// Schlick gives an approximation to Fresnel reflectance (see pg. 233 "Real-Time Rendering 3rd Ed.").
// R0 = ( (n-1)/(n+1) )^2, where n is the index of refraction.
// ʯ��������һ�ֱƽ������������ʵĽ��Ʒ���
// (�μ�"Real-Time Rendering 3rd Ed."��233ҳ)
// R0 = ( (n-1)/(n+1) )^2��ʽ�е�nΪ������
float3 SchlickFresnel(float3 R0, float3 normal, float3 lightVec)
{
    float cosIncidentAngle = saturate(dot(normal, lightVec));

    float f0 = 1.0f - cosIncidentAngle;
    float3 reflectPercent = R0 + (1.0f - R0)*(f0*f0*f0*f0*f0);

    return reflectPercent;
}

// ���㷴�䵽�۲������еĹ�������ֵΪ����������뾵�淴��������ܺ͡�
float3 BlinnPhong(float3 lightStrength, float3 lightVec, float3 normal, float3 toEye, Material mat)
{
    // m�ɹ�����Ƶ������������������ݴֲڶ����
    const float m = mat.Shininess * 256.0f;
    float3 halfVec = normalize(toEye + lightVec);

    float roughnessFactor = (m + 8.0f)*pow(max(dot(halfVec, normal), 0.0f), m) / 8.0f;
    float3 fresnelFactor = SchlickFresnel(mat.FresnelR0, halfVec, lightVec);

    float3 specAlbedo = fresnelFactor*roughnessFactor;

    // Our spec formula goes outside [0,1] range, but we are 
    // doing LDR rendering.  So scale it down a bit.
    // �������ǽ��е���LDR��low dynamic range���Ͷ�̬��Χ����Ⱦ����spec�����淴�䣩��ʽ�õ�
    // �Ľ���Իᳬ����Χ[0,1]������ֽ��䰴������СһЩ
    specAlbedo = specAlbedo / (specAlbedo + 1.0f);
    /*
        ���ǲ��õľ��淴���ʼ��㹫ʽ�����侵��ֵ����1�����ʾ�ǳ�ҫ�۵ĸ߹⡣Ȼ��������ȴϣ����ȾĿ�����ɫֵ��[0, 1]����Ͷ�̬��Χ�ڣ�
        ���һ����˵�����ڴ˷�Χ����ֵ�򵥵�ǯ��Ϊ1.0���ɡ����ǣ�Ϊ�˻�ø�����͵ľ���߹�Ͳ��ˡ�һ���С�ʽ��ǯ����ֵ�ˣ�
        ������Ҫ��������С���淴���ʣ�[specAlbedo = specAlbedo / (specAlbedo + 1.0f);]
        �߶�̬��Χ��HDR������ʹ�õ��ǹ���ֵ�ɳ�����Χ[0, 1]�ĸ�����ȾĿ�꣬�ڽ���ɫ����ͼ�������ʱ��������ʾ��Ŀ�ĻὫ�߶�̬��Χӳ���[0, 1]���䣬
        �������ת���Ĺ����У�����ϸ����Ϣ�Ǻ���Ҫ��һ������HDR��Ⱦ��ɫ��ӳ�䱾�����һ�ŵ�����ѧ�ơ�������̲�[Reinhard10]��
        ������һƪ���� [Pettineo12] ��Ҳ�����˱Ƚ��꾡�Ľ��ܣ������й���������ʵ�����ʾ����
    */

    return (mat.DiffuseAlbedo.rgb + specAlbedo) * lightStrength;
}

//---------------------------------------------------------------------------------------
// Evaluates the lighting equation for directional lights.
//---------------------------------------------------------------------------------------
/*
    ʵ�ַ����Դ
    �����۲�λ��[E]���������ԣ�����[n]Ϊ���ߵı����Ͽɼ�һ��[p]��
    ������HLSL�����������ĳ�����Դ�����������������Է���[v = normalize(E - p)]������۲������еĹ�����
    �����ǵ�ʾ���У��˺�������������ɫ�������ã��Ի��ڹ���ȷ�����ص���ɫ��
*/
float3 ComputeDirectionalLight(Light L, Material mat, float3 normal, float3 toEye)
{
    // The light vector aims opposite the direction the light rays travel.
    // ����������ߴ����ķ���պ��෴
    float3 lightVec = -L.Direction;

    // Scale light down by Lambert's cosine law.
    // ͨ���ʲ����Ҷ��ɰ��������͹�ǿ
    float ndotl = max(dot(lightVec, normal), 0.0f);
    float3 lightStrength = L.Strength * ndotl;

    return BlinnPhong(lightStrength, lightVec, normal, toEye, mat);
}

//---------------------------------------------------------------------------------------
// Evaluates the lighting equation for point lights.
//---------------------------------------------------------------------------------------
/*
    ʵ�ֵ��Դ
    �����۲��[E]����[n]��Ϊ���ߵı����Ͽ���һ��[p]�Լ��������ԣ��������HLSL������������ӵ��Դ�ų���
    ������������[v = normalize(E - p)]��������۲������еĹ����������ǵ�ʾ���У��ú�������������ɫ�������ã������ݹ�����ȷ�����ص���ɫ��
*/
float3 ComputePointLight(Light L, Material mat, float3 pos, float3 normal, float3 toEye)
{
    // The vector from the surface to the light.
    // �Ա���ָ���Դ������
    float3 lightVec = L.Position - pos;

    // The distance from surface to light.
    // �ɱ��浽��Դ�ľ���
    float d = length(lightVec);

    // Range test.
    // ��Χ���
    if(d > L.FalloffEnd)
        return 0.0f;

    // Normalize the light vector.
    // �Թ��������й淶������
    lightVec /= d;

    // Scale light down by Lambert's cosine law.
    // ͨ���ʲ����Ҷ��ɰ��������͹�ǿ
    float ndotl = max(dot(lightVec, normal), 0.0f);
    float3 lightStrength = L.Strength * ndotl;

    // Attenuate light by distance.
    // ���ݾ��������˥��
    float att = CalcAttenuation(d, L.FalloffStart, L.FalloffEnd);
    lightStrength *= att;

    return BlinnPhong(lightStrength, lightVec, normal, toEye, mat);
}

//---------------------------------------------------------------------------------------
// Evaluates the lighting equation for spot lights.
//---------------------------------------------------------------------------------------
/*
    ʵ�־۹�ƹ�Դ
    ָ���۲��[E]����[n]Ϊ���ߵı����Ͽ���һ��[p]�Լ��������ԣ��������HLSL��������������Ծ۹�ƹ�Դ��
    �������������Է���[v = normalize(E - p)]������۲������еĹ����������ǵ�ʾ���У��˺�������������ɫ���б����ã��Ը��ݹ���ȷ�����ص���ɫ��
*/
float3 ComputeSpotLight(Light L, Material mat, float3 pos, float3 normal, float3 toEye)
{
    // The vector from the surface to the light.
    // �ӱ���ָ���Դ������
    float3 lightVec = L.Position - pos;

    // The distance from surface to light.
    // �ɱ��浽��Դ�ľ���
    float d = length(lightVec);

    // Range test.
    // ��Χ���
    if(d > L.FalloffEnd)
        return 0.0f;

    // Normalize the light vector.
    // �Թ��������й淶������
    lightVec /= d;

    // Scale light down by Lambert's cosine law.
    // ͨ���ʲ����Ҷ��ɰ�������С���ǿ��
    float ndotl = max(dot(lightVec, normal), 0.0f);
    float3 lightStrength = L.Strength * ndotl;

    // Attenuate light by distance.
    // ���ݾ��������˥��
    float att = CalcAttenuation(d, L.FalloffStart, L.FalloffEnd);
    lightStrength *= att;

    // Scale by spotlight
    // ���ݾ۹������ģ�ͶԹ�ǿ�������Ŵ���
    float spotFactor = pow(max(dot(-lightVec, L.Direction), 0.0f), L.SpotPower);
    lightStrength *= spotFactor;

    return BlinnPhong(lightStrength, lightVec, normal, toEye, mat);
}

/*
    ��ǿ�ǿ��Ե��ӵġ���ˣ���֧�ֶ����Դ�ĳ����У�������Ҫ����ÿһ����Դ����������������Ҫ������յĵ�������ϵĹ���ֵ��͡�
    ʾ���������֧��16����Դ��ƾ�˱�����÷���⡢�����۹�����ֹ�Դ����������ϡ���Ȼ��ǰ���ǹ�Դ���������ܳ���16����
    ���⣬���������õ�Լ���Ƿ����Դ����λ�ڹ�������Ŀ�ʼ���֣����Դ��֮���۹�ƹ�Դ������ĩβ�����д������ڼ���ĳ�㴦�Ĺ��շ��̣�

    ���Թ۲쵽ÿ�����͹�Դ������ʵ�ɶ��#define�����Կ��ơ�����һ������ɫ���������ʵ������Ĺ�Դ���������й��շ��̵ļ��㡣
    ��ˣ����һ��Ӧ�ó���ֻ��3����Դ�������ǽ�����3����Դչ�����㡣
    ���Ӧ�ó����ڲ�ͬ�׶�Ҫ֧�ֲ�ͬ�����Ĺ�Դ����ôֻ�������Բ�ͬ#define������Ĳ�ͬ��ɫ�����ɡ�
*/
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

    ��ˣ�ͨ��[B = (A^-1)^T]������[A]����ת�þ��󣩶Է��������б任��
    ����ʹ����ֱ�ھ�����[A]�任���������[uA]��ע�⣬�������[A]���������󣨼�����[A^T = A^-1]����
    ��ô[B = (A^-1)^T=(A^T)^TA]��Ҳ����˵����������������������ټ���������ת�þ���
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


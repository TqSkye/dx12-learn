//***************************************************************************************
// LightingUtil.hlsl by Frank Luna (C) 2015 All Rights Reserved.
//
// Contains API for shader lighting.
//***************************************************************************************

#define MaxLights 16

struct Light
{
    float3 Strength;    //                              光源的颜色
    float FalloffStart; // point/spot light only        仅供点光源/聚光灯光源使用
    float3 Direction;   // directional/spot light only  仅供方向光源/聚光灯光源使用
    float FalloffEnd;   // point/spot light only        仅供点光源/聚光灯光源使用
    float3 Position;    // point light only             仅供点光源/聚光灯光源使用
    float SpotPower;    // spot light only              仅供聚光灯光源使用
};

struct Material
{
    float4 DiffuseAlbedo;
    float3 FresnelR0;
    float Shininess;
};

// 实现了一种线性衰减因子的计算方法，可将其应用于点光源与聚光灯光源。
float CalcAttenuation(float d, float falloffStart, float falloffEnd)
{
    // Linear falloff.
    // 线性衰减
    return saturate((falloffEnd-d) / (falloffEnd - falloffStart));
}

// 代替菲涅耳方程的石里克近似。此函数基于光向量[L]与表面法线[n]之间的夹角，并根据菲涅耳效应近似地计算出以[n]为法线的表面所反射光的百分比。
// Schlick gives an approximation to Fresnel reflectance (see pg. 233 "Real-Time Rendering 3rd Ed.").
// R0 = ( (n-1)/(n+1) )^2, where n is the index of refraction.
// 石里克提出的一种逼近菲涅耳反射率的近似方法
// (参见"Real-Time Rendering 3rd Ed."第233页)
// R0 = ( (n-1)/(n+1) )^2，式中的n为折射率
float3 SchlickFresnel(float3 R0, float3 normal, float3 lightVec)
{
    float cosIncidentAngle = saturate(dot(normal, lightVec));

    float f0 = 1.0f - cosIncidentAngle;
    float3 reflectPercent = R0 + (1.0f - R0)*(f0*f0*f0*f0*f0);

    return reflectPercent;
}

// 计算反射到观察者眼中的光量，该值为漫反射光量与镜面反射光量的总和。
float3 BlinnPhong(float3 lightStrength, float3 lightVec, float3 normal, float3 toEye, Material mat)
{
    // m由光泽度推导而来，而光泽度则根据粗糙度求得
    const float m = mat.Shininess * 256.0f;
    float3 halfVec = normalize(toEye + lightVec);

    float roughnessFactor = (m + 8.0f)*pow(max(dot(halfVec, normal), 0.0f), m) / 8.0f;
    float3 fresnelFactor = SchlickFresnel(mat.FresnelR0, halfVec, lightVec);

    float3 specAlbedo = fresnelFactor*roughnessFactor;

    // Our spec formula goes outside [0,1] range, but we are 
    // doing LDR rendering.  So scale it down a bit.
    // 尽管我们进行的是LDR（low dynamic range，低动态范围）渲染，但spec（镜面反射）公式得到
    // 的结果仍会超出范围[0,1]，因此现将其按比例缩小一些
    specAlbedo = specAlbedo / (specAlbedo + 1.0f);
    /*
        我们采用的镜面反照率计算公式允许其镜面值大于1，这表示非常耀眼的高光。然而，我们却希望渲染目标的颜色值在[0, 1]这个低动态范围内，
        因此一般来说将高于此范围的数值简单地钳制为1.0即可。但是，为了获得更加柔和的镜面高光就不宜“一刀切”式地钳制数值了，
        而是需要按比例缩小镜面反照率：[specAlbedo = specAlbedo / (specAlbedo + 1.0f);]
        高动态范围（HDR）光照使用的是光量值可超出范围[0, 1]的浮点渲染目标，在进行色调贴图这个步骤时，出于显示的目的会将高动态范围映射回[0, 1]区间，
        而在这个转换的过程中，保留细节信息是很重要的一项任务。HDR渲染与色调映射本身就是一门单独的学科——详见教材[Reinhard10]。
        而在另一篇文章 [Pettineo12] 中也给出了比较详尽的介绍，还附有供读者用以实验的演示程序。
    */

    return (mat.DiffuseAlbedo.rgb + specAlbedo) * lightStrength;
}

//---------------------------------------------------------------------------------------
// Evaluates the lighting equation for directional lights.
//---------------------------------------------------------------------------------------
/*
    实现方向光源
    给定观察位置[E]、材质属性，与以[n]为法线的表面上可见一点[p]，
    则下列HLSL函数将输出自某方向光源发出，经上述表面以方向[v = normalize(E - p)]反射入观察者眼中的光量。
    在我们的示例中，此函数将由像素着色器所调用，以基于光照确定像素的颜色。
*/
float3 ComputeDirectionalLight(Light L, Material mat, float3 normal, float3 toEye)
{
    // The light vector aims opposite the direction the light rays travel.
    // 光向量与光线传播的方向刚好相反
    float3 lightVec = -L.Direction;

    // Scale light down by Lambert's cosine law.
    // 通过朗伯余弦定律按比例降低光强
    float ndotl = max(dot(lightVec, normal), 0.0f);
    float3 lightStrength = L.Strength * ndotl;

    return BlinnPhong(lightStrength, lightVec, normal, toEye, mat);
}

//---------------------------------------------------------------------------------------
// Evaluates the lighting equation for point lights.
//---------------------------------------------------------------------------------------
/*
    实现点光源
    给出观察点[E]、以[n]作为法线的表面上可视一点[p]以及材质属性，则下面的HLSL函数将会输出从点光源放出，
    经上述表面在[v = normalize(E - p)]方向反射入观察者眼中的光量。在我们的示例中，该函数将被像素着色器所调用，并根据光照来确定像素的颜色。
*/
float3 ComputePointLight(Light L, Material mat, float3 pos, float3 normal, float3 toEye)
{
    // The vector from the surface to the light.
    // 自表面指向光源的向量
    float3 lightVec = L.Position - pos;

    // The distance from surface to light.
    // 由表面到光源的距离
    float d = length(lightVec);

    // Range test.
    // 范围检测
    if(d > L.FalloffEnd)
        return 0.0f;

    // Normalize the light vector.
    // 对光向量进行规范化处理
    lightVec /= d;

    // Scale light down by Lambert's cosine law.
    // 通过朗伯余弦定律按比例降低光强
    float ndotl = max(dot(lightVec, normal), 0.0f);
    float3 lightStrength = L.Strength * ndotl;

    // Attenuate light by distance.
    // 根据距离计算光的衰减
    float att = CalcAttenuation(d, L.FalloffStart, L.FalloffEnd);
    lightStrength *= att;

    return BlinnPhong(lightStrength, lightVec, normal, toEye, mat);
}

//---------------------------------------------------------------------------------------
// Evaluates the lighting equation for spot lights.
//---------------------------------------------------------------------------------------
/*
    实现聚光灯光源
    指定观察点[E]、以[n]为法线的表面上可视一点[p]以及材质属性，则下面的HLSL函数将会输出来自聚光灯光源，
    经过上述表面以方向[v = normalize(E - p)]反射入观察者眼中的光量。在我们的示例中，此函数将在像素着色器中被调用，以根据光照确定像素的颜色。
*/
float3 ComputeSpotLight(Light L, Material mat, float3 pos, float3 normal, float3 toEye)
{
    // The vector from the surface to the light.
    // 从表面指向光源的向量
    float3 lightVec = L.Position - pos;

    // The distance from surface to light.
    // 由表面到光源的距离
    float d = length(lightVec);

    // Range test.
    // 范围检测
    if(d > L.FalloffEnd)
        return 0.0f;

    // Normalize the light vector.
    // 对光向量进行规范化处理
    lightVec /= d;

    // Scale light down by Lambert's cosine law.
    // 通过朗伯余弦定律按比例缩小光的强度
    float ndotl = max(dot(lightVec, normal), 0.0f);
    float3 lightStrength = L.Strength * ndotl;

    // Attenuate light by distance.
    // 根据距离计算光的衰减
    float att = CalcAttenuation(d, L.FalloffStart, L.FalloffEnd);
    lightStrength *= att;

    // Scale by spotlight
    // 根据聚光灯照明模型对光强进行缩放处理
    float spotFactor = pow(max(dot(-lightVec, L.Direction), 0.0f), L.SpotPower);
    lightStrength *= spotFactor;

    return BlinnPhong(lightStrength, lightVec, normal, toEye, mat);
}

/*
    光强是可以叠加的。因此，在支持多个光源的场景中，我们需要遍历每一个光源，并把它们在我们要计算光照的点或像素上的贡献值求和。
    示例框架最多可支持16个光源，凭此便可以用方向光、点光与聚光灯三种光源进行若干组合。当然，前提是光源的总数不能超过16个。
    此外，代码所采用的约定是方向光源必须位于光照数组的开始部分，点光源次之，聚光灯光源则排在末尾。下列代码用于计算某点处的光照方程：

    可以观察到每种类型光源的数量实由多个#define来加以控制。这样一来，着色器将仅针对实际所需的光源数量来进行光照方程的计算。
    因此，如果一个应用程序只需3个光源，则我们仅对这3个光源展开计算。
    如果应用程序在不同阶段要支持不同数量的光源，那么只需生成以不同#define来定义的不同着色器即可。
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

    因此，通过[B = (A^-1)^T]（矩阵[A]的逆转置矩阵）对法向量进行变换后，
    即可使它垂直于经矩阵[A]变换后的切向量[uA]。注意，如果矩阵[A]是正交矩阵（即满足[A^T = A^-1]），
    那么[B = (A^-1)^T=(A^T)^TA]。也就是说，在这种情况下我们无需再计算它的逆转置矩阵，
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


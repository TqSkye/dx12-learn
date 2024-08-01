//***************************************************************************************
// Default.hlsl by Frank Luna (C) 2015 All Rights Reserved.
//***************************************************************************************

// Defaults for number of lights.
#ifndef NUM_DIR_LIGHTS
    #define NUM_DIR_LIGHTS 3
#endif

#ifndef NUM_POINT_LIGHTS
    #define NUM_POINT_LIGHTS 0
#endif

#ifndef NUM_SPOT_LIGHTS
    #define NUM_SPOT_LIGHTS 0
#endif

// Include common HLSL code.
#include "Common.hlsl"

struct VertexIn
{
	float3 PosL    : POSITION;
    float3 NormalL : NORMAL;
	float2 TexC    : TEXCOORD;
	float3 TangentU : TANGENT;
};

struct VertexOut
{
	float4 PosH    : SV_POSITION;
    float3 PosW    : POSITION;
    float3 NormalW : NORMAL;
	float3 TangentW : TANGENT;
	float2 TexC    : TEXCOORD;
};

VertexOut VS(VertexIn vin)
{
	VertexOut vout = (VertexOut)0.0f;

	// Fetch the material data.
    // 获取材质数据
	MaterialData matData = gMaterialData[gMaterialIndex];
	
    // Transform to world space.
    // 把顶点变换到世界空间
    float4 posW = mul(float4(vin.PosL, 1.0f), gWorld);
    vout.PosW = posW.xyz;

    // Assumes nonuniform scaling; otherwise, need to use inverse-transpose of world matrix.
    // 假设这里执行的是等比缩放，否则就需要使用世界矩阵的逆转置矩阵进行变换
    vout.NormalW = mul(vin.NormalL, (float3x3)gWorld);
	
	vout.TangentW = mul(vin.TangentU, (float3x3)gWorld);

    // Transform to homogeneous clip space.
    // 将顶点变换到齐次裁剪空间
    vout.PosH = mul(posW, gViewProj);
	
	// Output vertex attributes for interpolation across triangle.
    // 为三角形插值而输出顶点属性
	float4 texC = mul(float4(vin.TexC, 0.0f, 1.0f), gTexTransform);
	vout.TexC = mul(texC, matData.MatTransform).xy;
	
    return vout;
}

float4 PS(VertexOut pin) : SV_Target
{
	// Fetch the material data.
    // 获取材质数据
	MaterialData matData = gMaterialData[gMaterialIndex];
	float4 diffuseAlbedo = matData.DiffuseAlbedo;
	float3 fresnelR0 = matData.FresnelR0;
	float  roughness = matData.Roughness;
	uint diffuseMapIndex = matData.DiffuseMapIndex;
	uint normalMapIndex = matData.NormalMapIndex;
	
	// Interpolating normal can unnormalize it, so renormalize it.
    // 对法线插值可能使它非规范化，因此要再次对它进行规范化处理
    pin.NormalW = normalize(pin.NormalW);
	
	float4 normalMapSample = gTextureMaps[normalMapIndex].Sample(gsamAnisotropicWrap, pin.TexC);
	float3 bumpedNormalW = NormalSampleToWorldSpace(normalMapSample.rgb, pin.NormalW, pin.TangentW);

	// Uncomment to turn off normal mapping.
    // 去掉下列注释则禁用法线贴图
	//bumpedNormalW = pin.NormalW;

	// Dynamically look up the texture in the array.
    // 动态查找数组中的纹理
	diffuseAlbedo *= gTextureMaps[diffuseMapIndex].Sample(gsamAnisotropicWrap, pin.TexC);

    // Vector from point being lit to eye. 
    // 由表面上某点指向观察点的向量
    float3 toEyeW = normalize(gEyePosW - pin.PosW);

    // Light terms.
    // 光照项
    float4 ambient = gAmbientLight*diffuseAlbedo;

    /*
        可以看出，那些名为“凹凸法线（bumped normal）”的向量既可用于光照计算，又能用在以环境图模拟反射的反射效果计算当中。
        另外，我们在法线图的alpha通道存储的是光泽度掩码（shininess mask），它控制着逐像素级别上物体表面的光泽度（见图19.7）。
    */
    // Alpha通道存储的是逐像素级别上的光泽度
    const float shininess = (1.0f - roughness) * normalMapSample.a;
    Material mat = { diffuseAlbedo, fresnelR0, shininess };
    float3 shadowFactor = 1.0f;
    float4 directLight = ComputeLighting(gLights, mat, pin.PosW,
        bumpedNormalW, toEyeW, shadowFactor);

    float4 litColor = ambient + directLight;

	// Add in specular reflections.
    // 利用法线图中的法线计算镜面反射
	float3 r = reflect(-toEyeW, bumpedNormalW);
	float4 reflectionColor = gCubeMap.Sample(gsamLinearWrap, r);
	float3 fresnelFactor = SchlickFresnel(fresnelR0, bumpedNormalW, r);
    litColor.rgb += shininess * fresnelFactor * reflectionColor.rgb;
	
    // Common convention to take alpha from diffuse albedo.
    // 从漫反射反照率中获取alpha 值的常规方法
    litColor.a = diffuseAlbedo.a;

    return litColor;
}



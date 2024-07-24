//***************************************************************************************
// Default.hlsl by Frank Luna (C) 2015 All Rights Reserved.
//***************************************************************************************

// Defaults for number of lights.
// 默认的光源数量
#ifndef NUM_DIR_LIGHTS
    #define NUM_DIR_LIGHTS 3
#endif

#ifndef NUM_POINT_LIGHTS
    #define NUM_POINT_LIGHTS 0
#endif

#ifndef NUM_SPOT_LIGHTS
    #define NUM_SPOT_LIGHTS 0
#endif

// Include structures and functions for lighting.
// 包含了光照所用的结构体和函数
#include "LightingUtil.hlsl"

// 通过下列HLSL语法来定义纹理对象，并将其分配给特定的纹理寄存器：
Texture2D    gDiffuseMap : register(t0);

// 注意，纹理寄存器由tn来标定，其中，整数n表示的是纹理寄存器的槽号。此根签名的定义指出了由槽位参数到着色器寄存器的映射关系，
// 这便是应用程序代码能将SRV绑定到着色器中特定Texture2D对象的原因。类似地，下列HLSL语法定义了多个采样器对象，并将它们分别分配到了特定的采样器寄存器。
// 这些采样器对应于我们在上一节中所配置的静态采样器数组。注意，采样器寄存器由sn来指定，其中整数n表示的是采样器寄存器的槽号。
SamplerState gsamPointWrap        : register(s0);
SamplerState gsamPointClamp       : register(s1);
SamplerState gsamLinearWrap       : register(s2);
SamplerState gsamLinearClamp      : register(s3);
SamplerState gsamAnisotropicWrap  : register(s4);
SamplerState gsamAnisotropicClamp : register(s5);

// Constant data that varies per frame.
// 每一帧都有变化的常量数据
cbuffer cbPerObject : register(b0)
{
    float4x4 gWorld;
	float4x4 gTexTransform;
};

// Constant data that varies per material.
// 绘制过程中所用的杂项常量数据
cbuffer cbPass : register(b1)
{
    float4x4 gView;
    float4x4 gInvView;
    float4x4 gProj;
    float4x4 gInvProj;
    float4x4 gViewProj;
    float4x4 gInvViewProj;
    float3 gEyePosW;
    float cbPerObjectPad1;
    float2 gRenderTargetSize;
    float2 gInvRenderTargetSize;
    float gNearZ;
    float gFarZ;
    float gTotalTime;
    float gDeltaTime;
    float4 gAmbientLight;

    // Indices [0, NUM_DIR_LIGHTS) are directional lights;
    // indices [NUM_DIR_LIGHTS, NUM_DIR_LIGHTS+NUM_POINT_LIGHTS) are point lights;
    // indices [NUM_DIR_LIGHTS+NUM_POINT_LIGHTS, NUM_DIR_LIGHTS+NUM_POINT_LIGHT+NUM_SPOT_LIGHTS)
    // are spot lights for a maximum of MaxLights per object.
    // 对于每个以MaxLights为光源数量最大值的对象而言，索引[0,NUM_DIR_LIGHTS)表示的是方向光
    // 源，索引[NUM_DIR_LIGHTS, NUM_DIR_LIGHTS+NUM_POINT_LIGHTS)表示的是点光源
    // 索引[NUM_DIR_LIGHTS+NUM_POINT_LIGHTS, NUM_DIR_LIGHTS+NUM_POINT_LIGHT+NUM_SPOT_ LIGHTS)表示的是聚光灯光源
    Light gLights[MaxLights];
};

// 每种材质都有所区别的常量数据
cbuffer cbMaterial : register(b2)
{
	float4   gDiffuseAlbedo;
    float3   gFresnelR0;
    float    gRoughness;
	float4x4 gMatTransform;
};

struct VertexIn
{
	float3 PosL    : POSITION;
    float3 NormalL : NORMAL;
	float2 TexC    : TEXCOORD;
};

struct VertexOut
{
	float4 PosH    : SV_POSITION;
    float3 PosW    : POSITION;
    float3 NormalW : NORMAL;
	float2 TexC    : TEXCOORD;
};

VertexOut VS(VertexIn vin)
{
	VertexOut vout = (VertexOut)0.0f;
	
    // Transform to world space.
    // 把坐标变换到世界空间
    float4 posW = mul(float4(vin.PosL, 1.0f), gWorld);
    vout.PosW = posW.xyz;

    // Assumes nonuniform scaling; otherwise, need to use inverse-transpose of world matrix.
    // 假设这里正在进行的是等比缩放，否则便需要使用世界矩阵的逆转置矩阵
    vout.NormalW = mul(vin.NormalL, (float3x3)gWorld);

    // Transform to homogeneous clip space.
    // 将顶点变换到齐次裁剪空间
    vout.PosH = mul(posW, gViewProj);
	
    /*
        我们还未曾讨论过常量缓冲区变量gTexTransform与gMatTransform。这两个变量用于在顶点着色器中对输入的纹理坐标进行变换：
        // 为了对三角形进行插值操作而输出的顶点属性
    	float4 texC = mul(float4(vin.TexC, 0.0f, 1.0f), gTexTransform);
	    vout.TexC = mul(texC, gMatTransform).xy;
        纹理坐标表示的是纹理平面中的2D点。有了这种坐标，我们就能像其他的2D点一样，对纹理中的样点进行缩放、平移与旋转。下面是一些适用于纹理变换的示例：
        1．令砖块纹理随着一堵墙的模型而拉伸。假设此墙顶点的当前纹理坐标范围为[0, 1]。在将该纹理坐标放大4倍使它们变换到范围[0, 4]时，该砖块纹理将沿着墙面重复贴图[4 x 4]次。
        2．假设有许多云朵纹理绵延在万里无云的碧空背景之下，那么，通过随着时间函数来平移这些纹理的坐标，便能实现动态的白云浮过蔚蓝天空的效果。
        3．纹理的旋转操作有时也便于实现一些类似于粒子的效果。例如，可以随着时间的推移而令火球纹理进行旋转。
    
        注意，变换2D纹理坐标要利用[4 x 4]矩阵，因此我们先将它扩充为一个4D向量：
        vin.TexC ---> float4(vin.TexC, 0.0f, 1.0f)
        在完成乘法运算之后，从得到的4D向量中去掉[z]分量与[w]分量，使之强制转换回2D向量，即：
    	float4 texC = mul(float4(vin.TexC, 0.0f, 1.0f), gTexTransform);
	    vout.TexC = mul(texC, gMatTransform).xy;
    
        在此，我们运用了两个独立的纹理变换矩阵gTexTransform与gMatTransform，这样做是因为一种是关于材质的纹理变换（针对像水那样的动态材质），
        另一种是关于物体属性的纹理变换。由于这里使用的是2D纹理坐标，所以我们只关心前两个坐标轴的变换情况。
        例如，如果纹理矩阵平移了[z]坐标，这并不会对纹理坐标造成任何影响。
    */
    
	// Output vertex attributes for interpolation across triangle.
    // 为了对三角形进行插值操作而输出的顶点属性
	float4 texC = mul(float4(vin.TexC, 0.0f, 1.0f), gTexTransform);
	vout.TexC = mul(texC, gMatTransform).xy;
	
    return vout;
}

float4 PS(VertexOut pin) : SV_Target
{
    // 我们通过向Sample方法的第一个参数传递SamplerState对象来描述如何对纹理数据进行采样，再向第二个参数传递像素的纹理坐标(u,v)。
    // 这个方法将利用SamplerState对象所指定的过滤方法，返回纹理图在点(u,v)处的插值颜色。
    // 从纹理中提取此像素的漫反射反照率并将纹理样本与常量缓存区中的反照率相乘
    float4 diffuseAlbedo = gDiffuseMap.Sample(gsamAnisotropicWrap, pin.TexC) * gDiffuseAlbedo;
	
    // Interpolating normal can unnormalize it, so renormalize it.
    // 对法线插值可能使之非规范化，因此要对它再次进行规范化处理
    pin.NormalW = normalize(pin.NormalW);

    // Vector from point being lit to eye. 
    // 光线经表面上一点反射到观察点这一方向上的向量
    float3 toEyeW = normalize(gEyePosW - pin.PosW);

    // Light terms.
    // 光照项
    float4 ambient = gAmbientLight*diffuseAlbedo;

    const float shininess = 1.0f - gRoughness;
    Material mat = { diffuseAlbedo, gFresnelR0, shininess };
    float3 shadowFactor = 1.0f;
    float4 directLight = ComputeLighting(gLights, mat, pin.PosW,
        pin.NormalW, toEyeW, shadowFactor);

    float4 litColor = ambient + directLight;

    // Common convention to take alpha from diffuse albedo.
    // 从漫反射反照率获取alpha值的常见手段
    litColor.a = diffuseAlbedo.a;

    return litColor;
}



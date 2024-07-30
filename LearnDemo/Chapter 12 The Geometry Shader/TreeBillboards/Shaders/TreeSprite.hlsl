//***************************************************************************************
// TreeSprite.hlsl by Frank Luna (C) 2015 All Rights Reserved.
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

// Include structures and functions for lighting.
// 包含光照所需的结构体与函数
#include "LightingUtil.hlsl"

Texture2DArray gTreeMapArray : register(t0);


SamplerState gsamPointWrap        : register(s0);
SamplerState gsamPointClamp       : register(s1);
SamplerState gsamLinearWrap       : register(s2);
SamplerState gsamLinearClamp      : register(s3);
SamplerState gsamAnisotropicWrap  : register(s4);
SamplerState gsamAnisotropicClamp : register(s5);

// Constant data that varies per frame.
// 每一帧都在变化的常量数据
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

	float4 gFogColor;
	float gFogStart;
	float gFogRange;
	float2 cbPerObjectPad2;

    // Indices [0, NUM_DIR_LIGHTS) are directional lights;
    // indices [NUM_DIR_LIGHTS, NUM_DIR_LIGHTS+NUM_POINT_LIGHTS) are point lights;
    // indices [NUM_DIR_LIGHTS+NUM_POINT_LIGHTS, NUM_DIR_LIGHTS+NUM_POINT_LIGHT+NUM_SPOT_LIGHTS)
    // are spot lights for a maximum of MaxLights per object.
    // 对于每个以MaxLights为光源数量最大值的对象来说，索引[0,NUM_DIR_LIGHTS)表示的是方向光
    // 源，索引[NUM_DIR_LIGHTS, NUM_DIR_LIGHTS+NUM_POINT_LIGHTS]表示的是点光源
    // 索引[NUM_DIR_LIGHTS+NUM_POINT_LIGHTS, NUM_DIR_LIGHTS+NUM_POINT_LIGHT+
    // NUM_SPOT_LIGHTS)表示的则为聚光灯光源
    Light gLights[MaxLights];
};

// 每种材质都各有区别的常量数据
cbuffer cbMaterial : register(b2)
{
	float4   gDiffuseAlbedo;
    float3   gFresnelR0;
    float    gRoughness;
	float4x4 gMatTransform;
};
 
struct VertexIn
{
	float3 PosW  : POSITION;
	float2 SizeW : SIZE;
};

struct VertexOut
{
	float3 CenterW : POSITION;
	float2 SizeW   : SIZE;
};

struct GeoOut
{
	float4 PosH    : SV_POSITION;
    float3 PosW    : POSITION;
    float3 NormalW : NORMAL;
    float2 TexC    : TEXCOORD;
    uint   PrimID  : SV_PrimitiveID;
};

VertexOut VS(VertexIn vin)
{
	VertexOut vout;

	// Just pass data over to geometry shader.
    // 直接将数据传入几何着色器
	vout.CenterW = vin.PosW;
	vout.SizeW   = vin.SizeW;

	return vout;
}
 
 /*
    几何着色器的编写方式比较接近于顶点着色器和像素着色器，当然也存在若干区别。下列代码展示了几何着色器的一般编写格式：

    [maxvertexcount(N)]
    void ShaderName ( 
     PrimitiveType InputVertexType InputName[NumElements], 
     inout StreamOutputObject<OutputVertexType> OutputName)
    {
        // 几何着色器的具体实现
    }

    我们必须先指定几何着色器单次调用所输出的顶点数量最大值（每个图元都会调用一次几何着色器，走一遍其中的处理流程）。对此，可以使用下列属性语法来设置着色器定义之前的最大顶点数量：
        [maxvertexcount(N)]
    其中，N是几何着色器单次调用所输出的顶点数量最大值。几何着色器每次输出的顶点个数都可能各不相同，但是这个数量却不能超过之前定义的最大值。
    出于对性能方面的考量，我们应当令maxvertexcount的值尽可能地小。相关资料显示[NVIDIA08]，在GS（即几何着色器的缩写，geometry shader）
    每次输出的标量数量在1～20时，它将发挥出最佳的性能；而当GS每次输出的标量数量保持在27～40时，它的性能将下降到峰值性能的50%。
    每次调用几何着色器所输出的标量个数为：maxvertexcount与输出顶点类型结构体中标量个数的乘积[1]。

    几何着色器的输入顶点类型即为顶点着色器输出的顶点类型（例如VertexOut）。输入参数一定要以图元类型作为前缀，用以描述输入到几何着色器的具体图元类型。
    该前缀可以是下列类型之一：
        1．point：输入的图元为点。
        2．line：输入的图元为线列表或线条带。
        3．triangle：输入的图元为三角形列表或三角形带。
        4．lineadj：输入的图元为线列表及其邻接图元，或线条带及其邻接图元。
        5．triangleadj：输入的图元为三角形列表及其邻接图元，或三角形带及其邻接图元。

    输出参数一定要标有inout修饰符。另外，它必须是一种流类型（stream type。即某种类型的流输出对象）。
    流类型存有一系列顶点，它们定义了几何着色器输出的几何图形。几何着色器可以通过内置方法Append向输出流列表添加单个顶点：
        void StreamOutputObject<OutputVertexType>::Append(OutputVertexType v);
    流类型本质上是一种模板类型（template type），其模板参数用以指定输出顶点的具体类型（如GeoOut）。流类型有如下3种。
        1．PointStream<OutputVertexType>：一系列顶点所定义的点列表。
        2．LineStream<OutputVertexType>：一系列顶点所定义的线条带。
        3．TriangleStream<OutputVertexType>：一系列顶点所定义的三角形带。

    SV_PrimitiveID语义:
    若指定了该语义，则输入装配器阶段会自动为每个图元生成图元ID。在绘制(n)个图元的调用执行过程中，第一个图元被标记为0，第二个图元被标识为1，并以此类推，
    直到此绘制调用过程中最后一个图元被标记为(n - 1)为止。对于单次绘制调用来说，其中的图元ID都是唯一的。在公告牌示例中，几何着色器不会用到此ID（尽管它可以使用），
    它仅将图元ID写入到输出的顶点之中，以此将它们传递到像素着色器阶段。而像素着色器则把图元ID用作纹理数组的索引，这是下一小节将讲述的内容。

*/

 // We expand each point into a quad (4 vertices), so the maximum number of vertices
 // we output per geometry shader invocation is 4.
 // 由于我们要将每个点都扩展为一个四边形（即4个顶点），因此每次调用几何着色器最多输出4个顶点
[maxvertexcount(4)]
void GS(point VertexOut gin[1], 
        uint primID : SV_PrimitiveID, 
        inout TriangleStream<GeoOut> triStream)
{	
	//
	// Compute the local coordinate system of the sprite relative to the world
	// space such that the billboard is aligned with the y-axis and faces the eye.
	// 计算精灵[6]的局部坐标系与世界空间的相对关系，以使公告牌与y轴对齐且面向观察者
    //
    
	float3 up = float3(0.0f, 1.0f, 0.0f);
	float3 look = gEyePosW - gin[0].CenterW;
    look.y = 0.0f; // y-axis aligned, so project to xz-plane // 与y轴对齐，以此使公告牌立于xz平面
	look = normalize(look);
	float3 right = cross(up, look);

	//
	// Compute triangle strip vertices (quad) in world space.
    // 计算世界空间中三角形带的顶点（即四边形）
	//
	float halfWidth  = 0.5f*gin[0].SizeW.x;
	float halfHeight = 0.5f*gin[0].SizeW.y;
	
	float4 v[4];
	v[0] = float4(gin[0].CenterW + halfWidth*right - halfHeight*up, 1.0f);
	v[1] = float4(gin[0].CenterW + halfWidth*right + halfHeight*up, 1.0f);
	v[2] = float4(gin[0].CenterW - halfWidth*right - halfHeight*up, 1.0f);
	v[3] = float4(gin[0].CenterW - halfWidth*right + halfHeight*up, 1.0f);

	//
	// Transform quad vertices to world space and output 
	// them as a triangle strip.
    // 将四边形的顶点变换到世界空间，并将它们以三角形带的形式输出
	//
	
	float2 texC[4] = 
	{
		float2(0.0f, 1.0f),
		float2(0.0f, 0.0f),
		float2(1.0f, 1.0f),
		float2(1.0f, 0.0f)
	};
	
	GeoOut gout;
	[unroll]
	for(int i = 0; i < 4; ++i)
	{
		gout.PosH     = mul(v[i], gViewProj);
		gout.PosW     = v[i].xyz;
		gout.NormalW  = look;
		gout.TexC     = texC[i];
		gout.PrimID   = primID;
		
		triStream.Append(gout);
	}
}

/*
    3.纹理数组: 概述:
        顾名思义，纹理数组即为存放纹理的数组。就像所有的资源（纹理与缓冲区）一样，在C++代码中，纹理数组也由ID3D12Resource接口来表示。
        当创建ID3D12Resource对象时，可以通过设置DepthOrArraySize属性来指定纹理数组所存储的元素个数（对于3D纹理来说，此项设定的则为深度值）。
        我们在d3dApp.cpp文件中创建深度/模板纹理时，总是将该值设为1。如果查看Common/DDSTextureLoader.cpp中的CreateD3DResources12函数，
        便会明白这些代码究竟是如何来创建纹理数组与体纹理（volume texture）的。在HLSL文件中，纹理数组是通过Texture2DArray类型来表示的：
            Texture2DArray gTreeMapArray;
        现在，我们必须搞清楚为什么要使用纹理数组，而不是像下面那样做：
            Texture2D TexArray[4];
            ...
            float4 PS(GeoOut pin) : SV_Target
            {
                float4 c = TexArray[pin.PrimID%4].Sample(samLinear, pin.Tex);
        在着色器模型5.1（Direct3D 12所对应的新模型版本）中确实可以这样写，但这在之前的Direct3D版本中却是行不通的。
        再者，以这种方式来索引纹理可能会依具体硬件而产生少量开销，所以本章我们将使用纹理数组。
*/
float4 PS(GeoOut pin) : SV_Target
{
    // 对纹理数组采样
	float3 uvw = float3(pin.TexC, pin.PrimID%3);
    float4 diffuseAlbedo = gTreeMapArray.Sample(gsamAnisotropicWrap, uvw) * gDiffuseAlbedo;
	
#ifdef ALPHA_TEST
	// Discard pixel if texture alpha < 0.1.  We do this test as soon 
	// as possible in the shader so that we can potentially exit the
	// shader early, thereby skipping the rest of the shader code.
    // 忽略纹理alpha值 < 0.1的像素。这个测试要尽早完成，以便提前退出着色器，使满足此条件的像素
    // 跳过着色器中不必要的后续处理流程
	clip(diffuseAlbedo.a - 0.1f);
#endif

    // Interpolating normal can unnormalize it, so renormalize it.
    // 对法线插值可能导致其非规范化，因此需再次对它进行规范化处理
    pin.NormalW = normalize(pin.NormalW);

    // Vector from point being lit to eye. 
    // 光线经表面上一点反射到观察点这一方向上的向量
	float3 toEyeW = gEyePosW - pin.PosW;
	float distToEye = length(toEyeW);
    toEyeW /= distToEye; // normalize // 规范化处理

    // Light terms.
    // 光照项
    float4 ambient = gAmbientLight*diffuseAlbedo;

    const float shininess = 1.0f - gRoughness;
    Material mat = { diffuseAlbedo, gFresnelR0, shininess };
    float3 shadowFactor = 1.0f;
    float4 directLight = ComputeLighting(gLights, mat, pin.PosW,
        pin.NormalW, toEyeW, shadowFactor);

    float4 litColor = ambient + directLight;

#ifdef FOG
	float fogAmount = saturate((distToEye - gFogStart) / gFogRange);
	litColor = lerp(litColor, gFogColor, fogAmount);
#endif

    // Common convention to take alpha from diffuse albedo.
    // 从漫射反照率中获取alpha值的常用手段
    litColor.a = diffuseAlbedo.a;

    return litColor;
}



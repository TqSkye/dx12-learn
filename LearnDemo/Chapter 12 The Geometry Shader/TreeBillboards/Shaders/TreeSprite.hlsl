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
// ������������Ľṹ���뺯��
#include "LightingUtil.hlsl"

Texture2DArray gTreeMapArray : register(t0);


SamplerState gsamPointWrap        : register(s0);
SamplerState gsamPointClamp       : register(s1);
SamplerState gsamLinearWrap       : register(s2);
SamplerState gsamLinearClamp      : register(s3);
SamplerState gsamAnisotropicWrap  : register(s4);
SamplerState gsamAnisotropicClamp : register(s5);

// Constant data that varies per frame.
// ÿһ֡���ڱ仯�ĳ�������
cbuffer cbPerObject : register(b0)
{
    float4x4 gWorld;
	float4x4 gTexTransform;
};

// Constant data that varies per material.
// ���ƹ��������õ����������
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
    // ����ÿ����MaxLightsΪ��Դ�������ֵ�Ķ�����˵������[0,NUM_DIR_LIGHTS)��ʾ���Ƿ����
    // Դ������[NUM_DIR_LIGHTS, NUM_DIR_LIGHTS+NUM_POINT_LIGHTS]��ʾ���ǵ��Դ
    // ����[NUM_DIR_LIGHTS+NUM_POINT_LIGHTS, NUM_DIR_LIGHTS+NUM_POINT_LIGHT+
    // NUM_SPOT_LIGHTS)��ʾ����Ϊ�۹�ƹ�Դ
    Light gLights[MaxLights];
};

// ÿ�ֲ��ʶ���������ĳ�������
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
    // ֱ�ӽ����ݴ��뼸����ɫ��
	vout.CenterW = vin.PosW;
	vout.SizeW   = vin.SizeW;

	return vout;
}
 
 /*
    ������ɫ���ı�д��ʽ�ȽϽӽ��ڶ�����ɫ����������ɫ������ȻҲ���������������д���չʾ�˼�����ɫ����һ���д��ʽ��

    [maxvertexcount(N)]
    void ShaderName ( 
     PrimitiveType InputVertexType InputName[NumElements], 
     inout StreamOutputObject<OutputVertexType> OutputName)
    {
        // ������ɫ���ľ���ʵ��
    }

    ���Ǳ�����ָ��������ɫ�����ε���������Ķ����������ֵ��ÿ��ͼԪ�������һ�μ�����ɫ������һ�����еĴ������̣����Դˣ�����ʹ�����������﷨��������ɫ������֮ǰ����󶥵�������
        [maxvertexcount(N)]
    ���У�N�Ǽ�����ɫ�����ε���������Ķ����������ֵ��������ɫ��ÿ������Ķ�����������ܸ�����ͬ�������������ȴ���ܳ���֮ǰ��������ֵ��
    ���ڶ����ܷ���Ŀ���������Ӧ����maxvertexcount��ֵ�����ܵ�С�����������ʾ[NVIDIA08]����GS����������ɫ������д��geometry shader��
    ÿ������ı���������1��20ʱ���������ӳ���ѵ����ܣ�����GSÿ������ı�������������27��40ʱ���������ܽ��½�����ֵ���ܵ�50%��
    ÿ�ε��ü�����ɫ��������ı�������Ϊ��maxvertexcount������������ͽṹ���б��������ĳ˻�[1]��

    ������ɫ�������붥�����ͼ�Ϊ������ɫ������Ķ������ͣ�����VertexOut�����������һ��Ҫ��ͼԪ������Ϊǰ׺�������������뵽������ɫ���ľ���ͼԪ���͡�
    ��ǰ׺��������������֮һ��
        1��point�������ͼԪΪ�㡣
        2��line�������ͼԪΪ���б����������
        3��triangle�������ͼԪΪ�������б�������δ���
        4��lineadj�������ͼԪΪ���б����ڽ�ͼԪ���������������ڽ�ͼԪ��
        5��triangleadj�������ͼԪΪ�������б����ڽ�ͼԪ���������δ������ڽ�ͼԪ��

    �������һ��Ҫ����inout���η������⣬��������һ�������ͣ�stream type����ĳ�����͵���������󣩡�
    �����ʹ���һϵ�ж��㣬���Ƕ����˼�����ɫ������ļ���ͼ�Ρ�������ɫ������ͨ�����÷���Append��������б���ӵ������㣺
        void StreamOutputObject<OutputVertexType>::Append(OutputVertexType v);
    �����ͱ�������һ��ģ�����ͣ�template type������ģ���������ָ���������ľ������ͣ���GeoOut����������������3�֡�
        1��PointStream<OutputVertexType>��һϵ�ж���������ĵ��б�
        2��LineStream<OutputVertexType>��һϵ�ж������������������
        3��TriangleStream<OutputVertexType>��һϵ�ж���������������δ���

    SV_PrimitiveID����:
    ��ָ���˸����壬������װ�����׶λ��Զ�Ϊÿ��ͼԪ����ͼԪID���ڻ���(n)��ͼԪ�ĵ���ִ�й����У���һ��ͼԪ�����Ϊ0���ڶ���ͼԪ����ʶΪ1�����Դ����ƣ�
    ֱ���˻��Ƶ��ù��������һ��ͼԪ�����Ϊ(n - 1)Ϊֹ�����ڵ��λ��Ƶ�����˵�����е�ͼԪID����Ψһ�ġ��ڹ�����ʾ���У�������ɫ�������õ���ID������������ʹ�ã���
    ������ͼԪIDд�뵽����Ķ���֮�У��Դ˽����Ǵ��ݵ�������ɫ���׶Ρ���������ɫ�����ͼԪID�������������������������һС�ڽ����������ݡ�

*/

 // We expand each point into a quad (4 vertices), so the maximum number of vertices
 // we output per geometry shader invocation is 4.
 // ��������Ҫ��ÿ���㶼��չΪһ���ı��Σ���4�����㣩�����ÿ�ε��ü�����ɫ��������4������
[maxvertexcount(4)]
void GS(point VertexOut gin[1], 
        uint primID : SV_PrimitiveID, 
        inout TriangleStream<GeoOut> triStream)
{	
	//
	// Compute the local coordinate system of the sprite relative to the world
	// space such that the billboard is aligned with the y-axis and faces the eye.
	// ���㾫��[6]�ľֲ�����ϵ������ռ����Թ�ϵ����ʹ��������y�����������۲���
    //
    
	float3 up = float3(0.0f, 1.0f, 0.0f);
	float3 look = gEyePosW - gin[0].CenterW;
    look.y = 0.0f; // y-axis aligned, so project to xz-plane // ��y����룬�Դ�ʹ����������xzƽ��
	look = normalize(look);
	float3 right = cross(up, look);

	//
	// Compute triangle strip vertices (quad) in world space.
    // ��������ռ��������δ��Ķ��㣨���ı��Σ�
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
    // ���ı��εĶ���任������ռ䣬���������������δ�����ʽ���
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
    3.��������: ����:
        ����˼�壬�������鼴Ϊ�����������顣�������е���Դ�������뻺������һ������C++�����У���������Ҳ��ID3D12Resource�ӿ�����ʾ��
        ������ID3D12Resource����ʱ������ͨ������DepthOrArraySize������ָ�������������洢��Ԫ�ظ���������3D������˵�������趨����Ϊ���ֵ����
        ������d3dApp.cpp�ļ��д������/ģ������ʱ�����ǽ���ֵ��Ϊ1������鿴Common/DDSTextureLoader.cpp�е�CreateD3DResources12������
        ���������Щ���뾿�����������������������������volume texture���ġ���HLSL�ļ��У�����������ͨ��Texture2DArray��������ʾ�ģ�
            Texture2DArray gTreeMapArray;
        ���ڣ����Ǳ�������ΪʲôҪʹ���������飬��������������������
            Texture2D TexArray[4];
            ...
            float4 PS(GeoOut pin) : SV_Target
            {
                float4 c = TexArray[pin.PrimID%4].Sample(samLinear, pin.Tex);
        ����ɫ��ģ��5.1��Direct3D 12����Ӧ����ģ�Ͱ汾����ȷʵ��������д��������֮ǰ��Direct3D�汾��ȴ���в�ͨ�ġ�
        ���ߣ������ַ�ʽ������������ܻ�������Ӳ���������������������Ա������ǽ�ʹ���������顣
*/
float4 PS(GeoOut pin) : SV_Target
{
    // �������������
	float3 uvw = float3(pin.TexC, pin.PrimID%3);
    float4 diffuseAlbedo = gTreeMapArray.Sample(gsamAnisotropicWrap, uvw) * gDiffuseAlbedo;
	
#ifdef ALPHA_TEST
	// Discard pixel if texture alpha < 0.1.  We do this test as soon 
	// as possible in the shader so that we can potentially exit the
	// shader early, thereby skipping the rest of the shader code.
    // ��������alphaֵ < 0.1�����ء��������Ҫ������ɣ��Ա���ǰ�˳���ɫ����ʹ���������������
    // ������ɫ���в���Ҫ�ĺ�����������
	clip(diffuseAlbedo.a - 0.1f);
#endif

    // Interpolating normal can unnormalize it, so renormalize it.
    // �Է��߲�ֵ���ܵ�����ǹ淶����������ٴζ������й淶������
    pin.NormalW = normalize(pin.NormalW);

    // Vector from point being lit to eye. 
    // ���߾�������һ�㷴�䵽�۲����һ�����ϵ�����
	float3 toEyeW = gEyePosW - pin.PosW;
	float distToEye = length(toEyeW);
    toEyeW /= distToEye; // normalize // �淶������

    // Light terms.
    // ������
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
    // �����䷴�����л�ȡalphaֵ�ĳ����ֶ�
    litColor.a = diffuseAlbedo.a;

    return litColor;
}



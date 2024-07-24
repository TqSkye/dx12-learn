//***************************************************************************************
// Default.hlsl by Frank Luna (C) 2015 All Rights Reserved.
//***************************************************************************************

// Defaults for number of lights.
// Ĭ�ϵĹ�Դ����
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
// �����˹������õĽṹ��ͺ���
#include "LightingUtil.hlsl"

// ͨ������HLSL�﷨������������󣬲����������ض�������Ĵ�����
Texture2D    gDiffuseMap : register(t0);

// ע�⣬����Ĵ�����tn���궨�����У�����n��ʾ��������Ĵ����Ĳۺš��˸�ǩ���Ķ���ָ�����ɲ�λ��������ɫ���Ĵ�����ӳ���ϵ��
// �����Ӧ�ó�������ܽ�SRV�󶨵���ɫ�����ض�Texture2D�����ԭ�����Ƶأ�����HLSL�﷨�����˶�����������󣬲������Ƿֱ���䵽���ض��Ĳ������Ĵ�����
// ��Щ��������Ӧ����������һ���������õľ�̬���������顣ע�⣬�������Ĵ�����sn��ָ������������n��ʾ���ǲ������Ĵ����Ĳۺš�
SamplerState gsamPointWrap        : register(s0);
SamplerState gsamPointClamp       : register(s1);
SamplerState gsamLinearWrap       : register(s2);
SamplerState gsamLinearClamp      : register(s3);
SamplerState gsamAnisotropicWrap  : register(s4);
SamplerState gsamAnisotropicClamp : register(s5);

// Constant data that varies per frame.
// ÿһ֡���б仯�ĳ�������
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

    // Indices [0, NUM_DIR_LIGHTS) are directional lights;
    // indices [NUM_DIR_LIGHTS, NUM_DIR_LIGHTS+NUM_POINT_LIGHTS) are point lights;
    // indices [NUM_DIR_LIGHTS+NUM_POINT_LIGHTS, NUM_DIR_LIGHTS+NUM_POINT_LIGHT+NUM_SPOT_LIGHTS)
    // are spot lights for a maximum of MaxLights per object.
    // ����ÿ����MaxLightsΪ��Դ�������ֵ�Ķ�����ԣ�����[0,NUM_DIR_LIGHTS)��ʾ���Ƿ����
    // Դ������[NUM_DIR_LIGHTS, NUM_DIR_LIGHTS+NUM_POINT_LIGHTS)��ʾ���ǵ��Դ
    // ����[NUM_DIR_LIGHTS+NUM_POINT_LIGHTS, NUM_DIR_LIGHTS+NUM_POINT_LIGHT+NUM_SPOT_ LIGHTS)��ʾ���Ǿ۹�ƹ�Դ
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
    // ������任������ռ�
    float4 posW = mul(float4(vin.PosL, 1.0f), gWorld);
    vout.PosW = posW.xyz;

    // Assumes nonuniform scaling; otherwise, need to use inverse-transpose of world matrix.
    // �����������ڽ��е��ǵȱ����ţ��������Ҫʹ������������ת�þ���
    vout.NormalW = mul(vin.NormalL, (float3x3)gWorld);

    // Transform to homogeneous clip space.
    // ������任����βü��ռ�
    vout.PosH = mul(posW, gViewProj);
	
    /*
        ���ǻ�δ�����۹���������������gTexTransform��gMatTransform�����������������ڶ�����ɫ���ж����������������б任��
        // Ϊ�˶������ν��в�ֵ����������Ķ�������
    	float4 texC = mul(float4(vin.TexC, 0.0f, 1.0f), gTexTransform);
	    vout.TexC = mul(texC, gMatTransform).xy;
        ���������ʾ��������ƽ���е�2D�㡣�����������꣬���Ǿ�����������2D��һ�����������е�����������š�ƽ������ת��������һЩ����������任��ʾ����
        1����ש����������һ��ǽ��ģ�Ͷ����졣�����ǽ����ĵ�ǰ�������귶ΧΪ[0, 1]���ڽ�����������Ŵ�4��ʹ���Ǳ任����Χ[0, 4]ʱ����ש����������ǽ���ظ���ͼ[4 x 4]�Ρ�
        2������������ƶ������������������Ƶı̿ձ���֮�£���ô��ͨ������ʱ�亯����ƽ����Щ��������꣬����ʵ�ֶ�̬�İ��Ƹ���ε����յ�Ч����
        3���������ת������ʱҲ����ʵ��һЩ���������ӵ�Ч�������磬��������ʱ������ƶ���������������ת��
    
        ע�⣬�任2D��������Ҫ����[4 x 4]������������Ƚ�������Ϊһ��4D������
        vin.TexC ---> float4(vin.TexC, 0.0f, 1.0f)
        ����ɳ˷�����֮�󣬴ӵõ���4D������ȥ��[z]������[w]������ʹ֮ǿ��ת����2D����������
    	float4 texC = mul(float4(vin.TexC, 0.0f, 1.0f), gTexTransform);
	    vout.TexC = mul(texC, gMatTransform).xy;
    
        �ڴˣ�������������������������任����gTexTransform��gMatTransform������������Ϊһ���ǹ��ڲ��ʵ�����任�������ˮ�����Ķ�̬���ʣ���
        ��һ���ǹ����������Ե�����任����������ʹ�õ���2D�������꣬��������ֻ����ǰ����������ı任�����
        ���磬����������ƽ����[z]���꣬�Ⲣ�����������������κ�Ӱ�졣
    */
    
	// Output vertex attributes for interpolation across triangle.
    // Ϊ�˶������ν��в�ֵ����������Ķ�������
	float4 texC = mul(float4(vin.TexC, 0.0f, 1.0f), gTexTransform);
	vout.TexC = mul(texC, gMatTransform).xy;
	
    return vout;
}

float4 PS(VertexOut pin) : SV_Target
{
    // ����ͨ����Sample�����ĵ�һ����������SamplerState������������ζ��������ݽ��в���������ڶ��������������ص���������(u,v)��
    // �������������SamplerState������ָ���Ĺ��˷�������������ͼ�ڵ�(u,v)���Ĳ�ֵ��ɫ��
    // ����������ȡ�����ص������䷴���ʲ������������볣���������еķ��������
    float4 diffuseAlbedo = gDiffuseMap.Sample(gsamAnisotropicWrap, pin.TexC) * gDiffuseAlbedo;
	
    // Interpolating normal can unnormalize it, so renormalize it.
    // �Է��߲�ֵ����ʹ֮�ǹ淶�������Ҫ�����ٴν��й淶������
    pin.NormalW = normalize(pin.NormalW);

    // Vector from point being lit to eye. 
    // ���߾�������һ�㷴�䵽�۲����һ�����ϵ�����
    float3 toEyeW = normalize(gEyePosW - pin.PosW);

    // Light terms.
    // ������
    float4 ambient = gAmbientLight*diffuseAlbedo;

    const float shininess = 1.0f - gRoughness;
    Material mat = { diffuseAlbedo, gFresnelR0, shininess };
    float3 shadowFactor = 1.0f;
    float4 directLight = ComputeLighting(gLights, mat, pin.PosW,
        pin.NormalW, toEyeW, shadowFactor);

    float4 litColor = ambient + directLight;

    // Common convention to take alpha from diffuse albedo.
    // �������䷴���ʻ�ȡalphaֵ�ĳ����ֶ�
    litColor.a = diffuseAlbedo.a;

    return litColor;
}



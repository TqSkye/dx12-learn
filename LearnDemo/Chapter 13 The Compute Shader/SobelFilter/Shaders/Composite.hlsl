//***************************************************************************************
// Composite.hlsl by Frank Luna (C) 2015 All Rights Reserved.
//
// Combines two images.将两张图像组合在一起
//***************************************************************************************

Texture2D gBaseMap : register(t0);
Texture2D gEdgeMap : register(t1);

SamplerState gsamPointWrap        : register(s0);
SamplerState gsamPointClamp       : register(s1);
SamplerState gsamLinearWrap       : register(s2);
SamplerState gsamLinearClamp      : register(s3);
SamplerState gsamAnisotropicWrap  : register(s4);
SamplerState gsamAnisotropicClamp : register(s5);

static const float2 gTexCoords[6] = 
{
	float2(0.0f, 1.0f),
	float2(0.0f, 0.0f),
	float2(1.0f, 0.0f),
	float2(0.0f, 1.0f),
	float2(1.0f, 0.0f),
	float2(1.0f, 1.0f)
};

struct VertexOut
{
	float4 PosH    : SV_POSITION;
	float2 TexC    : TEXCOORD;
};

VertexOut VS(uint vid : SV_VertexID)
{
	VertexOut vout;
	
	vout.TexC = gTexCoords[vid];
	
	// Map [0,1]^2 to NDC space.
    // 将[0,1]^2区间映射到NDC（规格化设备坐标）空间
	vout.PosH = float4(2.0f*vout.TexC.x - 1.0f, 1.0f - 2.0f*vout.TexC.y, 0.0f, 1.0f);

    return vout;
}

float4 PS(VertexOut pin) : SV_Target
{
    float4 c = gBaseMap.SampleLevel(gsamPointClamp, pin.TexC, 0.0f);
	float4 e = gEdgeMap.SampleLevel(gsamPointClamp, pin.TexC, 0.0f);
	// 将边缘图与原始图像相乘
	return c*e;
}



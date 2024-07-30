//***************************************************************************************
// Default.hlsl by Frank Luna (C) 2015 All Rights Reserved.
//***************************************************************************************
/*
    ʵ������:
        �ڱ����ǰһ���У�ʵ�����ݶ���������װ��׶λ�ȡ�ġ��ڴ������벼�֣�input layout��ʱ������ͨ��ö����D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA���D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA
        ��ָ�����������Ϊ��ʵ����per-instance���������������𶥵㣨per-vertex������������ٽ��ڶ������㻺�����뺬��ʵ�����ݵ���������󶨡�
        [1]Direct3D 12��Ȼ֧����������ˮ�ߴ���ʵ�����ݵķ�ʽ����������Ҫ����һ�ָ�Ϊ�ִ����ķ�����

        ����ν�ġ�Ħ�ǡ���������Ϊ����ʵ��������һ��������ʵ�����ݵĽṹ�������������磬��Ҫ��ĳ������ʵ����100�Σ���Ӧ������һ������100��ʵ������Ԫ�صĽṹ����������
        ���ŰѴ˽ṹ����������Դ�󶨵���Ⱦ��ˮ���ϣ�������Ҫ���Ƶ�ʵ���ڶ�����ɫ����������Ӧ�����ݡ���ô�����������ڶ�����ɫ����ȷ��Ҫ���Ƶ�ʵ���أ�
        Ϊ�ˣ�Direct3D�ṩ��ϵͳֵ��ʶ��SV_InstanceID���ɹ��û��ڶ�����ɫ���з����ʵ������Ŀ�ġ����磬�����ɵ�һ��ʵ�����õĸ�����ͳһ���Ϊ0������ɵڶ���ʵ����������ͳһ���Ϊ1�����������ơ�
        �ݴˣ����Ǳ����ڶ�����ɫ���жԽṹ��������������������ȡ�����ʵ�����ݡ�������ɫ������չʾ����һϵ�еĹ������̣�
            (��������ǰ�ű��ļ�)

        ���Է��֣��������Ѳ��������峣����������cbPerObject������Ӱ����ʱ��ÿ����������ݽ���ʵ���������ṩ��
        �Ӵ����л��ܿ��������������ͨ����̬������Ϊÿ��ʵ��������ͬ�Ĳ���������ġ�����Ҫ���ǣ����ǿ����ڵ��λ��Ƶ����л�ȡ�����ģ���ʵ�����ݣ�
        Ϊ��������������ֽ�������ɫ����������Ӧ�ĸ�ǩ�������о����¡�
            CD3DX12_DESCRIPTOR_RANGE texTable;
            texTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 7, 0, 0);

            // �����������������������������������
            CD3DX12_ROOT_PARAMETER slotRootParameter[4];

            // ����С��ʾ�������Ƶ���ɸߵ��ͽ������У�Ч������
            slotRootParameter[0].InitAsShaderResourceView(0, 1);
            slotRootParameter[1].InitAsShaderResourceView(1, 1);
            slotRootParameter[2].InitAsConstantBufferView(0);
            slotRootParameter[3].InitAsDescriptorTable(1, &texTable, D3D12_SHADER_
                VISIBILITY_PIXEL);

            auto staticSamplers = GetStaticSamplers();

            // ��ǩ����һϵ�и����������
            CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(4, slotRootParameter,
              (UINT)staticSamplers.size(), staticSamplers.data(),
              D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
        ����һ���������Ĺ���һ������������Ⱦÿһ֡����ʱ��Ҫ��һ�γ����е�ȫ�����������������ÿ�λ��Ƶ���ʱ������ֻ�������ô�����Ӧʵ�����ݵĽṹ�����������ɡ�
            void InstancingAndCullingApp::Draw(const GameTimer& gt)
            {
              ...
              // �󶨴˳��������ȫ�����ʡ����ڽṹ�����������ԣ����ǿ����ƹ��������ѵ�ʹ�ö�����ֱ������
              // Ϊ��������

              auto matBuffer = mCurrFrameResource->MaterialBuffer->Resource();
              mCommandList->SetGraphicsRootShaderResourceView(1, matBuffer-
                >GetGPUVirtualAddress());

              auto passCB = mCurrFrameResource->PassCB->Resource();
              mCommandList->SetGraphicsRootConstantBufferView(2, passCB-
                >GetGPUVirtualAddress());

              // ����Ⱦ�˳������õ�һ������
              mCommandList->SetGraphicsRootDescriptorTable(3, 
                mSrvDescriptorHeap->GetGPUDescriptorHandleForHeapStart());

              DrawRenderItems(mCommandList.Get(), mOpaqueRitems);
              ...
            }

            void InstancingAndCullingApp::DrawRenderItems(
              ID3D12GraphicsCommandList* cmdList, 
              const std::vector<RenderItem*>& ritems)
            {
              // ���ÿ����Ⱦ���
              for(size_t i = 0; i < ritems.size(); ++i)
              {
                auto ri = ritems[i];

                cmdList->IASetVertexBuffers(0, 1, &ri->Geo->VertexBufferView());
                cmdList->IASetIndexBuffer(&ri->Geo->IndexBufferView());
                cmdList->IASetPrimitiveTopology(ri->PrimitiveType);

                // ���ô���Ⱦ��Ҫ�õ���ʵ��������
                // ���ڽṹ�����������������ǿ����ƹ��������Ѷ�����ֱ������Ϊ��������
                auto instanceBuffer = mCurrFrameResource->InstanceBuffer-
                >Resource();
                mCommandList->SetGraphicsRootShaderResourceView(
                  0, instanceBuffer->GetGPUVirtualAddress());

                cmdList->DrawIndexedInstanced(ri->IndexCount, 
                  ri->InstanceCount, ri->StartIndexLocation, 
                  ri->BaseVertexLocation, 0);
              }
            }
*/

// Defaults for number of lights.
// ��Դ������Ĭ��ֵ
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

struct InstanceData
{
	float4x4 World;
	float4x4 TexTransform;
	uint     MaterialIndex;
	uint     InstPad0;
	uint     InstPad1;
	uint     InstPad2;
};

struct MaterialData
{
	float4   DiffuseAlbedo;
    float3   FresnelR0;
    float    Roughness;
	float4x4 MatTransform;
	uint     DiffuseMapIndex;
	uint     MatPad0;
	uint     MatPad1;
	uint     MatPad2;
};

// An array of textures, which is only supported in shader model 5.1+.  Unlike Texture2DArray, the textures
// in this array can be different sizes and formats, making it more flexible than texture arrays.
// ֻ����ɫ��ģ��5.1+��֧�ֵ��������顣��Texture2Darray�������鲻ͬ���ǣ���������ɲ�ͬ��С
// ����ͬ��ʽ�������ɣ�����������������Ϊ���
Texture2D gDiffuseMap[7] : register(t0);

// Put in space1, so the texture array does not overlap with these resources.  
// The texture array will occupy registers t0, t1, ..., t6 in space0. 
// �������������ṹ������������space1�У��Ӷ���������������֮�ص�����������������ռ��t0��
// t1������t6�Ĵ�����space0
StructuredBuffer<InstanceData> gInstanceData : register(t0, space1);
StructuredBuffer<MaterialData> gMaterialData : register(t1, space1);

SamplerState gsamPointWrap        : register(s0);
SamplerState gsamPointClamp       : register(s1);
SamplerState gsamLinearWrap       : register(s2);
SamplerState gsamLinearClamp      : register(s3);
SamplerState gsamAnisotropicWrap  : register(s4);
SamplerState gsamAnisotropicClamp : register(s5);

// Constant data that varies per pass.
// ÿ�˻��ƹ����ж����ܻ������仯�ĳ�������
cbuffer cbPass : register(b0)
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
    // ����ÿ����MaxLightsΪ��Դ�������ֵ�Ķ�������������[0, NUM_DIR_LIGHTS)��ʾ���Ƿ����Դ��
    // ����[NUM_DIR_LIGHTS, NUM_DIR_LIGHTS+NUM_POINT_LIGHTS)��ʾ���ǵ��Դ��
    // ����[NUM_DIR_LIGHTS+NUM_POINT_LIGHTS+NUM_SPOT_ LIGHTS]��ʾ���Ǿ۹�ƹ�Դ
    Light gLights[MaxLights];
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
	
	// nointerpolation is used so the index is not interpolated 
	// across the triangle.
    // ���ڴ˴�ʹ�õ����η���nointerpolation����˸�����ָ��Ķ���δ����ֵ��������
	nointerpolation uint MatIndex  : MATINDEX;
};

VertexOut VS(VertexIn vin, uint instanceID : SV_InstanceID)
{
	VertexOut vout = (VertexOut)0.0f;
	
	// Fetch the instance data.
    // ��ȡʵ������
	InstanceData instData = gInstanceData[instanceID];
	float4x4 world = instData.World;
	float4x4 texTransform = instData.TexTransform;
	uint matIndex = instData.MaterialIndex;

	vout.MatIndex = matIndex;
	
	// Fetch the material data.
    // ��ȡ��������
	MaterialData matData = gMaterialData[matIndex];
	
    // Transform to world space.
    // ������任������ռ�
    float4 posW = mul(float4(vin.PosL, 1.0f), world);
    vout.PosW = posW.xyz;

    // Assumes nonuniform scaling; otherwise, need to use inverse-transpose of world matrix.
    // ����Ҫִ�е��ǵȱ����ţ��������Ҫʹ������������ת�þ�����м���
    vout.NormalW = mul(vin.NormalL, (float3x3)world);

    // Transform to homogeneous clip space.
    // �Ѷ���任����βü��ռ�
    vout.PosH = mul(posW, gViewProj);
	
	// Output vertex attributes for interpolation across triangle.
    // Ϊ�˶������ν��в�ֵ�������������
	float4 texC = mul(float4(vin.TexC, 0.0f, 1.0f), texTransform);
	vout.TexC = mul(texC, matData.MatTransform).xy;
	
    return vout;
}

float4 PS(VertexOut pin) : SV_Target
{
	// Fetch the material data.
    // ��ȡ��������
	MaterialData matData = gMaterialData[pin.MatIndex];
	float4 diffuseAlbedo = matData.DiffuseAlbedo;
    float3 fresnelR0 = matData.FresnelR0;
    float  roughness = matData.Roughness;
	uint diffuseTexIndex = matData.DiffuseMapIndex;
	
	// Dynamically look up the texture in the array.
    // �������ж�̬�ز�������
    diffuseAlbedo *= gDiffuseMap[diffuseTexIndex].Sample(gsamLinearWrap, pin.TexC);
	
    // Interpolating normal can unnormalize it, so renormalize it.
    // �Է��߲�ֵ����ʹ���ǹ淶���������Ҫ�ٴζ������й淶������
    pin.NormalW = normalize(pin.NormalW);

    // Vector from point being lit to eye. 
    // �ӱ�����һ��ָ��۲�������
    float3 toEyeW = normalize(gEyePosW - pin.PosW);

    // Light terms.
    // ������
    float4 ambient = gAmbientLight*diffuseAlbedo;

    const float shininess = 1.0f - roughness;
    Material mat = { diffuseAlbedo, fresnelR0, shininess };
    float3 shadowFactor = 1.0f;
    float4 directLight = ComputeLighting(gLights, mat, pin.PosW,
        pin.NormalW, toEyeW, shadowFactor);

    float4 litColor = ambient + directLight;

    // Common convention to take alpha from diffuse albedo.
    // �������䷴���ʻ�ȡalphaֵ�ĳ��÷���
    litColor.a = diffuseAlbedo.a;

    return litColor;
}



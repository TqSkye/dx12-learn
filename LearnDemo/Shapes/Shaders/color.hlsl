//***************************************************************************************
// color.hlsl by Frank Luna (C) 2015 All Rights Reserved.
//
// Transforms and colors geometry.
//***************************************************************************************
 
/*
    ��ʱ������Ҳ���޸������峣������������cbPerObject����ʹ֮���洢һ���������йصĳ�����
    ��Ŀǰ��������ԣ�Ϊ�˻������壬��֮Ψһ��صĳ������������������:
*/
cbuffer cbPerObject : register(b0)
{
	float4x4 gWorld; 
};

/*
    �������Լ�ʵ�ֵ�FrameResource����������һ���µĳ�����������
    std::unique_ptr<UploadBuffer<PassConstants>> PassCB = nullptr;
    
    ������ʾ���븴�ӶȵĲ������ӣ��û������д洢���������ݣ�����۲�λ�á��۲������ͶӰ�����Լ�����Ļ����ȾĿ�꣩�ֱ��ʵ���ص���Ϣ��
    ������ض�����Ⱦ���̣�rendering pass����ȷ������������Ҳ����������Ϸ��ʱ�йص���Ϣ����������ɫ��������Ҫ���ʵļ����õ����ݡ�
    ע�⣬���ǵ���ʾ������ܲ����õ����еĳ������ݣ��������ǵĴ���ȴʹ������ø��ӷ��㣬�����ṩ��Щ���������Ҳֻ������������
    ���磬��Ȼ������������֪����ȾĿ��ĳߴ磬����Ҫʵ��ĳЩ���ڴ���Ч��֮ʱ�������Ϣ���������ó���
*/
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
};

struct VertexIn
{
	float3 PosL  : POSITION;
    float4 Color : COLOR;
};

struct VertexOut
{
	float4 PosH  : SV_POSITION;
    float4 Color : COLOR;
};

/*
    ������Щ�����������ṹ�ĸı䣬����ҲҪ�Զ�����ɫ��������Ӧ�ĸ��£�
    (��ShapesApp.cpp line:411 => UpdateObjectCBs �� UpdateMainPassCB����)


    ���ڣ���ɫ����������������Դ�ѷ����˸ı䣬���������Ҫ��Ӧ�ص�����ǩ����ʹ֮��ȡ�������������������ʱ�����ǵ���ɫ��������Ҫ��ȡ������������
    ��Ϊ������CBV��������������ͼ�����Ų�ͬ�ĸ���Ƶ�ʡ�����Ⱦ����CBV������ÿ����Ⱦ����������һ�Σ�������CBV��Ҫ���ÿһ����Ⱦ��������ã���
    (�¶η�����ShapesApp.cpp �� BuildRootSignature()��ʵ��)
    CD3DX12_DESCRIPTOR_RANGE cbvTable0;
    cbvTable0.Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0);

    CD3DX12_DESCRIPTOR_RANGE cbvTable1;
    cbvTable1.Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 1);

    // �����������������������������������
    CD3DX12_ROOT_PARAMETER slotRootParameter[2];

    // ������CBV
    slotRootParameter[0].InitAsDescriptorTable(1, &cbvTable0);
    slotRootParameter[1].InitAsDescriptorTable(1, &cbvTable1);

    // ��ǩ����һϵ�и�����������
    CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(2, slotRootParameter, 0, nullptr, 
    D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

*/
VertexOut VS(VertexIn vin)
{
	VertexOut vout;
	
	// Transform to homogeneous clip space.
    // ������任����βü��ռ�
    float4 posW = mul(float4(vin.PosL, 1.0f), gWorld);
    vout.PosH = mul(posW, gViewProj);
	
	// Just pass vertex color into the pixel shader.
    // ֱ����������ɫ�����ݶ������ɫ����
    vout.Color = vin.Color;
    
    return vout;
}

float4 PS(VertexOut pin) : SV_Target
{
    return pin.Color;
}



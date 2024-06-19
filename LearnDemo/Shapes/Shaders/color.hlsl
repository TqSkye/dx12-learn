//***************************************************************************************
// color.hlsl by Frank Luna (C) 2015 All Rights Reserved.
//
// Transforms and colors geometry.
//***************************************************************************************
 
/*
    此时，我们也已修改了物体常量缓冲区（即cbPerObject），使之仅存储一个与物体有关的常量。
    就目前的情况而言，为了绘制物体，与之唯一相关的常量就是它的世界矩阵:
*/
cbuffer cbPerObject : register(b0)
{
	float4x4 gWorld; 
};

/*
    我们在自己实现的FrameResource类中引进了一个新的常量缓冲区：
    std::unique_ptr<UploadBuffer<PassConstants>> PassCB = nullptr;
    
    随着演示代码复杂度的不断增加，该缓冲区中存储的数据内容（例如观察位置、观察矩阵与投影矩阵以及与屏幕（渲染目标）分辨率等相关的信息）
    会根据特定的渲染过程（rendering pass）而确定下来。其中也包含了与游戏计时有关的信息，它们是着色器程序中要访问的极有用的数据。
    注意，我们的演示程序可能不会用到所有的常量数据，但是它们的存在却使工作变得更加方便，而且提供这些额外的数据也只需少量开销。
    例如，虽然我们现在无须知道渲染目标的尺寸，但当要实现某些后期处理效果之时，这个信息将会派上用场。
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
    随着这些常量缓冲区结构的改变，我们也要对顶点着色器进行相应的更新：
    (接ShapesApp.cpp line:411 => UpdateObjectCBs 和 UpdateMainPassCB方法)


    现在，着色器所期望的输入资源已发生了改变，因此我们需要相应地调整根签名来使之获取所需的两个描述符表（此时，我们的着色器程序需要获取两个描述符表，
    因为这两个CBV（常量缓冲区视图）有着不同的更新频率——渲染过程CBV仅需在每个渲染过程中设置一次，而物体CBV则要针对每一个渲染项进行配置）：
    (下段方法在ShapesApp.cpp 的 BuildRootSignature()中实现)
    CD3DX12_DESCRIPTOR_RANGE cbvTable0;
    cbvTable0.Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0);

    CD3DX12_DESCRIPTOR_RANGE cbvTable1;
    cbvTable1.Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 1);

    // 根参数可能是描述符表、根描述符或根常量
    CD3DX12_ROOT_PARAMETER slotRootParameter[2];

    // 创建根CBV
    slotRootParameter[0].InitAsDescriptorTable(1, &cbvTable0);
    slotRootParameter[1].InitAsDescriptorTable(1, &cbvTable1);

    // 根签名由一系列根参数所构成
    CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(2, slotRootParameter, 0, nullptr, 
    D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

*/
VertexOut VS(VertexIn vin)
{
	VertexOut vout;
	
	// Transform to homogeneous clip space.
    // 将顶点变换到齐次裁剪空间
    float4 posW = mul(float4(vin.PosL, 1.0f), gWorld);
    vout.PosH = mul(posW, gViewProj);
	
	// Just pass vertex color into the pixel shader.
    // 直接向像素着色器传递顶点的颜色数据
    vout.Color = vin.Color;
    
    return vout;
}

float4 PS(VertexOut pin) : SV_Target
{
    return pin.Color;
}



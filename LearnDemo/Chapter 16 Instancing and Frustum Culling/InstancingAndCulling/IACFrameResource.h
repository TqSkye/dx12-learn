#pragma once

#include "../../../Common/d3dUtil.h"
#include "../../../Common/MathHelper.h"
#include "../../../Common/UploadBuffer.h"

struct InstanceData
{
	DirectX::XMFLOAT4X4 World = MathHelper::Identity4x4();
	DirectX::XMFLOAT4X4 TexTransform = MathHelper::Identity4x4();
	UINT MaterialIndex;
	UINT InstancePad0;
	UINT InstancePad1;
	UINT InstancePad2;
};

struct PassConstants
{
    DirectX::XMFLOAT4X4 View = MathHelper::Identity4x4();
    DirectX::XMFLOAT4X4 InvView = MathHelper::Identity4x4();
    DirectX::XMFLOAT4X4 Proj = MathHelper::Identity4x4();
    DirectX::XMFLOAT4X4 InvProj = MathHelper::Identity4x4();
    DirectX::XMFLOAT4X4 ViewProj = MathHelper::Identity4x4();
    DirectX::XMFLOAT4X4 InvViewProj = MathHelper::Identity4x4();
    DirectX::XMFLOAT3 EyePosW = { 0.0f, 0.0f, 0.0f };
    float cbPerObjectPad1 = 0.0f;
    DirectX::XMFLOAT2 RenderTargetSize = { 0.0f, 0.0f };
    DirectX::XMFLOAT2 InvRenderTargetSize = { 0.0f, 0.0f };
    float NearZ = 0.0f;
    float FarZ = 0.0f;
    float TotalTime = 0.0f;
    float DeltaTime = 0.0f;

    DirectX::XMFLOAT4 AmbientLight = { 0.0f, 0.0f, 0.0f, 1.0f };

    // Indices [0, NUM_DIR_LIGHTS) are directional lights;
    // indices [NUM_DIR_LIGHTS, NUM_DIR_LIGHTS+NUM_POINT_LIGHTS) are point lights;
    // indices [NUM_DIR_LIGHTS+NUM_POINT_LIGHTS, NUM_DIR_LIGHTS+NUM_POINT_LIGHT+NUM_SPOT_LIGHTS)
    // are spot lights for a maximum of MaxLights per object.
    Light Lights[MaxLights];
};

struct MaterialData
{
	DirectX::XMFLOAT4 DiffuseAlbedo = { 1.0f, 1.0f, 1.0f, 1.0f };
	DirectX::XMFLOAT3 FresnelR0 = { 0.01f, 0.01f, 0.01f };
	float Roughness = 64.0f;

	// Used in texture mapping.
	DirectX::XMFLOAT4X4 MatTransform = MathHelper::Identity4x4();

	UINT DiffuseMapIndex = 0;
	UINT MaterialPad0;
	UINT MaterialPad1;
	UINT MaterialPad2;
};

struct Vertex
{
    DirectX::XMFLOAT3 Pos;
    DirectX::XMFLOAT3 Normal;
	DirectX::XMFLOAT2 TexC;
};

// Stores the resources needed for the CPU to build the command lists
// for a frame.  
struct IACFrameResource
{
public:
    
    IACFrameResource(ID3D12Device* device, UINT passCount, UINT maxInstanceCount, UINT materialCount);
    IACFrameResource(const IACFrameResource& rhs) = delete;
    IACFrameResource& operator=(const IACFrameResource& rhs) = delete;
    ~IACFrameResource();

    // We cannot reset the allocator until the GPU is done processing the commands.
    // So each frame needs their own allocator.
    // 在GPU未处理完命令之前，不能对其引用的命令分配器进行重置，因此每一帧都需要有它们自己的命令
    // 分配器
    Microsoft::WRL::ComPtr<ID3D12CommandAllocator> CmdListAlloc;

    // We cannot update a cbuffer until the GPU is done processing the commands
    // that reference it.  So each frame needs their own cbuffers.
   // std::unique_ptr<UploadBuffer<FrameConstants>> FrameCB = nullptr;
    // 同理，在GPU未执行完命令之前，也不能对其引用的常量缓冲区进行更新，因此每一帧都需要拥有它们
    // 自己的常量缓冲区
    std::unique_ptr<UploadBuffer<PassConstants>> PassCB = nullptr;
    std::unique_ptr<UploadBuffer<MaterialData>> MaterialBuffer = nullptr;

	// NOTE: In this demo, we instance only one render-item, so we only have one structured buffer to 
	// store instancing data.  To make this more general (i.e., to support instancing multiple render-items), 
	// you would need to have a structured buffer for each render-item, and allocate each buffer with enough
	// room for the maximum number of instances you would ever draw.  
	// This sounds like a lot, but it is actually no more than the amount of per-object constant data we 
	// would need if we were not using instancing.  For example, if we were drawing 1000 objects without instancing,
	// we would create a constant buffer with enough room for a 1000 objects.  With instancing, we would just
	// create a structured buffer large enough to store the instance data for 1000 instances.
    // 注意:在此演示程序中，实例只有一个渲染项，所以仅用了一个结构化缓冲区来存储实例数据。若要使程
    // 序更具通用性（即支持实例拥有多个渲染项），我们还需为每个渲染项都添加一个结构化缓冲区，并为每
    // 个缓冲区分配出足够大的空间来容纳要绘制的最多实例个数。虽然看起来涉及的数据有点多，但若不使
    // 用实例化技术，要用到的物体常量数据会更多。例如，如果要在不使用实例化技术的情况下绘制1000
    // 个物体，就必须创建出一个可容纳1000个物体信息的常量缓冲区。但采用了实例化技术后，仅需构建出
    // 能存储1000个实例数据的结构化缓冲区即可  
    std::unique_ptr<UploadBuffer<InstanceData>> InstanceBuffer = nullptr;

    // Fence value to mark commands up to this fence point.  This lets us
    // check if these frame resources are still in use by the GPU.
    // 通过围栏值将命令标记到此围栏点。这使我们可以检测到这些帧资源是否还被GPU所用
    UINT64 Fence = 0;
};
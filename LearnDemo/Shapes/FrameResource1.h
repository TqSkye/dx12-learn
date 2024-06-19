#pragma once

#include "../../Common/d3dUtil.h"
#include "../../Common/MathHelper.h"
#include "../../Common/UploadBuffer.h"

struct ObjectConstants
{
    DirectX::XMFLOAT4X4 World = MathHelper::Identity4x4();
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
};

struct Vertex
{
    DirectX::XMFLOAT3 Pos;
    DirectX::XMFLOAT4 Color;
};

/*
    所以，我们在每帧绘制的结尾都会调用D3DApp::FlushCommandQueue函数，以确保GPU在每一帧都能正确完成所有命令的执行。
    这种解决方案虽然奏效却效率低下，原因如下：
    1．在每帧的起始阶段，GPU不会执行任何命令，因为等待它处理的命令队列空空如也。
        这种情况将持续到CPU构建并提交一些供GPU执行的命令为止。
    2．在每帧的收尾阶段，CPU会等待GPU完成命令的处理。
        所以，CPU和GPU在每一帧都存在各自的空闲时间。
    解决此问题的一种方案是：以CPU每帧都需更新的资源作为基本元素，创建一个环形数组（circular array，也有译作循环数组）。
    我们称这些资源为帧资源（frame resource），而这种循环数组通常是由3个帧资源元素所构成的。
    该方案的思路是：在处理第[插图]帧的时候，CPU将周而复始地从帧资源数组中获取下一个可用的（即没被GPU使用中的）帧资源。
    趁着GPU还在处理此前帧之时，CPU将为第[插图]帧更新资源，并构建和提交对应的命令列表。
    随后，CPU会继续针对第[插图]帧执行同样的工作流程，并不断重复下去。
    如果帧资源数组共有3个元素，则令CPU比GPU提前处理两帧，以确保GPU可持续工作。
    下面所列的是帧资源类的例程，在本章中我们将利用“Shapes”（不同形状的几何体）程序配合演示。
    由于在此例中CPU只需修改常量缓冲区，所以程序中的帧资源类只含有常量缓冲区。

    据此，我们的应用程序类（ShapesApp）将实例化一个由3个帧资源('ShapesApp'中'gNumFrameResources==3')元素所构成的向量，
    并留有特定的成员变量来记录当前的帧资源：
*/

// Stores the resources needed for the CPU to build the command lists
// for a frame.  
// 存有CPU为构建每帧命令列表所需的资源
// 其中的数据将依程序而异，这取决于实际绘制所需的资源
struct FrameResource1
{
public:
    
    FrameResource1(ID3D12Device* device, UINT passCount, UINT objectCount);
    FrameResource1(const FrameResource1& rhs) = delete;
    FrameResource1& operator=(const FrameResource1& rhs) = delete;
    ~FrameResource1();

    // We cannot reset the allocator until the GPU is done processing the commands.
    // So each frame needs their own allocator.
    // 在GPU处理完与此命令分配器相关的命令之前，我们不能对它进行重置。
    // 所以每一帧都要有它们自己的命令分配器
    Microsoft::WRL::ComPtr<ID3D12CommandAllocator> CmdListAlloc;

    // We cannot update a cbuffer until the GPU is done processing the commands
    // that reference it.  So each frame needs their own cbuffers.
    // 在GPU执行完引用此常量缓冲区的命令之前，我们不能对它进行更新。
    // 因此每一帧都要有它们自己的常量缓冲区
    std::unique_ptr<UploadBuffer<PassConstants>> PassCB = nullptr;
    std::unique_ptr<UploadBuffer<ObjectConstants>> ObjectCB = nullptr;

    // Fence value to mark commands up to this fence point.  This lets us
    // check if these frame resources are still in use by the GPU.
    // 通过围栏值将命令标记到此围栏点，这使我们可以检测到GPU是否还在使用这些帧资源
    UINT64 Fence = 0;
};
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
    ���ԣ�������ÿ֡���ƵĽ�β�������D3DApp::FlushCommandQueue��������ȷ��GPU��ÿһ֡������ȷ������������ִ�С�
    ���ֽ��������Ȼ��ЧȴЧ�ʵ��£�ԭ�����£�
    1����ÿ֡����ʼ�׶Σ�GPU����ִ���κ������Ϊ�ȴ��������������пտ���Ҳ��
        ���������������CPU�������ύһЩ��GPUִ�е�����Ϊֹ��
    2����ÿ֡����β�׶Σ�CPU��ȴ�GPU�������Ĵ���
        ���ԣ�CPU��GPU��ÿһ֡�����ڸ��ԵĿ���ʱ�䡣
    ����������һ�ַ����ǣ���CPUÿ֡������µ���Դ��Ϊ����Ԫ�أ�����һ���������飨circular array��Ҳ������ѭ�����飩��
    ���ǳ���Щ��ԴΪ֡��Դ��frame resource����������ѭ������ͨ������3��֡��ԴԪ�������ɵġ�
    �÷�����˼·�ǣ��ڴ����[��ͼ]֡��ʱ��CPU���ܶ���ʼ�ش�֡��Դ�����л�ȡ��һ�����õģ���û��GPUʹ���еģ�֡��Դ��
    ����GPU���ڴ����ǰ֮֡ʱ��CPU��Ϊ��[��ͼ]֡������Դ�����������ύ��Ӧ�������б�
    ���CPU�������Ե�[��ͼ]ִ֡��ͬ���Ĺ������̣��������ظ���ȥ��
    ���֡��Դ���鹲��3��Ԫ�أ�����CPU��GPU��ǰ������֡����ȷ��GPU�ɳ���������
    �������е���֡��Դ������̣��ڱ��������ǽ����á�Shapes������ͬ��״�ļ����壩���������ʾ��
    �����ڴ�����CPUֻ���޸ĳ��������������Գ����е�֡��Դ��ֻ���г�����������

    �ݴˣ����ǵ�Ӧ�ó����ࣨShapesApp����ʵ����һ����3��֡��Դ('ShapesApp'��'gNumFrameResources==3')Ԫ�������ɵ�������
    �������ض��ĳ�Ա��������¼��ǰ��֡��Դ��
*/

// Stores the resources needed for the CPU to build the command lists
// for a frame.  
// ����CPUΪ����ÿ֡�����б��������Դ
// ���е����ݽ���������죬��ȡ����ʵ�ʻ����������Դ
struct FrameResource1
{
public:
    
    FrameResource1(ID3D12Device* device, UINT passCount, UINT objectCount);
    FrameResource1(const FrameResource1& rhs) = delete;
    FrameResource1& operator=(const FrameResource1& rhs) = delete;
    ~FrameResource1();

    // We cannot reset the allocator until the GPU is done processing the commands.
    // So each frame needs their own allocator.
    // ��GPU��������������������ص�����֮ǰ�����ǲ��ܶ����������á�
    // ����ÿһ֡��Ҫ�������Լ������������
    Microsoft::WRL::ComPtr<ID3D12CommandAllocator> CmdListAlloc;

    // We cannot update a cbuffer until the GPU is done processing the commands
    // that reference it.  So each frame needs their own cbuffers.
    // ��GPUִ�������ô˳���������������֮ǰ�����ǲ��ܶ������и��¡�
    // ���ÿһ֡��Ҫ�������Լ��ĳ���������
    std::unique_ptr<UploadBuffer<PassConstants>> PassCB = nullptr;
    std::unique_ptr<UploadBuffer<ObjectConstants>> ObjectCB = nullptr;

    // Fence value to mark commands up to this fence point.  This lets us
    // check if these frame resources are still in use by the GPU.
    // ͨ��Χ��ֵ�������ǵ���Χ���㣬��ʹ���ǿ��Լ�⵽GPU�Ƿ���ʹ����Щ֡��Դ
    UINT64 Fence = 0;
};
//***************************************************************************************
// BoxApp.cpp by Frank Luna (C) 2015 All Rights Reserved.
//
// Shows how to draw a box in Direct3D 12.
//
// Controls:
//   Hold the left mouse button down and move the mouse to rotate.
//   Hold the right mouse button down and move the mouse to zoom in and out.
//***************************************************************************************

/*
  �������ģ�ͣ�Component Object Model��COM����һ����DirectX���ܱ����������������ʹ֮�����ݵļ�����
  ����ͨ����COM������Ϊһ�ֽӿڣ������ǵ�ǰ��̵�Ŀ�ģ��콫������һ��C++����ʹ�á�
  ��C++���Ա�дDirectX����ʱ��COM�����������˴����ײ�ϸ�ڡ�
  ����ֻ��֪����Ҫ��ȡָ��ĳCOM�ӿڵ�ָ�룬������ض���������һCOM�ӿڵķ���������������C++�����еĹؼ���newȥ����һ��COM�ӿڡ�
  ���⣬COM�����ͳ�������ô�������ˣ���ʹ����ĳ�ӿ�ʱ�����Ǳ�Ӧ��������Release������COM�ӿڵ����й��ܶ��Ǵ�IUnknown���COM�ӿڼ̳ж����ģ�����Release�������ڣ���
  ��������delete��ɾ��������COM��������ü���Ϊ0ʱ�����������ͷ��Լ���ռ�õ��ڴ档
  Ϊ�˸����û�����COM������������ڣ�Windows����ʱ�⣨Windows Runtime Library��WRL��
  ר��Ϊ���ṩ��Microsoft::WRL::ComPtr�ࣨ#include <wrl.h>�������ǿ��԰���������COM���������ָ�롣
  ��һ��ComPtrʵ������������Χʱ��������Զ�������ӦCOM�����Release�������̶�ʡ���������ֶ����õ��鷳��
  �����г��õ�3��ComPtr�������¡�
      1��Get������һ��ָ��˵ײ�COM�ӿڵ�ָ�롣�˷��������ڰ�ԭʼ��COM�ӿ�ָ����Ϊ�������ݸ�����������:
          ComPtr<ID3D12RootSignature> mRootSignature;
          ...
          // SetGraphicsRootSignature��Ҫ��ȡID3D12RootSignature*���͵Ĳ���
          mCommandList->SetGraphicsRootSignature(mRootSignature.Get());
      2��GetAddressOf������ָ��˵ײ�COM�ӿ�ָ��ĵ�ַ��ƾ�˷����������ú�����������COM�ӿڵ�ָ�롣���磺
          ComPtr<ID3D12CommandAllocator> mDirectCmdListAlloc;
          ...
          ThrowIfFailed(md3dDevice->CreateCommandAllocator(
          D3D12_COMMAND_LIST_TYPE_DIRECT,
          mDirectCmdListAlloc.GetAddressOf()));
      3��Reset������ComPtrʵ������Ϊnullptr�ͷ���֮��ص��������ã�ͬʱ������ײ�COM�ӿڵ����ü�������
      �˷����Ĺ����뽫ComPtrĿ��ʵ����ֵΪnullptr��Ч����ͬ��
*/

#include "../../Common/d3dApp.h"
#include "../../Common/MathHelper.h"
#include "../../Common/UploadBuffer.h"

using Microsoft::WRL::ComPtr;
using namespace DirectX;
using namespace DirectX::PackedVector;

struct Vertex
{
    XMFLOAT3 Pos;
    XMFLOAT4 Color;
};

struct ObjectConstants
{
    XMFLOAT4X4 WorldViewProj = MathHelper::Identity4x4();
};

class BoxApp : public D3DApp
{
public:
    BoxApp(HINSTANCE hInstance);
    BoxApp(const BoxApp& rhs) = delete;
    BoxApp& operator=(const BoxApp& rhs) = delete;
    ~BoxApp();

    virtual bool Initialize()override;

private:
    virtual void OnResize()override;
    virtual void Update(const GameTimer& gt)override;
    virtual void Draw(const GameTimer& gt)override;

    virtual void OnMouseDown(WPARAM btnState, int x, int y)override;
    virtual void OnMouseUp(WPARAM btnState, int x, int y)override;
    virtual void OnMouseMove(WPARAM btnState, int x, int y)override;

    void BuildDescriptorHeaps();
    void BuildConstantBuffers();
    void BuildRootSignature();
    void BuildShadersAndInputLayout();
    void BuildBoxGeometry();
    void BuildPSO();

private:

    ComPtr<ID3D12RootSignature> mRootSignature = nullptr;
    ComPtr<ID3D12DescriptorHeap> mCbvHeap = nullptr;

    std::unique_ptr<UploadBuffer<ObjectConstants>> mObjectCB = nullptr;

    std::unique_ptr<MeshGeometry> mBoxGeo = nullptr;

    ComPtr<ID3DBlob> mvsByteCode = nullptr;
    ComPtr<ID3DBlob> mpsByteCode = nullptr;

    std::vector<D3D12_INPUT_ELEMENT_DESC> mInputLayout;

    ComPtr<ID3D12PipelineState> mPSO = nullptr;

    XMFLOAT4X4 mWorld = MathHelper::Identity4x4();
    XMFLOAT4X4 mView = MathHelper::Identity4x4();
    XMFLOAT4X4 mProj = MathHelper::Identity4x4();

    float mTheta = 1.5f * XM_PI;
    float mPhi = XM_PIDIV4;
    float mRadius = 5.0f;

    POINT mLastMousePos;
};

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE prevInstance,
    PSTR cmdLine, int showCmd)
{
    // Enable run-time memory check for debug builds.
#if defined(DEBUG) | defined(_DEBUG)
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

    try
    {
        BoxApp theApp(hInstance);
        if (!theApp.Initialize())
            return 0;

        return theApp.Run();
    }
    catch (DxException& e)
    {
        MessageBox(nullptr, e.ToString().c_str(), L"HR Failed", MB_OK);
        return 0;
    }
}

BoxApp::BoxApp(HINSTANCE hInstance)
    : D3DApp(hInstance)
{
}

BoxApp::~BoxApp()
{
}

bool BoxApp::Initialize()
{
    if (!D3DApp::Initialize())
        return false;

    // Reset the command list to prep for initialization commands.
    ThrowIfFailed(mCommandList->Reset(mDirectCmdListAlloc.Get(), nullptr));

    BuildDescriptorHeaps();
    BuildConstantBuffers();
    BuildRootSignature();
    BuildShadersAndInputLayout();
    BuildBoxGeometry();
    BuildPSO();

    // Execute the initialization commands.
    ThrowIfFailed(mCommandList->Close());
    ID3D12CommandList* cmdsLists[] = { mCommandList.Get() };
    mCommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

    // Wait until initialization is complete.
    FlushCommandQueue();

    return true;
}

void BoxApp::OnResize()
{
    D3DApp::OnResize();

    // The window resized, so update the aspect ratio and recompute the projection matrix.
    XMMATRIX P = XMMatrixPerspectiveFovLH(0.25f * MathHelper::Pi, AspectRatio(), 1.0f, 1000.0f);
    XMStoreFloat4x4(&mProj, P);
}

void BoxApp::Update(const GameTimer& gt)
{
    // Convert Spherical to Cartesian coordinates.
    float x = mRadius * sinf(mPhi) * cosf(mTheta);
    float z = mRadius * sinf(mPhi) * sinf(mTheta);
    float y = mRadius * cosf(mPhi);

    // Build the view matrix.
    XMVECTOR pos = XMVectorSet(x, y, z, 1.0f);
    XMVECTOR target = XMVectorZero();
    XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

    XMMATRIX view = XMMatrixLookAtLH(pos, target, up);
    XMStoreFloat4x4(&mView, view);

    XMMATRIX world = XMLoadFloat4x4(&mWorld);
    XMMATRIX proj = XMLoadFloat4x4(&mProj);
    XMMATRIX worldViewProj = world * view * proj;

    // Update the constant buffer with the latest worldViewProj matrix.
    ObjectConstants objConstants;
    XMStoreFloat4x4(&objConstants.WorldViewProj, XMMatrixTranspose(worldViewProj));
    mObjectCB->CopyData(0, objConstants);
}

void BoxApp::Draw(const GameTimer& gt)
{
    // Reuse the memory associated with command recording.
    // We can only reset when the associated command lists have finished execution on the GPU.
    ThrowIfFailed(mDirectCmdListAlloc->Reset());

    // A command list can be reset after it has been added to the command queue via ExecuteCommandList.
    // Reusing the command list reuses memory.
    ThrowIfFailed(mCommandList->Reset(mDirectCmdListAlloc.Get(), mPSO.Get()));

    mCommandList->RSSetViewports(1, &mScreenViewport);
    mCommandList->RSSetScissorRects(1, &mScissorRect);

    // Indicate a state transition on the resource usage.
    mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(),
        D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));

    // Clear the back buffer and depth buffer.
    mCommandList->ClearRenderTargetView(CurrentBackBufferView(), Colors::LightSteelBlue, 0, nullptr);
    mCommandList->ClearDepthStencilView(DepthStencilView(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);

    // Specify the buffers we are going to render to.
    mCommandList->OMSetRenderTargets(1, &CurrentBackBufferView(), true, &DepthStencilView());

    ID3D12DescriptorHeap* descriptorHeaps[] = { mCbvHeap.Get() };
    mCommandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

    mCommandList->SetGraphicsRootSignature(mRootSignature.Get());

    /*
    һ������Ͷ��㻺���� ���� VertexBufferView() : ���㻺���� d3dUtile.h=>VertexBufferView():

        Ϊ�˽����㻺�����󶨵���Ⱦ��ˮ���ϣ�������Ҫ��������Դ����һ�����㻺������ͼ��vertex buffer view����
        ��RTV��render target view����ȾĿ����ͼ����ͬ���ǣ���������Ϊ���㻺������ͼ�����������ѡ�
        ���ң����㻺������ͼ����D3D12_VERTEX_BUFFER_VIEW[5]�ṹ������ʾ��
            typedef struct D3D12_VERTEX_BUFFER_VIEW
            {
              D3D12_GPU_VIRTUAL_ADDRESS BufferLocation;
              UINT SizeInBytes;
              UINT StrideInBytes;
            }   D3D12_VERTEX_BUFFER_VIEW;
            1��BufferLocation����������ͼ�Ķ��㻺������Դ�����ַ�����ǿ���ͨ��ID3D12Resource::GetGPUVirtualAddress��������ô˵�ַ��
            2��SizeInBytes����������ͼ�Ķ��㻺������С�����ֽڱ�ʾ����
            3��StrideInBytes��ÿ������Ԫ����ռ�õ��ֽ�����

     ���� �ڶ��㻺���������Ӧ��ͼ������ɺ󣬱���Խ�������Ⱦ��ˮ���ϵ�һ������ۣ�input slot����󶨡�
        ����һ�������Ǿ�������ˮ���е�����װ�����׶δ��ݶ��������ˡ��˲�������ͨ�����з�����ʵ�֡�
            void ID3D12GraphicsCommandList::IASetVertexBuffers(
              UINT StartSlot,
              UINT NumView,
              const D3D12_VERTEX_BUFFER_VIEW *pViews);
            1��StartSlot���ڰ󶨶�����㻺����ʱ�����õ���ʼ����ۣ�������һ�����㻺��������������˲ۣ�������۹���16��������Ϊ0��15��
            2��NumViews����Ҫ������۰󶨵Ķ��㻺��������������ͼ����pViews����ͼ���������������ʼ�����StartSlot������ֵΪ[��ͼ]��������Ҫ��[��ͼ]�����㻺��������ô��Щ�������������������[��ͼ]��󶨡�
            3��pViews��ָ�򶥵㻺������ͼ�����е�һ��Ԫ�ص�ָ�롣
        �����Ǹú�����һ������ʾ��:
            D3D12_VERTEX_BUFFER_VIEW vbv;
            vbv.BufferLocation = VertexBufferGPU->GetGPUVirtualAddress();
            vbv.StrideInBytes = sizeof(Vertex);
            vbv.SizeInBytes = 8 * sizeof(Vertex);

            D3D12_VERTEX_BUFFER_VIEW vertexBuffers[1] = { vbv };
            mCommandList->IASetVertexBuffers(0, 1, vertexBuffers);

    ���������㻺�������õ�������ϲ��������ִ��ʵ�ʵĻ��Ʋ��������ǽ�Ϊ��������������Ⱦ��ˮ������׼�����ѡ�
        �����һ������ͨ��ID3D12GraphicsCommandList::DrawInstanced����[6]�����ػ��ƶ��㡣
        *** ע�⣺�������box�õ���ID3D12GraphicsCommandList::DrawIndexedInstanced����Ϊʹ�õ����������ƣ�����
            void ID3D12GraphicsCommandList::DrawInstanced (
              UINT VertexCountPerInstance,
              UINT InstanceCount,
              UINT StartVertexLocation,
              UINT StartInstanceLocation);
            1��VertexCountPerInstance��ÿ��ʵ��Ҫ���ƵĶ���������
            2��InstanceCount������ʵ��һ�ֱ�����ʵ������instancing���ĸ߼���������Ŀǰ��˵������ֻ����һ��ʵ����������˲�������Ϊ1��
            3��StartVertexLocation��ָ�����㻺�����ڵ�һ�������ƶ����������������ֵ��0Ϊ��׼����
            4��StartInstanceLocation������ʵ��һ�ֱ�����ʵ�����ĸ߼���������ʱֻ�轫������Ϊ0��
       VertexCountPerInstance��StartVertexLocation�������������˶��㻺�����н�Ҫ�����Ƶ�һ���������㣬��ͼ6.2��ʾ��
    */
    mCommandList->IASetVertexBuffers(0, 1, &mBoxGeo->VertexBufferView());

    /*
     һ������������������: ���� IndexBufferView() : ���������� d3dUtile.h=>IndexBufferView():
        �붥�����ƣ�Ϊ��ʹGPU���Է����������飬����Ҫ�����Ƿ�����GPU�Ļ�������Դ��ID3D12Resource���ڡ�
        ���ǳƴ洢�����Ļ�����Ϊ������������index buffer�������ڱ��������õ�d3dUtil::CreateDefaultBuffer������ͨ��void*������Ϊ�������뷺�����ݣ�
        �����ζ������Ҳ�����ô˺������������������������������͵�Ĭ�ϻ���������
        Ϊ��ʹ��������������Ⱦ��ˮ�߰󶨣�������Ҫ��������������Դ����һ��������������ͼ��index buffer view����
        ��ͬ���㻺������ͼһ��������Ҳ����Ϊ������������ͼ�����������ѡ���������������ͼҪ�ɽṹ��D3D12_INDEX_BUFFER_VIEW����ʾ��
        typedef struct D3D12_INDEX_BUFFER_VIEW
        {
          D3D12_GPU_VIRTUAL_ADDRESS BufferLocation;
          UINT SizeInBytes;
          DXGI_FORMAT Format;
        } D3D12_INDEX_BUFFER_VIEW;
        1��BufferLocation����������ͼ��������������Դ�����ַ�����ǿ���ͨ������ID3D12Resource::GetGPUVirtualAddress��������ȡ�˵�ַ��
        2��SizeInBytes����������ͼ��������������С�����ֽڱ�ʾ����
        3��Format�������ĸ�ʽ����Ϊ��ʾ16λ������DXGI_FORMAT_R16_UINT���ͣ����ʾ32λ������DXGI_FORMAT_R32_UINT���͡�
            16λ���������Լ����ڴ�ʹ����ռ�ã����������ֵ��Χ������16λ���ݵı�ﷶΧ����Ҳֻ�ܲ���32λ�����ˡ�

    �����붥�㻺�������ƣ�Ҳ����������Direct3D��Դ���ڣ�����ʹ��֮ǰ��������Ҫ�Ƚ����ǰ󶨵���Ⱦ��ˮ���ϡ�
        ͨ��ID3D12GraphicsCommandList::IASetIndexBuffer�������ɽ������������󶨵�����װ�����׶Ρ�
        ����Ĵ�����ʾ����������һ�����������������幹��������������Σ��Լ�Ϊ������������������ͼ�������󶨵���Ⱦ��ˮ�ߣ�
            std::uint16_t indices[] = {
              // ������ǰ����
              0, 1, 2,
              0, 2, 3,

              // ����������
              4, 6, 5,
              4, 7, 6,

              // �����������
              4, 5, 1,
              4, 1, 0,

              // �������ұ���
              3, 2, 6,
              3, 6, 7,

              // �������ϱ���
              1, 5, 6,
              1, 6, 2,

              // �������±���
              4, 0, 3,
              4, 3, 7
            };

            const UINT ibByteSize = 36 * sizeof(std::uint16_t);

            ComPtr<ID3D12Resource> IndexBufferGPU = nullptr;
            ComPtr<ID3D12Resource> IndexBufferUploader = nullptr;
            IndexBufferGPU = d3dUtil::CreateDefaultBuffer(md3dDevice.Get(),
              mCommandList.Get(), indices, ibByteSize, IndexBufferUploader);

            D3D12_INDEX_BUFFER_VIEW ibv;
            ibv.BufferLocation = IndexBufferGPU->GetGPUVirtualAddress();
            ibv.Format = DXGI_FORMAT_R16_UINT;
            ibv.SizeInBytes = ibByteSize;

            mCommandList->IASetIndexBuffer(&ibv);
    */
    mCommandList->IASetIndexBuffer(&mBoxGeo->IndexBufferView());

    /*
        ��ȻDrawInstanced����û��ָ�����㱻����Ϊ����ͼԪ����ô������Ӧ�ñ�����Ϊ�㡢���б����������б��أ�
        �ع�5.5.2�ڿ�֪��ͼԪ����״̬ʵ��ID3D12GraphicsCommandList::IASetPrimitiveTopology���������á�
        �������һ����صĵ���ʾ����
        cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    */
    mCommandList->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    mCommandList->SetGraphicsRootDescriptorTable(0, mCbvHeap->GetGPUDescriptorHandleForHeapStart());

    /*
        �����Ҫע����ǣ���ʹ��������ʱ������һ��Ҫ��ID3D12GraphicsCommandList::DrawIndexedInstanced��������DrawInstanced�������л��ơ�
        void ID3D12GraphicsCommandList::DrawIndexedInstanced(
          UINT IndexCountPerInstance,
          UINT InstanceCount,
          UINT StartIndexLocation,
          INT BaseVertexLocation,
          UINT StartInstanceLocation);
        1��IndexCountPerInstance��ÿ��ʵ����Ҫ���Ƶ�����������
        2��InstanceCount������ʵ��һ�ֱ�����ʵ�����ĸ߼���������Ŀǰ���ԣ�����ֻ����һ��ʵ�����������ֵ����Ϊ1��
        3��StartIndexLocation��ָ�������������е�ĳ��Ԫ�أ�������Ϊ����ȡ����ʼ������
        4��BaseVertexLocation���ڱ��λ��Ƶ��ö�ȡ����֮ǰ��ҪΪÿ�����������ϴ�����ֵ��
        5��StartInstanceLocation������ʵ��һ�ֱ���Ϊʵ�����ĸ߼��������ݽ�������Ϊ0��
    */
    mCommandList->DrawIndexedInstanced(
        mBoxGeo->DrawArgs["box"].IndexCount,
        1, 0, 0, 0);

    // Indicate a state transition on the resource usage.
    mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(),
        D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));

    // Done recording commands.
    ThrowIfFailed(mCommandList->Close());

    // Add the command list to the queue for execution.
    ID3D12CommandList* cmdsLists[] = { mCommandList.Get() };
    mCommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

    // swap the back and front buffers
    ThrowIfFailed(mSwapChain->Present(0, 0));
    mCurrBackBuffer = (mCurrBackBuffer + 1) % SwapChainBufferCount;

    // Wait until frame commands are complete.  This waiting is inefficient and is
    // done for simplicity.  Later we will show how to organize our rendering code
    // so we do not have to wait per frame.
    FlushCommandQueue();
}

void BoxApp::OnMouseDown(WPARAM btnState, int x, int y)
{
    mLastMousePos.x = x;
    mLastMousePos.y = y;

    SetCapture(mhMainWnd);
}

void BoxApp::OnMouseUp(WPARAM btnState, int x, int y)
{
    ReleaseCapture();
}

void BoxApp::OnMouseMove(WPARAM btnState, int x, int y)
{
    if ((btnState & MK_LBUTTON) != 0)
    {
        // Make each pixel correspond to a quarter of a degree.
        float dx = XMConvertToRadians(0.25f * static_cast<float>(x - mLastMousePos.x));
        float dy = XMConvertToRadians(0.25f * static_cast<float>(y - mLastMousePos.y));

        // Update angles based on input to orbit camera around box.
        mTheta += dx;
        mPhi += dy;

        // Restrict the angle mPhi.
        mPhi = MathHelper::Clamp(mPhi, 0.1f, MathHelper::Pi - 0.1f);
    }
    else if ((btnState & MK_RBUTTON) != 0)
    {
        // Make each pixel correspond to 0.005 unit in the scene.
        float dx = 0.005f * static_cast<float>(x - mLastMousePos.x);
        float dy = 0.005f * static_cast<float>(y - mLastMousePos.y);

        // Update the camera radius based on input.
        mRadius += dx - dy;

        // Restrict the radius.
        mRadius = MathHelper::Clamp(mRadius, 3.0f, 15.0f);
    }

    mLastMousePos.x = x;
    mLastMousePos.y = y;
}

void BoxApp::BuildDescriptorHeaps()
{
    D3D12_DESCRIPTOR_HEAP_DESC cbvHeapDesc;
    cbvHeapDesc.NumDescriptors = 1;
    cbvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    cbvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    cbvHeapDesc.NodeMask = 0;
    ThrowIfFailed(md3dDevice->CreateDescriptorHeap(&cbvHeapDesc,
        IID_PPV_ARGS(&mCbvHeap)));
}

void BoxApp::BuildConstantBuffers()
{
    mObjectCB = std::make_unique<UploadBuffer<ObjectConstants>>(md3dDevice.Get(), 1, true);

    UINT objCBByteSize = d3dUtil::CalcConstantBufferByteSize(sizeof(ObjectConstants));

    D3D12_GPU_VIRTUAL_ADDRESS cbAddress = mObjectCB->Resource()->GetGPUVirtualAddress();
    // Offset to the ith object constant buffer in the buffer.
    int boxCBufIndex = 0;
    cbAddress += boxCBufIndex * objCBByteSize;

    D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc;
    cbvDesc.BufferLocation = cbAddress;
    cbvDesc.SizeInBytes = d3dUtil::CalcConstantBufferByteSize(sizeof(ObjectConstants));

    md3dDevice->CreateConstantBufferView(
        &cbvDesc,
        mCbvHeap->GetCPUDescriptorHandleForHeapStart());
}

void BoxApp::BuildRootSignature()
{
    // Shader programs typically require resources as input (constant buffers,
    // textures, samplers).  The root signature defines the resources the shader
    // programs expect.  If we think of the shader programs as a function, and
    // the input resources as function parameters, then the root signature can be
    // thought of as defining the function signature.  

    // Root parameter can be a table, root descriptor or root constants.
    CD3DX12_ROOT_PARAMETER slotRootParameter[1];

    // Create a single descriptor table of CBVs.
    CD3DX12_DESCRIPTOR_RANGE cbvTable;
    cbvTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0);
    slotRootParameter[0].InitAsDescriptorTable(1, &cbvTable);

    // A root signature is an array of root parameters.
    CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(1, slotRootParameter, 0, nullptr,
        D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

    // create a root signature with a single slot which points to a descriptor range consisting of a single constant buffer
    ComPtr<ID3DBlob> serializedRootSig = nullptr;
    ComPtr<ID3DBlob> errorBlob = nullptr;
    HRESULT hr = D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1,
        serializedRootSig.GetAddressOf(), errorBlob.GetAddressOf());

    if (errorBlob != nullptr)
    {
        ::OutputDebugStringA((char*)errorBlob->GetBufferPointer());
    }
    ThrowIfFailed(hr);

    ThrowIfFailed(md3dDevice->CreateRootSignature(
        0,
        serializedRootSig->GetBufferPointer(),
        serializedRootSig->GetBufferSize(),
        IID_PPV_ARGS(&mRootSignature)));
}

/*
    ���˿ռ�λ�ã�Direct3D�еĶ��㻹���Դ洢�����������ݡ�Ϊ�˹����Զ���Ķ����ʽ����������Ҫ����һ���ṹ��������ѡ���Ķ������ݡ�
    ���磬�����г������ֲ�ͬ���͵Ķ����ʽ��һ����λ�ú���ɫ��Ϣ��ɣ���һ������λ�á��������Լ�����2D�������깹��.
    struct Vertex1
    {
      XMFLOAT3 Pos;
      XMFLOAT4 Color;
    };

    struct Vertex2
    {
      XMFLOAT3 Pos;
      XMFLOAT3 Normal;
      XMFLOAT2 Tex0;
      XMFLOAT2 Tex1;
    };

    �����˶���ṹ��֮�����ǻ���Ҫ��Direct3D�ṩ�ö���ṹ���������ʹ���˽�Ӧ����������ṹ���е�ÿ����Ա��
    �û��ṩ��Direct3D��������������Ϊ���벼��������input layout description����
    �ýṹ��D3D12_INPUT_LAYOUT_DESC����ʾ��(�����'mInputLayout'����)

    ���벼������ʵ����������ɣ���һ����D3D12_INPUT_ELEMENT_DESCԪ�ع��ɵ����飬�Լ�һ����ʾ�������е�Ԫ��������������
    D3D12_INPUT_ELEMENT_DESC�����е�Ԫ�����������˶���ṹ��������Ӧ�ĳ�Ա��
    �����˵�����ĳ����ṹ������������Ա����ô��֮��Ӧ��D3D12_INPUT_ELEMENT_DESC����Ҳ����������Ԫ�ء�

    /// --------------------------------------------------
    ͼ6.1����:
        // D3D12_INPUT_ELEMENT_DESC�����е�Ԫ���붥��ṹ���еĳ�Աһһ��Ӧ��
        // ����������������Ϊ����Ԫ��ӳ�䵽������ɫ���ж�Ӧ�Ĳ����ṩ��;��
        struct Vertex
        {
            XMFLOAT3 Pos;       ===> VertexOut VS�е�iPos����
            XMFLOAT3 Normal;    ===> VertexOut VS�е�iNormal����
            XMFLOAT2 Tex0;      ===> VertexOut VS�е�iTex0����
            XMFLOAT2 Tex1;      ===> VertexOut VS�е�iTex1����
        };

        D3D12_INPUT_ELEMENT_DESC vertexDesc[] =
        {
            { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
            { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
            { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
            { "TEXCOORD", 1, DXGI_FORMAT_R32G32_FLOAT, 0, 32, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
        };

        VertexOut VS(float iPos : POSITION,
                        float iNormal : NORMAL,
                        float iTex0 : TEXCOORD0,
                        float iTex1 : TEXCOORD1)
    /// --------------------------------------------------

    D3D12_INPUT_ELEMENT_DESC�ṹ��Ķ������£�
    typedef struct D3D12_INPUT_ELEMENT_DESC
    {
      LPCSTR SemanticName;
      UINT SemanticIndex;
      DXGI_FORMAT Format;
      UINT InputSlot;
      UINT AlignedByteOffset;
      D3D12_INPUT_CLASSIFICATION InputSlotClass;
      UINT InstanceDataStepRate;
    } D3D12_INPUT_ELEMENT_DESC;

    1��SemanticName��һ����Ԫ����������ض��ַ��������ǳ�֮Ϊ���壨semantic������������Ԫ�ص�Ԥ����;��
        �ò�������������Ϸ�����������ͨ�����弴�ɽ�����ṹ�壨ͼ6.1�е�struct Vertex���е�Ԫ���붥����ɫ������ǩ��[1]
        ��vertex shader input signature����ͼ6.1 �е�VertexOut VS���ڲ������е�Ԫ��һһӳ����������ͼ6.1��ʾ��
    2��SemanticIndex�����ӵ������ϵ��������˳�Ա����ƶ����ɴ�ͼ6.1�п��������磬����ṹ���е�����������ܲ�ֹһ�飬
        ������������β�����һ�������������ڲ�����������������������ֳ������鲻ͬ���������ꡣ
        ����ɫ�������У�δ�������������彫Ĭ��������ֵΪ0��Ҳ����˵��ͼ6.1�е�POSITION��POSITION0�ȼۡ�
    3��Format����Direct3D�У�Ҫͨ��ö������DXGI_FORMAT�еĳ�Ա��ָ������Ԫ�صĸ�ʽ�����������ͣ���������һЩ���õĸ�ʽ��
        DXGI_FORMAT_R32_FLOAT     // 1D 32λ�������
        DXGI_FORMAT_R32G32_FLOAT    // 2D 32λ��������
        DXGI_FORMAT_R32G32B32_FLOAT  // 3D 32λ��������
        DXGI_FORMAT_R32G32B32A32_FLOAT // 4D 32λ��������

        DXGI_FORMAT_R8_UINT     // 1D 8λ�޷������ͱ���
        DXGI_FORMAT_R16G16_SINT   // 2D 16λ�з�����������
        DXGI_FORMAT_R32G32B32_UINT // 3D 32λ�޷�����������
        DXGI_FORMAT_R8G8B8A8_SINT  // 4D 8λ�з�����������
        DXGI_FORMAT_R8G8B8A8_UINT  // 4D 8λ�޷�����������
    4��InputSlot��ָ������Ԫ�����õ�����ۣ�input slot index��������Direct3D��֧��16������ۣ�����ֵΪ0��15����
        ����ͨ��������������װ��׶δ��ݶ������ݡ�Ŀǰ����ֻ���õ������0�������еĶ���Ԫ�ض�����ͬһ������ۣ���
        ���ڱ��µ�ϰ��2�н����漰������۵ı��ʵ����
    5��AlignedByteOffset�����ض�������У���C++����ṹ����׵�ַ������ĳ��Ԫ����ʼ��ַ��ƫ���������ֽڱ�ʾ����
        ���磬�����ж���ṹ���У�Ԫ��Pos��ƫ����Ϊ0�ֽڣ���Ϊ������ʼ��ַ�붥��ṹ����׵�ַһ�£�
        Ԫ��Normal��ƫ����Ϊ12�ֽڣ���Ϊ����Pos��ռ�õ��ֽ��������ҵ�Normal����ʼ��ַ��
        Ԫ��Tex0��ƫ����Ϊ24�ֽڣ���Ϊ���Pos��Normal�����ֽ����ſɻ�ȡTex0����ʼ��ַ��
        ͬ��Ԫ��Tex1��ƫ����Ϊ32�ֽڣ�ֻ������Pos��Normal��Tex0��3��Ԫ�ص����ֽ�������Ѱ��Tex1����ʼ��ַ��
        struct Vertex2
        {
          XMFLOAT3 Pos;  // ƫ����Ϊ0�ֽ�
          XMFLOAT3 Normal; // ƫ����Ϊ12�ֽ�
          XMFLOAT2 Tex0;  // ƫ����Ϊ24�ֽ�
          XMFLOAT2 Tex1;  // ƫ����Ϊ32�ֽ�
        };
    6��InputSlotClass���������ҰѴ˲���ָ��ΪD3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA[2]��
        ����һѡ�D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA[3]��������ʵ��ʵ������instancing�����ָ߼�������
    7��InstanceDataStepRate��Ŀǰ������ֵָ��Ϊ0����Ҫ����ʵ�������ָ߼��������򽫴˲�����Ϊ1��
        ��ǰ��Vertex1��Vertex2����������ṹ���������˵������Ӧ�����벼������Ϊ��
        D3D12_INPUT_ELEMENT_DESC desc1[] =
        {
          {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
          {"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}
        };

        D3D12_INPUT_ELEMENT_DESC desc2[] =
        {
          {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
          {"NORMAL",  0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
          {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,  0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}
          {"TEXCOORD", 1, DXGI_FORMAT_R32G32_FLOAT,  0, 32,  D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}
        };
*/
void BoxApp::BuildShadersAndInputLayout()
{
    HRESULT hr = S_OK;

    mvsByteCode = d3dUtil::CompileShader(L"Shaders\\color.hlsl", nullptr, "VS", "vs_5_0");
    mpsByteCode = d3dUtil::CompileShader(L"Shaders\\color.hlsl", nullptr, "PS", "ps_5_0");

    mInputLayout =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
    };
}

void BoxApp::BuildBoxGeometry()
{
    std::array<Vertex, 8> vertices =
    {
        Vertex({ XMFLOAT3(-1.0f, -1.0f, -1.0f), XMFLOAT4(Colors::White) }),
        Vertex({ XMFLOAT3(-1.0f, +1.0f, -1.0f), XMFLOAT4(Colors::Black) }),
        Vertex({ XMFLOAT3(+1.0f, +1.0f, -1.0f), XMFLOAT4(Colors::Red) }),
        Vertex({ XMFLOAT3(+1.0f, -1.0f, -1.0f), XMFLOAT4(Colors::Green) }),
        Vertex({ XMFLOAT3(-1.0f, -1.0f, +1.0f), XMFLOAT4(Colors::Blue) }),
        Vertex({ XMFLOAT3(-1.0f, +1.0f, +1.0f), XMFLOAT4(Colors::Yellow) }),
        Vertex({ XMFLOAT3(+1.0f, +1.0f, +1.0f), XMFLOAT4(Colors::Cyan) }),
        Vertex({ XMFLOAT3(+1.0f, -1.0f, +1.0f), XMFLOAT4(Colors::Magenta) })
    };

    std::array<std::uint16_t, 36> indices =
    {
        // front face
        0, 1, 2,
        0, 2, 3,

        // back face
        4, 6, 5,
        4, 7, 6,

        // left face
        4, 5, 1,
        4, 1, 0,

        // right face
        3, 2, 6,
        3, 6, 7,

        // top face
        1, 5, 6,
        1, 6, 2,

        // bottom face
        4, 0, 3,
        4, 3, 7
    };

    const UINT vbByteSize = (UINT)vertices.size() * sizeof(Vertex);
    const UINT ibByteSize = (UINT)indices.size() * sizeof(std::uint16_t);

    mBoxGeo = std::make_unique<MeshGeometry>();
    mBoxGeo->Name = "boxGeo";

    ThrowIfFailed(D3DCreateBlob(vbByteSize, &mBoxGeo->VertexBufferCPU));
    CopyMemory(mBoxGeo->VertexBufferCPU->GetBufferPointer(), vertices.data(), vbByteSize);

    ThrowIfFailed(D3DCreateBlob(ibByteSize, &mBoxGeo->IndexBufferCPU));
    CopyMemory(mBoxGeo->IndexBufferCPU->GetBufferPointer(), indices.data(), ibByteSize);

    mBoxGeo->VertexBufferGPU = d3dUtil::CreateDefaultBuffer(md3dDevice.Get(),
        mCommandList.Get(), vertices.data(), vbByteSize, mBoxGeo->VertexBufferUploader);

    mBoxGeo->IndexBufferGPU = d3dUtil::CreateDefaultBuffer(md3dDevice.Get(),
        mCommandList.Get(), indices.data(), ibByteSize, mBoxGeo->IndexBufferUploader);

    mBoxGeo->VertexByteStride = sizeof(Vertex);
    mBoxGeo->VertexBufferByteSize = vbByteSize;
    mBoxGeo->IndexFormat = DXGI_FORMAT_R16_UINT;
    mBoxGeo->IndexBufferByteSize = ibByteSize;

    SubmeshGeometry submesh;
    submesh.IndexCount = (UINT)indices.size();
    submesh.StartIndexLocation = 0;
    submesh.BaseVertexLocation = 0;

    mBoxGeo->DrawArgs["box"] = submesh;
}

void BoxApp::BuildPSO()
{
    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc;
    ZeroMemory(&psoDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
    psoDesc.InputLayout = { mInputLayout.data(), (UINT)mInputLayout.size() };
    psoDesc.pRootSignature = mRootSignature.Get();
    psoDesc.VS =
    {
        reinterpret_cast<BYTE*>(mvsByteCode->GetBufferPointer()),
        mvsByteCode->GetBufferSize()
    };
    psoDesc.PS =
    {
        reinterpret_cast<BYTE*>(mpsByteCode->GetBufferPointer()),
        mpsByteCode->GetBufferSize()
    };
    psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
    psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
    psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
    psoDesc.SampleMask = UINT_MAX;
    psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    psoDesc.NumRenderTargets = 1;
    psoDesc.RTVFormats[0] = mBackBufferFormat;
    psoDesc.SampleDesc.Count = m4xMsaaState ? 4 : 1;
    psoDesc.SampleDesc.Quality = m4xMsaaState ? (m4xMsaaQuality - 1) : 0;
    psoDesc.DSVFormat = mDepthStencilFormat;
    ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&mPSO)));
}
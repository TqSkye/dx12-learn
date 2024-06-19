////***************************************************************************************
//// BoxApp.cpp by Frank Luna (C) 2015 All Rights Reserved.
////
//// Shows how to draw a box in Direct3D 12.
////
//// Controls:
////   Hold the left mouse button down and move the mouse to rotate.
////   Hold the right mouse button down and move the mouse to zoom in and out.
////***************************************************************************************
//
///*
//  �������ģ�ͣ�Component Object Model��COM����һ����DirectX���ܱ����������������ʹ֮�����ݵļ�����
//  ����ͨ����COM������Ϊһ�ֽӿڣ������ǵ�ǰ��̵�Ŀ�ģ��콫������һ��C++����ʹ�á�
//  ��C++���Ա�дDirectX����ʱ��COM�����������˴����ײ�ϸ�ڡ�
//  ����ֻ��֪����Ҫ��ȡָ��ĳCOM�ӿڵ�ָ�룬������ض���������һCOM�ӿڵķ���������������C++�����еĹؼ���newȥ����һ��COM�ӿڡ�
//  ���⣬COM�����ͳ�������ô�������ˣ���ʹ����ĳ�ӿ�ʱ�����Ǳ�Ӧ��������Release������COM�ӿڵ����й��ܶ��Ǵ�IUnknown���COM�ӿڼ̳ж����ģ�����Release�������ڣ���
//  ��������delete��ɾ��������COM��������ü���Ϊ0ʱ�����������ͷ��Լ���ռ�õ��ڴ档
//  Ϊ�˸����û�����COM������������ڣ�Windows����ʱ�⣨Windows Runtime Library��WRL��
//  ר��Ϊ���ṩ��Microsoft::WRL::ComPtr�ࣨ#include <wrl.h>�������ǿ��԰���������COM���������ָ�롣
//  ��һ��ComPtrʵ������������Χʱ��������Զ�������ӦCOM�����Release�������̶�ʡ���������ֶ����õ��鷳��
//  �����г��õ�3��ComPtr�������¡�
//      1��Get������һ��ָ��˵ײ�COM�ӿڵ�ָ�롣�˷��������ڰ�ԭʼ��COM�ӿ�ָ����Ϊ�������ݸ�����������:
//          ComPtr<ID3D12RootSignature> mRootSignature;
//          ...
//          // SetGraphicsRootSignature��Ҫ��ȡID3D12RootSignature*���͵Ĳ���
//          mCommandList->SetGraphicsRootSignature(mRootSignature.Get());
//      2��GetAddressOf������ָ��˵ײ�COM�ӿ�ָ��ĵ�ַ��ƾ�˷����������ú�����������COM�ӿڵ�ָ�롣���磺
//          ComPtr<ID3D12CommandAllocator> mDirectCmdListAlloc;
//          ...
//          ThrowIfFailed(md3dDevice->CreateCommandAllocator(
//          D3D12_COMMAND_LIST_TYPE_DIRECT,
//          mDirectCmdListAlloc.GetAddressOf()));
//      3��Reset������ComPtrʵ������Ϊnullptr�ͷ���֮��ص��������ã�ͬʱ������ײ�COM�ӿڵ����ü�������
//      �˷����Ĺ����뽫ComPtrĿ��ʵ����ֵΪnullptr��Ч����ͬ��
//*/
//
//#include "../../Common/d3dApp.h"
//#include "../../Common/MathHelper.h"
//#include "../../Common/UploadBuffer.h"
//
//using Microsoft::WRL::ComPtr;
//using namespace DirectX;
//using namespace DirectX::PackedVector;
//
//struct Vertex
//{
//    XMFLOAT3 Pos;
//    XMFLOAT4 Color;
//};
//
///// <summary>
///// �����������õĳ�������
///// </summary>
//struct ObjectConstants
//{
//    XMFLOAT4X4 WorldViewProj = MathHelper::Identity4x4();
//};
//
//class BoxApp : public D3DApp
//{
//public:
//    BoxApp(HINSTANCE hInstance);
//    BoxApp(const BoxApp& rhs) = delete;
//    BoxApp& operator=(const BoxApp& rhs) = delete;
//    ~BoxApp();
//
//    virtual bool Initialize()override;
//
//private:
//    virtual void OnResize()override;
//    virtual void Update(const GameTimer& gt)override;
//    virtual void Draw(const GameTimer& gt)override;
//
//    virtual void OnMouseDown(WPARAM btnState, int x, int y)override;
//    virtual void OnMouseUp(WPARAM btnState, int x, int y)override;
//    virtual void OnMouseMove(WPARAM btnState, int x, int y)override;
//
//    void BuildDescriptorHeaps();
//    void BuildConstantBuffers();
//    void BuildRootSignature();
//    void BuildShadersAndInputLayout();
//    void BuildBoxGeometry();
//    void BuildPSO();
//
//private:
//
//    ComPtr<ID3D12RootSignature> mRootSignature = nullptr;
//    ComPtr<ID3D12DescriptorHeap> mCbvHeap = nullptr;
//
//    std::unique_ptr<UploadBuffer<ObjectConstants>> mObjectCB = nullptr;
//
//    std::unique_ptr<MeshGeometry> mBoxGeo = nullptr;
//
//    ComPtr<ID3DBlob> mvsByteCode = nullptr;
//    ComPtr<ID3DBlob> mpsByteCode = nullptr;
//
//    std::vector<D3D12_INPUT_ELEMENT_DESC> mInputLayout;
//
//    ComPtr<ID3D12PipelineState> mPSO = nullptr;
//
//    XMFLOAT4X4 mWorld = MathHelper::Identity4x4();
//    XMFLOAT4X4 mView = MathHelper::Identity4x4();
//    XMFLOAT4X4 mProj = MathHelper::Identity4x4();
//
//    float mTheta = 1.5f * XM_PI;
//    float mPhi = XM_PIDIV4;
//    float mRadius = 5.0f;
//
//    POINT mLastMousePos;
//};
//
//int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE prevInstance,
//    PSTR cmdLine, int showCmd)
//{
//    // Enable run-time memory check for debug builds.
//#if defined(DEBUG) | defined(_DEBUG)
//    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
//#endif
//
//    try
//    {
//        BoxApp theApp(hInstance);
//        if (!theApp.Initialize())
//            return 0;
//
//        return theApp.Run();
//    }
//    catch (DxException& e)
//    {
//        MessageBox(nullptr, e.ToString().c_str(), L"HR Failed", MB_OK);
//        return 0;
//    }
//}
//
//BoxApp::BoxApp(HINSTANCE hInstance)
//    : D3DApp(hInstance)
//{
//}
//
//BoxApp::~BoxApp()
//{
//}
//
//bool BoxApp::Initialize()
//{
//    if (!D3DApp::Initialize())
//        return false;
//
//    // Reset the command list to prep for initialization commands.
//    // ���������б�Ϊִ�г�ʼ����������׼������
//    ThrowIfFailed(mCommandList->Reset(mDirectCmdListAlloc.Get(), nullptr));
//
//    // ����������������
//    BuildDescriptorHeaps();
//    // ��������������
//    BuildConstantBuffers();
//    // ��ǩ������������
//    BuildRootSignature();
//    // ������ɫ��
//    BuildShadersAndInputLayout();
//    BuildBoxGeometry();
//    BuildPSO();
//
//    // Execute the initialization commands.
//    // ִ�г�ʼ������
//    ThrowIfFailed(mCommandList->Close());
//    ID3D12CommandList* cmdsLists[] = { mCommandList.Get() };
//    mCommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);
//
//    // Wait until initialization is complete.
//    // �ȴ���ʼ�����
//    FlushCommandQueue();
//
//    return true;
//}
//
//void BoxApp::OnResize()
//{
//    D3DApp::OnResize();
//
//    // The window resized, so update the aspect ratio and recompute the projection matrix.
//    // ���û������˴��ڳߴ磬������ݺ�Ȳ����¼���ͶӰ����
//    XMMATRIX P = XMMatrixPerspectiveFovLH(0.25f * MathHelper::Pi, AspectRatio(), 1.0f, 1000.0f);
//    XMStoreFloat4x4(&mProj, P);
//}
//
//void BoxApp::Update(const GameTimer& gt)
//{
//    // Convert Spherical to Cartesian coordinates.
//    // �������꣨Ҳ�������������꣩ת��Ϊ�ѿ�������
//    float x = mRadius * sinf(mPhi) * cosf(mTheta);
//    float z = mRadius * sinf(mPhi) * sinf(mTheta);
//    float y = mRadius * cosf(mPhi);
//
//    // Build the view matrix.
//    // �����۲����
//    XMVECTOR pos = XMVectorSet(x, y, z, 1.0f);
//    XMVECTOR target = XMVectorZero();
//    XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
//
//    XMMATRIX view = XMMatrixLookAtLH(pos, target, up);
//    XMStoreFloat4x4(&mView, view);
//
//    XMMATRIX world = XMLoadFloat4x4(&mWorld);
//    XMMATRIX proj = XMLoadFloat4x4(&mProj);
//    XMMATRIX worldViewProj = world * view * proj;
//
//    // Update the constant buffer with the latest worldViewProj matrix.
//    // �����µ�worldViewProj ���������³���������
//    ObjectConstants objConstants;
//    XMStoreFloat4x4(&objConstants.WorldViewProj, XMMatrixTranspose(worldViewProj));
//    mObjectCB->CopyData(0, objConstants);
//}
//
//void BoxApp::Draw(const GameTimer& gt)
//{
//    // Reuse the memory associated with command recording.
//    // We can only reset when the associated command lists have finished execution on the GPU.
//    // ���ü�¼�������õ��ڴ�
//    // ֻ�е�GPU�е������б�ִ����Ϻ����ǲſɶ����������
//    ThrowIfFailed(mDirectCmdListAlloc->Reset());
//
//    // A command list can be reset after it has been added to the command queue via ExecuteCommandList.
//    // Reusing the command list reuses memory.
//    // ͨ������ExecuteCommandList�������б����������к󣬱�ɶ�����������
//    // ���������б���������Ӧ���ڴ�
//    ThrowIfFailed(mCommandList->Reset(mDirectCmdListAlloc.Get(), mPSO.Get()));
//
//    mCommandList->RSSetViewports(1, &mScreenViewport);
//    mCommandList->RSSetScissorRects(1, &mScissorRect);
//
//    // Indicate a state transition on the resource usage.
//    // ������Դ����;ָʾ��״̬��ת�䣬�˴�����Դ�ӳ���״̬ת��Ϊ��ȾĿ��״̬
//    mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(),
//        D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));
//
//    // Clear the back buffer and depth buffer.
//    // �����̨����������Ȼ�����
//    mCommandList->ClearRenderTargetView(CurrentBackBufferView(), Colors::LightSteelBlue, 0, nullptr);
//    mCommandList->ClearDepthStencilView(DepthStencilView(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);
//
//    // Specify the buffers we are going to render to.
//    // ָ����Ҫ��Ⱦ��Ŀ�껺����
//    mCommandList->OMSetRenderTargets(1, &CurrentBackBufferView(), true, &DepthStencilView());
//
//    ID3D12DescriptorHeap* descriptorHeaps[] = { mCbvHeap.Get() };
//    mCommandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);
//
//    mCommandList->SetGraphicsRootSignature(mRootSignature.Get());
//
//    /*
//    һ������Ͷ��㻺���� ���� VertexBufferView() : ���㻺���� d3dUtile.h=>VertexBufferView():
//
//        Ϊ�˽����㻺�����󶨵���Ⱦ��ˮ���ϣ�������Ҫ��������Դ����һ�����㻺������ͼ��vertex buffer view����
//        ��RTV��render target view����ȾĿ����ͼ����ͬ���ǣ���������Ϊ���㻺������ͼ�����������ѡ�
//        ���ң����㻺������ͼ����D3D12_VERTEX_BUFFER_VIEW[5]�ṹ������ʾ��
//            typedef struct D3D12_VERTEX_BUFFER_VIEW
//            {
//              D3D12_GPU_VIRTUAL_ADDRESS BufferLocation;
//              UINT SizeInBytes;
//              UINT StrideInBytes;
//            }   D3D12_VERTEX_BUFFER_VIEW;
//            1��BufferLocation����������ͼ�Ķ��㻺������Դ�����ַ�����ǿ���ͨ��ID3D12Resource::GetGPUVirtualAddress��������ô˵�ַ��
//            2��SizeInBytes����������ͼ�Ķ��㻺������С�����ֽڱ�ʾ����
//            3��StrideInBytes��ÿ������Ԫ����ռ�õ��ֽ�����
//
//     ���� �ڶ��㻺���������Ӧ��ͼ������ɺ󣬱���Խ�������Ⱦ��ˮ���ϵ�һ������ۣ�input slot����󶨡�
//        ����һ�������Ǿ�������ˮ���е�����װ�����׶δ��ݶ��������ˡ��˲�������ͨ�����з�����ʵ�֡�
//            void ID3D12GraphicsCommandList::IASetVertexBuffers(
//              UINT StartSlot,
//              UINT NumView,
//              const D3D12_VERTEX_BUFFER_VIEW *pViews);
//            1��StartSlot���ڰ󶨶�����㻺����ʱ�����õ���ʼ����ۣ�������һ�����㻺��������������˲ۣ�������۹���16��������Ϊ0��15��
//            2��NumViews����Ҫ������۰󶨵Ķ��㻺��������������ͼ����pViews����ͼ���������������ʼ�����StartSlot������ֵΪ[��ͼ]��������Ҫ��[��ͼ]�����㻺��������ô��Щ�������������������[��ͼ]��󶨡�
//            3��pViews��ָ�򶥵㻺������ͼ�����е�һ��Ԫ�ص�ָ�롣
//        �����Ǹú�����һ������ʾ��:
//            D3D12_VERTEX_BUFFER_VIEW vbv;
//            vbv.BufferLocation = VertexBufferGPU->GetGPUVirtualAddress();
//            vbv.StrideInBytes = sizeof(Vertex);
//            vbv.SizeInBytes = 8 * sizeof(Vertex);
//
//            D3D12_VERTEX_BUFFER_VIEW vertexBuffers[1] = { vbv };
//            mCommandList->IASetVertexBuffers(0, 1, vertexBuffers);
//
//    ���������㻺�������õ�������ϲ��������ִ��ʵ�ʵĻ��Ʋ��������ǽ�Ϊ��������������Ⱦ��ˮ������׼�����ѡ�
//        �����һ������ͨ��ID3D12GraphicsCommandList::DrawInstanced����[6]�����ػ��ƶ��㡣
//        *** ע�⣺�������box�õ���ID3D12GraphicsCommandList::DrawIndexedInstanced����Ϊʹ�õ����������ƣ�����
//            void ID3D12GraphicsCommandList::DrawInstanced (
//              UINT VertexCountPerInstance,
//              UINT InstanceCount,
//              UINT StartVertexLocation,
//              UINT StartInstanceLocation);
//            1��VertexCountPerInstance��ÿ��ʵ��Ҫ���ƵĶ���������
//            2��InstanceCount������ʵ��һ�ֱ�����ʵ������instancing���ĸ߼���������Ŀǰ��˵������ֻ����һ��ʵ����������˲�������Ϊ1��
//            3��StartVertexLocation��ָ�����㻺�����ڵ�һ�������ƶ����������������ֵ��0Ϊ��׼����
//            4��StartInstanceLocation������ʵ��һ�ֱ�����ʵ�����ĸ߼���������ʱֻ�轫������Ϊ0��
//       VertexCountPerInstance��StartVertexLocation�������������˶��㻺�����н�Ҫ�����Ƶ�һ���������㣬��ͼ6.2��ʾ��
//    */
//    mCommandList->IASetVertexBuffers(0, 1, &mBoxGeo->VertexBufferView());
//
//    /*
//     һ������������������: ���� IndexBufferView() : ���������� d3dUtile.h=>IndexBufferView():
//        �붥�����ƣ�Ϊ��ʹGPU���Է����������飬����Ҫ�����Ƿ�����GPU�Ļ�������Դ��ID3D12Resource���ڡ�
//        ���ǳƴ洢�����Ļ�����Ϊ������������index buffer�������ڱ��������õ�d3dUtil::CreateDefaultBuffer������ͨ��void*������Ϊ�������뷺�����ݣ�
//        �����ζ������Ҳ�����ô˺������������������������������͵�Ĭ�ϻ���������
//        Ϊ��ʹ��������������Ⱦ��ˮ�߰󶨣�������Ҫ��������������Դ����һ��������������ͼ��index buffer view����
//        ��ͬ���㻺������ͼһ��������Ҳ����Ϊ������������ͼ�����������ѡ���������������ͼҪ�ɽṹ��D3D12_INDEX_BUFFER_VIEW����ʾ��
//        typedef struct D3D12_INDEX_BUFFER_VIEW
//        {
//          D3D12_GPU_VIRTUAL_ADDRESS BufferLocation;
//          UINT SizeInBytes;
//          DXGI_FORMAT Format;
//        } D3D12_INDEX_BUFFER_VIEW;
//        1��BufferLocation����������ͼ��������������Դ�����ַ�����ǿ���ͨ������ID3D12Resource::GetGPUVirtualAddress��������ȡ�˵�ַ��
//        2��SizeInBytes����������ͼ��������������С�����ֽڱ�ʾ����
//        3��Format�������ĸ�ʽ����Ϊ��ʾ16λ������DXGI_FORMAT_R16_UINT���ͣ����ʾ32λ������DXGI_FORMAT_R32_UINT���͡�
//            16λ���������Լ����ڴ�ʹ����ռ�ã����������ֵ��Χ������16λ���ݵı�ﷶΧ����Ҳֻ�ܲ���32λ�����ˡ�
//
//    �����붥�㻺�������ƣ�Ҳ����������Direct3D��Դ���ڣ�����ʹ��֮ǰ��������Ҫ�Ƚ����ǰ󶨵���Ⱦ��ˮ���ϡ�
//        ͨ��ID3D12GraphicsCommandList::IASetIndexBuffer�������ɽ������������󶨵�����װ�����׶Ρ�
//        ����Ĵ�����ʾ����������һ�����������������幹��������������Σ��Լ�Ϊ������������������ͼ�������󶨵���Ⱦ��ˮ�ߣ�
//            std::uint16_t indices[] = {
//              // ������ǰ����
//              0, 1, 2,
//              0, 2, 3,
//
//              // ����������
//              4, 6, 5,
//              4, 7, 6,
//
//              // �����������
//              4, 5, 1,
//              4, 1, 0,
//
//              // �������ұ���
//              3, 2, 6,
//              3, 6, 7,
//
//              // �������ϱ���
//              1, 5, 6,
//              1, 6, 2,
//
//              // �������±���
//              4, 0, 3,
//              4, 3, 7
//            };
//
//            const UINT ibByteSize = 36 * sizeof(std::uint16_t);
//
//            ComPtr<ID3D12Resource> IndexBufferGPU = nullptr;
//            ComPtr<ID3D12Resource> IndexBufferUploader = nullptr;
//            IndexBufferGPU = d3dUtil::CreateDefaultBuffer(md3dDevice.Get(),
//              mCommandList.Get(), indices, ibByteSize, IndexBufferUploader);
//
//            D3D12_INDEX_BUFFER_VIEW ibv;
//            ibv.BufferLocation = IndexBufferGPU->GetGPUVirtualAddress();
//            ibv.Format = DXGI_FORMAT_R16_UINT;
//            ibv.SizeInBytes = ibByteSize;
//
//            mCommandList->IASetIndexBuffer(&ibv);
//    */
//    mCommandList->IASetIndexBuffer(&mBoxGeo->IndexBufferView());
//
//    /*
//        ��ȻDrawInstanced����û��ָ�����㱻����Ϊ����ͼԪ����ô������Ӧ�ñ�����Ϊ�㡢���б����������б��أ�
//        �ع�5.5.2�ڿ�֪��ͼԪ����״̬ʵ��ID3D12GraphicsCommandList::IASetPrimitiveTopology���������á�
//        �������һ����صĵ���ʾ����
//        cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
//    */
//    mCommandList->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
//
//    /*
//        ͨ�������б�command list�����úø�ǩ�������Ǿ�����ID3D12GraphicsCommandList::SetGraphicsRootDescriptorTable������������������Ⱦ��ˮ����󶨡�
//        void ID3D12GraphicsCommandList::SetGraphicsRootDescriptorTable( 
//          UINT RootParameterIndex,
//          D3D12_GPU_DESCRIPTOR_HANDLE BaseDescriptor);
//        1��RootParameterIndex�������������������������󶨵��ļĴ����ۺţ��������á�
//        2��BaseDescriptor���˲���ָ�����ǽ�Ҫ����ɫ���󶨵����������е�һ��������λ�����������еľ����
//            ����˵�������ǩ��ָ����ǰ���������й��� 5 ��������������е�BaseDescriptor��������4���������������õ������������С�
//    */
//    mCommandList->SetGraphicsRootDescriptorTable(0, mCbvHeap->GetGPUDescriptorHandleForHeapStart());
//
//    /*
//        �����Ҫע����ǣ���ʹ��������ʱ������һ��Ҫ��ID3D12GraphicsCommandList::DrawIndexedInstanced��������DrawInstanced�������л��ơ�
//        void ID3D12GraphicsCommandList::DrawIndexedInstanced(
//          UINT IndexCountPerInstance,
//          UINT InstanceCount,
//          UINT StartIndexLocation,
//          INT BaseVertexLocation,
//          UINT StartInstanceLocation);
//        1��IndexCountPerInstance��ÿ��ʵ����Ҫ���Ƶ�����������
//        2��InstanceCount������ʵ��һ�ֱ�����ʵ�����ĸ߼���������Ŀǰ���ԣ�����ֻ����һ��ʵ�����������ֵ����Ϊ1��
//        3��StartIndexLocation��ָ�������������е�ĳ��Ԫ�أ�������Ϊ����ȡ����ʼ������
//        4��BaseVertexLocation���ڱ��λ��Ƶ��ö�ȡ����֮ǰ��ҪΪÿ�����������ϴ�����ֵ��
//        5��StartInstanceLocation������ʵ��һ�ֱ���Ϊʵ�����ĸ߼��������ݽ�������Ϊ0��
//    */
//    mCommandList->DrawIndexedInstanced(
//        mBoxGeo->DrawArgs["box"].IndexCount,
//        1, 0, 0, 0);
//
//    // Indicate a state transition on the resource usage.
//    // ������Դ����;ָʾ��״̬��ת�䣬�˴�����Դ����ȾĿ��״̬ת��Ϊ����״̬
//    mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(),
//        D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));
//
//    // Done recording commands.
//    // �������ļ�¼
//    ThrowIfFailed(mCommandList->Close());
//
//    // Add the command list to the queue for execution.
//    // ��������������ִ�е������б�
//    ID3D12CommandList* cmdsLists[] = { mCommandList.Get() };
//    mCommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);
//
//    // swap the back and front buffers
//    // ������̨��������ǰ̨������
//    ThrowIfFailed(mSwapChain->Present(0, 0));
//    mCurrBackBuffer = (mCurrBackBuffer + 1) % SwapChainBufferCount;
//
//    // Wait until frame commands are complete.  This waiting is inefficient and is
//    // done for simplicity.  Later we will show how to organize our rendering code
//    // so we do not have to wait per frame.
//    // �ȴ����ƴ�֡��һϵ������ִ����ϡ����ֵȴ��ķ�����Ȼ��ȴҲ��Ч
//    // �ں��潫չʾ���������֯��Ⱦ���룬ʹ���ǲ����ڻ���ÿһ֡ʱ���ȴ�
//    FlushCommandQueue();
//}
//
//
//void BoxApp::OnMouseDown(WPARAM btnState, int x, int y)
//{
//    mLastMousePos.x = x;
//    mLastMousePos.y = y;
//
//    SetCapture(mhMainWnd);
//}
//
//void BoxApp::OnMouseUp(WPARAM btnState, int x, int y)
//{
//    ReleaseCapture();
//}
//
///* 
//    ������:
//
//    һ�������������������������ƶ�/��ת/���Ŷ��ı䣬�۲������������������ƶ�/��ת���ı䣬ͶӰ�����洰�ڴ�С�ĵ������ı䡣
//    �ڱ��µ���ʾ�����У��û�����ͨ���������ת���ƶ���������任�۲�Ƕȡ���ˣ�������ÿһ֡��Ҫ��Update������
//    ���µĹ۲���������¡����硪�۲졪ͶӰ��3�־�����϶��ɵĸ��Ͼ���
//*/
//void BoxApp::OnMouseMove(WPARAM btnState, int x, int y)
//{
//    if ((btnState & MK_LBUTTON) != 0)
//    {
//        // Make each pixel correspond to a quarter of a degree.
//        // ���������ƶ����������ת�Ƕȣ���ÿ�����ذ��˽Ƕȵ�1/4������ת
//        float dx = XMConvertToRadians(0.25f * static_cast<float>(x - mLastMousePos.x));
//        float dy = XMConvertToRadians(0.25f * static_cast<float>(y - mLastMousePos.y));
//
//        // Update angles based on input to orbit camera around box.
//        // ���������������������������������ת�ĽǶ�
//        mTheta += dx;
//        mPhi += dy;
//
//        // Restrict the angle mPhi.
//        // ���ƽǶ�mPhi�ķ�Χ
//        mPhi = MathHelper::Clamp(mPhi, 0.1f, MathHelper::Pi - 0.1f);
//    }
//    else if ((btnState & MK_RBUTTON) != 0)
//    {
//        // Make each pixel correspond to 0.005 unit in the scene.
//        // ʹ�����е�ÿ�����ذ�����ƶ������0.005����������
//        float dx = 0.005f * static_cast<float>(x - mLastMousePos.x);
//        float dy = 0.005f * static_cast<float>(y - mLastMousePos.y);
//
//        // Update the camera radius based on input.
//        // ���������������������Ŀ��ӷ�Χ�뾶
//        mRadius += dx - dy;
//
//        // Restrict the radius.
//        // ���ƿ��Ӱ뾶�ķ�Χ
//        mRadius = MathHelper::Clamp(mRadius, 3.0f, 15.0f);
//    }
//
//    mLastMousePos.x = x;
//    mLastMousePos.y = y;
//}
//
///// <summary>
///// ����������������
///// 
///// ������������������Ҫ�������D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV�������������������
///// ���ֶ��ڿ��Ի�ϴ洢��������������������ɫ����Դ��������������ʣ�unordered access����������
///// Ϊ�˴����Щ�����͵���������������ҪΪ֮�����������͵���ʽ�������ѣ�
///// 
///// ��δ���������֮ǰ������ȾĿ������/ģ�建������������Դ�������ѵĹ��̺����ơ�Ȼ��������ȴ����һ����Ҫ������
///// �Ǿ����ڴ�������ɫ�����������Դ��������ʱ������Ҫ�ѱ�־Flagsָ��ΪDESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE��
///// �ڱ��µ�ʾ�������У����ǲ�û��ʹ��SRV��shader resource view����ɫ����Դ��ͼ����������UAV��unordered access view��
///// ���������ͼ�������������ǻ�����һ��������ѣ����ֻ�贴��һ�����е���CBV�������ĶѼ��ɡ�
///// </summary>
//void BoxApp::BuildDescriptorHeaps()
//{
//    D3D12_DESCRIPTOR_HEAP_DESC cbvHeapDesc;
//    cbvHeapDesc.NumDescriptors = 1;
//    cbvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
//    cbvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
//    cbvHeapDesc.NodeMask = 0;
//    ThrowIfFailed(md3dDevice->CreateDescriptorHeap(&cbvHeapDesc,
//        IID_PPV_ARGS(&mCbvHeap)));
//}
//
///// <summary>
///// ��������������
///// 
//
///// 
///// </summary>
//void BoxApp::BuildConstantBuffers()
//{
//    // �˳����������洢�˻���n(����n==1)����������ĳ�������
//    mObjectCB = std::make_unique<UploadBuffer<ObjectConstants>>(md3dDevice.Get(), 1, true);
//
//    UINT objCBByteSize = d3dUtil::CalcConstantBufferByteSize(sizeof(ObjectConstants));
//
//    // ����������ʼ��ַ(������Ϊ0���Ǹ������������ĵ�ַ��
//    D3D12_GPU_VIRTUAL_ADDRESS cbAddress = mObjectCB->Resource()->GetGPUVirtualAddress();
//    // Offset to the ith object constant buffer in the buffer.
//    // ƫ�Ƶ������������е�i����������Ӧ�ĳ�������
//    // ����ȡi = 0
//    int boxCBufIndex = 0;
//    cbAddress += boxCBufIndex * objCBByteSize;
//
//    /// �ṹ��D3D12_CONSTANT_BUFFER_VIEW_DESC�������ǰ󶨵�HLSL�����������ṹ��ĳ�����������Դ�Ӽ���
//    /// ����ǰ�����ᵽ�ģ���������������洢��һ������[��ͼ]�����峣�����ݵĳ������飬
//    /// ��ô���ǾͿ���ͨ��BufferLocation��SizeInBytes��������ȡ��[��ͼ]������ĳ������ݡ�
//    /// ���ǵ�Ӳ�������󣨼�Ӳ������С����ռ䣩����ԱSizeInBytes��BufferLocation����Ϊ256B����������
//    /// ���磬��������������Ա��ֵ��ָ��Ϊ64����ô���ǽ��������е��Դ���
//    /*
//        error:
//        D3D12 ERROR: ID3D12Device::CreateConstantBufferView: SizeInBytes of 64 is invalid. Device requires SizeInBytes be a multiple of 256.
//        D3D12����ID3D12Device::CreateConstantBufferView: ��SizeInBytes��ֵ����Ϊ64����Ч�ġ��豸Ҫ��SizeInBytes��ֵΪ256����������
//        D3D12 ERROR: ID3D12Device:: CreateConstantBufferView: BufferLocation of 64 is invalid. Device requires BufferLocation be a multiple of 256.
//        D3D12����:ID3D12Device:: CreateConstantBufferView:��BufferLocation��ֵ����Ϊ64����Ч�ġ��豸Ҫ��BufferLocation��ֵΪ256����������
//    */
//    D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc;
//    cbvDesc.BufferLocation = cbAddress;
//    cbvDesc.SizeInBytes = d3dUtil::CalcConstantBufferByteSize(sizeof(ObjectConstants));
//
//    md3dDevice->CreateConstantBufferView(
//        &cbvDesc,
//        mCbvHeap->GetCPUDescriptorHandleForHeapStart());
//}
//
///// <summary>
///// ��ǩ������������
///// 
///// ͨ���������ڻ��Ƶ��ÿ�ʼִ��֮ǰ������Ӧ����ͬ����ɫ����������ĸ������͵���Դ�󶨵���Ⱦ��ˮ���ϡ�
///// ʵ���ϣ���ͬ���͵���Դ�ᱻ�󶨵��ض��ļĴ����ۣ�register slot���ϣ��Թ���ɫ��������ʡ�
///// ����˵��ǰ�Ĵ����еĶ�����ɫ����������ɫ����Ҫ����һ���󶨵��Ĵ���b0�ĳ������������ڱ���ĺ��������У�
///// ���ǻ��õ���������ɫ�����߼������÷�������ʹ�������������������texture���Ͳ�������sampler��
///// ��������ԵļĴ��������[14]��
///// 
///// // ��������Դ�󶨵�����Ĵ�����0
//    //Texture2D  gDiffuseMap : register(t0);
//    //
//    //// �����в�������Դ���ΰ󶨵��������Ĵ�����0~5
//    //SamplerState gsamPointWrap : register(s0);
//    //SamplerState gsamPointClamp : register(s1);
//    //SamplerState gsamLinearWrap : register(s2);
//    //SamplerState gsamLinearClamp : register(s3);
//    //SamplerState gsamAnisotropicWrap : register(s4);
//    //SamplerState gsamAnisotropicClamp : register(s5);
//    //
//    //// ��������������Դ��cbuffer���󶨵������������Ĵ�����0
//    //cbuffer cbPerObject : register(b0)
//    //{
//    //    float4x4 gWorld;
//    //    float4x4 gTexTransform;
//    //};
//    //
//    //// ���ƹ��������õ����������
//    //cbuffer cbPass : register(b1)
//    //{
//    //    float4x4 gView;
//    //    float4x4 gProj;
//    //    [...] // Ϊƪ����ʡ�Ե������ֶ�
//    //};
//    //
//    //// ����ÿ�ֲ�������ĸ��ֲ�ͬ�ĳ�������
//    //cbuffer cbMaterial : register(b2)
//    //{
//    //    float4  gDiffuseAlbedo;
//    //    float3  gFresnelR0;
//    //    float  gRoughness;
//    //    float4x4 gMatTransform;
//    //};
///// 
///// ��ǩ����root signature��������ǣ���ִ�л�������֮ǰ����ЩӦ�ó��򽫰󶨵���Ⱦ��ˮ���ϵ���Դ�����ǻᱻӳ�䵽��ɫ���Ķ�Ӧ����Ĵ�����
///// ��ǩ��һ��Ҫ��ʹ��������ɫ������ݣ����ڻ��ƿ�ʼ֮ǰ����ǩ��һ��ҪΪ��ɫ���ṩ��ִ���ڼ���Ҫ�󶨵���Ⱦ��ˮ�ߵ�������Դ����
///// �ڴ�����ˮ��״̬����pipeline state object��ʱ��Դ˽�����֤���μ�6.9�ڣ�����ͬ�Ļ��Ƶ��ÿ��ܻ��õ�һ�鲻ͬ����ɫ������
///// ��Ҳ����ζ��Ҫ�õ���ͬ�ĸ�ǩ����
///// 
///// ��Direct3D�У���ǩ����ID3D12RootSignature�ӿ�����ʾ������һ���������Ƶ��ù�������ɫ��������Դ�ĸ�������root parameter��
///// ������ɡ������������Ǹ�������root constant��������������root descriptor��������������descriptor table����
///// �����ڱ����н�ʹ�����������������������ڵ�7���н������ۡ���������ָ���������������д�����������һ����������
///// 
///// ����Ĵ��봴����һ����ǩ�������ĸ�����Ϊһ�������������С��������һ��CBV��������������ͼ��constant buffer view����
///// 
///// </summary>
//void BoxApp::BuildRootSignature()
//{
//    // Shader programs typically require resources as input (constant buffers,
//    // textures, samplers).  The root signature defines the resources the shader
//    // programs expect.  If we think of the shader programs as a function, and
//    // the input resources as function parameters, then the root signature can be
//    // thought of as defining the function signature.  
//    // ��ɫ������һ����Ҫ����Դ��Ϊ���루���糣���������������������ȣ�
//    // ��ǩ����������ɫ����������ľ�����Դ
//    // �������ɫ��������һ�������������������Դ�����������ݵĲ������ݣ���ô������Ƶ���Ϊ��ǩ��
//    // ������Ǻ���ǩ��
//    // 
//    // Root parameter can be a table, root descriptor or root constants.
//    // �����������������������������������
//    CD3DX12_ROOT_PARAMETER slotRootParameter[1];
//
//    /*
//    * ������δ���:
//    * ��δ��봴����һ����������Ŀ���ǽ�����һ��CBV����������󶨵������������Ĵ���0����HLSL�����е�register(b0)��
//    */
//    // Create a single descriptor table of CBVs.
//    // �����ɵ���CBV����ɵ���������
//    CD3DX12_DESCRIPTOR_RANGE cbvTable;
//    cbvTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 
//        1, // ���е�����������
//        0);// �������������������˻�׼��ɫ���Ĵ�����base shader register��
//    slotRootParameter[0].InitAsDescriptorTable(
//        1,          // ���������������
//        &cbvTable); // ָ�����������������ָ��
//
//    // A root signature is an array of root parameters.
//    // ��ǩ����һ�����������
//    CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(1, slotRootParameter, 0, nullptr,
//        D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
//
//    // create a root signature with a single slot which points to a descriptor range consisting of a single constant buffer
//    // �õ����Ĵ�����������һ����ǩ�����ò�λָ��һ�������е�������������������������
//    ComPtr<ID3DBlob> serializedRootSig = nullptr;
//    ComPtr<ID3DBlob> errorBlob = nullptr;
//    HRESULT hr = D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1,
//        serializedRootSig.GetAddressOf(), errorBlob.GetAddressOf());
//
//    if (errorBlob != nullptr)
//    {
//        ::OutputDebugStringA((char*)errorBlob->GetBufferPointer());
//    }
//    ThrowIfFailed(hr);
//
//    ThrowIfFailed(md3dDevice->CreateRootSignature(
//        0,
//        serializedRootSig->GetBufferPointer(),
//        serializedRootSig->GetBufferSize(),
//        IID_PPV_ARGS(&mRootSignature)));
//}
//
///*
//    ���˿ռ�λ�ã�Direct3D�еĶ��㻹���Դ洢�����������ݡ�Ϊ�˹����Զ���Ķ����ʽ����������Ҫ����һ���ṹ��������ѡ���Ķ������ݡ�
//    ���磬�����г������ֲ�ͬ���͵Ķ����ʽ��һ����λ�ú���ɫ��Ϣ��ɣ���һ������λ�á��������Լ�����2D�������깹��.
//    struct Vertex1
//    {
//      XMFLOAT3 Pos;
//      XMFLOAT4 Color;
//    };
//
//    struct Vertex2
//    {
//      XMFLOAT3 Pos;
//      XMFLOAT3 Normal;
//      XMFLOAT2 Tex0;
//      XMFLOAT2 Tex1;
//    };
//
//    �����˶���ṹ��֮�����ǻ���Ҫ��Direct3D�ṩ�ö���ṹ���������ʹ���˽�Ӧ����������ṹ���е�ÿ����Ա��
//    �û��ṩ��Direct3D��������������Ϊ���벼��������input layout description����
//    �ýṹ��D3D12_INPUT_LAYOUT_DESC����ʾ��(�����'mInputLayout'����)
//
//    ���벼������ʵ����������ɣ���һ����D3D12_INPUT_ELEMENT_DESCԪ�ع��ɵ����飬�Լ�һ����ʾ�������е�Ԫ��������������
//    D3D12_INPUT_ELEMENT_DESC�����е�Ԫ�����������˶���ṹ��������Ӧ�ĳ�Ա��
//    �����˵�����ĳ����ṹ������������Ա����ô��֮��Ӧ��D3D12_INPUT_ELEMENT_DESC����Ҳ����������Ԫ�ء�
//
//    /// --------------------------------------------------
//    ͼ6.1����:
//        // D3D12_INPUT_ELEMENT_DESC�����е�Ԫ���붥��ṹ���еĳ�Աһһ��Ӧ��
//        // ����������������Ϊ����Ԫ��ӳ�䵽������ɫ���ж�Ӧ�Ĳ����ṩ��;��
//        struct Vertex
//        {
//            XMFLOAT3 Pos;       ===> VertexOut VS�е�iPos����
//            XMFLOAT3 Normal;    ===> VertexOut VS�е�iNormal����
//            XMFLOAT2 Tex0;      ===> VertexOut VS�е�iTex0����
//            XMFLOAT2 Tex1;      ===> VertexOut VS�е�iTex1����
//        };
//
//        D3D12_INPUT_ELEMENT_DESC vertexDesc[] =
//        {
//            { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
//            { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
//            { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
//            { "TEXCOORD", 1, DXGI_FORMAT_R32G32_FLOAT, 0, 32, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
//        };
//
//        VertexOut VS(float iPos : POSITION,
//                        float iNormal : NORMAL,
//                        float iTex0 : TEXCOORD0,
//                        float iTex1 : TEXCOORD1)
//    /// --------------------------------------------------
//
//    D3D12_INPUT_ELEMENT_DESC�ṹ��Ķ������£�
//    typedef struct D3D12_INPUT_ELEMENT_DESC
//    {
//      LPCSTR SemanticName;
//      UINT SemanticIndex;
//      DXGI_FORMAT Format;
//      UINT InputSlot;
//      UINT AlignedByteOffset;
//      D3D12_INPUT_CLASSIFICATION InputSlotClass;
//      UINT InstanceDataStepRate;
//    } D3D12_INPUT_ELEMENT_DESC;
//
//    1��SemanticName��һ����Ԫ����������ض��ַ��������ǳ�֮Ϊ���壨semantic������������Ԫ�ص�Ԥ����;��
//        �ò�������������Ϸ�����������ͨ�����弴�ɽ�����ṹ�壨ͼ6.1�е�struct Vertex���е�Ԫ���붥����ɫ������ǩ��[1]
//        ��vertex shader input signature����ͼ6.1 �е�VertexOut VS���ڲ������е�Ԫ��һһӳ����������ͼ6.1��ʾ��
//    2��SemanticIndex�����ӵ������ϵ��������˳�Ա����ƶ����ɴ�ͼ6.1�п��������磬����ṹ���е�����������ܲ�ֹһ�飬
//        ������������β�����һ�������������ڲ�����������������������ֳ������鲻ͬ���������ꡣ
//        ����ɫ�������У�δ�������������彫Ĭ��������ֵΪ0��Ҳ����˵��ͼ6.1�е�POSITION��POSITION0�ȼۡ�
//    3��Format����Direct3D�У�Ҫͨ��ö������DXGI_FORMAT�еĳ�Ա��ָ������Ԫ�صĸ�ʽ�����������ͣ���������һЩ���õĸ�ʽ��
//        DXGI_FORMAT_R32_FLOAT     // 1D 32λ�������
//        DXGI_FORMAT_R32G32_FLOAT    // 2D 32λ��������
//        DXGI_FORMAT_R32G32B32_FLOAT  // 3D 32λ��������
//        DXGI_FORMAT_R32G32B32A32_FLOAT // 4D 32λ��������
//
//        DXGI_FORMAT_R8_UINT     // 1D 8λ�޷������ͱ���
//        DXGI_FORMAT_R16G16_SINT   // 2D 16λ�з�����������
//        DXGI_FORMAT_R32G32B32_UINT // 3D 32λ�޷�����������
//        DXGI_FORMAT_R8G8B8A8_SINT  // 4D 8λ�з�����������
//        DXGI_FORMAT_R8G8B8A8_UINT  // 4D 8λ�޷�����������
//    4��InputSlot��ָ������Ԫ�����õ�����ۣ�input slot index��������Direct3D��֧��16������ۣ�����ֵΪ0��15����
//        ����ͨ��������������װ��׶δ��ݶ������ݡ�Ŀǰ����ֻ���õ������0�������еĶ���Ԫ�ض�����ͬһ������ۣ���
//        ���ڱ��µ�ϰ��2�н����漰������۵ı��ʵ����
//    5��AlignedByteOffset�����ض�������У���C++����ṹ����׵�ַ������ĳ��Ԫ����ʼ��ַ��ƫ���������ֽڱ�ʾ����
//        ���磬�����ж���ṹ���У�Ԫ��Pos��ƫ����Ϊ0�ֽڣ���Ϊ������ʼ��ַ�붥��ṹ����׵�ַһ�£�
//        Ԫ��Normal��ƫ����Ϊ12�ֽڣ���Ϊ����Pos��ռ�õ��ֽ��������ҵ�Normal����ʼ��ַ��
//        Ԫ��Tex0��ƫ����Ϊ24�ֽڣ���Ϊ���Pos��Normal�����ֽ����ſɻ�ȡTex0����ʼ��ַ��
//        ͬ��Ԫ��Tex1��ƫ����Ϊ32�ֽڣ�ֻ������Pos��Normal��Tex0��3��Ԫ�ص����ֽ�������Ѱ��Tex1����ʼ��ַ��
//        struct Vertex2
//        {
//          XMFLOAT3 Pos;  // ƫ����Ϊ0�ֽ�
//          XMFLOAT3 Normal; // ƫ����Ϊ12�ֽ�
//          XMFLOAT2 Tex0;  // ƫ����Ϊ24�ֽ�
//          XMFLOAT2 Tex1;  // ƫ����Ϊ32�ֽ�
//        };
//    6��InputSlotClass���������ҰѴ˲���ָ��ΪD3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA[2]��
//        ����һѡ�D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA[3]��������ʵ��ʵ������instancing�����ָ߼�������
//    7��InstanceDataStepRate��Ŀǰ������ֵָ��Ϊ0����Ҫ����ʵ�������ָ߼��������򽫴˲�����Ϊ1��
//        ��ǰ��Vertex1��Vertex2����������ṹ���������˵������Ӧ�����벼������Ϊ��
//        D3D12_INPUT_ELEMENT_DESC desc1[] =
//        {
//          {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
//          {"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}
//        };
//
//        D3D12_INPUT_ELEMENT_DESC desc2[] =
//        {
//          {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
//          {"NORMAL",  0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
//          {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,  0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}
//          {"TEXCOORD", 1, DXGI_FORMAT_R32G32_FLOAT,  0, 32,  D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}
//        };
//*/
///// ������ɫ��
//void BoxApp::BuildShadersAndInputLayout()
//{
//    HRESULT hr = S_OK;
//
//    /*
//        ��Direct3D�У���ɫ����������ȱ�����Ϊһ�ֿ���ֲ���ֽ��롣��������ͼ���������򽫻�ȡ��Щ�ֽ��룬
//        ���������±���Ϊ��Ե�ǰϵͳGPU���Ż��ı���ָ��[ATI1]�����ǿ����������ڼ������к�������ɫ�����б��롣
//        HRESULT D3DCompileFromFile(
//          LPCWSTR pFileName,
//          const D3D_SHADER_MACRO *pDefines,
//          ID3DInclude *pInclude,
//          LPCSTR pEntrypoint,
//          LPCSTR pTarget,
//          UINT Flags1,
//          UINT Flags2,
//          ID3DBlob **ppCode,
//          ID3DBlob **ppErrorMsgs);
//
//        ** D3DCompileFromFile������d3dUtil::CompileShader�У�����D3DCompileFromFileԭ�͸�����˵������d3dUtil::CompileShader�鿴��
//    */
//
//    mvsByteCode = d3dUtil::CompileShader(L"E:\\DX12Book\\DX12LearnProject\\DX12Learn\\LearnDemo\\Box\\Shaders\\color.hlsl", nullptr, "VS", "vs_5_0");
//    mpsByteCode = d3dUtil::CompileShader(L"E:\\DX12Book\\DX12LearnProject\\DX12Learn\\LearnDemo\\Box\\Shaders\\color.hlsl", nullptr, "PS", "ps_5_0");
//
//    mInputLayout =
//    {
//        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
//        { "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
//    };
//}
//
//void BoxApp::BuildBoxGeometry()
//{
//    std::array<Vertex, 8> vertices =
//    {
//        Vertex({ XMFLOAT3(-1.0f, -1.0f, -1.0f), XMFLOAT4(Colors::White) }),
//        Vertex({ XMFLOAT3(-1.0f, +1.0f, -1.0f), XMFLOAT4(Colors::Black) }),
//        Vertex({ XMFLOAT3(+1.0f, +1.0f, -1.0f), XMFLOAT4(Colors::Red) }),
//        Vertex({ XMFLOAT3(+1.0f, -1.0f, -1.0f), XMFLOAT4(Colors::Green) }),
//        Vertex({ XMFLOAT3(-1.0f, -1.0f, +1.0f), XMFLOAT4(Colors::Blue) }),
//        Vertex({ XMFLOAT3(-1.0f, +1.0f, +1.0f), XMFLOAT4(Colors::Yellow) }),
//        Vertex({ XMFLOAT3(+1.0f, +1.0f, +1.0f), XMFLOAT4(Colors::Cyan) }),
//        Vertex({ XMFLOAT3(+1.0f, -1.0f, +1.0f), XMFLOAT4(Colors::Magenta) })
//    };
//
//    std::array<std::uint16_t, 36> indices =
//    {
//        // front face   // ������ǰ����
//        0, 1, 2,
//        0, 2, 3,
//
//        // back face    // ����������
//        4, 6, 5,
//        4, 7, 6,
//
//        // left face    // �����������
//        4, 5, 1,
//        4, 1, 0,
//
//        // right face   // �������ұ���
//        3, 2, 6,
//        3, 6, 7,
//
//        // top face     // �������ϱ���
//        1, 5, 6,
//        1, 6, 2,
//
//        // bottom face  // �������±���
//        4, 0, 3,
//        4, 3, 7
//    };
//
//    const UINT vbByteSize = (UINT)vertices.size() * sizeof(Vertex);
//    const UINT ibByteSize = (UINT)indices.size() * sizeof(std::uint16_t);
//
//    mBoxGeo = std::make_unique<MeshGeometry>();
//    mBoxGeo->Name = "boxGeo";
//
//    ThrowIfFailed(D3DCreateBlob(vbByteSize, &mBoxGeo->VertexBufferCPU));
//    CopyMemory(mBoxGeo->VertexBufferCPU->GetBufferPointer(), vertices.data(), vbByteSize);
//
//    ThrowIfFailed(D3DCreateBlob(ibByteSize, &mBoxGeo->IndexBufferCPU));
//    CopyMemory(mBoxGeo->IndexBufferCPU->GetBufferPointer(), indices.data(), ibByteSize);
//
//    mBoxGeo->VertexBufferGPU = d3dUtil::CreateDefaultBuffer(md3dDevice.Get(),
//        mCommandList.Get(), vertices.data(), vbByteSize, mBoxGeo->VertexBufferUploader);
//
//    mBoxGeo->IndexBufferGPU = d3dUtil::CreateDefaultBuffer(md3dDevice.Get(),
//        mCommandList.Get(), indices.data(), ibByteSize, mBoxGeo->IndexBufferUploader);
//
//    mBoxGeo->VertexByteStride = sizeof(Vertex);
//    mBoxGeo->VertexBufferByteSize = vbByteSize;
//    mBoxGeo->IndexFormat = DXGI_FORMAT_R16_UINT;
//    mBoxGeo->IndexBufferByteSize = ibByteSize;
//
//    SubmeshGeometry submesh;
//    submesh.IndexCount = (UINT)indices.size();
//    submesh.StartIndexLocation = 0;
//    submesh.BaseVertexLocation = 0;
//
//    mBoxGeo->DrawArgs["box"] = submesh;
//}
//
///*
//    ��ˮ��״̬����:
//    ��ĿǰΪֹ�������Ѿ�չʾ����д���벼������������������ɫ����������ɫ�����Լ����ù�դ��״̬����3�����裬
//    ��δ��������ν���Щ����󶨵�ͼ����ˮ���ϣ�����ʵ�ʻ���ͼ�Ρ����������ͼ����ˮ��״̬�Ķ���ͳ��Ϊ��ˮ��״̬����
//    Pipeline State Object��PSO������ID3D12PipelineState�ӿ�����ʾ��Ҫ����PSO����������Ҫ��дһ��������ϸ�ڵ�D3D12_GRAPHICS_PIPELINE_STATE_DESC�ṹ��ʵ����
//    typedef struct D3D12_GRAPHICS_PIPELINE_STATE_DESC
//    {
//      ID3D12RootSignature *pRootSignature;
//      D3D12_SHADER_BYTECODE VS;
//      D3D12_SHADER_BYTECODE PS;
//      D3D12_SHADER_BYTECODE DS;
//      D3D12_SHADER_BYTECODE HS;
//      D3D12_SHADER_BYTECODE GS;
//      D3D12_STREAM_OUTPUT_DESC StreamOutput;
//      D3D12_BLEND_DESC BlendState;
//      UINT SampleMask;
//      D3D12_RASTERIZER_DESC RasterizerState;
//      D3D12_DEPTH_STENCIL_DESC DepthStencilState;
//      D3D12_INPUT_LAYOUT_DESC InputLayout;
//      D3D12_PRIMITIVE_TOPOLOGY_TYPE PrimitiveTopologyType;
//      UINT NumRenderTargets;
//      DXGI_FORMAT RTVFormats[8];
//      DXGI_FORMAT DSVFormat;
//      DXGI_SAMPLE_DESC SampleDesc;
//    } D3D12_GRAPHICS_PIPELINE_STATE_DESC;[20] 
//    1��pRootSignature��ָ��һ�����PSO��󶨵ĸ�ǩ����ָ�롣�ø�ǩ��һ��Ҫ���PSOָ������ɫ������ݡ�
//    2��VS�����󶨵Ķ�����ɫ�����˳�Ա�ɽṹ��D3D12_SHADER_BYTECODE��ʾ������ṹ�����ָ���ѱ���õ��ֽ������ݵ�ָ�룬�Լ����ֽ���������ռ���ֽڴ�С��
//          typedef struct D3D12_SHADER_BYTECODE {
//           const void *pShaderBytecode;
//           SIZE_T   BytecodeLength;
//          } D3D12_SHADER_BYTECODE;
//    3��PS�����󶨵�������ɫ����
//    4��DS�����󶨵�����ɫ�������ǽ��ں����½��н�������͵���ɫ������
//    5��HS�����󶨵������ɫ�������ǽ��ں����½��н�������͵���ɫ������
//    6��GS�����󶨵ļ�����ɫ�������ǽ��ں����½��н�������͵���ɫ������
//    7��StreamOutput������ʵ��һ�ֳ����������stream-out���ĸ߼�������Ŀǰ���ǽ������ֶ����㡣
//    8��BlendState��ָ����ϣ�blending���������õĻ��״̬�����ǽ��ں����½������۴�״̬�飬Ŀǰ�����˳�Աָ��ΪĬ�ϵ�CD3DX12_BLEND_DESC(D3D12_DEFAULT)��
//    9��SampleMask�����ز������ɲɼ�32����������˲�����32λ����ֵ����������ÿ��������Ĳɼ�������ɼ����ֹ�ɼ�����
//        ���磬�������˵�5λ������5λ����Ϊ0�����򽫲���Ե�5���������в�������Ȼ��Ҫ��ֹ�ɼ���5 ��������ǰ���ǣ����õĶ��ز�������Ҫ��5��������
//        ����һ��Ӧ�ó����ʹ���˵�������single sampling������ôֻ����Ըò����ĵ�1λ�������á�һ����˵��ʹ�õĶ���Ĭ��ֵ0xffffffff������ʾ�����еĲ����㶼���в�����
//    10��RasterizerState��ָ���������ù�դ���Ĺ�դ��״̬��
//    11��DepthStencilState��ָ�������������/ģ����Ե����/ģ��״̬�����ǽ��ں����½��жԴ�״̬�������ۣ�Ŀǰֻ������ΪĬ�ϵ�CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT)��
//    12��InputLayout�����벼���������˽ṹ������������Ա��һ����D3D12_INPUT_ELEMENT_DESCԪ�ع��ɵ����飬�Լ�һ����ʾ��������Ԫ���������޷���������
//          typedef struct D3D12_INPUT_LAYOUT_DESC
//          {
//            const D3D12_INPUT_ELEMENT_DESC *pInputElementDescs;
//            UINT NumElements;
//          } D3D12_INPUT_LAYOUT_DESC;
//    13��PrimitiveTopologyType��ָ��ͼԪ���������͡�
//          typedef enum D3D12_PRIMITIVE_TOPOLOGY_TYPE { 
//           D3D12_PRIMITIVE_TOPOLOGY_TYPE_UNDEFINED = 0,
//           D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT   = 1,
//           D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE    = 2,
//           D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE  = 3,
//           D3D12_PRIMITIVE_TOPOLOGY_TYPE_PATCH   = 4
//          } D3D12_PRIMITIVE_TOPOLOGY_TYPE;
//    14��NumRenderTargets��ͬʱ���õ���ȾĿ����������RTVFormats��������ȾĿ���ʽ����������
//    15��RTVFormats����ȾĿ��ĸ�ʽ�����ø�����ʵ�������ȾĿ��ͬʱ����д������ʹ�ô�PSO����ȾĿ��ĸ�ʽ�趨Ӧ����˲�����ƥ�䡣
//    16��DSVFormat�����/ģ�建�����ĸ�ʽ��ʹ�ô�PSO�����/ģ�建�����ĸ�ʽ�趨Ӧ����˲�����ƥ�䡣
//    17��SampleDesc���������ز�����ÿ�����ز��������������������𡣴˲���Ӧ����ȾĿ��Ķ�Ӧ������ƥ�䡣
//        ��D3D12_GRAPHICS_PIPELINE_STATE_DESCʵ����д��Ϻ����Ǽ�����ID3D12Device::CreateGraphicsPipelineState����������ID3D12PipelineState����
// */
//void BoxApp::BuildPSO()
//{
//    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc;
//    ZeroMemory(&psoDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
//    psoDesc.InputLayout = { mInputLayout.data(), (UINT)mInputLayout.size() };
//    psoDesc.pRootSignature = mRootSignature.Get();
//    psoDesc.VS =
//    {
//        reinterpret_cast<BYTE*>(mvsByteCode->GetBufferPointer()),
//        mvsByteCode->GetBufferSize()
//    };
//    psoDesc.PS =
//    {
//        reinterpret_cast<BYTE*>(mpsByteCode->GetBufferPointer()),
//        mpsByteCode->GetBufferSize()
//    };
//    // ��դ��״̬���á���ӦD3DX12_RASTERIZER_DESC����������d3d12.h�ļ���
//    psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
//    psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
//    psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
//    psoDesc.SampleMask = UINT_MAX;
//    psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
//    psoDesc.NumRenderTargets = 1;
//    psoDesc.RTVFormats[0] = mBackBufferFormat;
//    psoDesc.SampleDesc.Count = m4xMsaaState ? 4 : 1;
//    psoDesc.SampleDesc.Quality = m4xMsaaState ? (m4xMsaaQuality - 1) : 0;
//    psoDesc.DSVFormat = mDepthStencilFormat;
//    ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&mPSO)));
//}
//
///*
//    С��:
//    1�����˿ռ�λ����Ϣ��Direct3D�еĶ��㻹���Դ洢�������͵��������ݡ�Ϊ�˴����Զ���Ķ����ʽ����������Ҫ��ѡ���Ķ������ݶ���Ϊһ���ṹ�塣
//        ������ṹ�嶨��ú󣬱�������벼��������D3D12_INPUT_LAYOUT_DESC����Direct3D�ṩ��ϸ�ڡ����벼����������������ɣ�һ��D3D12_INPUT_ELEMENT_DESCԪ�ع��ɵ����飬
//        һ����¼��������Ԫ�ظ������޷���������D3D12_INPUT_ELEMENT_DESC�����е�Ԫ��Ҫ�붥��ṹ���еĳ�Աһһ��Ӧ����ʵ�ϣ����벼������Ϊ�ṹ��D3D12_GRAPHICS_PIPELINE_STATE_DESC�е�һ���ֶΣ�
//        �����˵����ʵΪPSO��һ����ɲ��֣������붥����ɫ������ǩ�����и�ʽ�ıȶ���֤����ˣ���PSO����Ⱦ��ˮ�߰�ʱ�����벼��Ҳ����PSO���Ԫ�ص������PSO����Ⱦ��ˮ�ߵ�IA�׶���󶨡�
//    2��Ϊ��ʹGPU���Է��ʶ���/�������飬����Ҫ��������һ����Ϊ��������buffer������Դ֮�ڡ������������������ݵĻ������ֱ��Ϊ���㻺������vertex buffer����������������index buffer����
//        �������ýӿ�ID3D12Resource��ʾ��Ҫ������������Դ��Ҫ��дD3D12_RESOURCE_DESC�ṹ�壬�ٵ���ID3D12Device::CreateCommittedResource������
//        ���㻺��������ͼ����������������ͼ�ֱ���D3D12_VERTEX_BUFFER_VIEW��D3D12_INDEX_BUFFER_VIEW�ṹ�����������
//        ��󣬼���ͨ��ID3D12GraphicsCommandList::IASetVertexBuffers������ID3D12GraphicsCommandList::IASetIndexBuffer�����ֱ𽫶��㻺�����������������󶨵���Ⱦ��ˮ�ߵ�IA�׶Ρ�
//        ���Ҫ���Ʒ�������non-indexed�������ļ����壨���Զ������������Ƶļ����壩�ɽ���ID3D12GraphicsCommandList::DrawInstanced�������������������ļ��������ID3D12GraphicsCommandList::DrawIndexedInstanced�������л��ơ�
//    3��������ɫ����һ����HLSL��д����GPU�����еĳ������Ե���������Ϊ�����������ÿ�������ƵĶ��㶼Ҫ����������ɫ���׶Ρ���ʹ�ó���Ա�ܹ��ڴ��Զ���Ϊ������λ���д����̶���ȡ���ֶ�������ȾЧ�����Ӷ�����ɫ����������ݽ�������Ⱦ��ˮ�ߵ���һ���׶Ρ�
//    4��������������һ��GPU��Դ��ID3D12Resource�������������ݿɹ���ɫ���������á����Ǳ��������ϴ��ѣ�upload heap������Ĭ�϶ѣ�default heap���С���ˣ�Ӧ�ó����ͨ�������ݴ�ϵͳ�ڴ渴�Ƶ��Դ��������³�����������
//        ���һ����C++Ӧ�ó���Ϳ�����ɫ��ͨ�ţ������³�������������ɫ����������ݡ����磬C++������Խ������ַ�ʽ����ɫ�����õġ����硪�۲졪ͶӰ��������и��ġ��ڴˣ����ǽ�����߿������ݸ��µ�Ƶ���̶ȣ��Դ�Ϊ������������ͬ�ĳ�����������
//        Ч�����ǻ��ֳ����������Ķ������ڶ�һ���������������и��µ�ʱ�����е����б���������֮���£�����νǣһ������ȫ����ˣ�Ӧ���ݸ���Ƶ�ʽ�������Ч����֯Ϊ��ͬ�ĳ������������Դ���������ν������ĸ��£��Ӷ����Ч�ʡ�
//    5��������ɫ����һ����HLSL��д��������GPU�ϵĳ������Ծ�����ֵ�������õ��Ķ���������Ϊ���룬��������������֮��Ӧ��һ����ɫֵ������Ӳ���Ż���ԭ��ĳЩ����Ƭ�ο��ܻ�δ��������ɫ�����ѱ���Ⱦ��ˮ���޳��ˣ������������ǰ����޳�������early-z rejection����
//        ������ɫ����ʹ����Ա������Ϊ������λ���д����Ӷ���ñ仯��ǧ����ȾЧ������������ɫ����������ݽ����ƽ�����Ⱦ��ˮ�ߵ���һ���׶Ρ�
//    6�����������ͼ����ˮ��״̬��Direct3D���󶼱�ָ������һ�ֳ�����ˮ��״̬����pipeline state object��PSO���ļ���֮�У�����ID3D12PipelineState�ӿ�����ʾ��
//        ���ǽ���Щ������������ͳһ����Ⱦ��ˮ�߽������ã��ǳ��ڶ��������صĿ��ǡ�����һ����Direct3D������֤���е�״̬�Ƿ�˴˼��ݣ�����������Ҳ��������ǰ����Ӳ������ָ���״̬��
//*/
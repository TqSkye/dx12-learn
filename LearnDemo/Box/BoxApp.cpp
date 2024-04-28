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
  组件对象模型（Component Object Model，COM）是一种令DirectX不受编程语言束缚，并且使之向后兼容的技术。
  我们通常将COM对象视为一种接口，但考虑当前编程的目的，遂将它当作一个C++类来使用。
  用C++语言编写DirectX程序时，COM帮我们隐藏了大量底层细节。
  我们只需知道：要获取指向某COM接口的指针，需借助特定函数或另一COM接口的方法——而不是用C++语言中的关键字new去创建一个COM接口。
  另外，COM对象会统计其引用次数；因此，在使用完某接口时，我们便应调用它的Release方法（COM接口的所有功能都是从IUnknown这个COM接口继承而来的，包括Release方法在内），
  而不是用delete来删除——当COM对象的引用计数为0时，它将自行释放自己所占用的内存。
  为了辅助用户管理COM对象的生命周期，Windows运行时库（Windows Runtime Library，WRL）
  专门为此提供了Microsoft::WRL::ComPtr类（#include <wrl.h>），我们可以把它当作是COM对象的智能指针。
  当一个ComPtr实例超出作用域范围时，它便会自动调用相应COM对象的Release方法，继而省掉了我们手动调用的麻烦。
  本书中常用的3个ComPtr方法如下。
      1．Get：返回一个指向此底层COM接口的指针。此方法常用于把原始的COM接口指针作为参数传递给函数。例如:
          ComPtr<ID3D12RootSignature> mRootSignature;
          ...
          // SetGraphicsRootSignature需要获取ID3D12RootSignature*类型的参数
          mCommandList->SetGraphicsRootSignature(mRootSignature.Get());
      2．GetAddressOf：返回指向此底层COM接口指针的地址。凭此方法即可利用函数参数返回COM接口的指针。例如：
          ComPtr<ID3D12CommandAllocator> mDirectCmdListAlloc;
          ...
          ThrowIfFailed(md3dDevice->CreateCommandAllocator(
          D3D12_COMMAND_LIST_TYPE_DIRECT,
          mDirectCmdListAlloc.GetAddressOf()));
      3．Reset：将此ComPtr实例设置为nullptr释放与之相关的所有引用（同时减少其底层COM接口的引用计数）。
      此方法的功能与将ComPtr目标实例赋值为nullptr的效果相同。
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
    一、顶点和顶点缓冲区 创建 VertexBufferView() : 顶点缓存区 d3dUtile.h=>VertexBufferView():

        为了将顶点缓冲区绑定到渲染流水线上，我们需要给这种资源创建一个顶点缓冲区视图（vertex buffer view）。
        与RTV（render target view，渲染目标视图）不同的是，我们无须为顶点缓冲区视图创建描述符堆。
        而且，顶点缓冲区视图是由D3D12_VERTEX_BUFFER_VIEW[5]结构体来表示。
            typedef struct D3D12_VERTEX_BUFFER_VIEW
            {
              D3D12_GPU_VIRTUAL_ADDRESS BufferLocation;
              UINT SizeInBytes;
              UINT StrideInBytes;
            }   D3D12_VERTEX_BUFFER_VIEW;
            1．BufferLocation：待创建视图的顶点缓冲区资源虚拟地址。我们可以通过ID3D12Resource::GetGPUVirtualAddress方法来获得此地址。
            2．SizeInBytes：待创建视图的顶点缓冲区大小（用字节表示）。
            3．StrideInBytes：每个顶点元素所占用的字节数。

     二、 在顶点缓冲区及其对应视图创建完成后，便可以将它与渲染流水线上的一个输入槽（input slot）相绑定。
        这样一来，我们就能向流水线中的输入装配器阶段传递顶点数据了。此操作可以通过下列方法来实现。
            void ID3D12GraphicsCommandList::IASetVertexBuffers(
              UINT StartSlot,
              UINT NumView,
              const D3D12_VERTEX_BUFFER_VIEW *pViews);
            1．StartSlot：在绑定多个顶点缓冲区时，所用的起始输入槽（若仅有一个顶点缓冲区，则将其绑定至此槽）。输入槽共有16个，索引为0～15。
            2．NumViews：将要与输入槽绑定的顶点缓冲区数量（即视图数组pViews中视图的数量）。如果起始输入槽StartSlot的索引值为[插图]，且我们要绑定[插图]个顶点缓冲区，那么这些缓冲区将依次与输入槽[插图]相绑定。
            3．pViews：指向顶点缓冲区视图数组中第一个元素的指针。
        下面是该函数的一个调用示例:
            D3D12_VERTEX_BUFFER_VIEW vbv;
            vbv.BufferLocation = VertexBufferGPU->GetGPUVirtualAddress();
            vbv.StrideInBytes = sizeof(Vertex);
            vbv.SizeInBytes = 8 * sizeof(Vertex);

            D3D12_VERTEX_BUFFER_VIEW vertexBuffers[1] = { vbv };
            mCommandList->IASetVertexBuffers(0, 1, vertexBuffers);

    三、将顶点缓冲区设置到输入槽上并不会对其执行实际的绘制操作，而是仅为顶点数据送至渲染流水线做好准备而已。
        这最后一步才是通过ID3D12GraphicsCommandList::DrawInstanced方法[6]真正地绘制顶点。
        *** 注意：这里绘制box用的是ID3D12GraphicsCommandList::DrawIndexedInstanced，因为使用的是索引绘制！！！
            void ID3D12GraphicsCommandList::DrawInstanced (
              UINT VertexCountPerInstance,
              UINT InstanceCount,
              UINT StartVertexLocation,
              UINT StartInstanceLocation);
            1．VertexCountPerInstance：每个实例要绘制的顶点数量。
            2．InstanceCount：用于实现一种被称作实例化（instancing）的高级技术。就目前来说，我们只绘制一个实例，因而将此参数设置为1。
            3．StartVertexLocation：指定顶点缓冲区内第一个被绘制顶点的索引（该索引值以0为基准）。
            4．StartInstanceLocation：用于实现一种被称作实例化的高级技术，暂时只需将其设置为0。
       VertexCountPerInstance和StartVertexLocation两个参数定义了顶点缓冲区中将要被绘制的一组连续顶点，如图6.2所示。
    */
    mCommandList->IASetVertexBuffers(0, 1, &mBoxGeo->VertexBufferView());

    /*
     一、索引和索引缓冲区: 创建 IndexBufferView() : 索引缓存区 d3dUtile.h=>IndexBufferView():
        与顶点相似，为了使GPU可以访问索引数组，就需要将它们放置于GPU的缓冲区资源（ID3D12Resource）内。
        我们称存储索引的缓冲区为索引缓冲区（index buffer）。由于本书所采用的d3dUtil::CreateDefaultBuffer函数是通过void*类型作为参数引入泛型数据，
        这就意味着我们也可以用此函数来创建索引缓冲区（或任意类型的默认缓冲区）。
        为了使索引缓冲区与渲染流水线绑定，我们需要给索引缓冲区资源创建一个索引缓冲区视图（index buffer view）。
        如同顶点缓冲区视图一样，我们也无须为索引缓冲区视图创建描述符堆。但索引缓冲区视图要由结构体D3D12_INDEX_BUFFER_VIEW来表示。
        typedef struct D3D12_INDEX_BUFFER_VIEW
        {
          D3D12_GPU_VIRTUAL_ADDRESS BufferLocation;
          UINT SizeInBytes;
          DXGI_FORMAT Format;
        } D3D12_INDEX_BUFFER_VIEW;
        1．BufferLocation：待创建视图的索引缓冲区资源虚拟地址。我们可以通过调用ID3D12Resource::GetGPUVirtualAddress方法来获取此地址。
        2．SizeInBytes：待创建视图的索引缓冲区大小（以字节表示）。
        3．Format：索引的格式必须为表示16位索引的DXGI_FORMAT_R16_UINT类型，或表示32位索引的DXGI_FORMAT_R32_UINT类型。
            16位的索引可以减少内存和带宽的占用，但如果索引值范围超过了16位数据的表达范围，则也只能采用32位索引了。

    二、与顶点缓冲区相似（也包括其他的Direct3D资源在内），在使用之前，我们需要先将它们绑定到渲染流水线上。
        通过ID3D12GraphicsCommandList::IASetIndexBuffer方法即可将索引缓冲区绑定到输入装配器阶段。
        下面的代码演示了怎样创建一个索引缓冲区来定义构成立方体的三角形，以及为该索引缓冲区创建视图并将它绑定到渲染流水线：
            std::uint16_t indices[] = {
              // 立方体前表面
              0, 1, 2,
              0, 2, 3,

              // 立方体后表面
              4, 6, 5,
              4, 7, 6,

              // 立方体左表面
              4, 5, 1,
              4, 1, 0,

              // 立方体右表面
              3, 2, 6,
              3, 6, 7,

              // 立方体上表面
              1, 5, 6,
              1, 6, 2,

              // 立方体下表面
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
        既然DrawInstanced方法没有指定顶点被定义为何种图元，那么，它们应该被绘制为点、线列表还是三角形列表呢？
        回顾5.5.2节可知，图元拓扑状态实由ID3D12GraphicsCommandList::IASetPrimitiveTopology方法来设置。
        下面给出一个相关的调用示例：
        cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    */
    mCommandList->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    mCommandList->SetGraphicsRootDescriptorTable(0, mCbvHeap->GetGPUDescriptorHandleForHeapStart());

    /*
        最后需要注意的是，在使用索引的时候，我们一定要用ID3D12GraphicsCommandList::DrawIndexedInstanced方法代替DrawInstanced方法进行绘制。
        void ID3D12GraphicsCommandList::DrawIndexedInstanced(
          UINT IndexCountPerInstance,
          UINT InstanceCount,
          UINT StartIndexLocation,
          INT BaseVertexLocation,
          UINT StartInstanceLocation);
        1．IndexCountPerInstance：每个实例将要绘制的索引数量。
        2．InstanceCount：用于实现一种被称作实例化的高级技术。就目前而言，我们只绘制一个实例，因而将此值设置为1。
        3．StartIndexLocation：指向索引缓冲区中的某个元素，将其标记为欲读取的起始索引。
        4．BaseVertexLocation：在本次绘制调用读取顶点之前，要为每个索引都加上此整数值。
        5．StartInstanceLocation：用于实现一种被称为实例化的高级技术，暂将其设置为0。
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
    除了空间位置，Direct3D中的顶点还可以存储其他属性数据。为了构建自定义的顶点格式，我们首先要创建一个结构体来容纳选定的顶点数据。
    例如，下面列出了两种不同类型的顶点格式：一种由位置和颜色信息组成，另一种则由位置、法向量以及两组2D纹理坐标构成.
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

    定义了顶点结构体之后，我们还需要向Direct3D提供该顶点结构体的描述，使它了解应怎样来处理结构体中的每个成员。
    用户提供给Direct3D的这种描述被称为输入布局描述（input layout description），
    用结构体D3D12_INPUT_LAYOUT_DESC来表示：(下面的'mInputLayout'变量)

    输入布局描述实由两部分组成：即一个以D3D12_INPUT_ELEMENT_DESC元素构成的数组，以及一个表示该数组中的元素数量的整数。
    D3D12_INPUT_ELEMENT_DESC数组中的元素依次描述了顶点结构体中所对应的成员。
    这就是说，如果某顶点结构体中有两个成员，那么与之对应的D3D12_INPUT_ELEMENT_DESC数组也将存有两个元素。

    /// --------------------------------------------------
    图6.1解释:
        // D3D12_INPUT_ELEMENT_DESC数组中的元素与顶点结构体中的成员一一对应，
        // 而语义名与索引则为顶点元素映射到顶点着色器中对应的参数提供了途径
        struct Vertex
        {
            XMFLOAT3 Pos;       ===> VertexOut VS中的iPos参数
            XMFLOAT3 Normal;    ===> VertexOut VS中的iNormal参数
            XMFLOAT2 Tex0;      ===> VertexOut VS中的iTex0参数
            XMFLOAT2 Tex1;      ===> VertexOut VS中的iTex1参数
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

    D3D12_INPUT_ELEMENT_DESC结构体的定义如下：
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

    1．SemanticName：一个与元素相关联的特定字符串，我们称之为语义（semantic），它传达了元素的预期用途。
        该参数可以是任意合法的语义名。通过语义即可将顶点结构体（图6.1中的struct Vertex）中的元素与顶点着色器输入签名[1]
        （vertex shader input signature，即图6.1 中的VertexOut VS的众参数）中的元素一一映射起来，如图6.1所示。
    2．SemanticIndex：附加到语义上的索引。此成员的设计动机可从图6.1中看出。例如，顶点结构体中的纹理坐标可能不止一组，
        而仅在语义名尾部添加一个索引，即可在不引入新语义名的情况下区分出这两组不同的纹理坐标。
        在着色器代码中，未标明索引的语义将默认其索引值为0，也就是说，图6.1中的POSITION与POSITION0等价。
    3．Format：在Direct3D中，要通过枚举类型DXGI_FORMAT中的成员来指定顶点元素的格式（即数据类型）。下面是一些常用的格式。
        DXGI_FORMAT_R32_FLOAT     // 1D 32位浮点标量
        DXGI_FORMAT_R32G32_FLOAT    // 2D 32位浮点向量
        DXGI_FORMAT_R32G32B32_FLOAT  // 3D 32位浮点向量
        DXGI_FORMAT_R32G32B32A32_FLOAT // 4D 32位浮点向量

        DXGI_FORMAT_R8_UINT     // 1D 8位无符号整型标量
        DXGI_FORMAT_R16G16_SINT   // 2D 16位有符号整型向量
        DXGI_FORMAT_R32G32B32_UINT // 3D 32位无符号整型向量
        DXGI_FORMAT_R8G8B8A8_SINT  // 4D 8位有符号整型向量
        DXGI_FORMAT_R8G8B8A8_UINT  // 4D 8位无符号整型向量
    4．InputSlot：指定传递元素所用的输入槽（input slot index）索引。Direct3D共支持16个输入槽（索引值为0～15），
        可以通过它们来向输入装配阶段传递顶点数据。目前我们只会用到输入槽0（即所有的顶点元素都来自同一个输入槽），
        但在本章的习题2中将会涉及多输入槽的编程实践。
    5．AlignedByteOffset：在特定输入槽中，从C++顶点结构体的首地址到其中某点元素起始地址的偏移量（用字节表示）。
        例如，在下列顶点结构体中，元素Pos的偏移量为0字节，因为它的起始地址与顶点结构体的首地址一致；
        元素Normal的偏移量为12字节，因为跳过Pos所占用的字节数才能找到Normal的起始地址；
        元素Tex0的偏移量为24字节，因为跨过Pos和Normal的总字节数才可获取Tex0的起始地址。
        同理，元素Tex1的偏移量为32字节，只有跳过Pos、Normal和Tex0这3个元素的总字节数方可寻得Tex1的起始地址。
        struct Vertex2
        {
          XMFLOAT3 Pos;  // 偏移量为0字节
          XMFLOAT3 Normal; // 偏移量为12字节
          XMFLOAT2 Tex0;  // 偏移量为24字节
          XMFLOAT2 Tex1;  // 偏移量为32字节
        };
    6．InputSlotClass：我们暂且把此参数指定为D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA[2]。
        而另一选项（D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA[3]）则用于实现实例化（instancing）这种高级技术。
    7．InstanceDataStepRate：目前仅将此值指定为0。若要采用实例化这种高级技术，则将此参数设为1。
        就前面Vertex1和Vertex2这两个顶点结构体的例子来说，其相应的输入布局描述为：
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
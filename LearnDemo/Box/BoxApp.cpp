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
//  组件对象模型（Component Object Model，COM）是一种令DirectX不受编程语言束缚，并且使之向后兼容的技术。
//  我们通常将COM对象视为一种接口，但考虑当前编程的目的，遂将它当作一个C++类来使用。
//  用C++语言编写DirectX程序时，COM帮我们隐藏了大量底层细节。
//  我们只需知道：要获取指向某COM接口的指针，需借助特定函数或另一COM接口的方法——而不是用C++语言中的关键字new去创建一个COM接口。
//  另外，COM对象会统计其引用次数；因此，在使用完某接口时，我们便应调用它的Release方法（COM接口的所有功能都是从IUnknown这个COM接口继承而来的，包括Release方法在内），
//  而不是用delete来删除——当COM对象的引用计数为0时，它将自行释放自己所占用的内存。
//  为了辅助用户管理COM对象的生命周期，Windows运行时库（Windows Runtime Library，WRL）
//  专门为此提供了Microsoft::WRL::ComPtr类（#include <wrl.h>），我们可以把它当作是COM对象的智能指针。
//  当一个ComPtr实例超出作用域范围时，它便会自动调用相应COM对象的Release方法，继而省掉了我们手动调用的麻烦。
//  本书中常用的3个ComPtr方法如下。
//      1．Get：返回一个指向此底层COM接口的指针。此方法常用于把原始的COM接口指针作为参数传递给函数。例如:
//          ComPtr<ID3D12RootSignature> mRootSignature;
//          ...
//          // SetGraphicsRootSignature需要获取ID3D12RootSignature*类型的参数
//          mCommandList->SetGraphicsRootSignature(mRootSignature.Get());
//      2．GetAddressOf：返回指向此底层COM接口指针的地址。凭此方法即可利用函数参数返回COM接口的指针。例如：
//          ComPtr<ID3D12CommandAllocator> mDirectCmdListAlloc;
//          ...
//          ThrowIfFailed(md3dDevice->CreateCommandAllocator(
//          D3D12_COMMAND_LIST_TYPE_DIRECT,
//          mDirectCmdListAlloc.GetAddressOf()));
//      3．Reset：将此ComPtr实例设置为nullptr释放与之相关的所有引用（同时减少其底层COM接口的引用计数）。
//      此方法的功能与将ComPtr目标实例赋值为nullptr的效果相同。
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
///// 绘制物体所用的常量数据
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
//    // 重置命令列表为执行初始化命令做好准备工作
//    ThrowIfFailed(mCommandList->Reset(mDirectCmdListAlloc.Get(), nullptr));
//
//    // 常量缓冲区描述符
//    BuildDescriptorHeaps();
//    // 创建常量缓冲区
//    BuildConstantBuffers();
//    // 根签名和描述符表
//    BuildRootSignature();
//    // 编译着色器
//    BuildShadersAndInputLayout();
//    BuildBoxGeometry();
//    BuildPSO();
//
//    // Execute the initialization commands.
//    // 执行初始化命令
//    ThrowIfFailed(mCommandList->Close());
//    ID3D12CommandList* cmdsLists[] = { mCommandList.Get() };
//    mCommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);
//
//    // Wait until initialization is complete.
//    // 等待初始化完成
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
//    // 若用户调整了窗口尺寸，则更新纵横比并重新计算投影矩阵
//    XMMATRIX P = XMMatrixPerspectiveFovLH(0.25f * MathHelper::Pi, AspectRatio(), 1.0f, 1000.0f);
//    XMStoreFloat4x4(&mProj, P);
//}
//
//void BoxApp::Update(const GameTimer& gt)
//{
//    // Convert Spherical to Cartesian coordinates.
//    // 由球坐标（也有译作球面坐标）转换为笛卡儿坐标
//    float x = mRadius * sinf(mPhi) * cosf(mTheta);
//    float z = mRadius * sinf(mPhi) * sinf(mTheta);
//    float y = mRadius * cosf(mPhi);
//
//    // Build the view matrix.
//    // 构建观察矩阵
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
//    // 用最新的worldViewProj 矩阵来更新常量缓冲区
//    ObjectConstants objConstants;
//    XMStoreFloat4x4(&objConstants.WorldViewProj, XMMatrixTranspose(worldViewProj));
//    mObjectCB->CopyData(0, objConstants);
//}
//
//void BoxApp::Draw(const GameTimer& gt)
//{
//    // Reuse the memory associated with command recording.
//    // We can only reset when the associated command lists have finished execution on the GPU.
//    // 复用记录命令所用的内存
//    // 只有当GPU中的命令列表执行完毕后，我们才可对其进行重置
//    ThrowIfFailed(mDirectCmdListAlloc->Reset());
//
//    // A command list can be reset after it has been added to the command queue via ExecuteCommandList.
//    // Reusing the command list reuses memory.
//    // 通过函数ExecuteCommandList将命令列表加入命令队列后，便可对它进行重置
//    // 复用命令列表即复用其相应的内存
//    ThrowIfFailed(mCommandList->Reset(mDirectCmdListAlloc.Get(), mPSO.Get()));
//
//    mCommandList->RSSetViewports(1, &mScreenViewport);
//    mCommandList->RSSetScissorRects(1, &mScissorRect);
//
//    // Indicate a state transition on the resource usage.
//    // 按照资源的用途指示其状态的转变，此处将资源从呈现状态转换为渲染目标状态
//    mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(),
//        D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));
//
//    // Clear the back buffer and depth buffer.
//    // 清除后台缓冲区和深度缓冲区
//    mCommandList->ClearRenderTargetView(CurrentBackBufferView(), Colors::LightSteelBlue, 0, nullptr);
//    mCommandList->ClearDepthStencilView(DepthStencilView(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);
//
//    // Specify the buffers we are going to render to.
//    // 指定将要渲染的目标缓冲区
//    mCommandList->OMSetRenderTargets(1, &CurrentBackBufferView(), true, &DepthStencilView());
//
//    ID3D12DescriptorHeap* descriptorHeaps[] = { mCbvHeap.Get() };
//    mCommandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);
//
//    mCommandList->SetGraphicsRootSignature(mRootSignature.Get());
//
//    /*
//    一、顶点和顶点缓冲区 创建 VertexBufferView() : 顶点缓存区 d3dUtile.h=>VertexBufferView():
//
//        为了将顶点缓冲区绑定到渲染流水线上，我们需要给这种资源创建一个顶点缓冲区视图（vertex buffer view）。
//        与RTV（render target view，渲染目标视图）不同的是，我们无须为顶点缓冲区视图创建描述符堆。
//        而且，顶点缓冲区视图是由D3D12_VERTEX_BUFFER_VIEW[5]结构体来表示。
//            typedef struct D3D12_VERTEX_BUFFER_VIEW
//            {
//              D3D12_GPU_VIRTUAL_ADDRESS BufferLocation;
//              UINT SizeInBytes;
//              UINT StrideInBytes;
//            }   D3D12_VERTEX_BUFFER_VIEW;
//            1．BufferLocation：待创建视图的顶点缓冲区资源虚拟地址。我们可以通过ID3D12Resource::GetGPUVirtualAddress方法来获得此地址。
//            2．SizeInBytes：待创建视图的顶点缓冲区大小（用字节表示）。
//            3．StrideInBytes：每个顶点元素所占用的字节数。
//
//     二、 在顶点缓冲区及其对应视图创建完成后，便可以将它与渲染流水线上的一个输入槽（input slot）相绑定。
//        这样一来，我们就能向流水线中的输入装配器阶段传递顶点数据了。此操作可以通过下列方法来实现。
//            void ID3D12GraphicsCommandList::IASetVertexBuffers(
//              UINT StartSlot,
//              UINT NumView,
//              const D3D12_VERTEX_BUFFER_VIEW *pViews);
//            1．StartSlot：在绑定多个顶点缓冲区时，所用的起始输入槽（若仅有一个顶点缓冲区，则将其绑定至此槽）。输入槽共有16个，索引为0～15。
//            2．NumViews：将要与输入槽绑定的顶点缓冲区数量（即视图数组pViews中视图的数量）。如果起始输入槽StartSlot的索引值为[插图]，且我们要绑定[插图]个顶点缓冲区，那么这些缓冲区将依次与输入槽[插图]相绑定。
//            3．pViews：指向顶点缓冲区视图数组中第一个元素的指针。
//        下面是该函数的一个调用示例:
//            D3D12_VERTEX_BUFFER_VIEW vbv;
//            vbv.BufferLocation = VertexBufferGPU->GetGPUVirtualAddress();
//            vbv.StrideInBytes = sizeof(Vertex);
//            vbv.SizeInBytes = 8 * sizeof(Vertex);
//
//            D3D12_VERTEX_BUFFER_VIEW vertexBuffers[1] = { vbv };
//            mCommandList->IASetVertexBuffers(0, 1, vertexBuffers);
//
//    三、将顶点缓冲区设置到输入槽上并不会对其执行实际的绘制操作，而是仅为顶点数据送至渲染流水线做好准备而已。
//        这最后一步才是通过ID3D12GraphicsCommandList::DrawInstanced方法[6]真正地绘制顶点。
//        *** 注意：这里绘制box用的是ID3D12GraphicsCommandList::DrawIndexedInstanced，因为使用的是索引绘制！！！
//            void ID3D12GraphicsCommandList::DrawInstanced (
//              UINT VertexCountPerInstance,
//              UINT InstanceCount,
//              UINT StartVertexLocation,
//              UINT StartInstanceLocation);
//            1．VertexCountPerInstance：每个实例要绘制的顶点数量。
//            2．InstanceCount：用于实现一种被称作实例化（instancing）的高级技术。就目前来说，我们只绘制一个实例，因而将此参数设置为1。
//            3．StartVertexLocation：指定顶点缓冲区内第一个被绘制顶点的索引（该索引值以0为基准）。
//            4．StartInstanceLocation：用于实现一种被称作实例化的高级技术，暂时只需将其设置为0。
//       VertexCountPerInstance和StartVertexLocation两个参数定义了顶点缓冲区中将要被绘制的一组连续顶点，如图6.2所示。
//    */
//    mCommandList->IASetVertexBuffers(0, 1, &mBoxGeo->VertexBufferView());
//
//    /*
//     一、索引和索引缓冲区: 创建 IndexBufferView() : 索引缓存区 d3dUtile.h=>IndexBufferView():
//        与顶点相似，为了使GPU可以访问索引数组，就需要将它们放置于GPU的缓冲区资源（ID3D12Resource）内。
//        我们称存储索引的缓冲区为索引缓冲区（index buffer）。由于本书所采用的d3dUtil::CreateDefaultBuffer函数是通过void*类型作为参数引入泛型数据，
//        这就意味着我们也可以用此函数来创建索引缓冲区（或任意类型的默认缓冲区）。
//        为了使索引缓冲区与渲染流水线绑定，我们需要给索引缓冲区资源创建一个索引缓冲区视图（index buffer view）。
//        如同顶点缓冲区视图一样，我们也无须为索引缓冲区视图创建描述符堆。但索引缓冲区视图要由结构体D3D12_INDEX_BUFFER_VIEW来表示。
//        typedef struct D3D12_INDEX_BUFFER_VIEW
//        {
//          D3D12_GPU_VIRTUAL_ADDRESS BufferLocation;
//          UINT SizeInBytes;
//          DXGI_FORMAT Format;
//        } D3D12_INDEX_BUFFER_VIEW;
//        1．BufferLocation：待创建视图的索引缓冲区资源虚拟地址。我们可以通过调用ID3D12Resource::GetGPUVirtualAddress方法来获取此地址。
//        2．SizeInBytes：待创建视图的索引缓冲区大小（以字节表示）。
//        3．Format：索引的格式必须为表示16位索引的DXGI_FORMAT_R16_UINT类型，或表示32位索引的DXGI_FORMAT_R32_UINT类型。
//            16位的索引可以减少内存和带宽的占用，但如果索引值范围超过了16位数据的表达范围，则也只能采用32位索引了。
//
//    二、与顶点缓冲区相似（也包括其他的Direct3D资源在内），在使用之前，我们需要先将它们绑定到渲染流水线上。
//        通过ID3D12GraphicsCommandList::IASetIndexBuffer方法即可将索引缓冲区绑定到输入装配器阶段。
//        下面的代码演示了怎样创建一个索引缓冲区来定义构成立方体的三角形，以及为该索引缓冲区创建视图并将它绑定到渲染流水线：
//            std::uint16_t indices[] = {
//              // 立方体前表面
//              0, 1, 2,
//              0, 2, 3,
//
//              // 立方体后表面
//              4, 6, 5,
//              4, 7, 6,
//
//              // 立方体左表面
//              4, 5, 1,
//              4, 1, 0,
//
//              // 立方体右表面
//              3, 2, 6,
//              3, 6, 7,
//
//              // 立方体上表面
//              1, 5, 6,
//              1, 6, 2,
//
//              // 立方体下表面
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
//        既然DrawInstanced方法没有指定顶点被定义为何种图元，那么，它们应该被绘制为点、线列表还是三角形列表呢？
//        回顾5.5.2节可知，图元拓扑状态实由ID3D12GraphicsCommandList::IASetPrimitiveTopology方法来设置。
//        下面给出一个相关的调用示例：
//        cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
//    */
//    mCommandList->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
//
//    /*
//        通过命令列表（command list）设置好根签名，我们就能用ID3D12GraphicsCommandList::SetGraphicsRootDescriptorTable方法令描述符表与渲染流水线相绑定。
//        void ID3D12GraphicsCommandList::SetGraphicsRootDescriptorTable( 
//          UINT RootParameterIndex,
//          D3D12_GPU_DESCRIPTOR_HANDLE BaseDescriptor);
//        1．RootParameterIndex：将根参数按此索引（即欲绑定到的寄存器槽号）进行设置。
//        2．BaseDescriptor：此参数指定的是将要向着色器绑定的描述符表中第一个描述符位于描述符堆中的句柄。
//            比如说，如果根签名指明当前描述符表中共有 5 个描述符，则堆中的BaseDescriptor及其后面的4个描述符将被设置到此描述符表中。
//    */
//    mCommandList->SetGraphicsRootDescriptorTable(0, mCbvHeap->GetGPUDescriptorHandleForHeapStart());
//
//    /*
//        最后需要注意的是，在使用索引的时候，我们一定要用ID3D12GraphicsCommandList::DrawIndexedInstanced方法代替DrawInstanced方法进行绘制。
//        void ID3D12GraphicsCommandList::DrawIndexedInstanced(
//          UINT IndexCountPerInstance,
//          UINT InstanceCount,
//          UINT StartIndexLocation,
//          INT BaseVertexLocation,
//          UINT StartInstanceLocation);
//        1．IndexCountPerInstance：每个实例将要绘制的索引数量。
//        2．InstanceCount：用于实现一种被称作实例化的高级技术。就目前而言，我们只绘制一个实例，因而将此值设置为1。
//        3．StartIndexLocation：指向索引缓冲区中的某个元素，将其标记为欲读取的起始索引。
//        4．BaseVertexLocation：在本次绘制调用读取顶点之前，要为每个索引都加上此整数值。
//        5．StartInstanceLocation：用于实现一种被称为实例化的高级技术，暂将其设置为0。
//    */
//    mCommandList->DrawIndexedInstanced(
//        mBoxGeo->DrawArgs["box"].IndexCount,
//        1, 0, 0, 0);
//
//    // Indicate a state transition on the resource usage.
//    // 按照资源的用途指示其状态的转变，此处将资源从渲染目标状态转换为呈现状态
//    mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(),
//        D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));
//
//    // Done recording commands.
//    // 完成命令的记录
//    ThrowIfFailed(mCommandList->Close());
//
//    // Add the command list to the queue for execution.
//    // 向命令队列添加欲执行的命令列表
//    ID3D12CommandList* cmdsLists[] = { mCommandList.Get() };
//    mCommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);
//
//    // swap the back and front buffers
//    // 交换后台缓冲区与前台缓冲区
//    ThrowIfFailed(mSwapChain->Present(0, 0));
//    mCurrBackBuffer = (mCurrBackBuffer + 1) % SwapChainBufferCount;
//
//    // Wait until frame commands are complete.  This waiting is inefficient and is
//    // done for simplicity.  Later we will show how to organize our rendering code
//    // so we do not have to wait per frame.
//    // 等待绘制此帧的一系列命令执行完毕。这种等待的方法虽然简单却也低效
//    // 在后面将展示如何重新组织渲染代码，使我们不必在绘制每一帧时都等待
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
//    鼠标操作:
//
//    一般来讲，物体的世界矩阵将随其移动/旋转/缩放而改变，观察矩阵随虚拟摄像机的移动/旋转而改变，投影矩阵随窗口大小的调整而改变。
//    在本章的演示程序中，用户可以通过鼠标来旋转和移动摄像机，变换观察角度。因此，我们在每一帧都要用Update函数，
//    以新的观察矩阵来更新“世界—观察—投影”3种矩阵组合而成的复合矩阵：
//*/
//void BoxApp::OnMouseMove(WPARAM btnState, int x, int y)
//{
//    if ((btnState & MK_LBUTTON) != 0)
//    {
//        // Make each pixel correspond to a quarter of a degree.
//        // 根据鼠标的移动距离计算旋转角度，令每个像素按此角度的1/4进行旋转
//        float dx = XMConvertToRadians(0.25f * static_cast<float>(x - mLastMousePos.x));
//        float dy = XMConvertToRadians(0.25f * static_cast<float>(y - mLastMousePos.y));
//
//        // Update angles based on input to orbit camera around box.
//        // 根据鼠标的输入来更新摄像机绕立方体旋转的角度
//        mTheta += dx;
//        mPhi += dy;
//
//        // Restrict the angle mPhi.
//        // 限制角度mPhi的范围
//        mPhi = MathHelper::Clamp(mPhi, 0.1f, MathHelper::Pi - 0.1f);
//    }
//    else if ((btnState & MK_RBUTTON) != 0)
//    {
//        // Make each pixel correspond to 0.005 unit in the scene.
//        // 使场景中的每个像素按鼠标移动距离的0.005倍进行缩放
//        float dx = 0.005f * static_cast<float>(x - mLastMousePos.x);
//        float dy = 0.005f * static_cast<float>(y - mLastMousePos.y);
//
//        // Update the camera radius based on input.
//        // 根据鼠标的输入更新摄像机的可视范围半径
//        mRadius += dx - dy;
//
//        // Restrict the radius.
//        // 限制可视半径的范围
//        mRadius = MathHelper::Clamp(mRadius, 3.0f, 15.0f);
//    }
//
//    mLastMousePos.x = x;
//    mLastMousePos.y = y;
//}
//
///// <summary>
///// 常量缓冲区描述符
///// 
///// 常量缓冲区描述符都要存放在以D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV类型所建的描述符堆里。
///// 这种堆内可以混合存储常量缓冲区描述符、着色器资源描述符和无序访问（unordered access）描述符。
///// 为了存放这些新类型的描述符，我们需要为之创建以下类型的新式描述符堆：
///// 
///// 这段代码与我们之前创建渲染目标和深度/模板缓冲区这两种资源描述符堆的过程很相似。然而，其中却有着一个重要的区别，
///// 那就是在创建供着色器程序访问资源的描述符时，我们要把标志Flags指定为DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE。
///// 在本章的示范程序中，我们并没有使用SRV（shader resource view，着色器资源视图）描述符或UAV（unordered access view，
///// 无序访问视图）描述符，仅是绘制了一个物体而已，因此只需创建一个存有单个CBV描述符的堆即可。
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
///// 创建常量缓冲区
///// 
//
///// 
///// </summary>
//void BoxApp::BuildConstantBuffers()
//{
//    // 此常量缓冲区存储了绘制n(这里n==1)个物体所需的常量数据
//    mObjectCB = std::make_unique<UploadBuffer<ObjectConstants>>(md3dDevice.Get(), 1, true);
//
//    UINT objCBByteSize = d3dUtil::CalcConstantBufferByteSize(sizeof(ObjectConstants));
//
//    // 缓冲区的起始地址(即索引为0的那个常量缓冲区的地址）
//    D3D12_GPU_VIRTUAL_ADDRESS cbAddress = mObjectCB->Resource()->GetGPUVirtualAddress();
//    // Offset to the ith object constant buffer in the buffer.
//    // 偏移到常量缓冲区中第i个物体所对应的常量数据
//    // 这里取i = 0
//    int boxCBufIndex = 0;
//    cbAddress += boxCBufIndex * objCBByteSize;
//
//    /// 结构体D3D12_CONSTANT_BUFFER_VIEW_DESC描述的是绑定到HLSL常量缓冲区结构体的常量缓冲区资源子集。
//    /// 正如前面所提到的，如果常量缓冲区存储了一个内有[插图]个物体常量数据的常量数组，
//    /// 那么我们就可以通过BufferLocation和SizeInBytes参数来获取第[插图]个物体的常量数据。
//    /// 考虑到硬件的需求（即硬件的最小分配空间），成员SizeInBytes与BufferLocation必须为256B的整数倍。
//    /// 例如，若将上述两个成员的值都指定为64，那么我们将看到下列调试错误：
//    /*
//        error:
//        D3D12 ERROR: ID3D12Device::CreateConstantBufferView: SizeInBytes of 64 is invalid. Device requires SizeInBytes be a multiple of 256.
//        D3D12错误：ID3D12Device::CreateConstantBufferView: 将SizeInBytes的值设置为64是无效的。设备要求SizeInBytes的值为256的整数倍。
//        D3D12 ERROR: ID3D12Device:: CreateConstantBufferView: BufferLocation of 64 is invalid. Device requires BufferLocation be a multiple of 256.
//        D3D12错误:ID3D12Device:: CreateConstantBufferView:将BufferLocation的值设置为64是无效的。设备要求BufferLocation的值为256的整数倍。
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
///// 根签名和描述符表
///// 
///// 通常来讲，在绘制调用开始执行之前，我们应将不同的着色器程序所需的各种类型的资源绑定到渲染流水线上。
///// 实际上，不同类型的资源会被绑定到特定的寄存器槽（register slot）上，以供着色器程序访问。
///// 比如说，前文代码中的顶点着色器和像素着色器需要的是一个绑定到寄存器b0的常量缓冲区。在本书的后续内容中，
///// 我们会用到这两种着色器更高级的配置方法，以使多个常量缓冲区、纹理（texture）和采样器（sampler）
///// 都能与各自的寄存器槽相绑定[14]：
///// 
///// // 将纹理资源绑定到纹理寄存器槽0
//    //Texture2D  gDiffuseMap : register(t0);
//    //
//    //// 把下列采样器资源依次绑定到采样器寄存器槽0~5
//    //SamplerState gsamPointWrap : register(s0);
//    //SamplerState gsamPointClamp : register(s1);
//    //SamplerState gsamLinearWrap : register(s2);
//    //SamplerState gsamLinearClamp : register(s3);
//    //SamplerState gsamAnisotropicWrap : register(s4);
//    //SamplerState gsamAnisotropicClamp : register(s5);
//    //
//    //// 将常量缓冲区资源（cbuffer）绑定到常量缓冲区寄存器槽0
//    //cbuffer cbPerObject : register(b0)
//    //{
//    //    float4x4 gWorld;
//    //    float4x4 gTexTransform;
//    //};
//    //
//    //// 绘制过程中所用的杂项常量数据
//    //cbuffer cbPass : register(b1)
//    //{
//    //    float4x4 gView;
//    //    float4x4 gProj;
//    //    [...] // 为篇幅而省略的其他字段
//    //};
//    //
//    //// 绘制每种材质所需的各种不同的常量数据
//    //cbuffer cbMaterial : register(b2)
//    //{
//    //    float4  gDiffuseAlbedo;
//    //    float3  gFresnelR0;
//    //    float  gRoughness;
//    //    float4x4 gMatTransform;
//    //};
///// 
///// 根签名（root signature）定义的是：在执行绘制命令之前，那些应用程序将绑定到渲染流水线上的资源，它们会被映射到着色器的对应输入寄存器。
///// 根签名一定要与使用它的着色器相兼容（即在绘制开始之前，根签名一定要为着色器提供其执行期间需要绑定到渲染流水线的所有资源），
///// 在创建流水线状态对象（pipeline state object）时会对此进行验证（参见6.9节）。不同的绘制调用可能会用到一组不同的着色器程序，
///// 这也就意味着要用到不同的根签名。
///// 
///// 在Direct3D中，根签名由ID3D12RootSignature接口来表示，并以一组描述绘制调用过程中着色器所需资源的根参数（root parameter）
///// 定义而成。根参数可以是根常量（root constant）、根描述符（root descriptor）或者描述符表（descriptor table）。
///// 我们在本章中仅使用描述符表，其他根参数均在第7章中进行讨论。描述符表指定的是描述符堆中存有描述符的一块连续区域。
///// 
///// 下面的代码创建了一个根签名，它的根参数为一个描述符表，其大小足以容下一个CBV（常量缓冲区视图，constant buffer view）。
///// 
///// </summary>
//void BoxApp::BuildRootSignature()
//{
//    // Shader programs typically require resources as input (constant buffers,
//    // textures, samplers).  The root signature defines the resources the shader
//    // programs expect.  If we think of the shader programs as a function, and
//    // the input resources as function parameters, then the root signature can be
//    // thought of as defining the function signature.  
//    // 着色器程序一般需要以资源作为输入（例如常量缓冲区、纹理、采样器等）
//    // 根签名则定义了着色器程序所需的具体资源
//    // 如果把着色器程序看作一个函数，而将输入的资源当作向函数传递的参数数据，那么便可类似地认为根签名
//    // 定义的是函数签名
//    // 
//    // Root parameter can be a table, root descriptor or root constants.
//    // 根参数可以是描述符表、根描述符或根常量
//    CD3DX12_ROOT_PARAMETER slotRootParameter[1];
//
//    /*
//    * 下面这段代码:
//    * 这段代码创建了一个根参数，目的是将含有一个CBV的描述符表绑定到常量缓冲区寄存器0，即HLSL代码中的register(b0)。
//    */
//    // Create a single descriptor table of CBVs.
//    // 创建由单个CBV所组成的描述符表
//    CD3DX12_DESCRIPTOR_RANGE cbvTable;
//    cbvTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 
//        1, // 表中的描述符数量
//        0);// 将这段描述符区域绑定至此基准着色器寄存器（base shader register）
//    slotRootParameter[0].InitAsDescriptorTable(
//        1,          // 描述符区域的数量
//        &cbvTable); // 指向描述符区域数组的指针
//
//    // A root signature is an array of root parameters.
//    // 根签名由一组根参数构成
//    CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(1, slotRootParameter, 0, nullptr,
//        D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
//
//    // create a root signature with a single slot which points to a descriptor range consisting of a single constant buffer
//    // 用单个寄存器槽来创建一个根签名，该槽位指向一个仅含有单个常量缓冲区的描述符区域
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
//    除了空间位置，Direct3D中的顶点还可以存储其他属性数据。为了构建自定义的顶点格式，我们首先要创建一个结构体来容纳选定的顶点数据。
//    例如，下面列出了两种不同类型的顶点格式：一种由位置和颜色信息组成，另一种则由位置、法向量以及两组2D纹理坐标构成.
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
//    定义了顶点结构体之后，我们还需要向Direct3D提供该顶点结构体的描述，使它了解应怎样来处理结构体中的每个成员。
//    用户提供给Direct3D的这种描述被称为输入布局描述（input layout description），
//    用结构体D3D12_INPUT_LAYOUT_DESC来表示：(下面的'mInputLayout'变量)
//
//    输入布局描述实由两部分组成：即一个以D3D12_INPUT_ELEMENT_DESC元素构成的数组，以及一个表示该数组中的元素数量的整数。
//    D3D12_INPUT_ELEMENT_DESC数组中的元素依次描述了顶点结构体中所对应的成员。
//    这就是说，如果某顶点结构体中有两个成员，那么与之对应的D3D12_INPUT_ELEMENT_DESC数组也将存有两个元素。
//
//    /// --------------------------------------------------
//    图6.1解释:
//        // D3D12_INPUT_ELEMENT_DESC数组中的元素与顶点结构体中的成员一一对应，
//        // 而语义名与索引则为顶点元素映射到顶点着色器中对应的参数提供了途径
//        struct Vertex
//        {
//            XMFLOAT3 Pos;       ===> VertexOut VS中的iPos参数
//            XMFLOAT3 Normal;    ===> VertexOut VS中的iNormal参数
//            XMFLOAT2 Tex0;      ===> VertexOut VS中的iTex0参数
//            XMFLOAT2 Tex1;      ===> VertexOut VS中的iTex1参数
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
//    D3D12_INPUT_ELEMENT_DESC结构体的定义如下：
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
//    1．SemanticName：一个与元素相关联的特定字符串，我们称之为语义（semantic），它传达了元素的预期用途。
//        该参数可以是任意合法的语义名。通过语义即可将顶点结构体（图6.1中的struct Vertex）中的元素与顶点着色器输入签名[1]
//        （vertex shader input signature，即图6.1 中的VertexOut VS的众参数）中的元素一一映射起来，如图6.1所示。
//    2．SemanticIndex：附加到语义上的索引。此成员的设计动机可从图6.1中看出。例如，顶点结构体中的纹理坐标可能不止一组，
//        而仅在语义名尾部添加一个索引，即可在不引入新语义名的情况下区分出这两组不同的纹理坐标。
//        在着色器代码中，未标明索引的语义将默认其索引值为0，也就是说，图6.1中的POSITION与POSITION0等价。
//    3．Format：在Direct3D中，要通过枚举类型DXGI_FORMAT中的成员来指定顶点元素的格式（即数据类型）。下面是一些常用的格式。
//        DXGI_FORMAT_R32_FLOAT     // 1D 32位浮点标量
//        DXGI_FORMAT_R32G32_FLOAT    // 2D 32位浮点向量
//        DXGI_FORMAT_R32G32B32_FLOAT  // 3D 32位浮点向量
//        DXGI_FORMAT_R32G32B32A32_FLOAT // 4D 32位浮点向量
//
//        DXGI_FORMAT_R8_UINT     // 1D 8位无符号整型标量
//        DXGI_FORMAT_R16G16_SINT   // 2D 16位有符号整型向量
//        DXGI_FORMAT_R32G32B32_UINT // 3D 32位无符号整型向量
//        DXGI_FORMAT_R8G8B8A8_SINT  // 4D 8位有符号整型向量
//        DXGI_FORMAT_R8G8B8A8_UINT  // 4D 8位无符号整型向量
//    4．InputSlot：指定传递元素所用的输入槽（input slot index）索引。Direct3D共支持16个输入槽（索引值为0～15），
//        可以通过它们来向输入装配阶段传递顶点数据。目前我们只会用到输入槽0（即所有的顶点元素都来自同一个输入槽），
//        但在本章的习题2中将会涉及多输入槽的编程实践。
//    5．AlignedByteOffset：在特定输入槽中，从C++顶点结构体的首地址到其中某点元素起始地址的偏移量（用字节表示）。
//        例如，在下列顶点结构体中，元素Pos的偏移量为0字节，因为它的起始地址与顶点结构体的首地址一致；
//        元素Normal的偏移量为12字节，因为跳过Pos所占用的字节数才能找到Normal的起始地址；
//        元素Tex0的偏移量为24字节，因为跨过Pos和Normal的总字节数才可获取Tex0的起始地址。
//        同理，元素Tex1的偏移量为32字节，只有跳过Pos、Normal和Tex0这3个元素的总字节数方可寻得Tex1的起始地址。
//        struct Vertex2
//        {
//          XMFLOAT3 Pos;  // 偏移量为0字节
//          XMFLOAT3 Normal; // 偏移量为12字节
//          XMFLOAT2 Tex0;  // 偏移量为24字节
//          XMFLOAT2 Tex1;  // 偏移量为32字节
//        };
//    6．InputSlotClass：我们暂且把此参数指定为D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA[2]。
//        而另一选项（D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA[3]）则用于实现实例化（instancing）这种高级技术。
//    7．InstanceDataStepRate：目前仅将此值指定为0。若要采用实例化这种高级技术，则将此参数设为1。
//        就前面Vertex1和Vertex2这两个顶点结构体的例子来说，其相应的输入布局描述为：
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
///// 编译着色器
//void BoxApp::BuildShadersAndInputLayout()
//{
//    HRESULT hr = S_OK;
//
//    /*
//        在Direct3D中，着色器程序必须先被编译为一种可移植的字节码。接下来，图形驱动程序将获取这些字节码，
//        并将其重新编译为针对当前系统GPU所优化的本地指令[ATI1]。我们可以在运行期间用下列函数对着色器进行编译。
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
//        ** D3DCompileFromFile定义在d3dUtil::CompileShader中（上面D3DCompileFromFile原型个参数说明请在d3dUtil::CompileShader查看）
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
//        // front face   // 立方体前表面
//        0, 1, 2,
//        0, 2, 3,
//
//        // back face    // 立方体后表面
//        4, 6, 5,
//        4, 7, 6,
//
//        // left face    // 立方体左表面
//        4, 5, 1,
//        4, 1, 0,
//
//        // right face   // 立方体右表面
//        3, 2, 6,
//        3, 6, 7,
//
//        // top face     // 立方体上表面
//        1, 5, 6,
//        1, 6, 2,
//
//        // bottom face  // 立方体下表面
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
//    流水线状态对象:
//    到目前为止，我们已经展示过编写输入布局描述、创建顶点着色器和像素着色器，以及配置光栅器状态组这3个步骤，
//    还未曾讲解如何将这些对象绑定到图形流水线上，用以实际绘制图形。大多数控制图形流水线状态的对象被统称为流水线状态对象（
//    Pipeline State Object，PSO），用ID3D12PipelineState接口来表示。要创建PSO，我们首先要填写一份描述其细节的D3D12_GRAPHICS_PIPELINE_STATE_DESC结构体实例。
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
//    1．pRootSignature：指向一个与此PSO相绑定的根签名的指针。该根签名一定要与此PSO指定的着色器相兼容。
//    2．VS：待绑定的顶点着色器。此成员由结构体D3D12_SHADER_BYTECODE表示，这个结构体存有指向已编译好的字节码数据的指针，以及该字节码数据所占的字节大小。
//          typedef struct D3D12_SHADER_BYTECODE {
//           const void *pShaderBytecode;
//           SIZE_T   BytecodeLength;
//          } D3D12_SHADER_BYTECODE;
//    3．PS：待绑定的像素着色器。
//    4．DS：待绑定的域着色器（我们将在后续章节中讲解此类型的着色器）。
//    5．HS：待绑定的外壳着色器（我们将在后续章节中讲解此类型的着色器）。
//    6．GS：待绑定的几何着色器（我们将在后续章节中讲解此类型的着色器）。
//    7．StreamOutput：用于实现一种称作流输出（stream-out）的高级技术。目前我们仅将此字段清零。
//    8．BlendState：指定混合（blending）操作所用的混合状态。我们将在后续章节中讨论此状态组，目前仅将此成员指定为默认的CD3DX12_BLEND_DESC(D3D12_DEFAULT)。
//    9．SampleMask：多重采样最多可采集32个样本。借此参数的32位整数值，即可设置每个采样点的采集情况（采集或禁止采集）。
//        例如，若禁用了第5位（将第5位设置为0），则将不会对第5个样本进行采样。当然，要禁止采集第5 个样本的前提是，所用的多重采样至少要有5个样本。
//        假如一个应用程序仅使用了单采样（single sampling），那么只能针对该参数的第1位进行配置。一般来说，使用的都是默认值0xffffffff，即表示对所有的采样点都进行采样。
//    10．RasterizerState：指定用来配置光栅器的光栅化状态。
//    11．DepthStencilState：指定用于配置深度/模板测试的深度/模板状态。我们将在后续章节中对此状态进行讨论，目前只把它设为默认的CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT)。
//    12．InputLayout：输入布局描述，此结构体中有两个成员：一个由D3D12_INPUT_ELEMENT_DESC元素构成的数组，以及一个表示此数组中元素数量的无符号整数。
//          typedef struct D3D12_INPUT_LAYOUT_DESC
//          {
//            const D3D12_INPUT_ELEMENT_DESC *pInputElementDescs;
//            UINT NumElements;
//          } D3D12_INPUT_LAYOUT_DESC;
//    13．PrimitiveTopologyType：指定图元的拓扑类型。
//          typedef enum D3D12_PRIMITIVE_TOPOLOGY_TYPE { 
//           D3D12_PRIMITIVE_TOPOLOGY_TYPE_UNDEFINED = 0,
//           D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT   = 1,
//           D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE    = 2,
//           D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE  = 3,
//           D3D12_PRIMITIVE_TOPOLOGY_TYPE_PATCH   = 4
//          } D3D12_PRIMITIVE_TOPOLOGY_TYPE;
//    14．NumRenderTargets：同时所用的渲染目标数量（即RTVFormats数组中渲染目标格式的数量）。
//    15．RTVFormats：渲染目标的格式。利用该数组实现向多渲染目标同时进行写操作。使用此PSO的渲染目标的格式设定应当与此参数相匹配。
//    16．DSVFormat：深度/模板缓冲区的格式。使用此PSO的深度/模板缓冲区的格式设定应当与此参数相匹配。
//    17．SampleDesc：描述多重采样对每个像素采样的数量及其质量级别。此参数应与渲染目标的对应设置相匹配。
//        在D3D12_GRAPHICS_PIPELINE_STATE_DESC实例填写完毕后，我们即可用ID3D12Device::CreateGraphicsPipelineState方法来创建ID3D12PipelineState对象。
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
//    // 光栅器状态设置。对应D3DX12_RASTERIZER_DESC参数讲解在d3d12.h文件中
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
//    小结:
//    1．除了空间位置信息，Direct3D中的顶点还可以存储其他类型的属性数据。为了创建自定义的顶点格式，我们首先要将选定的顶点数据定义为一个结构体。
//        待顶点结构体定义好后，便可用输入布局描述（D3D12_INPUT_LAYOUT_DESC）向Direct3D提供其细节。输入布局描述由两部分组成：一个D3D12_INPUT_ELEMENT_DESC元素构成的数组，
//        一个记录该数组中元素个数的无符号整数。D3D12_INPUT_ELEMENT_DESC数组中的元素要与顶点结构体中的成员一一对应。事实上，输入布局描述为结构体D3D12_GRAPHICS_PIPELINE_STATE_DESC中的一个字段，
//        这就是说，它实为PSO的一个组成部分，用于与顶点着色器输入签名进行格式的比对验证。因此，当PSO与渲染流水线绑定时，输入布局也将以PSO组成元素的身份随PSO与渲染流水线的IA阶段相绑定。
//    2．为了使GPU可以访问顶点/索引数组，便需要将其置于一种名为缓冲区（buffer）的资源之内。顶点数据与索引数据的缓冲区分别称为顶点缓冲区（vertex buffer）和索引缓冲区（index buffer）。
//        缓冲区用接口ID3D12Resource表示，要创建缓冲区资源需要填写D3D12_RESOURCE_DESC结构体，再调用ID3D12Device::CreateCommittedResource方法。
//        顶点缓冲区的视图与索引缓冲区的视图分别用D3D12_VERTEX_BUFFER_VIEW与D3D12_INDEX_BUFFER_VIEW结构体加以描述。
//        随后，即可通过ID3D12GraphicsCommandList::IASetVertexBuffers方法与ID3D12GraphicsCommandList::IASetIndexBuffer方法分别将顶点缓冲区与索引缓冲区绑定到渲染流水线的IA阶段。
//        最后，要绘制非索引（non-indexed）描述的几何体（即以顶点数据来绘制的几何体）可借助ID3D12GraphicsCommandList::DrawInstanced方法，而以索引描述的几何体可由ID3D12GraphicsCommandList::DrawIndexedInstanced方法进行绘制。
//    3．顶点着色器是一种用HLSL编写并在GPU上运行的程序，它以单个顶点作为输入与输出。每个待绘制的顶点都要流经顶点着色器阶段。这使得程序员能够在此以顶点为基本单位进行处理，继而获取多种多样的渲染效果。从顶点着色器输出的数据将传至渲染流水线的下一个阶段。
//    4．常量缓冲区是一种GPU资源（ID3D12Resource），其数据内容可供着色器程序引用。它们被创建在上传堆（upload heap）而非默认堆（default heap）中。因此，应用程序可通过将数据从系统内存复制到显存中来更新常量缓冲区。
//        如此一来，C++应用程序就可与着色器通信，并更新常量缓冲区内着色器所需的数据。例如，C++程序可以借助这种方式对着色器所用的“世界—观察—投影”矩阵进行更改。在此，我们建议读者考量数据更新的频繁程度，以此为依据来创建不同的常量缓冲区。
//        效率乃是划分常量缓冲区的动机。在对一个常量缓冲区进行更新的时候，其中的所有变量都会随之更新，正所谓牵一发而动全身。因此，应根据更新频率将数据有效地组织为不同的常量缓冲区，以此来避免无谓的冗余的更新，从而提高效率。
//    5．像素着色器是一种用HLSL编写且运行在GPU上的程序，它以经过插值计算所得到的顶点数据作为输入，待处理后，再输出与之对应的一种颜色值。由于硬件优化的原因，某些像素片段可能还未到像素着色器就已被渲染流水线剔除了（例如采用了提前深度剔除技术，early-z rejection）。
//        像素着色器可使程序员以像素为基本单位进行处理，从而获得变化万千的渲染效果。从像素着色器输出的数据将被移交至渲染流水线的下一个阶段。
//    6．大多数控制图形流水线状态的Direct3D对象都被指定到了一种称作流水线状态对象（pipeline state object，PSO）的集合之中，并用ID3D12PipelineState接口来表示。
//        我们将这些对象集总起来再统一对渲染流水线进行设置，是出于对性能因素的考虑。这样一来，Direct3D就能验证所有的状态是否彼此兼容，而驱动程序也将可以提前生成硬件本地指令及其状态。
//*/
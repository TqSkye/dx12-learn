////***************************************************************************************
//// ShapesApp.cpp by Frank Luna (C) 2015 All Rights Reserved.
////
//// Hold down '1' key to view scene in wireframe mode.
////***************************************************************************************
//
//#include "../../Common/d3dApp.h"
//#include "../../Common/MathHelper.h"
//#include "../../Common/UploadBuffer.h"
//#include "../../Common/GeometryGenerator.h"
//#include "../../LearnDemo/Shapes/FrameResource1.h"
//
//using Microsoft::WRL::ComPtr;
//using namespace DirectX;
//using namespace DirectX::PackedVector;
//
//// 我们的应用程序类（ShapesApp）将实例化一个由3个帧资源元素所构成的向量，并留有特定的成员变量来记录当前的帧资源：
//const int gNumFrameResources = 3;
//
///*
//渲染项
//    绘制一个物体需要设置多种参数，例如绑定顶点缓冲区和索引缓冲区、绑定与物体有关的常量数据、设定图元类型以及指定DrawIndexedInstanced方法的参数。
//    随着场景中所绘物体的逐渐增多，如果我们能创建一个轻量级结构来存储绘制物体所需的数据，那真是极好的；
//    由于每个物体的特征不同，绘制过程中所需的数据也会有所变化，因此该结构中的数据也会因具体程序而异。
//    我们把单次绘制调用过程中，需要向渲染流水线提交的数据集称为渲染项（render item）。
//    对于当前的演示程序而言，渲染项RenderItem结构体如下：
//*/
//// Lightweight structure stores parameters to draw a shape.  This will
//// vary from app-to-app.
//// 存储绘制图形所需参数的轻量级结构体。它会随着不同的应用程序而有所差别
//struct RenderItem
//{
//	RenderItem() = default;
//
//    // World matrix of the shape that describes the object's local space
//    // relative to the world space, which defines the position, orientation,
//    // and scale of the object in the world.
//    // 描述物体局部空间相对于世界空间的世界矩阵
//    // 它定义了物体位于世界空间中的位置、朝向以及大小
//    XMFLOAT4X4 World = MathHelper::Identity4x4();
//
//	// Dirty flag indicating the object data has changed and we need to update the constant buffer.
//	// Because we have an object cbuffer for each FrameResource, we have to apply the
//	// update to each FrameResource.  Thus, when we modify obect data we should set 
//	// NumFramesDirty = gNumFrameResources so that each frame resource gets the update.
//    // 用已更新标志（dirty flag）来表示物体的相关数据已发生改变，这意味着我们此时需要更新常量缓
//    // 冲区。由于每个FrameResource中都有一个物体常量缓冲区，所以我们必须对每个FrameResource
//    // 都进行更新。即，当我们修改物体数据的时候，应当按NumFramesDirty = gNumFrameResources
//    // 进行设置，从而使每个帧资源都得到更新
//	int NumFramesDirty = gNumFrameResources;
//
//	// Index into GPU constant buffer corresponding to the ObjectCB for this render item.
//    // 该索引指向的GPU常量缓冲区对应于当前渲染项中的物体常量缓冲区
//	UINT ObjCBIndex = -1;
//
//    // 此渲染项参与绘制的几何体。注意，绘制一个几何体可能会用到多个渲染项
//	MeshGeometry* Geo = nullptr;
//
//    // Primitive topology.
//    // 图元拓扑
//    D3D12_PRIMITIVE_TOPOLOGY PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
//
//    // DrawIndexedInstanced parameters.
//    // DrawIndexedInstanced方法的参数
//    UINT IndexCount = 0;
//    UINT StartIndexLocation = 0;
//    int BaseVertexLocation = 0;
//};
//
//class ShapesApp : public D3DApp
//{
//public:
//    ShapesApp(HINSTANCE hInstance);
//    ShapesApp(const ShapesApp& rhs) = delete;
//    ShapesApp& operator=(const ShapesApp& rhs) = delete;
//    ~ShapesApp();
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
//    void OnKeyboardInput(const GameTimer& gt);
//	void UpdateCamera(const GameTimer& gt);
//	void UpdateObjectCBs(const GameTimer& gt);
//	void UpdateMainPassCB(const GameTimer& gt);
//
//    void BuildDescriptorHeaps();
//    void BuildConstantBufferViews();
//    void BuildRootSignature();
//    void BuildShadersAndInputLayout();
//    void BuildShapeGeometry();
//    void BuildPSOs();
//    void BuildFrameResources();
//    void BuildRenderItems();
//    void DrawRenderItems(ID3D12GraphicsCommandList* cmdList, const std::vector<RenderItem*>& ritems);
// 
//private:
//
//    std::vector<std::unique_ptr<FrameResource1>> mFrameResources;
//    FrameResource1* mCurrFrameResource = nullptr;
//    int mCurrFrameResourceIndex = 0;
//
//    ComPtr<ID3D12RootSignature> mRootSignature = nullptr;
//    ComPtr<ID3D12DescriptorHeap> mCbvHeap = nullptr;
//
//	ComPtr<ID3D12DescriptorHeap> mSrvDescriptorHeap = nullptr;
//
//	std::unordered_map<std::string, std::unique_ptr<MeshGeometry>> mGeometries;
//	std::unordered_map<std::string, ComPtr<ID3DBlob>> mShaders;
//    std::unordered_map<std::string, ComPtr<ID3D12PipelineState>> mPSOs;
//
//    std::vector<D3D12_INPUT_ELEMENT_DESC> mInputLayout;
//
//	// List of all the render items.
//    // 存有所有渲染项的向量
//	std::vector<std::unique_ptr<RenderItem>> mAllRitems;
//
//	// Render items divided by PSO.
//    // 根据PSO(流水线状态对象)来划分渲染项
//	std::vector<RenderItem*> mOpaqueRitems;         // 不透明
//    // std::vector<RenderItem*> mTransparentRitems; // 透明
//
//    PassConstants mMainPassCB;
//
//    UINT mPassCbvOffset = 0;
//
//    bool mIsWireframe = false;
//
//	XMFLOAT3 mEyePos = { 0.0f, 0.0f, 0.0f };
//	XMFLOAT4X4 mView = MathHelper::Identity4x4();
//	XMFLOAT4X4 mProj = MathHelper::Identity4x4();
//
//    float mTheta = 1.5f*XM_PI;
//    float mPhi = 0.2f*XM_PI;
//    float mRadius = 15.0f;
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
//        ShapesApp theApp(hInstance);
//        if(!theApp.Initialize())
//            return 0;
//
//        return theApp.Run();
//    }
//    catch(DxException& e)
//    {
//        MessageBox(nullptr, e.ToString().c_str(), L"HR Failed", MB_OK);
//        return 0;
//    }
//}
//
//ShapesApp::ShapesApp(HINSTANCE hInstance)
//    : D3DApp(hInstance)
//{
//}
//
//ShapesApp::~ShapesApp()
//{
//    if(md3dDevice != nullptr)
//        FlushCommandQueue();
//}
//
//bool ShapesApp::Initialize()
//{
//    if(!D3DApp::Initialize())
//        return false;
//
//    // Reset the command list to prep for initialization commands.
//    ThrowIfFailed(mCommandList->Reset(mDirectCmdListAlloc.Get(), nullptr));
//
//    BuildRootSignature();
//    BuildShadersAndInputLayout();
//    BuildShapeGeometry();
//    BuildRenderItems();
//    BuildFrameResources();
//    BuildDescriptorHeaps();
//    BuildConstantBufferViews();
//    BuildPSOs();
//
//    // Execute the initialization commands.
//    ThrowIfFailed(mCommandList->Close());
//    ID3D12CommandList* cmdsLists[] = { mCommandList.Get() };
//    mCommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);
//
//    // Wait until initialization is complete.
//    FlushCommandQueue();
//
//    return true;
//}
// 
//void ShapesApp::OnResize()
//{
//    D3DApp::OnResize();
//
//    // The window resized, so update the aspect ratio and recompute the projection matrix.
//    XMMATRIX P = XMMatrixPerspectiveFovLH(0.25f*MathHelper::Pi, AspectRatio(), 1.0f, 1000.0f);
//    XMStoreFloat4x4(&mProj, P);
//}
//
//// 现在，CPU端处理第n帧的算法是这样的：
//void ShapesApp::Update(const GameTimer& gt)
//{
//    OnKeyboardInput(gt);
//	UpdateCamera(gt);
//
//    // Cycle through the circular frame resource array.
//    // 循环往复地获取帧资源循环数组中的元素
//    mCurrFrameResourceIndex = (mCurrFrameResourceIndex + 1) % gNumFrameResources;
//    mCurrFrameResource = mFrameResources[mCurrFrameResourceIndex].get();
//
//    // Has the GPU finished processing the commands of the current frame resource?
//    // If not, wait until the GPU has completed commands up to this fence point.
//    // GPU端是否已经执行完处理当前帧资源的所有命令呢？
//    // 如果还没有就令CPU等待，直到GPU完成命令的执行并抵达这个围栏点
//    if(mCurrFrameResource->Fence != 0 && mFence->GetCompletedValue() < mCurrFrameResource->Fence)
//    {
//        HANDLE eventHandle = CreateEventEx(nullptr, false, false, EVENT_ALL_ACCESS);
//        ThrowIfFailed(mFence->SetEventOnCompletion(mCurrFrameResource->Fence, eventHandle));
//        WaitForSingleObject(eventHandle, INFINITE);
//        CloseHandle(eventHandle);
//    }
//
//    // [...] 更新mCurrFrameResource内的资源（例如常量缓冲区）
//	UpdateObjectCBs(gt);
//	UpdateMainPassCB(gt);
//
//    /*  cpu使用循环帧向gpu发送:
//        不难看出，这种解决方案还是无法完全避免等待情况的发生。如果两种处理器处理帧的速度差距过大，
//        则前者终将不得不等待后来者追上，因为差距过大将导致帧资源数据被错误地复写。
//        如果GPU处理命令的速度快于CPU提交命令列表的速度，则GPU会进入空闲状态。
//        通常来讲，若要尝试淋漓尽致地发挥系统图形处理方面的能力，就应当避免此情况的发生，
//        因为这并没有充分利用GPU资源。此外，如果CPU处理帧的速度总是遥遥领先于GPU，则CPU一定存在等待的时间。
//        而这正是我们所期待的情景，因为这使GPU被完全调动了起来，而CPU多出来的空闲时间总是可以被游戏的其他部分所利用，
//        如AI（人工智能）、物理模拟以及游戏业务逻辑等。
//        因此，如果说采用多个帧资源也无法避免等待现象的发生，那么它对我们究竟有何用处呢？
//        答案是：它使我们可以持续向GPU提供数据。也就是说，当GPU在处理第[n]帧的命令时，
//        CPU可以继续构建和提交绘制第[n + 1]帧和第[n + 2]帧所用的命令。这将令命令队列保持非空状态，从而使GPU总有任务去执行。
//    */
//}
//
//void ShapesApp::Draw(const GameTimer& gt)
//{
//    auto cmdListAlloc = mCurrFrameResource->CmdListAlloc;
//
//    // Reuse the memory associated with command recording.
//    // We can only reset when the associated command lists have finished execution on the GPU.
//    // 复用与记录命令有关的内存
//    // 只有在GPU 执行完与该内存相关联的命令列表时，才能对此命令列表分配器进行重置
//    ThrowIfFailed(cmdListAlloc->Reset());
//
//    // A command list can be reset after it has been added to the command queue via ExecuteCommandList.
//    // Reusing the command list reuses memory.
//    // 在通过ExecuteCommandList方法将命令列表添加到命令队列中之后，我们就可以对它进行重置
//    // 复用命令列表即复用与之相关的内存
//    if(mIsWireframe)
//    {
//        ThrowIfFailed(mCommandList->Reset(cmdListAlloc.Get(), mPSOs["opaque_wireframe"].Get()));
//    }
//    else
//    {
//        ThrowIfFailed(mCommandList->Reset(cmdListAlloc.Get(), mPSOs["opaque"].Get()));
//    }
//
//    mCommandList->RSSetViewports(1, &mScreenViewport);
//    mCommandList->RSSetScissorRects(1, &mScissorRect);
//
//    // Indicate a state transition on the resource usage.
//    // 根据资源的用途指示资源状态的转换
//	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(),
//		D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));
//
//    // Clear the back buffer and depth buffer.
//    // 清除后台缓冲区和深度缓冲区
//    mCommandList->ClearRenderTargetView(CurrentBackBufferView(), Colors::LightSteelBlue, 0, nullptr);
//    mCommandList->ClearDepthStencilView(DepthStencilView(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);
//
//    // Specify the buffers we are going to render to.
//    // 指定要渲染的目标缓冲区
//    mCommandList->OMSetRenderTargets(1, &CurrentBackBufferView(), true, &DepthStencilView());
//
//    ID3D12DescriptorHeap* descriptorHeaps[] = { mCbvHeap.Get() };
//    mCommandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);
//
//	mCommandList->SetGraphicsRootSignature(mRootSignature.Get());
//
//    int passCbvIndex = mPassCbvOffset + mCurrFrameResourceIndex;
//    auto passCbvHandle = CD3DX12_GPU_DESCRIPTOR_HANDLE(mCbvHeap->GetGPUDescriptorHandleForHeapStart());
//    passCbvHandle.Offset(passCbvIndex, mCbvSrvUavDescriptorSize);
//    mCommandList->SetGraphicsRootDescriptorTable(1, passCbvHandle);
//
//    // 在执行主要绘制函数Draw的过程中调用DrawRenderItems方法：
//    DrawRenderItems(mCommandList.Get(), mOpaqueRitems);
//
//    // Indicate a state transition on the resource usage.
//    // 按照资源的用途指示资源状态的转换
//	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(),
//		D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));
//
//    // Done recording commands.
//    // 完成命令的记录
//    ThrowIfFailed(mCommandList->Close());
//
//    // Add the command list to the queue for execution.
//    // 将命令列表加入到命令队列中用于执行
//    ID3D12CommandList* cmdsLists[] = { mCommandList.Get() };
//    mCommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);
//
//    // Swap the back and front buffers
//    // 交换前后台缓冲区
//    ThrowIfFailed(mSwapChain->Present(0, 0));
//	mCurrBackBuffer = (mCurrBackBuffer + 1) % SwapChainBufferCount;
//
//    // [...] 构建和提交本帧的命令列表
//
//    // Advance the fence value to mark commands up to this fence point.
//    // 增加围栏值，将命令标记到此围栏点
//    mCurrFrameResource->Fence = ++mCurrentFence;
//    
//    // Add an instruction to the command queue to set a new fence point. 
//    // Because we are on the GPU timeline, the new fence point won't be 
//    // set until the GPU finishes processing all the commands prior to this Signal().
//    // 向命令队列添加一条指令来设置一个新的围栏点
//    // 由于当前的GPU正在执行绘制命令，所以在GPU处理完Signal()函数之前的所有命令以前，并不会设置此新的围栏点
//    mCommandQueue->Signal(mFence.Get(), mCurrentFence);
//
//    // 值得注意的是，GPU此时可能仍然在处理上一帧数据，但是这也没什么问题，因为我们这些操作并没有影响与之前帧相关联的帧资源
//}
//
//void ShapesApp::OnMouseDown(WPARAM btnState, int x, int y)
//{
//    mLastMousePos.x = x;
//    mLastMousePos.y = y;
//
//    SetCapture(mhMainWnd);
//}
//
//void ShapesApp::OnMouseUp(WPARAM btnState, int x, int y)
//{
//    ReleaseCapture();
//}
//
//void ShapesApp::OnMouseMove(WPARAM btnState, int x, int y)
//{
//    if((btnState & MK_LBUTTON) != 0)
//    {
//        // Make each pixel correspond to a quarter of a degree.
//        float dx = XMConvertToRadians(0.25f*static_cast<float>(x - mLastMousePos.x));
//        float dy = XMConvertToRadians(0.25f*static_cast<float>(y - mLastMousePos.y));
//
//        // Update angles based on input to orbit camera around box.
//        mTheta += dx;
//        mPhi += dy;
//
//        // Restrict the angle mPhi.
//        mPhi = MathHelper::Clamp(mPhi, 0.1f, MathHelper::Pi - 0.1f);
//    }
//    else if((btnState & MK_RBUTTON) != 0)
//    {
//        // Make each pixel correspond to 0.2 unit in the scene.
//        float dx = 0.05f*static_cast<float>(x - mLastMousePos.x);
//        float dy = 0.05f*static_cast<float>(y - mLastMousePos.y);
//
//        // Update the camera radius based on input.
//        mRadius += dx - dy;
//
//        // Restrict the radius.
//        mRadius = MathHelper::Clamp(mRadius, 5.0f, 150.0f);
//    }
//
//    mLastMousePos.x = x;
//    mLastMousePos.y = y;
//}
// 
//void ShapesApp::OnKeyboardInput(const GameTimer& gt)
//{
//    if(GetAsyncKeyState('1') & 0x8000)
//        mIsWireframe = true;
//    else
//        mIsWireframe = false;
//}
// 
//void ShapesApp::UpdateCamera(const GameTimer& gt)
//{
//	// Convert Spherical to Cartesian coordinates.
//	mEyePos.x = mRadius*sinf(mPhi)*cosf(mTheta);
//	mEyePos.z = mRadius*sinf(mPhi)*sinf(mTheta);
//	mEyePos.y = mRadius*cosf(mPhi);
//
//	// Build the view matrix.
//	XMVECTOR pos = XMVectorSet(mEyePos.x, mEyePos.y, mEyePos.z, 1.0f);
//	XMVECTOR target = XMVectorZero();
//	XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
//
//	XMMATRIX view = XMMatrixLookAtLH(pos, target, up);
//	XMStoreFloat4x4(&mView, view);
//}
//
///*
//    我们做出上述调整的思路为：基于资源的更新频率对常量数据进行分组。在每次渲染过程（render pass）中，只需将本次所用的常量（cbPass）更新一次；
//    而每当某个物体的世界矩阵发生改变时，只需更新该物体的相关常量（cbPerObject）即可。
//    如果场景中有一个静态物体，比如一棵树，则只需对它的物体常量缓冲区设置一次（树的）世界矩阵，而后就再也不必对它进行更新了。
//    在我们的演示程序中，将通过下列方法来更新渲染过程常量缓冲区以及物体常量缓冲区。
//    在绘制每一帧画面时，这两个方法(UpdateObjectCBs(物体常量缓冲区)和UpdateMainPassCB(渲染过程常量缓冲区))都将被Update函数调用一次。
//*/
//void ShapesApp::UpdateObjectCBs(const GameTimer& gt)
//{
//	auto currObjectCB = mCurrFrameResource->ObjectCB.get();
//	for(auto& e : mAllRitems)
//	{
//		// Only update the cbuffer data if the constants have changed.  
//		// This needs to be tracked per frame resource.
//        // 只要常量发生了改变就得更新常量缓冲区内的数据。而且要对每个帧资源都进行更新
//		if(e->NumFramesDirty > 0)
//		{
//			XMMATRIX world = XMLoadFloat4x4(&e->World);
//
//			ObjectConstants objConstants;
//			XMStoreFloat4x4(&objConstants.World, XMMatrixTranspose(world));
//
//			currObjectCB->CopyData(e->ObjCBIndex, objConstants);
//
//			// Next FrameResource1 need to be updated too.
//            // 还需要对下一个FrameResource进行更新
//			e->NumFramesDirty--;
//		}
//	}
//}
//
//void ShapesApp::UpdateMainPassCB(const GameTimer& gt)
//{
//	XMMATRIX view = XMLoadFloat4x4(&mView);
//	XMMATRIX proj = XMLoadFloat4x4(&mProj);
//
//	XMMATRIX viewProj = XMMatrixMultiply(view, proj);
//	XMMATRIX invView = XMMatrixInverse(&XMMatrixDeterminant(view), view);
//	XMMATRIX invProj = XMMatrixInverse(&XMMatrixDeterminant(proj), proj);
//	XMMATRIX invViewProj = XMMatrixInverse(&XMMatrixDeterminant(viewProj), viewProj);
//
//	XMStoreFloat4x4(&mMainPassCB.View, XMMatrixTranspose(view));
//	XMStoreFloat4x4(&mMainPassCB.InvView, XMMatrixTranspose(invView));
//	XMStoreFloat4x4(&mMainPassCB.Proj, XMMatrixTranspose(proj));
//	XMStoreFloat4x4(&mMainPassCB.InvProj, XMMatrixTranspose(invProj));
//	XMStoreFloat4x4(&mMainPassCB.ViewProj, XMMatrixTranspose(viewProj));
//	XMStoreFloat4x4(&mMainPassCB.InvViewProj, XMMatrixTranspose(invViewProj));
//	mMainPassCB.EyePosW = mEyePos;
//	mMainPassCB.RenderTargetSize = XMFLOAT2((float)mClientWidth, (float)mClientHeight);
//	mMainPassCB.InvRenderTargetSize = XMFLOAT2(1.0f / mClientWidth, 1.0f / mClientHeight);
//	mMainPassCB.NearZ = 1.0f;
//	mMainPassCB.FarZ = 1000.0f;
//	mMainPassCB.TotalTime = gt.TotalTime();
//	mMainPassCB.DeltaTime = gt.DeltaTime();
//
//	auto currPassCB = mCurrFrameResource->PassCB.get();
//	currPassCB->CopyData(0, mMainPassCB);
//}
//
///*
//    帧资源和常量缓冲区视图
//    回顾前文可知，我们已经创建了一个由FrameResource类型元素所构成的向量，每个FrameResource中都有上传缓冲区，
//    用于为场景中的每个渲染项存储渲染过程常量以及物体常量数据。
//    std::unique_ptr<UploadBuffer<PassConstants>> PassCB = nullptr;
//    std::unique_ptr<UploadBuffer<ObjectConstants>> ObjectCB = nullptr;
//
//    如果有3个帧资源与[n]个渲染项，那么就应存在[3n]个物体常量缓冲区（object constant buffer）以及3个渲染过程常量缓冲区（pass constant buffer）。
//    因此，我们也就需要创建[3(n+1)]个常量缓冲区视图（CBV）。这样一来，我们还要修改CBV堆以容纳额外的描述符：
//*/
//
//void ShapesApp::BuildDescriptorHeaps()
//{
//    UINT objCount = (UINT)mOpaqueRitems.size();
//
//    // Need a CBV descriptor for each object for each frame resource,
//    // +1 for the perPass CBV for each frame resource.
//    // 我们需要为每个帧资源中的每一个物体都创建一个CBV描述符，
//    // 为了容纳每个帧资源中的渲染过程CBV而+1
//    UINT numDescriptors = (objCount+1) * gNumFrameResources;
//
//    // Save an offset to the start of the pass CBVs.  These are the last 3 descriptors.
//    // 保存渲染过程CBV的起始偏移量。在本程序中，这是排在最后面的3个描述符
//    mPassCbvOffset = objCount * gNumFrameResources;
//
//    D3D12_DESCRIPTOR_HEAP_DESC cbvHeapDesc;
//    cbvHeapDesc.NumDescriptors = numDescriptors;
//    cbvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
//    cbvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
//    cbvHeapDesc.NodeMask = 0;
//    ThrowIfFailed(md3dDevice->CreateDescriptorHeap(&cbvHeapDesc,
//        IID_PPV_ARGS(&mCbvHeap)));
//}
//
///*
//    紧接着BuildDescriptorHeaps() 之后:
//    现在，我们就可以用下列代码来填充CBV堆，其中描述符0至描述符[n-1]包含了第0个帧资源的物体CBV，
//    描述符[n]至描述符[2n-1]容纳了第1个帧资源的物体CBV，以此类推，描述符[2n]至描述符[3n-1]包含了第2个帧资源的物体CBV。
//    最后，[3n]、[3n+1]以及[3n+2]分别存有第0个、第1个和第2个帧资源的渲染过程CBV：
//*/
//void ShapesApp::BuildConstantBufferViews()
//{
//    UINT objCBByteSize = d3dUtil::CalcConstantBufferByteSize(sizeof(ObjectConstants));
//
//    UINT objCount = (UINT)mOpaqueRitems.size();
//
//    // Need a CBV descriptor for each object for each frame resource.
//    // 每个帧资源中的每一个物体都需要一个对应的CBV描述符
//    for(int frameIndex = 0; frameIndex < gNumFrameResources; ++frameIndex)
//    {
//        auto objectCB = mFrameResources[frameIndex]->ObjectCB->Resource();
//        for(UINT i = 0; i < objCount; ++i)
//        {
//            D3D12_GPU_VIRTUAL_ADDRESS cbAddress = objectCB->GetGPUVirtualAddress();
//
//            // Offset to the ith object constant buffer in the buffer.
//            // 偏移到缓冲区中第i个物体的常量缓冲区
//            cbAddress += i*objCBByteSize;
//
//            // Offset to the object cbv in the descriptor heap.
//            // 偏移到该物体在描述符堆中的CBV
//            int heapIndex = frameIndex*objCount + i;
//            auto handle = CD3DX12_CPU_DESCRIPTOR_HANDLE(mCbvHeap->GetCPUDescriptorHandleForHeapStart());
//            handle.Offset(heapIndex, mCbvSrvUavDescriptorSize);
//
//            D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc;
//            cbvDesc.BufferLocation = cbAddress;
//            cbvDesc.SizeInBytes = objCBByteSize;
//
//            md3dDevice->CreateConstantBufferView(&cbvDesc, handle);
//        }
//    }
//
//    UINT passCBByteSize = d3dUtil::CalcConstantBufferByteSize(sizeof(PassConstants));
//
//    // Last three descriptors are the pass CBVs for each frame resource.
//    // 最后3个描述符依次是每个帧资源的渲染过程CBV
//    for(int frameIndex = 0; frameIndex < gNumFrameResources; ++frameIndex)
//    {
//        auto passCB = mFrameResources[frameIndex]->PassCB->Resource();
//        // 每个帧资源的渲染过程缓冲区中只存有一个常量缓冲区
//        D3D12_GPU_VIRTUAL_ADDRESS cbAddress = passCB->GetGPUVirtualAddress();
//
//        // Offset to the pass cbv in the descriptor heap.
//        // 偏移到描述符堆中对应的渲染过程CBV
//        int heapIndex = mPassCbvOffset + frameIndex;
//        auto handle = CD3DX12_CPU_DESCRIPTOR_HANDLE(mCbvHeap->GetCPUDescriptorHandleForHeapStart());
//        handle.Offset(heapIndex, mCbvSrvUavDescriptorSize);
//
//        D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc;
//        cbvDesc.BufferLocation = cbAddress;
//        cbvDesc.SizeInBytes = passCBByteSize;
//        
//        md3dDevice->CreateConstantBufferView(&cbvDesc, handle);
//    }
//}
//
///*
//    根参数
//    回顾根签名的相关知识可知，根签名是由一系列根参数定义而成。到目前为止，我们只创建过存有一个描述符表的根参数。
//    然而，根参数其实有3个类型可选。
//    1．描述符表（descriptor table）：描述符表引用的是描述符堆中的一块连续范围，用于确定要绑定的资源。
//    2．根描述符（root descriptor，又称为内联描述符，inline descriptor）：
//        通过直接设置根描述符即可指示要绑定的资源，而且无需将它存于描述符堆中。但是，只有常量缓冲区的CBV，
//        以及缓冲区的SRV/UAV（着色器资源视图/无序访问视图）才可以根描述符的身份进行绑定。这也就意味着纹理的SRV并不能作为根描述符来实现资源绑定。
//    3．根常量（root constant）：借助根常量可直接绑定一系列32位的常量值。考虑到性能因素，可放入一个根签名的数据以64 DWORD为限。
//    3种根参数类型占用空间的情况如下。
//        1．描述符表：每个描述符表占用1 DWORD。
//        2．根描述符：每个根描述符（64位的GPU虚拟地址）占用2DWORD。
//        3．根常量：每个常量32位，占用1 DWORD。
//    我们可以创建出任意组合的根签名，只要它不超过64DWORD的上限即可。根常量虽然用起来方便，但是它的空间消耗增加迅速。
//*/
//
//void ShapesApp::BuildRootSignature()
//{
//    CD3DX12_DESCRIPTOR_RANGE cbvTable0;
//    cbvTable0.Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0);
//
//    CD3DX12_DESCRIPTOR_RANGE cbvTable1;
//    cbvTable1.Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 1);
//
//    /*
//        在代码中要通过填写CD3DX12_ROOT_PARAMETER结构体来描述根参数。就像我们之前曾见到过的以CD3DX作为前缀的其他辅助结构体一样，
//        CD3DX12_ROOT_PARAMETER是对结构体D3D12_ROOT_PARAMETER进行扩展，并增加一些辅助初始化函数而得来的[2]。
//    */
//	// Root parameter can be a table, root descriptor or root constants.
//    // 根参数可能是描述符表、根描述符或根常量
//	CD3DX12_ROOT_PARAMETER slotRootParameter[2];
//
//	// Create root CBVs.
//    // 创建根CBV
//    slotRootParameter[0].InitAsDescriptorTable(1, &cbvTable0);
//    slotRootParameter[1].InitAsDescriptorTable(1, &cbvTable1);
//
//	// A root signature is an array of root parameters.
//    // 根签名由一系列根参数所构成
//	CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(2, slotRootParameter, 0, nullptr, 
//        D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
//
//	// create a root signature with a single slot which points to a descriptor range consisting of a single constant buffer
//	ComPtr<ID3DBlob> serializedRootSig = nullptr;
//	ComPtr<ID3DBlob> errorBlob = nullptr;
//	HRESULT hr = D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1,
//		serializedRootSig.GetAddressOf(), errorBlob.GetAddressOf());
//
//	if(errorBlob != nullptr)
//	{
//		::OutputDebugStringA((char*)errorBlob->GetBufferPointer());
//	}
//	ThrowIfFailed(hr);
//
//	ThrowIfFailed(md3dDevice->CreateRootSignature(
//		0,
//		serializedRootSig->GetBufferPointer(),
//		serializedRootSig->GetBufferSize(),
//		IID_PPV_ARGS(mRootSignature.GetAddressOf())));
//}
//
//void ShapesApp::BuildShadersAndInputLayout()
//{
//	mShaders["standardVS"] = d3dUtil::CompileShader(L"E:\\DX12Book\\DX12LearnProject\\DX12Learn\\LearnDemo\\Shapes\\Shaders\\color.hlsl", nullptr, "VS", "vs_5_1");
//	mShaders["opaquePS"] = d3dUtil::CompileShader(L"E:\\DX12Book\\DX12LearnProject\\DX12Learn\\LearnDemo\\Shapes\\Shaders\\color.hlsl", nullptr, "PS", "ps_5_1");
//	
//    mInputLayout =
//    {
//        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
//        { "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
//    };
//}
//
///*
//    点缓冲区和索引缓冲区正如图7.6所示，在Shapes演示程序中，我们要绘制长方体、栅格、柱体（圆台）及球体。尽管在示例中要绘制多个球体和柱体，但实际上我们只需对一组球体和圆台数据的副本。
//    通过利用不同的世界矩阵，对这组数据进行多次绘制，即可绘制出多个预定的球体与圆台。所以说，这也是一个几何体实例化（instancing）的范例，借助此技术可以减少内存资源的占用。
//
//    通过将不同物体的顶点和索引合并起来，我们把所有几何体网格的顶点和索引都装进一个顶点缓冲区及一个索引缓冲区内。
//    这意味着，在绘制一个物体时，我们只需绘制此物体位于顶点和索引这两种缓冲区中的数据子集。为了调用ID3D12GraphicsCommandList::DrawIndexedInstanced方法而仅绘制几何体的子集，
//    我们需要掌握3种数据（可参照图6.3并回顾6.3节的相关讨论），它们分别是：待绘制物体在合并索引缓冲区中的起始索引、待绘制物体的索引数量，以及基准顶点地址——即待绘制物体第一个顶点在合并顶点缓冲区中的索引。
//    经回顾可知，基准顶点地址是一个整数值，我们要在绘制调用过程中、获取顶点数据之前，将它与物体的原索引值依次相加，以此获取合并顶点缓冲区中所绘物体的正确顶点子集（参见第5章的练习2）。
//    下列代码展示了创建几何体缓冲区、缓存绘制调用所需参数值以及绘制物体的具体过程。
//*/
//void ShapesApp::BuildShapeGeometry()
//{
//    GeometryGenerator geoGen;
//	GeometryGenerator::MeshData box = geoGen.CreateBox(1.5f, 0.5f, 1.5f, 3);
//	GeometryGenerator::MeshData grid = geoGen.CreateGrid(20.0f, 30.0f, 60, 40);
//	GeometryGenerator::MeshData sphere = geoGen.CreateSphere(0.5f, 20, 20);
//	GeometryGenerator::MeshData cylinder = geoGen.CreateCylinder(0.5f, 0.3f, 3.0f, 20, 20);
//
//	//
//	// We are concatenating all the geometry into one big vertex/index buffer.  So
//	// define the regions in the buffer each submesh covers.
//	// 将所有的几何体数据都合并到一对大的顶点/索引缓冲区中
//    // 以此来定义每个子网格数据在缓冲区中所占的范围
//    //
//
//	// Cache the vertex offsets to each object in the concatenated vertex buffer.
//    // 对合并顶点缓冲区中每个物体的顶点偏移量进行缓存
//	UINT boxVertexOffset = 0;
//	UINT gridVertexOffset = (UINT)box.Vertices.size();
//	UINT sphereVertexOffset = gridVertexOffset + (UINT)grid.Vertices.size();
//	UINT cylinderVertexOffset = sphereVertexOffset + (UINT)sphere.Vertices.size();
//
//	// Cache the starting index for each object in the concatenated index buffer.
//    // 对合并索引缓冲区中每个物体的起始索引进行缓存
//	UINT boxIndexOffset = 0;
//	UINT gridIndexOffset = (UINT)box.Indices32.size();
//	UINT sphereIndexOffset = gridIndexOffset + (UINT)grid.Indices32.size();
//	UINT cylinderIndexOffset = sphereIndexOffset + (UINT)sphere.Indices32.size();
//
//    // Define the SubmeshGeometry that cover different 
//    // regions of the vertex/index buffers.
//    // 定义的多个SubmeshGeometry结构体中包含了顶点/索引缓冲区内不同几何体的子网格数据
//
//	SubmeshGeometry boxSubmesh;
//	boxSubmesh.IndexCount = (UINT)box.Indices32.size();
//	boxSubmesh.StartIndexLocation = boxIndexOffset;
//	boxSubmesh.BaseVertexLocation = boxVertexOffset;
//
//	SubmeshGeometry gridSubmesh;
//	gridSubmesh.IndexCount = (UINT)grid.Indices32.size();
//	gridSubmesh.StartIndexLocation = gridIndexOffset;
//	gridSubmesh.BaseVertexLocation = gridVertexOffset;
//
//	SubmeshGeometry sphereSubmesh;
//	sphereSubmesh.IndexCount = (UINT)sphere.Indices32.size();
//	sphereSubmesh.StartIndexLocation = sphereIndexOffset;
//	sphereSubmesh.BaseVertexLocation = sphereVertexOffset;
//
//	SubmeshGeometry cylinderSubmesh;
//	cylinderSubmesh.IndexCount = (UINT)cylinder.Indices32.size();
//	cylinderSubmesh.StartIndexLocation = cylinderIndexOffset;
//	cylinderSubmesh.BaseVertexLocation = cylinderVertexOffset;
//
//	//
//	// Extract the vertex elements we are interested in and pack the
//	// vertices of all the meshes into one vertex buffer.
//	// 提取出所需的顶点元素，再将所有网格的顶点装进一个顶点缓冲区
//    //
//
//	auto totalVertexCount =
//		box.Vertices.size() +
//		grid.Vertices.size() +
//		sphere.Vertices.size() +
//		cylinder.Vertices.size();
//
//	std::vector<Vertex> vertices(totalVertexCount);
//
//	UINT k = 0;
//	for(size_t i = 0; i < box.Vertices.size(); ++i, ++k)
//	{
//		vertices[k].Pos = box.Vertices[i].Position;
//        vertices[k].Color = XMFLOAT4(DirectX::Colors::DarkGreen);
//	}
//
//	for(size_t i = 0; i < grid.Vertices.size(); ++i, ++k)
//	{
//		vertices[k].Pos = grid.Vertices[i].Position;
//        vertices[k].Color = XMFLOAT4(DirectX::Colors::ForestGreen);
//	}
//
//	for(size_t i = 0; i < sphere.Vertices.size(); ++i, ++k)
//	{
//		vertices[k].Pos = sphere.Vertices[i].Position;
//        vertices[k].Color = XMFLOAT4(DirectX::Colors::Crimson);
//	}
//
//	for(size_t i = 0; i < cylinder.Vertices.size(); ++i, ++k)
//	{
//		vertices[k].Pos = cylinder.Vertices[i].Position;
//		vertices[k].Color = XMFLOAT4(DirectX::Colors::SteelBlue);
//	}
//
//	std::vector<std::uint16_t> indices;
//	indices.insert(indices.end(), std::begin(box.GetIndices16()), std::end(box.GetIndices16()));
//	indices.insert(indices.end(), std::begin(grid.GetIndices16()), std::end(grid.GetIndices16()));
//	indices.insert(indices.end(), std::begin(sphere.GetIndices16()), std::end(sphere.GetIndices16()));
//	indices.insert(indices.end(), std::begin(cylinder.GetIndices16()), std::end(cylinder.GetIndices16()));
//
//    const UINT vbByteSize = (UINT)vertices.size() * sizeof(Vertex);
//    const UINT ibByteSize = (UINT)indices.size()  * sizeof(std::uint16_t);
//
//	auto geo = std::make_unique<MeshGeometry>();
//	geo->Name = "shapeGeo";
//
//	ThrowIfFailed(D3DCreateBlob(vbByteSize, &geo->VertexBufferCPU));
//	CopyMemory(geo->VertexBufferCPU->GetBufferPointer(), vertices.data(), vbByteSize);
//
//	ThrowIfFailed(D3DCreateBlob(ibByteSize, &geo->IndexBufferCPU));
//	CopyMemory(geo->IndexBufferCPU->GetBufferPointer(), indices.data(), ibByteSize);
//
//	geo->VertexBufferGPU = d3dUtil::CreateDefaultBuffer(md3dDevice.Get(),
//		mCommandList.Get(), vertices.data(), vbByteSize, geo->VertexBufferUploader);
//
//	geo->IndexBufferGPU = d3dUtil::CreateDefaultBuffer(md3dDevice.Get(),
//		mCommandList.Get(), indices.data(), ibByteSize, geo->IndexBufferUploader);
//
//	geo->VertexByteStride = sizeof(Vertex);
//	geo->VertexBufferByteSize = vbByteSize;
//	geo->IndexFormat = DXGI_FORMAT_R16_UINT;
//	geo->IndexBufferByteSize = ibByteSize;
//
//	geo->DrawArgs["box"] = boxSubmesh;
//	geo->DrawArgs["grid"] = gridSubmesh;
//	geo->DrawArgs["sphere"] = sphereSubmesh;
//	geo->DrawArgs["cylinder"] = cylinderSubmesh;
//
//	mGeometries[geo->Name] = std::move(geo);
//}
//
//void ShapesApp::BuildPSOs()
//{
//    D3D12_GRAPHICS_PIPELINE_STATE_DESC opaquePsoDesc;
//
//	//
//	// PSO for opaque objects.
//	//
//    ZeroMemory(&opaquePsoDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
//	opaquePsoDesc.InputLayout = { mInputLayout.data(), (UINT)mInputLayout.size() };
//	opaquePsoDesc.pRootSignature = mRootSignature.Get();
//	opaquePsoDesc.VS = 
//	{ 
//		reinterpret_cast<BYTE*>(mShaders["standardVS"]->GetBufferPointer()), 
//		mShaders["standardVS"]->GetBufferSize()
//	};
//	opaquePsoDesc.PS = 
//	{ 
//		reinterpret_cast<BYTE*>(mShaders["opaquePS"]->GetBufferPointer()),
//		mShaders["opaquePS"]->GetBufferSize()
//	};
//	opaquePsoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
//    opaquePsoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_WIREFRAME;
//	opaquePsoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
//	opaquePsoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
//	opaquePsoDesc.SampleMask = UINT_MAX;
//	opaquePsoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
//	opaquePsoDesc.NumRenderTargets = 1;
//	opaquePsoDesc.RTVFormats[0] = mBackBufferFormat;
//	opaquePsoDesc.SampleDesc.Count = m4xMsaaState ? 4 : 1;
//	opaquePsoDesc.SampleDesc.Quality = m4xMsaaState ? (m4xMsaaQuality - 1) : 0;
//	opaquePsoDesc.DSVFormat = mDepthStencilFormat;
//    ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&opaquePsoDesc, IID_PPV_ARGS(&mPSOs["opaque"])));
//
//
//    //
//    // PSO for opaque wireframe objects.
//    //
//
//    D3D12_GRAPHICS_PIPELINE_STATE_DESC opaqueWireframePsoDesc = opaquePsoDesc;
//    opaqueWireframePsoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_WIREFRAME;
//    ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&opaqueWireframePsoDesc, IID_PPV_ARGS(&mPSOs["opaque_wireframe"])));
//}
//
//void ShapesApp::BuildFrameResources()
//{
//    for(int i = 0; i < gNumFrameResources; ++i)
//    {
//        mFrameResources.push_back(std::make_unique<FrameResource1>(md3dDevice.Get(),
//            1, (UINT)mAllRitems.size()));
//    }
//}
//
///*
//    渲染项
//    现在我们来定义场景中的渲染项。通过观察以下代码可知，所有的渲染项共用同一个MeshGeometry，
//    所以，我们通过DrawArgs获取DrawIndexedInstanced方法的参数，并以此来绘制顶点/索引缓冲区的子区域（也就是单个几何体）。
//
//    // ShapesApp类中的成员变量
//    std::vector<std::unique_ptr<RenderItem>> mAllRitems;
//    std::vector<RenderItem*> mOpaqueRitems;
//*/
//void ShapesApp::BuildRenderItems()
//{
//	auto boxRitem = std::make_unique<RenderItem>();
//	XMStoreFloat4x4(&boxRitem->World, XMMatrixScaling(2.0f, 2.0f, 2.0f)*XMMatrixTranslation(0.0f, 0.5f, 0.0f));
//	boxRitem->ObjCBIndex = 0;
//	boxRitem->Geo = mGeometries["shapeGeo"].get();
//	boxRitem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
//	boxRitem->IndexCount = boxRitem->Geo->DrawArgs["box"].IndexCount;
//	boxRitem->StartIndexLocation = boxRitem->Geo->DrawArgs["box"].StartIndexLocation;
//	boxRitem->BaseVertexLocation = boxRitem->Geo->DrawArgs["box"].BaseVertexLocation;
//	mAllRitems.push_back(std::move(boxRitem));
//
//    auto gridRitem = std::make_unique<RenderItem>();
//    gridRitem->World = MathHelper::Identity4x4();
//	gridRitem->ObjCBIndex = 1;
//	gridRitem->Geo = mGeometries["shapeGeo"].get();
//	gridRitem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
//    gridRitem->IndexCount = gridRitem->Geo->DrawArgs["grid"].IndexCount;
//    gridRitem->StartIndexLocation = gridRitem->Geo->DrawArgs["grid"].StartIndexLocation;
//    gridRitem->BaseVertexLocation = gridRitem->Geo->DrawArgs["grid"].BaseVertexLocation;
//	mAllRitems.push_back(std::move(gridRitem));
//
//    // 构建图7.6所示的柱体和球体
//	UINT objCBIndex = 2;
//	for(int i = 0; i < 5; ++i)
//	{
//		auto leftCylRitem = std::make_unique<RenderItem>();
//		auto rightCylRitem = std::make_unique<RenderItem>();
//		auto leftSphereRitem = std::make_unique<RenderItem>();
//		auto rightSphereRitem = std::make_unique<RenderItem>();
//
//		XMMATRIX leftCylWorld = XMMatrixTranslation(-5.0f, 1.5f, -10.0f + i*5.0f);
//		XMMATRIX rightCylWorld = XMMatrixTranslation(+5.0f, 1.5f, -10.0f + i*5.0f);
//
//		XMMATRIX leftSphereWorld = XMMatrixTranslation(-5.0f, 3.5f, -10.0f + i*5.0f);
//		XMMATRIX rightSphereWorld = XMMatrixTranslation(+5.0f, 3.5f, -10.0f + i*5.0f);
//
//		XMStoreFloat4x4(&leftCylRitem->World, rightCylWorld);
//		leftCylRitem->ObjCBIndex = objCBIndex++;
//		leftCylRitem->Geo = mGeometries["shapeGeo"].get();
//		leftCylRitem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
//		leftCylRitem->IndexCount = leftCylRitem->Geo->DrawArgs["cylinder"].IndexCount;
//		leftCylRitem->StartIndexLocation = leftCylRitem->Geo->DrawArgs["cylinder"].StartIndexLocation;
//		leftCylRitem->BaseVertexLocation = leftCylRitem->Geo->DrawArgs["cylinder"].BaseVertexLocation;
//
//		XMStoreFloat4x4(&rightCylRitem->World, leftCylWorld);
//		rightCylRitem->ObjCBIndex = objCBIndex++;
//		rightCylRitem->Geo = mGeometries["shapeGeo"].get();
//		rightCylRitem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
//		rightCylRitem->IndexCount = rightCylRitem->Geo->DrawArgs["cylinder"].IndexCount;
//		rightCylRitem->StartIndexLocation = rightCylRitem->Geo->DrawArgs["cylinder"].StartIndexLocation;
//		rightCylRitem->BaseVertexLocation = rightCylRitem->Geo->DrawArgs["cylinder"].BaseVertexLocation;
//
//		XMStoreFloat4x4(&leftSphereRitem->World, leftSphereWorld);
//		leftSphereRitem->ObjCBIndex = objCBIndex++;
//		leftSphereRitem->Geo = mGeometries["shapeGeo"].get();
//		leftSphereRitem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
//		leftSphereRitem->IndexCount = leftSphereRitem->Geo->DrawArgs["sphere"].IndexCount;
//		leftSphereRitem->StartIndexLocation = leftSphereRitem->Geo->DrawArgs["sphere"].StartIndexLocation;
//		leftSphereRitem->BaseVertexLocation = leftSphereRitem->Geo->DrawArgs["sphere"].BaseVertexLocation;
//
//		XMStoreFloat4x4(&rightSphereRitem->World, rightSphereWorld);
//		rightSphereRitem->ObjCBIndex = objCBIndex++;
//		rightSphereRitem->Geo = mGeometries["shapeGeo"].get();
//		rightSphereRitem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
//		rightSphereRitem->IndexCount = rightSphereRitem->Geo->DrawArgs["sphere"].IndexCount;
//		rightSphereRitem->StartIndexLocation = rightSphereRitem->Geo->DrawArgs["sphere"].StartIndexLocation;
//		rightSphereRitem->BaseVertexLocation = rightSphereRitem->Geo->DrawArgs["sphere"].BaseVertexLocation;
//
//		mAllRitems.push_back(std::move(leftCylRitem));
//		mAllRitems.push_back(std::move(rightCylRitem));
//		mAllRitems.push_back(std::move(leftSphereRitem));
//		mAllRitems.push_back(std::move(rightSphereRitem));
//	}
//
//	// All the render items are opaque.
//    // 此演示程序中的所有渲染项都是非透明的
//	for(auto& e : mAllRitems)
//		mOpaqueRitems.push_back(e.get());
//}
//
///*
//    绘制场景
//    最后一步就是绘制渲染项了，其中最复杂的部分莫过于为了绘制物体而根据偏移量找到它在堆中所对应的CBV。
//    先来留心观察一下，渲染项是如何来存储一个索引，并使之指向与它有关的常量缓冲区。
//
//    在执行主要绘制函数Draw的过程中调用DrawRenderItems方法：
//*/
//void ShapesApp::DrawRenderItems(ID3D12GraphicsCommandList* cmdList, const std::vector<RenderItem*>& ritems)
//{
//    UINT objCBByteSize = d3dUtil::CalcConstantBufferByteSize(sizeof(ObjectConstants));
// 
//	auto objectCB = mCurrFrameResource->ObjectCB->Resource();
//
//    // For each render item...
//    // 对于每个渲染项来说...
//    for(size_t i = 0; i < ritems.size(); ++i)
//    {
//        auto ri = ritems[i];
//
//        cmdList->IASetVertexBuffers(0, 1, &ri->Geo->VertexBufferView());
//        cmdList->IASetIndexBuffer(&ri->Geo->IndexBufferView());
//        cmdList->IASetPrimitiveTopology(ri->PrimitiveType);
//
//        // Offset to the CBV in the descriptor heap for this object and for this frame resource.
//        // 为了绘制当前的帧资源和当前物体，偏移到描述符堆中对应的CBV处
//        UINT cbvIndex = mCurrFrameResourceIndex*(UINT)mOpaqueRitems.size() + ri->ObjCBIndex;
//        auto cbvHandle = CD3DX12_GPU_DESCRIPTOR_HANDLE(mCbvHeap->GetGPUDescriptorHandleForHeapStart());
//        cbvHandle.Offset(cbvIndex, mCbvSrvUavDescriptorSize);
//
//        cmdList->SetGraphicsRootDescriptorTable(0, cbvHandle);
//
//        cmdList->DrawIndexedInstanced(ri->IndexCount, 1, ri->StartIndexLocation, ri->BaseVertexLocation, 0);
//    }
//}

////***************************************************************************************
//// LandAndWavesApp.cpp by Frank Luna (C) 2015 All Rights Reserved.
////
//// Hold down '1' key to view scene in wireframe mode.
////***************************************************************************************
//
//#include "../../Common/d3dApp.h"
//#include "../../Common/MathHelper.h"
//#include "../../Common/UploadBuffer.h"
//#include "../../Common/GeometryGenerator.h"
//#include "FrameResource.h"
//#include "Waves.h"
//
//using Microsoft::WRL::ComPtr;
//using namespace DirectX;
//using namespace DirectX::PackedVector;
//
//const int gNumFrameResources = 3;
//
//// Lightweight structure stores parameters to draw a shape.  This will
//// vary from app-to-app.
//struct RenderItem
//{
//	RenderItem() = default;
//
//	// World matrix of the shape that describes the object's local space
//	// relative to the world space, which defines the position, orientation,
//	// and scale of the object in the world.
//	XMFLOAT4X4 World = MathHelper::Identity4x4();
//
//	// Dirty flag indicating the object data has changed and we need to update the constant buffer.
//	// Because we have an object cbuffer for each FrameResource, we have to apply the
//	// update to each FrameResource.  Thus, when we modify obect data we should set 
//	// NumFramesDirty = gNumFrameResources so that each frame resource gets the update.
//	int NumFramesDirty = gNumFrameResources;
//
//	// Index into GPU constant buffer corresponding to the ObjectCB for this render item.
//	UINT ObjCBIndex = -1;
//
//	MeshGeometry* Geo = nullptr;
//
//	// Primitive topology.
//	D3D12_PRIMITIVE_TOPOLOGY PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
//
//	// DrawIndexedInstanced parameters.
//	UINT IndexCount = 0;
//	UINT StartIndexLocation = 0;
//	int BaseVertexLocation = 0;
//};
//
//enum class RenderLayer : int
//{
//	Opaque = 0,
//	Count
//};
//
//class LandAndWavesApp : public D3DApp
//{
//public:
//    LandAndWavesApp(HINSTANCE hInstance);
//    LandAndWavesApp(const LandAndWavesApp& rhs) = delete;
//    LandAndWavesApp& operator=(const LandAndWavesApp& rhs) = delete;
//    ~LandAndWavesApp();
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
//	void OnKeyboardInput(const GameTimer& gt);
//	void UpdateCamera(const GameTimer& gt);
//	void UpdateObjectCBs(const GameTimer& gt);
//	void UpdateMainPassCB(const GameTimer& gt);
//	void UpdateWaves(const GameTimer& gt);
//
//    void BuildRootSignature();
//    void BuildShadersAndInputLayout();
//    void BuildLandGeometry();
//    void BuildWavesGeometryBuffers();
//    void BuildPSOs();
//    void BuildFrameResources();
//    void BuildRenderItems();
//	void DrawRenderItems(ID3D12GraphicsCommandList* cmdList, const std::vector<RenderItem*>& ritems);
//
//    float GetHillsHeight(float x, float z)const;
//    XMFLOAT3 GetHillsNormal(float x, float z)const;
//
//private:
//
//    std::vector<std::unique_ptr<FrameResource>> mFrameResources;
//    FrameResource* mCurrFrameResource = nullptr;
//    int mCurrFrameResourceIndex = 0;
//
//    UINT mCbvSrvDescriptorSize = 0;
//
//    ComPtr<ID3D12RootSignature> mRootSignature = nullptr;
//
//	std::unordered_map<std::string, std::unique_ptr<MeshGeometry>> mGeometries;
//	std::unordered_map<std::string, ComPtr<ID3DBlob>> mShaders;
//	std::unordered_map<std::string, ComPtr<ID3D12PipelineState>> mPSOs;
//
//	std::vector<D3D12_INPUT_ELEMENT_DESC> mInputLayout;
//
//	RenderItem* mWavesRitem = nullptr;
//
//	// List of all the render items.
//	std::vector<std::unique_ptr<RenderItem>> mAllRitems;
//
//	// Render items divided by PSO.
//	std::vector<RenderItem*> mRitemLayer[(int)RenderLayer::Count];
//
//	std::unique_ptr<Waves> mWaves;
//
//    PassConstants mMainPassCB;
//
//    bool mIsWireframe = false;
//
//	XMFLOAT3 mEyePos = { 0.0f, 0.0f, 0.0f };
//	XMFLOAT4X4 mView = MathHelper::Identity4x4();
//	XMFLOAT4X4 mProj = MathHelper::Identity4x4();
//
//    float mTheta = 1.5f*XM_PI;
//    float mPhi = XM_PIDIV2 - 0.1f;
//    float mRadius = 50.0f;
//
//	float mSunTheta = 1.25f*XM_PI;
//	float mSunPhi = XM_PIDIV4;
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
//        LandAndWavesApp theApp(hInstance);
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
//LandAndWavesApp::LandAndWavesApp(HINSTANCE hInstance)
//    : D3DApp(hInstance)
//{
//}
//
//LandAndWavesApp::~LandAndWavesApp()
//{
//    if(md3dDevice != nullptr)
//        FlushCommandQueue();
//}
//
//bool LandAndWavesApp::Initialize()
//{
//    if(!D3DApp::Initialize())
//        return false;
//
//    // Reset the command list to prep for initialization commands.
//    ThrowIfFailed(mCommandList->Reset(mDirectCmdListAlloc.Get(), nullptr));
//
//	mWaves = std::make_unique<Waves>(128, 128, 1.0f, 0.03f, 4.0f, 0.2f);
//
//    BuildRootSignature();
//    BuildShadersAndInputLayout();
//	BuildLandGeometry();
//    BuildWavesGeometryBuffers();
//    BuildRenderItems();
//	BuildRenderItems();
//    BuildFrameResources();
//	BuildPSOs();
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
//void LandAndWavesApp::OnResize()
//{
//    D3DApp::OnResize();
//
//    // The window resized, so update the aspect ratio and recompute the projection matrix.
//    XMMATRIX P = XMMatrixPerspectiveFovLH(0.25f*MathHelper::Pi, AspectRatio(), 1.0f, 1000.0f);
//    XMStoreFloat4x4(&mProj, P);
//}
//
//void LandAndWavesApp::Update(const GameTimer& gt)
//{
//	OnKeyboardInput(gt);
//	UpdateCamera(gt);
//
//	// Cycle through the circular frame resource array.
//	mCurrFrameResourceIndex = (mCurrFrameResourceIndex + 1) % gNumFrameResources;
//	mCurrFrameResource = mFrameResources[mCurrFrameResourceIndex].get();
//
//	// Has the GPU finished processing the commands of the current frame resource?
//	// If not, wait until the GPU has completed commands up to this fence point.
//	if(mCurrFrameResource->Fence != 0 && mFence->GetCompletedValue() < mCurrFrameResource->Fence)
//	{
//		HANDLE eventHandle = CreateEventEx(nullptr, false, false, EVENT_ALL_ACCESS);
//		ThrowIfFailed(mFence->SetEventOnCompletion(mCurrFrameResource->Fence, eventHandle));
//		WaitForSingleObject(eventHandle, INFINITE);
//		CloseHandle(eventHandle);
//	}
//
//	UpdateObjectCBs(gt);
//	UpdateMainPassCB(gt);
//	UpdateWaves(gt);
//}
//
//void LandAndWavesApp::Draw(const GameTimer& gt)
//{
//	auto cmdListAlloc = mCurrFrameResource->CmdListAlloc;
//
//	// Reuse the memory associated with command recording.
//	// We can only reset when the associated command lists have finished execution on the GPU.
//	ThrowIfFailed(cmdListAlloc->Reset());
//
//	// A command list can be reset after it has been added to the command queue via ExecuteCommandList.
//	// Reusing the command list reuses memory.
//    if(mIsWireframe)
//    {
//        ThrowIfFailed(mCommandList->Reset(cmdListAlloc.Get(), mPSOs["opaque_wireframe"].Get()));
//    }
//    else
//    {
//        ThrowIfFailed(mCommandList->Reset(cmdListAlloc.Get(), mPSOs["opaque"].Get()));
//    }
//
//	mCommandList->RSSetViewports(1, &mScreenViewport);
//	mCommandList->RSSetScissorRects(1, &mScissorRect);
//
//	// Indicate a state transition on the resource usage.
//	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(),
//		D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));
//
//	// Clear the back buffer and depth buffer.
//	mCommandList->ClearRenderTargetView(CurrentBackBufferView(), Colors::LightSteelBlue, 0, nullptr);
//	mCommandList->ClearDepthStencilView(DepthStencilView(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);
//
//	// Specify the buffers we are going to render to.
//	mCommandList->OMSetRenderTargets(1, &CurrentBackBufferView(), true, &DepthStencilView());
//
//	mCommandList->SetGraphicsRootSignature(mRootSignature.Get());
//
//    // Bind per-pass constant buffer.  We only need to do this once per-pass.
//    // 绑定渲染过程中所用的常量缓冲区。在每个渲染过程中，这段代码只需执行一次
//	auto passCB = mCurrFrameResource->PassCB->Resource();
//	mCommandList->SetGraphicsRootConstantBufferView(1, passCB->GetGPUVirtualAddress());
//
//	DrawRenderItems(mCommandList.Get(), mRitemLayer[(int)RenderLayer::Opaque]);
//
//	// Indicate a state transition on the resource usage.
//	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(),
//		D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));
//
//	// Done recording commands.
//	ThrowIfFailed(mCommandList->Close());
//
//	// Add the command list to the queue for execution.
//	ID3D12CommandList* cmdsLists[] = { mCommandList.Get() };
//	mCommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);
//
//	// Swap the back and front buffers
//	ThrowIfFailed(mSwapChain->Present(0, 0));
//	mCurrBackBuffer = (mCurrBackBuffer + 1) % SwapChainBufferCount;
//
//	// Advance the fence value to mark commands up to this fence point.
//	mCurrFrameResource->Fence = ++mCurrentFence;
//
//	// Add an instruction to the command queue to set a new fence point. 
//    // Because we are on the GPU timeline, the new fence point won't be 
//    // set until the GPU finishes processing all the commands prior to this Signal().
//	mCommandQueue->Signal(mFence.Get(), mCurrentFence);
//}
//
//void LandAndWavesApp::OnMouseDown(WPARAM btnState, int x, int y)
//{
//    mLastMousePos.x = x;
//    mLastMousePos.y = y;
//
//    SetCapture(mhMainWnd);
//}
//
//void LandAndWavesApp::OnMouseUp(WPARAM btnState, int x, int y)
//{
//    ReleaseCapture();
//}
//
//void LandAndWavesApp::OnMouseMove(WPARAM btnState, int x, int y)
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
//        float dx = 0.2f*static_cast<float>(x - mLastMousePos.x);
//        float dy = 0.2f*static_cast<float>(y - mLastMousePos.y);
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
//void LandAndWavesApp::OnKeyboardInput(const GameTimer& gt)
//{
//    if(GetAsyncKeyState('1') & 0x8000)
//        mIsWireframe = true;
//    else
//        mIsWireframe = false;
//}
//
//void LandAndWavesApp::UpdateCamera(const GameTimer& gt)
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
//void LandAndWavesApp::UpdateObjectCBs(const GameTimer& gt)
//{
//	auto currObjectCB = mCurrFrameResource->ObjectCB.get();
//	for(auto& e : mAllRitems)
//	{
//		// Only update the cbuffer data if the constants have changed.  
//		// This needs to be tracked per frame resource.
//		if(e->NumFramesDirty > 0)
//		{
//			XMMATRIX world = XMLoadFloat4x4(&e->World);
//
//			ObjectConstants objConstants;
//			XMStoreFloat4x4(&objConstants.World, XMMatrixTranspose(world));
//
//			currObjectCB->CopyData(e->ObjCBIndex, objConstants);
//
//			// Next FrameResource need to be updated too.
//			e->NumFramesDirty--;
//		}
//	}
//}
//
//void LandAndWavesApp::UpdateMainPassCB(const GameTimer& gt)
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
//void LandAndWavesApp::UpdateWaves(const GameTimer& gt)
//{
//	// Every quarter second, generate a random wave.
//    // 每隔1/4秒就要生成一个随机波浪
//	static float t_base = 0.0f;
//	if((mTimer.TotalTime() - t_base) >= 0.25f)
//	{
//		t_base += 0.25f;
//
//		int i = MathHelper::Rand(4, mWaves->RowCount() - 5);
//		int j = MathHelper::Rand(4, mWaves->ColumnCount() - 5);
//
//		float r = MathHelper::RandF(0.2f, 0.5f);
//
//		mWaves->Disturb(i, j, r);
//	}
//
//	// Update the wave simulation.
//    // 更新模拟的波浪
//	mWaves->Update(gt.DeltaTime());
//
//	// Update the wave vertex buffer with the new solution.
//    // 用波浪方程求出的新数据来更新波浪顶点缓冲区
//	auto currWavesVB = mCurrFrameResource->WavesVB.get();
//	for(int i = 0; i < mWaves->VertexCount(); ++i)
//	{
//		Vertex v;
//
//		v.Pos = mWaves->Position(i);
//        v.Color = XMFLOAT4(DirectX::Colors::Blue);
//
//		currWavesVB->CopyData(i, v);
//	}
//
//	// Set the dynamic VB of the wave renderitem to the current frame VB.
//    // 将波浪渲染项的动态顶点缓冲区设置到当前帧的顶点缓冲区
//	mWavesRitem->Geo->VertexBufferGPU = currWavesVB->Resource();
//
//    /*
//        我们保存了一份波浪渲染项的引用（mWavesRitem），从而可以动态地调整其顶点缓冲区。
//        由于渲染项的顶点缓冲区是个动态的缓冲区，并且每一帧都在发生改变，因此这样做很有必要。
//
//        在使用动态缓冲区时会不可避免的产生一些开销，因为必须将新数据从CPU端内存回传至GPU端显存。
//        这样说来，如果静态缓冲区能实现相同的工作，那么应当比动态缓冲区更受青睐。
//        Direct3D在最近的版本中已经引入新的特性，以减少对动态缓冲区的需求。
//        例如：
//        1．简单的动画可以在顶点着色器中实现。
//        2．可以使用渲染到纹理（render to texture），或者计算着色器（compute shader）
//            与顶点纹理拾取（vertex texture fetch）等技术来实现上述波浪模拟，而且全程皆是在GPU中进行的。
//        3．几何着色器为GPU创建或销毁图元提供了支持，在几何着色器出现之前，一般用CPU来处理相关任务。
//        4．在曲面细分阶段中可以通过GPU来对几何体进行镶嵌化处理，在硬件曲面细分出现之前，通常用CPU来处理相关任务。
//    */
//}
//
//void LandAndWavesApp::BuildRootSignature()
//{
//    /*
//        根常量缓冲区视图此“Land and Waves”演示程序较之前“Shape”例程的另一个区别是：
//        我们在前者中使用了根描述符，因此就可以摆脱描述符堆而直接绑定CBV了。为此，程序还要做如下改动：
//        1．根签名需要变为取两个根CBV，而不再是两个描述符表。
//        2．不采用CBV堆，更无需向其填充描述符。
//        3．涉及一种用于绑定根描述符的新语法。
//        新的根签名定义如下：
//    */
//
//    // Root parameter can be a table, root descriptor or root constants.
//    // 根参数可以是描述符表，根描述符或根常量
//    CD3DX12_ROOT_PARAMETER slotRootParameter[2];
//
//    // Create root CBV.
//    // 创建根CBV。
//    slotRootParameter[0].InitAsConstantBufferView(0);   // 物体的CBV
//    slotRootParameter[1].InitAsConstantBufferView(1);   // 渲染过程CBV
//
//    /*
//        由此可以看出，我们使用辅助方法InitAsConstantBufferView来创建根CBV，并通过其参数来指定此根参数将要绑定的着色器寄存器（在上面的示例中，指定的着色器常量缓冲区寄存器分别为“b0”和“b1”）。
//        现在，让我们利用下列方法，以传递参数的方式将CBV与某个根描述符相绑定：
//        void ID3D12GraphicsCommandList::SetGraphicsRootConstantBufferView(
//          UINT RootParameterIndex,
//          D3D12_GPU_VIRTUAL_ADDRESS BufferLocation);
//    */
//
//    // A root signature is an array of root parameters.
//    // 根签名即是一系列根参数的组合
//	CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(2, slotRootParameter, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
//
//    // create a root signature with a single slot which points to a descriptor range consisting of a single constant buffer
//    ComPtr<ID3DBlob> serializedRootSig = nullptr;
//    ComPtr<ID3DBlob> errorBlob = nullptr;
//    HRESULT hr = D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1,
//        serializedRootSig.GetAddressOf(), errorBlob.GetAddressOf());
//
//    if(errorBlob != nullptr)
//    {
//        ::OutputDebugStringA((char*)errorBlob->GetBufferPointer());
//    }
//    ThrowIfFailed(hr);
//
//    ThrowIfFailed(md3dDevice->CreateRootSignature(
//		0,
//        serializedRootSig->GetBufferPointer(),
//        serializedRootSig->GetBufferSize(),
//        IID_PPV_ARGS(mRootSignature.GetAddressOf())));
//}
//
//void LandAndWavesApp::BuildShadersAndInputLayout()
//{
//	mShaders["standardVS"] = d3dUtil::CompileShader(L"E:\\DX12Book\\DX12LearnProject\\DX12Learn\\LearnDemo\\LandAndWaves\\Shaders\\color.hlsl", nullptr, "VS", "vs_5_0");
//	mShaders["opaquePS"] = d3dUtil::CompileShader(L"E:\\DX12Book\\DX12LearnProject\\DX12Learn\\LearnDemo\\LandAndWaves\\Shaders\\color.hlsl", nullptr, "PS", "ps_5_0");
//
//    mInputLayout =
//    {
//        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
//        { "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
//    };
//}
//
///*
//    应用计算高度的函数
//    待栅格创建完成后，我们就能从MeshData栅格中获取所需的顶点元素，
//    根据顶点的高度（[插图]坐标）将平坦的栅格变为用于表现山峰起伏的曲面，并为它生成对应的颜色。
//*/
//void LandAndWavesApp::BuildLandGeometry()
//{
//	GeometryGenerator geoGen;
//	GeometryGenerator::MeshData grid = geoGen.CreateGrid(160.0f, 160.0f, 50, 50);
//
//	//
//	// Extract the vertex elements we are interested and apply the height function to
//	// each vertex.  In addition, color the vertices based on their height so we have
//	// sandy looking beaches, grassy low hills, and snow mountain peaks.
//    // 获取我们所需要的顶点元素，并利用高度函数计算每个顶点的高度值
//    // 另外，顶点的颜色要基于它们的高度而定
//    // 所以，图像中才会有看起来如沙质的沙滩、山腰处的植被以及山峰处的积雪
//	//
//
//	std::vector<Vertex> vertices(grid.Vertices.size());
//	for(size_t i = 0; i < grid.Vertices.size(); ++i)
//	{
//		auto& p = grid.Vertices[i].Position;
//		vertices[i].Pos = p;
//		vertices[i].Pos.y = GetHillsHeight(p.x, p.z);
//
//        // Color the vertex based on its height.
//        // 基于顶点高度为它上色
//        if(vertices[i].Pos.y < -10.0f)
//        {
//            // Sandy beach color.
//            // 沙滩的颜色
//            vertices[i].Color = XMFLOAT4(1.0f, 0.96f, 0.62f, 1.0f);
//        }
//        else if(vertices[i].Pos.y < 5.0f)
//        {
//            // Light yellow-green.
//            // 浅黄绿色
//            vertices[i].Color = XMFLOAT4(0.48f, 0.77f, 0.46f, 1.0f);
//        }
//        else if(vertices[i].Pos.y < 12.0f)
//        {
//            // Dark yellow-green.
//            // 深黄绿色
//            vertices[i].Color = XMFLOAT4(0.1f, 0.48f, 0.19f, 1.0f);
//        }
//        else if(vertices[i].Pos.y < 20.0f)
//        {
//            // Dark brown.
//            // 深棕色
//            vertices[i].Color = XMFLOAT4(0.45f, 0.39f, 0.34f, 1.0f);
//        }
//        else
//        {
//            // White snow.
//            // 白雪皑皑
//            vertices[i].Color = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
//        }
//	}
//    
//	const UINT vbByteSize = (UINT)vertices.size() * sizeof(Vertex);
//
//	std::vector<std::uint16_t> indices = grid.GetIndices16();
//	const UINT ibByteSize = (UINT)indices.size() * sizeof(std::uint16_t);
//
//	auto geo = std::make_unique<MeshGeometry>();
//	geo->Name = "landGeo";
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
//	SubmeshGeometry submesh;
//	submesh.IndexCount = (UINT)indices.size();
//	submesh.StartIndexLocation = 0;
//	submesh.BaseVertexLocation = 0;
//
//	geo->DrawArgs["grid"] = submesh;
//
//	mGeometries["landGeo"] = std::move(geo);
//}
//
//void LandAndWavesApp::BuildWavesGeometryBuffers()
//{
//	std::vector<std::uint16_t> indices(3 * mWaves->TriangleCount()); // 3 indices per face
//	assert(mWaves->VertexCount() < 0x0000ffff);
//
//	// Iterate over each quad.
//	int m = mWaves->RowCount();
//	int n = mWaves->ColumnCount();
//	int k = 0;
//	for(int i = 0; i < m - 1; ++i)
//	{
//		for(int j = 0; j < n - 1; ++j)
//		{
//			indices[k] = i*n + j;
//			indices[k + 1] = i*n + j + 1;
//			indices[k + 2] = (i + 1)*n + j;
//
//			indices[k + 3] = (i + 1)*n + j;
//			indices[k + 4] = i*n + j + 1;
//			indices[k + 5] = (i + 1)*n + j + 1;
//
//			k += 6; // next quad
//		}
//	}
//
//	UINT vbByteSize = mWaves->VertexCount()*sizeof(Vertex);
//	UINT ibByteSize = (UINT)indices.size()*sizeof(std::uint16_t);
//
//	auto geo = std::make_unique<MeshGeometry>();
//	geo->Name = "waterGeo";
//
//	// Set dynamically.
//	geo->VertexBufferCPU = nullptr;
//	geo->VertexBufferGPU = nullptr;
//
//	ThrowIfFailed(D3DCreateBlob(ibByteSize, &geo->IndexBufferCPU));
//	CopyMemory(geo->IndexBufferCPU->GetBufferPointer(), indices.data(), ibByteSize);
//
//	geo->IndexBufferGPU = d3dUtil::CreateDefaultBuffer(md3dDevice.Get(),
//		mCommandList.Get(), indices.data(), ibByteSize, geo->IndexBufferUploader);
//
//	geo->VertexByteStride = sizeof(Vertex);
//	geo->VertexBufferByteSize = vbByteSize;
//	geo->IndexFormat = DXGI_FORMAT_R16_UINT;
//	geo->IndexBufferByteSize = ibByteSize;
//
//	SubmeshGeometry submesh;
//	submesh.IndexCount = (UINT)indices.size();
//	submesh.StartIndexLocation = 0;
//	submesh.BaseVertexLocation = 0;
//
//	geo->DrawArgs["grid"] = submesh;
//
//	mGeometries["waterGeo"] = std::move(geo);
//}
//
//void LandAndWavesApp::BuildPSOs()
//{
//	D3D12_GRAPHICS_PIPELINE_STATE_DESC opaquePsoDesc;
//
//	//
//	// PSO for opaque objects.
//	//
//	ZeroMemory(&opaquePsoDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
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
//	opaquePsoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
//	opaquePsoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
//	opaquePsoDesc.SampleMask = UINT_MAX;
//	opaquePsoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
//	opaquePsoDesc.NumRenderTargets = 1;
//	opaquePsoDesc.RTVFormats[0] = mBackBufferFormat;
//	opaquePsoDesc.SampleDesc.Count = m4xMsaaState ? 4 : 1;
//	opaquePsoDesc.SampleDesc.Quality = m4xMsaaState ? (m4xMsaaQuality - 1) : 0;
//	opaquePsoDesc.DSVFormat = mDepthStencilFormat;
//	ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&opaquePsoDesc, IID_PPV_ARGS(&mPSOs["opaque"])));
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
//void LandAndWavesApp::BuildFrameResources()
//{
//    for(int i = 0; i < gNumFrameResources; ++i)
//    {
//        mFrameResources.push_back(std::make_unique<FrameResource>(md3dDevice.Get(),
//            1, (UINT)mAllRitems.size(), mWaves->VertexCount()));
//    }
//}
//
//void LandAndWavesApp::BuildRenderItems()
//{
//	auto wavesRitem = std::make_unique<RenderItem>();
//	wavesRitem->World = MathHelper::Identity4x4();
//	wavesRitem->ObjCBIndex = 0;
//	wavesRitem->Geo = mGeometries["waterGeo"].get();
//	wavesRitem->PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
//	wavesRitem->IndexCount = wavesRitem->Geo->DrawArgs["grid"].IndexCount;
//	wavesRitem->StartIndexLocation = wavesRitem->Geo->DrawArgs["grid"].StartIndexLocation;
//	wavesRitem->BaseVertexLocation = wavesRitem->Geo->DrawArgs["grid"].BaseVertexLocation;
//
//	mWavesRitem = wavesRitem.get();
//
//	mRitemLayer[(int)RenderLayer::Opaque].push_back(wavesRitem.get());
//
//	auto gridRitem = std::make_unique<RenderItem>();
//	gridRitem->World = MathHelper::Identity4x4();
//	gridRitem->ObjCBIndex = 1;
//	gridRitem->Geo = mGeometries["landGeo"].get();
//	gridRitem->PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
//	gridRitem->IndexCount = gridRitem->Geo->DrawArgs["grid"].IndexCount;
//	gridRitem->StartIndexLocation = gridRitem->Geo->DrawArgs["grid"].StartIndexLocation;
//	gridRitem->BaseVertexLocation = gridRitem->Geo->DrawArgs["grid"].BaseVertexLocation;
//
//	mRitemLayer[(int)RenderLayer::Opaque].push_back(gridRitem.get());
//
//	mAllRitems.push_back(std::move(wavesRitem));
//	mAllRitems.push_back(std::move(gridRitem));
//}
//
//void LandAndWavesApp::DrawRenderItems(ID3D12GraphicsCommandList* cmdList, const std::vector<RenderItem*>& ritems)
//{
//	UINT objCBByteSize = d3dUtil::CalcConstantBufferByteSize(sizeof(ObjectConstants));
//
//	auto objectCB = mCurrFrameResource->ObjectCB->Resource();
//
//	// For each render item...
//    // 对于每个渲染项来说...
//	for(size_t i = 0; i < ritems.size(); ++i)
//	{
//		auto ri = ritems[i];
//
//		cmdList->IASetVertexBuffers(0, 1, &ri->Geo->VertexBufferView());
//		cmdList->IASetIndexBuffer(&ri->Geo->IndexBufferView());
//		cmdList->IASetPrimitiveTopology(ri->PrimitiveType);
//
//        D3D12_GPU_VIRTUAL_ADDRESS objCBAddress = objectCB->GetGPUVirtualAddress();
//        objCBAddress += ri->ObjCBIndex*objCBByteSize;
//
//		cmdList->SetGraphicsRootConstantBufferView(0, objCBAddress);
//
//		cmdList->DrawIndexedInstanced(ri->IndexCount, 1, ri->StartIndexLocation, ri->BaseVertexLocation, 0);
//	}
//}
//
//float LandAndWavesApp::GetHillsHeight(float x, float z)const
//{
//    return 0.3f*(z*sinf(0.1f*x) + x*cosf(0.1f*z));
//}
//
//XMFLOAT3 LandAndWavesApp::GetHillsNormal(float x, float z)const
//{
//    // n = (-df/dx, 1, -df/dz)
//    XMFLOAT3 n(
//        -0.03f*z*cosf(0.1f*x) - 0.3f*cosf(0.1f*z),
//        1.0f,
//        -0.3f*sinf(0.1f*x) + 0.03f*x*sinf(0.1f*z));
//
//    XMVECTOR unitNormal = XMVector3Normalize(XMLoadFloat3(&n));
//    XMStoreFloat3(&n, unitNormal);
//
//    return n;
//}

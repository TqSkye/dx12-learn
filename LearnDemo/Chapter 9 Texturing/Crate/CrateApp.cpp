////***************************************************************************************
//// CrateApp.cpp by Frank Luna (C) 2015 All Rights Reserved.
////***************************************************************************************
//
///*
//    纹理:
//    纹理不同于缓冲区资源，因为缓冲区资源仅存储数据数组，而纹理却可以具有多个mipmap层级（后文有介绍），GPU会基于这个层级进行相应的特殊操作，
//    例如运用过滤器以及多重采样。支持这些特殊操作纹理的资源都被限定为一些特定的数据格式。而缓冲区资源就没有这项限制，它们可以存储任意类型的数据。
//    纹理所支持的数据格式由枚举类型DXGI_FORMAT来表示。这是其中的一些格式示例：
//    1．DXGI_FORMAT_R32G32B32_FLOAT：每个元素由3个32位浮点数分量构成。
//    2．DXGI_FORMAT_R16G16B16A16_UNORM：每个元素由4个16位分量组成，每个分量都将被映射到范围[0, 1]之间。
//    3．DXGI_FORMAT_R32G32_UINT：每个元素由2个32位无符号整数分量构成。
//    4．DXGI_FORMAT_R8G8B8A8_UNORM：每个元素由4个8位无符号分量构成，每个分量都将被映射到范围[0, 1]之间。
//    5．DXGI_FORMAT_R8G8B8A8_SNORM：每个元素由4个8位有符号分量构成，每个分量都将被映射到范围[-1, 1]之间。
//    6．DXGI_FORMAT_R8G8B8A8_SINT：每个元素由4个8位有符号整数分量构成，每个分量都将被映射到范围[-128, 127]之间。
//    7．DXGI_FORMAT_R8G8B8A8_UINT：每个元素由4个8位无符号整数分量构成，每个分量都将被映射到范围[0, 255]之间。
//    顾名思义，R、G、B、A这4个字母分别用于表示红色、绿色、蓝色以及alpha值。然而，正如前面所说，纹理不一定要存储颜色信息。
//    例如，格式[DXGI_FORMAT_R32G32B32_FLOAT]具有3个浮点数分量，因此它能够存储一个3D向量的浮点坐标（并不是颜色向量）。
//    还有一种无类型（typeless）格式，我们仅用它来预留一块内存，并要在稍后将纹理绑定到渲染流水线的时候指出如何重新解释其中的数据（有点像类型强制转换）。
//    例如，下列无类型格式就为每个元素预留了4个8位分量，但是并没有指定其具体的数据类型（如整数、浮点数、无符号整数等）：[DXGI_FORMAT_R8G8B8_TYPELESS]
//
//    纹理坐标:
//    Direct3D所采用的纹理坐标系，是由指向图像水平正方向的[u]轴与指向图像垂直正方向的[v]轴所组成的。
//    取值范围为[0<=u]的坐标[v<=1]标定的是一种称为纹素（texel）的纹理元素。观察图9.2可以发现，[v]轴的正方向指向图像的“正下”方。
//    此外，还可以看出纹理坐标采用的是归一化坐标区间[0, 1]，通过这种方式便可令Direct3D的工作摆脱具体纹理尺寸的干扰。
//    例如，无论纹理实际的大小是[256x256]像素、[512x1024]像素还是[2048x2048]像素，纹理坐标(0.5, 0.5)总是表示纹理正中间的纹素；坐标(0.25, 0.75)总是表示在纹理中，
//    位于水平方向总宽度1/4、垂直方向总高度3/4处的纹素。目前所讨论的纹理坐标都是在区间[0, 1]之内，在后面，我们会解释若超出了此范围会发生什么。
//    (0,0)           (1,0) +u
//    --|--------------->
//      |
//      |
//      |
//      |
//      v
//    (0,1)+v
//
//    纹理DDS格式:
//    DDS对于3D图形来说是一种理想的格式，因为它支持一些专用于3D图形的特殊格式以及纹理类型。从本质上来讲，它实为一种针对GPU而专门设计的图像格式。
//    例如，DDS纹理满足用于3D图形开发的以下特征：
//    1．mipmap。
//    2．GPU能自行解压的压缩格式。
//    3．纹理数组。
//    4．立方体图（cube map，也有译作立方体贴图）。
//    5．体纹理（volume texture，也有译作体积纹理、立体纹理等）。
//
//    DDS格式能够支援不同的像素格式。像素格式由枚举类型DXGI_FORMAT中的成员来表示，但是并非所有的格式都适用于DDS纹理。非压缩图像数据一般会采用下列格式。
//    1．DXGI_FORMAT_B8G8R8A8_UNORM或DXGI_FORMAT_B8G8R8X8_UNORM：适用于低动态范围（low-dynamic-range）图像。
//    2．DXGI_FORMAT_R16G16B16A16_FLOAT：适用于高动态范围（high-dynamic-range）图像。
//
//    随着虚拟场景中纹理数量的大幅增长，对GPU端显存的需求也在迅速增加（还记得吗，我们需要将所有的纹理都置于显存当中，以便在程序中快速地运用这些资源）。
//    为了缓解这些内存的需求压力，Direct3D支持下列几种压缩纹理格式（也称作块压缩，block compression）。
//    1．BC1（DXGI_FORMAT_BC1_UNORM）：如果我们需要将图片压缩为支持3个颜色通道和仅有1位（开/关）alpha分量的格式，则使用此格式。
//    2．BC2（DXGI_FORMAT_BC2_UNORM）：如果我们需要将图片压缩为支持3个颜色通道和仅有4位alpha分量的格式，则应用此格式。
//    3．BC3（DXGI_FORMAT_BC3_UNORM）：如果我们需要将图片压缩为支持3个颜色通道和8位alpha分量的格式，则采用此格式。
//    4．BC4（DXGI_FORMAT_BC4_UNORM）：如果我们需要将图片压缩为仅含有1个颜色通道的格式（如灰度图像），则运用此格式。
//    5．BC5（DXGI_FORMAT_BC5_UNORM）：如果我们需要将图片压缩为只支持2个颜色通道的格式，则使用此格式。
//    6．BC6（DXGI_FORMAT_BC6H_UF16）：如果我们需要将图片压缩为HDR（高动态范围）图像数据，则应用此格式。
//    7．BC7（DXGI_FORMAT_BC7_UNORM）：此格式用于对RGBA数据进行高质量的压缩。特别是，此格式可极大地减少因压缩法线图而造成的误差。
//*/
//
//#include "../../../Common/d3dApp.h"
//#include "../../../Common/MathHelper.h"
//#include "../../../Common/UploadBuffer.h"
//#include "../../../Common/GeometryGenerator.h"
//#include "CrateFrameResource.h"
//
//using Microsoft::WRL::ComPtr;
//using namespace DirectX;
//using namespace DirectX::PackedVector;
//
//#pragma comment(lib, "d3dcompiler.lib")
//#pragma comment(lib, "D3D12.lib")
//
//const int gNumFrameResources = 3;
//
//// Lightweight structure stores parameters to draw a shape.  This will
//// vary from app-to-app.
//struct RenderItem
//{
//	RenderItem() = default;
//
//    // World matrix of the shape that describes the object's local space
//    // relative to the world space, which defines the position, orientation,
//    // and scale of the object in the world.
//    XMFLOAT4X4 World = MathHelper::Identity4x4();
//
//	XMFLOAT4X4 TexTransform = MathHelper::Identity4x4();
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
//	Material* Mat = nullptr;
//	MeshGeometry* Geo = nullptr;
//
//    // Primitive topology.
//    D3D12_PRIMITIVE_TOPOLOGY PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
//
//    // DrawIndexedInstanced parameters.
//    UINT IndexCount = 0;
//    UINT StartIndexLocation = 0;
//    int BaseVertexLocation = 0;
//};
//
//class CrateApp : public D3DApp
//{
//public:
//    CrateApp(HINSTANCE hInstance);
//    CrateApp(const CrateApp& rhs) = delete;
//    CrateApp& operator=(const CrateApp& rhs) = delete;
//    ~CrateApp();
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
//	void AnimateMaterials(const GameTimer& gt);
//	void UpdateObjectCBs(const GameTimer& gt);
//	void UpdateMaterialCBs(const GameTimer& gt);
//	void UpdateMainPassCB(const GameTimer& gt);
//
//	void LoadTextures();
//    void BuildRootSignature();
//	void BuildDescriptorHeaps();
//    void BuildShadersAndInputLayout();
//    void BuildShapeGeometry();
//    void BuildPSOs();
//    void BuildFrameResources();
//    void BuildMaterials();
//    void BuildRenderItems();
//    void DrawRenderItems(ID3D12GraphicsCommandList* cmdList, const std::vector<RenderItem*>& ritems);
//
//	std::array<const CD3DX12_STATIC_SAMPLER_DESC, 6> GetStaticSamplers();
//
//private:
//
//    std::vector<std::unique_ptr<CrateFrameResource>> mFrameResources;
//    CrateFrameResource* mCurrFrameResource = nullptr;
//    int mCurrFrameResourceIndex = 0;
//
//    UINT mCbvSrvDescriptorSize = 0;
//
//    ComPtr<ID3D12RootSignature> mRootSignature = nullptr;
//
//	ComPtr<ID3D12DescriptorHeap> mSrvDescriptorHeap = nullptr;
//
//	std::unordered_map<std::string, std::unique_ptr<MeshGeometry>> mGeometries;
//	std::unordered_map<std::string, std::unique_ptr<Material>> mMaterials;
//	std::unordered_map<std::string, std::unique_ptr<Texture>> mTextures;
//	std::unordered_map<std::string, ComPtr<ID3DBlob>> mShaders;
//
//    std::vector<D3D12_INPUT_ELEMENT_DESC> mInputLayout;
//
//    ComPtr<ID3D12PipelineState> mOpaquePSO = nullptr;
// 
//	// List of all the render items.
//	std::vector<std::unique_ptr<RenderItem>> mAllRitems;
//
//	// Render items divided by PSO.
//	std::vector<RenderItem*> mOpaqueRitems;
//
//    PassConstants mMainPassCB;
//
//	XMFLOAT3 mEyePos = { 0.0f, 0.0f, 0.0f };
//	XMFLOAT4X4 mView = MathHelper::Identity4x4();
//	XMFLOAT4X4 mProj = MathHelper::Identity4x4();
//
//	float mTheta = 1.3f*XM_PI;
//	float mPhi = 0.4f*XM_PI;
//	float mRadius = 2.5f;
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
//        CrateApp theApp(hInstance);
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
//CrateApp::CrateApp(HINSTANCE hInstance)
//    : D3DApp(hInstance)
//{
//}
//
//CrateApp::~CrateApp()
//{
//    if(md3dDevice != nullptr)
//        FlushCommandQueue();
//}
//
//bool CrateApp::Initialize()
//{
//    if(!D3DApp::Initialize())
//        return false;
//
//    // Reset the command list to prep for initialization commands.
//    ThrowIfFailed(mCommandList->Reset(mDirectCmdListAlloc.Get(), nullptr));
//
//    // Get the increment size of a descriptor in this heap type.  This is hardware specific, 
//	// so we have to query this information.
//    mCbvSrvDescriptorSize = md3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
//
// 
//	LoadTextures();
//    BuildRootSignature();
//	BuildDescriptorHeaps();
//    BuildShadersAndInputLayout();
//    BuildShapeGeometry();
//	BuildMaterials();
//    BuildRenderItems();
//    BuildFrameResources();
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
//void CrateApp::OnResize()
//{
//    D3DApp::OnResize();
//
//    // The window resized, so update the aspect ratio and recompute the projection matrix.
//    XMMATRIX P = XMMatrixPerspectiveFovLH(0.25f*MathHelper::Pi, AspectRatio(), 1.0f, 1000.0f);
//    XMStoreFloat4x4(&mProj, P);
//}
//
//void CrateApp::Update(const GameTimer& gt)
//{
//    OnKeyboardInput(gt);
//	UpdateCamera(gt);
//
//    // Cycle through the circular frame resource array.
//    mCurrFrameResourceIndex = (mCurrFrameResourceIndex + 1) % gNumFrameResources;
//    mCurrFrameResource = mFrameResources[mCurrFrameResourceIndex].get();
//
//    // Has the GPU finished processing the commands of the current frame resource?
//    // If not, wait until the GPU has completed commands up to this fence point.
//    if(mCurrFrameResource->Fence != 0 && mFence->GetCompletedValue() < mCurrFrameResource->Fence)
//    {
//        HANDLE eventHandle = CreateEventEx(nullptr, false, false, EVENT_ALL_ACCESS);
//        ThrowIfFailed(mFence->SetEventOnCompletion(mCurrFrameResource->Fence, eventHandle));
//        WaitForSingleObject(eventHandle, INFINITE);
//        CloseHandle(eventHandle);
//    }
//
//	AnimateMaterials(gt);
//	UpdateObjectCBs(gt);
//	UpdateMaterialCBs(gt);
//	UpdateMainPassCB(gt);
//}
//
//void CrateApp::Draw(const GameTimer& gt)
//{
//    auto cmdListAlloc = mCurrFrameResource->CmdListAlloc;
//
//    // Reuse the memory associated with command recording.
//    // We can only reset when the associated command lists have finished execution on the GPU.
//    ThrowIfFailed(cmdListAlloc->Reset());
//
//    // A command list can be reset after it has been added to the command queue via ExecuteCommandList.
//    // Reusing the command list reuses memory.
//    ThrowIfFailed(mCommandList->Reset(cmdListAlloc.Get(), mOpaquePSO.Get()));
//
//    mCommandList->RSSetViewports(1, &mScreenViewport);
//    mCommandList->RSSetScissorRects(1, &mScissorRect);
//
//    // Indicate a state transition on the resource usage.
//	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(),
//		D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));
//
//    // Clear the back buffer and depth buffer.
//    mCommandList->ClearRenderTargetView(CurrentBackBufferView(), Colors::LightSteelBlue, 0, nullptr);
//    mCommandList->ClearDepthStencilView(DepthStencilView(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);
//
//    // Specify the buffers we are going to render to.
//    mCommandList->OMSetRenderTargets(1, &CurrentBackBufferView(), true, &DepthStencilView());
//
//	ID3D12DescriptorHeap* descriptorHeaps[] = { mSrvDescriptorHeap.Get() };
//	mCommandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);
//
//	mCommandList->SetGraphicsRootSignature(mRootSignature.Get());
//
//	auto passCB = mCurrFrameResource->PassCB->Resource();
//	mCommandList->SetGraphicsRootConstantBufferView(2, passCB->GetGPUVirtualAddress());
//
//    DrawRenderItems(mCommandList.Get(), mOpaqueRitems);
//
//    // Indicate a state transition on the resource usage.
//	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(),
//		D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));
//
//    // Done recording commands.
//    ThrowIfFailed(mCommandList->Close());
//
//    // Add the command list to the queue for execution.
//    ID3D12CommandList* cmdsLists[] = { mCommandList.Get() };
//    mCommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);
//
//    // Swap the back and front buffers
//    ThrowIfFailed(mSwapChain->Present(0, 0));
//	mCurrBackBuffer = (mCurrBackBuffer + 1) % SwapChainBufferCount;
//
//    // Advance the fence value to mark commands up to this fence point.
//    mCurrFrameResource->Fence = ++mCurrentFence;
//
//    // Add an instruction to the command queue to set a new fence point. 
//    // Because we are on the GPU timeline, the new fence point won't be 
//    // set until the GPU finishes processing all the commands prior to this Signal().
//    mCommandQueue->Signal(mFence.Get(), mCurrentFence);
//}
//
//void CrateApp::OnMouseDown(WPARAM btnState, int x, int y)
//{
//    mLastMousePos.x = x;
//    mLastMousePos.y = y;
//
//    SetCapture(mhMainWnd);
//}
//
//void CrateApp::OnMouseUp(WPARAM btnState, int x, int y)
//{
//    ReleaseCapture();
//}
//
//void CrateApp::OnMouseMove(WPARAM btnState, int x, int y)
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
//void CrateApp::OnKeyboardInput(const GameTimer& gt)
//{
//}
// 
//void CrateApp::UpdateCamera(const GameTimer& gt)
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
//void CrateApp::AnimateMaterials(const GameTimer& gt)
//{
//	
//}
//
//void CrateApp::UpdateObjectCBs(const GameTimer& gt)
//{
//	auto currObjectCB = mCurrFrameResource->ObjectCB.get();
//	for(auto& e : mAllRitems)
//	{
//		// Only update the cbuffer data if the constants have changed.  
//		// This needs to be tracked per frame resource.
//		if(e->NumFramesDirty > 0)
//		{
//			XMMATRIX world = XMLoadFloat4x4(&e->World);
//			XMMATRIX texTransform = XMLoadFloat4x4(&e->TexTransform);
//
//			ObjectConstants objConstants;
//			XMStoreFloat4x4(&objConstants.World, XMMatrixTranspose(world));
//			XMStoreFloat4x4(&objConstants.TexTransform, XMMatrixTranspose(texTransform));
//
//			currObjectCB->CopyData(e->ObjCBIndex, objConstants);
//
//			// Next FrameResource need to be updated too.
//			e->NumFramesDirty--;
//		}
//	}
//}
//
//void CrateApp::UpdateMaterialCBs(const GameTimer& gt)
//{
//	auto currMaterialCB = mCurrFrameResource->MaterialCB.get();
//	for(auto& e : mMaterials)
//	{
//		// Only update the cbuffer data if the constants have changed.  If the cbuffer
//		// data changes, it needs to be updated for each FrameResource.
//		Material* mat = e.second.get();
//		if(mat->NumFramesDirty > 0)
//		{
//			XMMATRIX matTransform = XMLoadFloat4x4(&mat->MatTransform);
//
//			MaterialConstants matConstants;
//			matConstants.DiffuseAlbedo = mat->DiffuseAlbedo;
//			matConstants.FresnelR0 = mat->FresnelR0;
//			matConstants.Roughness = mat->Roughness;
//			XMStoreFloat4x4(&matConstants.MatTransform, XMMatrixTranspose(matTransform));
//
//			currMaterialCB->CopyData(mat->MatCBIndex, matConstants);
//
//			// Next FrameResource need to be updated too.
//			mat->NumFramesDirty--;
//		}
//	}
//}
//
//void CrateApp::UpdateMainPassCB(const GameTimer& gt)
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
//	mMainPassCB.AmbientLight = { 0.25f, 0.25f, 0.35f, 1.0f };
//	mMainPassCB.Lights[0].Direction = { 0.57735f, -0.57735f, 0.57735f };
//	mMainPassCB.Lights[0].Strength = { 0.6f, 0.6f, 0.6f };
//	mMainPassCB.Lights[1].Direction = { -0.57735f, -0.57735f, 0.57735f };
//	mMainPassCB.Lights[1].Strength = { 0.3f, 0.3f, 0.3f };
//	mMainPassCB.Lights[2].Direction = { 0.0f, -0.707f, -0.707f };
//	mMainPassCB.Lights[2].Strength = { 0.15f, 0.15f, 0.15f };
//
//	auto currPassCB = mCurrFrameResource->PassCB.get();
//	currPassCB->CopyData(0, mMainPassCB);
//}
//
//void CrateApp::LoadTextures()
//{
//	auto woodCrateTex = std::make_unique<Texture>();
//	woodCrateTex->Name = "woodCrateTex";
//	woodCrateTex->Filename = L"E:/DX12Book/DX12LearnProject/DX12Learn/Textures/WoodCrate01.dds";
//	ThrowIfFailed(DirectX::CreateDDSTextureFromFile12(md3dDevice.Get(),
//		mCommandList.Get(), woodCrateTex->Filename.c_str(),
//		woodCrateTex->Resource, woodCrateTex->UploadHeap));
// 
//	mTextures[woodCrateTex->Name] = std::move(woodCrateTex);
//}
//
//void CrateApp::BuildRootSignature()
//{
//	CD3DX12_DESCRIPTOR_RANGE texTable;
//	texTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);
//
//    // Root parameter can be a table, root descriptor or root constants.
//    CD3DX12_ROOT_PARAMETER slotRootParameter[4];
//
//	// Perfomance TIP: Order from most frequent to least frequent.
//	slotRootParameter[0].InitAsDescriptorTable(1, &texTable, D3D12_SHADER_VISIBILITY_PIXEL);
//    slotRootParameter[1].InitAsConstantBufferView(0);
//    slotRootParameter[2].InitAsConstantBufferView(1);
//    slotRootParameter[3].InitAsConstantBufferView(2);
//
//	auto staticSamplers = GetStaticSamplers();
//
//    // A root signature is an array of root parameters.
//	CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(4, slotRootParameter,
//		(UINT)staticSamplers.size(), staticSamplers.data(),
//		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
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
//void CrateApp::BuildDescriptorHeaps()
//{
//	//
//	// Create the SRV heap.
//	//
//	D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {};
//	srvHeapDesc.NumDescriptors = 1;
//	srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
//	srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
//	ThrowIfFailed(md3dDevice->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&mSrvDescriptorHeap)));
//
//	// Fill out the heap with actual descriptors.
//	// 获取指向描述符堆起始处的指针
//	CD3DX12_CPU_DESCRIPTOR_HANDLE hDescriptor(mSrvDescriptorHeap->GetCPUDescriptorHandleForHeapStart());
//
//	auto woodCrateTex = mTextures["woodCrateTex"]->Resource;
// 
//	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
//	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
//	srvDesc.Format = woodCrateTex->GetDesc().Format;
//	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
//	srvDesc.Texture2D.MostDetailedMip = 0;
//	srvDesc.Texture2D.MipLevels = woodCrateTex->GetDesc().MipLevels;
//	srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;
//
//	md3dDevice->CreateShaderResourceView(woodCrateTex.Get(), &srvDesc, hDescriptor);
//}
//
//void CrateApp::BuildShadersAndInputLayout()
//{
//	mShaders["standardVS"] = d3dUtil::CompileShader(L"E:\\DX12Book\\DX12LearnProject\\DX12Learn\\LearnDemo\\Chapter 9 Texturing\\Crate\\Shaders\\Default.hlsl", nullptr, "VS", "vs_5_0");
//	mShaders["opaquePS"] = d3dUtil::CompileShader(L"E:\\DX12Book\\DX12LearnProject\\DX12Learn\\LearnDemo\\Chapter 9 Texturing\\Crate\\Shaders\\Default.hlsl", nullptr, "PS", "ps_5_0");
//	
//    mInputLayout =
//    {
//        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
//        { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
//		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
//    };
//}
//
//void CrateApp::BuildShapeGeometry()
//{
//    GeometryGenerator geoGen;
//	GeometryGenerator::MeshData box = geoGen.CreateBox(1.0f, 1.0f, 1.0f, 3);
// 
//	SubmeshGeometry boxSubmesh;
//	boxSubmesh.IndexCount = (UINT)box.Indices32.size();
//	boxSubmesh.StartIndexLocation = 0;
//	boxSubmesh.BaseVertexLocation = 0;
//
// 
//	std::vector<Vertex> vertices(box.Vertices.size());
//
//	for(size_t i = 0; i < box.Vertices.size(); ++i)
//	{
//		vertices[i].Pos = box.Vertices[i].Position;
//		vertices[i].Normal = box.Vertices[i].Normal;
//		vertices[i].TexC = box.Vertices[i].TexC;
//	}
//
//	std::vector<std::uint16_t> indices = box.GetIndices16();
//
//    const UINT vbByteSize = (UINT)vertices.size() * sizeof(Vertex);
//    const UINT ibByteSize = (UINT)indices.size()  * sizeof(std::uint16_t);
//
//	auto geo = std::make_unique<MeshGeometry>();
//	geo->Name = "boxGeo";
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
//
//	mGeometries[geo->Name] = std::move(geo);
//}
//
//void CrateApp::BuildPSOs()
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
//	opaquePsoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
//	opaquePsoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
//	opaquePsoDesc.SampleMask = UINT_MAX;
//	opaquePsoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
//	opaquePsoDesc.NumRenderTargets = 1;
//	opaquePsoDesc.RTVFormats[0] = mBackBufferFormat;
//	opaquePsoDesc.SampleDesc.Count = m4xMsaaState ? 4 : 1;
//	opaquePsoDesc.SampleDesc.Quality = m4xMsaaState ? (m4xMsaaQuality - 1) : 0;
//	opaquePsoDesc.DSVFormat = mDepthStencilFormat;
//    ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&opaquePsoDesc, IID_PPV_ARGS(&mOpaquePSO)));
//}
//
//void CrateApp::BuildFrameResources()
//{
//    for(int i = 0; i < gNumFrameResources; ++i)
//    {
//        mFrameResources.push_back(std::make_unique<CrateFrameResource>(md3dDevice.Get(),
//            1, (UINT)mAllRitems.size(), (UINT)mMaterials.size()));
//    }
//}
//
//void CrateApp::BuildMaterials()
//{
//	auto woodCrate = std::make_unique<Material>();
//	woodCrate->Name = "woodCrate";
//	woodCrate->MatCBIndex = 0;
//	woodCrate->DiffuseSrvHeapIndex = 0;
//	woodCrate->DiffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
//	woodCrate->FresnelR0 = XMFLOAT3(0.05f, 0.05f, 0.05f);
//	woodCrate->Roughness = 0.2f;
//
//	mMaterials["woodCrate"] = std::move(woodCrate);
//}
//
//void CrateApp::BuildRenderItems()
//{
//	auto boxRitem = std::make_unique<RenderItem>();
//	boxRitem->ObjCBIndex = 0;
//	boxRitem->Mat = mMaterials["woodCrate"].get();
//	boxRitem->Geo = mGeometries["boxGeo"].get();
//	boxRitem->PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
//	boxRitem->IndexCount = boxRitem->Geo->DrawArgs["box"].IndexCount;
//	boxRitem->StartIndexLocation = boxRitem->Geo->DrawArgs["box"].StartIndexLocation;
//	boxRitem->BaseVertexLocation = boxRitem->Geo->DrawArgs["box"].BaseVertexLocation;
//	mAllRitems.push_back(std::move(boxRitem));
//
//	// All the render items are opaque.
//	for(auto& e : mAllRitems)
//		mOpaqueRitems.push_back(e.get());
//}
//
//void CrateApp::DrawRenderItems(ID3D12GraphicsCommandList* cmdList, const std::vector<RenderItem*>& ritems)
//{
//    UINT objCBByteSize = d3dUtil::CalcConstantBufferByteSize(sizeof(ObjectConstants));
//    UINT matCBByteSize = d3dUtil::CalcConstantBufferByteSize(sizeof(MaterialConstants));
// 
//	auto objectCB = mCurrFrameResource->ObjectCB->Resource();
//	auto matCB = mCurrFrameResource->MaterialCB->Resource();
//
//    // For each render item...
//    for(size_t i = 0; i < ritems.size(); ++i)
//    {
//        auto ri = ritems[i];
//
//        cmdList->IASetVertexBuffers(0, 1, &ri->Geo->VertexBufferView());
//        cmdList->IASetIndexBuffer(&ri->Geo->IndexBufferView());
//        cmdList->IASetPrimitiveTopology(ri->PrimitiveType);
//
//        // 获取欲绑定纹理的SRV
//		CD3DX12_GPU_DESCRIPTOR_HANDLE tex(mSrvDescriptorHeap->GetGPUDescriptorHandleForHeapStart());
//		tex.Offset(ri->Mat->DiffuseSrvHeapIndex, mCbvSrvDescriptorSize);
//
//        D3D12_GPU_VIRTUAL_ADDRESS objCBAddress = objectCB->GetGPUVirtualAddress() + ri->ObjCBIndex*objCBByteSize;
//		D3D12_GPU_VIRTUAL_ADDRESS matCBAddress = matCB->GetGPUVirtualAddress() + ri->Mat->MatCBIndex*matCBByteSize;
//
//        // 将纹理SRV绑定到根参数0。根参数指示了该纹理将要具体绑定到哪一个着色器寄存器槽
//		cmdList->SetGraphicsRootDescriptorTable(0, tex);
//        cmdList->SetGraphicsRootConstantBufferView(1, objCBAddress);
//        cmdList->SetGraphicsRootConstantBufferView(3, matCBAddress);
//
//        cmdList->DrawIndexedInstanced(ri->IndexCount, 1, ri->StartIndexLocation, ri->BaseVertexLocation, 0);
//    }
//}
//
//std::array<const CD3DX12_STATIC_SAMPLER_DESC, 6> CrateApp::GetStaticSamplers()
//{
//	// Applications usually only need a handful of samplers.  So just define them all up front
//	// and keep them available as part of the root signature.  
//
//	const CD3DX12_STATIC_SAMPLER_DESC pointWrap(
//		0, // shaderRegister
//		D3D12_FILTER_MIN_MAG_MIP_POINT, // filter
//		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressU
//		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressV
//		D3D12_TEXTURE_ADDRESS_MODE_WRAP); // addressW
//
//	const CD3DX12_STATIC_SAMPLER_DESC pointClamp(
//		1, // shaderRegister
//		D3D12_FILTER_MIN_MAG_MIP_POINT, // filter
//		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressU
//		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressV
//		D3D12_TEXTURE_ADDRESS_MODE_CLAMP); // addressW
//
//	const CD3DX12_STATIC_SAMPLER_DESC linearWrap(
//		2, // shaderRegister
//		D3D12_FILTER_MIN_MAG_MIP_LINEAR, // filter
//		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressU
//		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressV
//		D3D12_TEXTURE_ADDRESS_MODE_WRAP); // addressW
//
//	const CD3DX12_STATIC_SAMPLER_DESC linearClamp(
//		3, // shaderRegister
//		D3D12_FILTER_MIN_MAG_MIP_LINEAR, // filter
//		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressU
//		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressV
//		D3D12_TEXTURE_ADDRESS_MODE_CLAMP); // addressW
//
//	const CD3DX12_STATIC_SAMPLER_DESC anisotropicWrap(
//		4, // shaderRegister
//		D3D12_FILTER_ANISOTROPIC, // filter
//		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressU
//		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressV
//		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressW
//		0.0f,                             // mipLODBias
//		8);                               // maxAnisotropy
//
//	const CD3DX12_STATIC_SAMPLER_DESC anisotropicClamp(
//		5, // shaderRegister
//		D3D12_FILTER_ANISOTROPIC, // filter
//		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressU
//		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressV
//		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressW
//		0.0f,                              // mipLODBias
//		8);                                // maxAnisotropy
//
//	return { 
//		pointWrap, pointClamp,
//		linearWrap, linearClamp, 
//		anisotropicWrap, anisotropicClamp };
//}
//

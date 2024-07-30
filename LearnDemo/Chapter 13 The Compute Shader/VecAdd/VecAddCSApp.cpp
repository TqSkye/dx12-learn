////***************************************************************************************
//// VecAddCSApp.cpp by Frank Luna (C) 2015 All Rights Reserved.
////***************************************************************************************
//
//#include "../../../Common/d3dApp.h"
//#include "../../../Common/MathHelper.h"
//#include "../../../Common/UploadBuffer.h"
//#include "../../../Common/GeometryGenerator.h"
//#include "VecAddFrameResource.h"
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
//struct Data
//{
//	XMFLOAT3 v1;
//	XMFLOAT2 v2;
//};
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
//enum class RenderLayer : int
//{
//	Opaque = 0,
//	Transparent,
//	AlphaTested,
//	Count
//};
//
//class VecAddCSApp : public D3DApp
//{
//public:
//    VecAddCSApp(HINSTANCE hInstance);
//    VecAddCSApp(const VecAddCSApp& rhs) = delete;
//    VecAddCSApp& operator=(const VecAddCSApp& rhs) = delete;
//    ~VecAddCSApp();
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
//	void DoComputeWork();
//
//	void BuildBuffers();
//    void BuildRootSignature();
//	void BuildDescriptorHeaps();
//    void BuildShadersAndInputLayout();
//    void BuildPSOs();
//    void BuildFrameResources();
//
//	std::array<const CD3DX12_STATIC_SAMPLER_DESC, 6> GetStaticSamplers();
//
//private:
//
//    std::vector<std::unique_ptr<VecAddFrameResource>> mFrameResources;
//    VecAddFrameResource* mCurrFrameResource = nullptr;
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
//	std::unordered_map<std::string, ComPtr<ID3D12PipelineState>> mPSOs;
//
//    std::vector<D3D12_INPUT_ELEMENT_DESC> mInputLayout;
// 
//    RenderItem* mWavesRitem = nullptr;
//
//	// List of all the render items.
//	std::vector<std::unique_ptr<RenderItem>> mAllRitems;
//
//	// Render items divided by PSO.
//	std::vector<RenderItem*> mRitemLayer[(int)RenderLayer::Count];
//
//	const int NumDataElements = 32;
//
//	ComPtr<ID3D12Resource> mInputBufferA = nullptr;
//	ComPtr<ID3D12Resource> mInputUploadBufferA = nullptr;
//	ComPtr<ID3D12Resource> mInputBufferB = nullptr;
//	ComPtr<ID3D12Resource> mInputUploadBufferB = nullptr;
//	ComPtr<ID3D12Resource> mOutputBuffer = nullptr;
//	ComPtr<ID3D12Resource> mReadBackBuffer = nullptr;
//
//    PassConstants mMainPassCB;
//
//	XMFLOAT3 mEyePos = { 0.0f, 0.0f, 0.0f };
//	XMFLOAT4X4 mView = MathHelper::Identity4x4();
//	XMFLOAT4X4 mProj = MathHelper::Identity4x4();
//
//    float mTheta = 1.5f*XM_PI;
//    float mPhi = XM_PIDIV2 - 0.1f;
//    float mRadius = 50.0f;
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
//        VecAddCSApp theApp(hInstance);
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
//VecAddCSApp::VecAddCSApp(HINSTANCE hInstance)
//    : D3DApp(hInstance)
//{
//}
//
//VecAddCSApp::~VecAddCSApp()
//{
//    if(md3dDevice != nullptr)
//        FlushCommandQueue();
//}
//
//bool VecAddCSApp::Initialize()
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
//	BuildBuffers();
//    BuildRootSignature();
//	BuildDescriptorHeaps();
//    BuildShadersAndInputLayout();
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
//	DoComputeWork();
//
//    return true;
//}
// 
//void VecAddCSApp::OnResize()
//{
//    D3DApp::OnResize();
//
//    // The window resized, so update the aspect ratio and recompute the projection matrix.
//    XMMATRIX P = XMMatrixPerspectiveFovLH(0.25f*MathHelper::Pi, AspectRatio(), 1.0f, 1000.0f);
//    XMStoreFloat4x4(&mProj, P);
//}
//
//void VecAddCSApp::Update(const GameTimer& gt)
//{
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
//}
//
//void VecAddCSApp::Draw(const GameTimer& gt)
//{
//    auto cmdListAlloc = mCurrFrameResource->CmdListAlloc;
//
//    // Reuse the memory associated with command recording.
//    // We can only reset when the associated command lists have finished execution on the GPU.
//    ThrowIfFailed(cmdListAlloc->Reset());
//
//    // A command list can be reset after it has been added to the command queue via ExecuteCommandList.
//    // Reusing the command list reuses memory.
//    ThrowIfFailed(mCommandList->Reset(cmdListAlloc.Get(), mPSOs["opaque"].Get()));
//
//    mCommandList->RSSetViewports(1, &mScreenViewport);
//    mCommandList->RSSetScissorRects(1, &mScissorRect);
//
//    // Indicate a state transition on the resource usage.
////	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(),
////		D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));
//
//    // Clear the back buffer and depth buffer.
//    mCommandList->ClearRenderTargetView(CurrentBackBufferView(), (float*)&mMainPassCB.FogColor, 0, nullptr);
//    mCommandList->ClearDepthStencilView(DepthStencilView(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);
//
//    // Specify the buffers we are going to render to.
//    mCommandList->OMSetRenderTargets(1, &CurrentBackBufferView(), true, &DepthStencilView());
//
//
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
//void VecAddCSApp::OnMouseDown(WPARAM btnState, int x, int y)
//{
//    mLastMousePos.x = x;
//    mLastMousePos.y = y;
//
//    SetCapture(mhMainWnd);
//}
//
//void VecAddCSApp::OnMouseUp(WPARAM btnState, int x, int y)
//{
//    ReleaseCapture();
//}
//
//void VecAddCSApp::OnMouseMove(WPARAM btnState, int x, int y)
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
//void VecAddCSApp::DoComputeWork()
//{
//	// Reuse the memory associated with command recording.
//	// We can only reset when the associated command lists have finished execution on the GPU.
//	ThrowIfFailed(mDirectCmdListAlloc->Reset());
//
//	// A command list can be reset after it has been added to the command queue via ExecuteCommandList.
//	// Reusing the command list reuses memory.
//	ThrowIfFailed(mCommandList->Reset(mDirectCmdListAlloc.Get(), mPSOs["vecAdd"].Get()));
//
//	mCommandList->SetComputeRootSignature(mRootSignature.Get());
//
//	mCommandList->SetComputeRootShaderResourceView(0, mInputBufferA->GetGPUVirtualAddress());
//	mCommandList->SetComputeRootShaderResourceView(1, mInputBufferB->GetGPUVirtualAddress());
//	mCommandList->SetComputeRootUnorderedAccessView(2, mOutputBuffer->GetGPUVirtualAddress());
// 
//	mCommandList->Dispatch(1, 1, 1);
//
//	// Schedule to copy the data to the default buffer to the readback buffer.
//    // 按计划将数据从默认缓冲区复制到回读缓冲区（即系统内存缓冲区）中
//	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mOutputBuffer.Get(),
//		D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_SOURCE));
//
//	mCommandList->CopyResource(mReadBackBuffer.Get(), mOutputBuffer.Get());
//
//	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mOutputBuffer.Get(),
//		D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_COMMON));
//
//	// Done recording commands.
//    // 命令记录完成
//	ThrowIfFailed(mCommandList->Close());
//
//	// Add the command list to the queue for execution.
//    // 将命令列表添加到命令队列中用于执行
//	ID3D12CommandList* cmdsLists[] = { mCommandList.Get() };
//	mCommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);
//
//	// Wait for the work to finish.
//    // 等待命令执行完毕
//	FlushCommandQueue();
//
//	// Map the data so we can read it on CPU.
//    // 对数据进行映射，以便CPU读取
//	Data* mappedData = nullptr;
//	ThrowIfFailed(mReadBackBuffer->Map(0, nullptr, reinterpret_cast<void**>(&mappedData)));
//    // 结果保存
//	std::ofstream fout("E:\\DX12Book\\DX12LearnProject\\DX12Learn\\LearnDemo\\Chapter 13 The Compute Shader\\VecAdd\\results.txt");
//
//	for(int i = 0; i < NumDataElements; ++i)
//	{
//		fout << "(" << mappedData[i].v1.x << ", " << mappedData[i].v1.y << ", " << mappedData[i].v1.z <<
//			", " << mappedData[i].v2.x << ", " << mappedData[i].v2.y << ")" << std::endl;
//	}
//
//	mReadBackBuffer->Unmap(0, nullptr);
//}
//
///*
//    结构化缓冲区是一种由相同类型元素所构成的简单缓冲区——其本质上是一种数组。正如我们所看到的，该元素类型可以是用户以HLSL定义的结构体。
//    我们可以把为顶点缓冲区与索引缓冲区创建SRV的方法同样用于创建结构化缓冲区的SRV。
//    除了“必须指定D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS标志”这一条之外，将结构化缓冲区用作UAV也与之前的操作基本一致。
//    设置此标志的目的是用于把资源转换为D3D12_RESOURCE_STATE_UNORDERED_ACCESS状态。
//*/
//void VecAddCSApp::BuildBuffers()
//{
//	// Generate some data.// 生成一些数据来填充SRV缓冲区
//	std::vector<Data> dataA(NumDataElements);
//	std::vector<Data> dataB(NumDataElements);
//	for(int i = 0; i < NumDataElements; ++i)
//	{
//		dataA[i].v1 = XMFLOAT3(i, i, i);
//		dataA[i].v2 = XMFLOAT2(i, 0);
//
//		dataB[i].v1 = XMFLOAT3(-i, i, 0.0f);
//		dataB[i].v2 = XMFLOAT2(0, -i);
//	}
//
//	UINT64 byteSize = dataA.size()*sizeof(Data);
//
//	// Create some buffers to be used as SRVs.// 创建若干缓冲区用作SRV
//	mInputBufferA = d3dUtil::CreateDefaultBuffer(
//		md3dDevice.Get(),
//		mCommandList.Get(),
//		dataA.data(),
//		byteSize,
//		mInputUploadBufferA);
//
//	mInputBufferB = d3dUtil::CreateDefaultBuffer(
//		md3dDevice.Get(),
//		mCommandList.Get(),
//		dataB.data(),
//		byteSize,
//		mInputUploadBufferB);
//
//	// Create the buffer that will be a UAV.// 创建用作UAV的缓冲区
//	ThrowIfFailed(md3dDevice->CreateCommittedResource(
//		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
//		D3D12_HEAP_FLAG_NONE,
//		&CD3DX12_RESOURCE_DESC::Buffer(byteSize, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS),
//		D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
//		nullptr,
//		IID_PPV_ARGS(&mOutputBuffer)));
//	
//    // 创建一个系统内存缓冲区，以便读回处理结果
//	ThrowIfFailed(md3dDevice->CreateCommittedResource(
//		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_READBACK),
//		D3D12_HEAP_FLAG_NONE,
//		&CD3DX12_RESOURCE_DESC::Buffer(byteSize),
//		D3D12_RESOURCE_STATE_COPY_DEST,
//		nullptr,
//		IID_PPV_ARGS(&mReadBackBuffer)));
//}
//
///*
//    结构化缓冲区可以像纹理那样与流水线相绑定。我们为它们创建SRV或UAV的描述符，再将这些描述符作为参数传入需要获取描述符表的根参数。
//    或者，我们还能定义以根描述符为参数的根签名，由此便可以将资源的虚拟地址作为根参数直接进行传递，而无须涉及描述符堆
//    （这种方式仅限于创建缓冲区资源的SRV或UAV，并不适用于纹理）。考虑下列的根签名描述：
//*/
//void VecAddCSApp::BuildRootSignature()
//{
//    // Root parameter can be a table, root descriptor or root constants.
//    // 根参数可以是描述符表、根描述符或根常量
//    CD3DX12_ROOT_PARAMETER slotRootParameter[3];
//
//	// Perfomance TIP: Order from most frequent to least frequent.
//    // 性能优化小提示：按变更频率由高到低的顺序来填充根参数
//	slotRootParameter[0].InitAsShaderResourceView(0);
//    slotRootParameter[1].InitAsShaderResourceView(1);
//    slotRootParameter[2].InitAsUnorderedAccessView(0);
//
//    // A root signature is an array of root parameters.
//    // 根签名由一系列根参数所构成
//	CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(3, slotRootParameter,
//		0, nullptr,
//		D3D12_ROOT_SIGNATURE_FLAG_NONE);
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
//void VecAddCSApp::BuildDescriptorHeaps()
//{
//	
//}
//
//void VecAddCSApp::BuildShadersAndInputLayout()
//{
//	mShaders["vecAddCS"] = d3dUtil::CompileShader(L"E:\\DX12Book\\DX12LearnProject\\DX12Learn\\LearnDemo\\Chapter 13 The Compute Shader\\VecAdd\\Shaders\\VecAdd.hlsl", nullptr, "CS", "cs_5_0");
//}
//
//void VecAddCSApp::BuildPSOs()
//{
//	D3D12_COMPUTE_PIPELINE_STATE_DESC computePsoDesc = {};
//	computePsoDesc.pRootSignature = mRootSignature.Get();
//	computePsoDesc.CS =
//	{
//		reinterpret_cast<BYTE*>(mShaders["vecAddCS"]->GetBufferPointer()),
//		mShaders["vecAddCS"]->GetBufferSize()
//	};
//	computePsoDesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
//	ThrowIfFailed(md3dDevice->CreateComputePipelineState(&computePsoDesc, IID_PPV_ARGS(&mPSOs["vecAdd"])));
//}
//
//void VecAddCSApp::BuildFrameResources()
//{
//    for(int i = 0; i < gNumFrameResources; ++i)
//    {
//        mFrameResources.push_back(std::make_unique<VecAddFrameResource>(md3dDevice.Get(),
//            1));
//    }
//}
//
//std::array<const CD3DX12_STATIC_SAMPLER_DESC, 6> VecAddCSApp::GetStaticSamplers()
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

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
//// ���ǵ�Ӧ�ó����ࣨShapesApp����ʵ����һ����3��֡��ԴԪ�������ɵ��������������ض��ĳ�Ա��������¼��ǰ��֡��Դ��
//const int gNumFrameResources = 3;
//
///*
//��Ⱦ��
//    ����һ��������Ҫ���ö��ֲ���������󶨶��㻺���������������������������йصĳ������ݡ��趨ͼԪ�����Լ�ָ��DrawIndexedInstanced�����Ĳ�����
//    ���ų�������������������࣬��������ܴ���һ���������ṹ���洢����������������ݣ������Ǽ��õģ�
//    ����ÿ�������������ͬ�����ƹ��������������Ҳ�������仯����˸ýṹ�е�����Ҳ������������졣
//    ���ǰѵ��λ��Ƶ��ù����У���Ҫ����Ⱦ��ˮ���ύ�����ݼ���Ϊ��Ⱦ�render item����
//    ���ڵ�ǰ����ʾ������ԣ���Ⱦ��RenderItem�ṹ�����£�
//*/
//// Lightweight structure stores parameters to draw a shape.  This will
//// vary from app-to-app.
//// �洢����ͼ������������������ṹ�塣�������Ų�ͬ��Ӧ�ó�����������
//struct RenderItem
//{
//	RenderItem() = default;
//
//    // World matrix of the shape that describes the object's local space
//    // relative to the world space, which defines the position, orientation,
//    // and scale of the object in the world.
//    // ��������ֲ��ռ����������ռ���������
//    // ������������λ������ռ��е�λ�á������Լ���С
//    XMFLOAT4X4 World = MathHelper::Identity4x4();
//
//	// Dirty flag indicating the object data has changed and we need to update the constant buffer.
//	// Because we have an object cbuffer for each FrameResource, we have to apply the
//	// update to each FrameResource.  Thus, when we modify obect data we should set 
//	// NumFramesDirty = gNumFrameResources so that each frame resource gets the update.
//    // ���Ѹ��±�־��dirty flag������ʾ�������������ѷ����ı䣬����ζ�����Ǵ�ʱ��Ҫ���³�����
//    // ����������ÿ��FrameResource�ж���һ�����峣�����������������Ǳ����ÿ��FrameResource
//    // �����и��¡������������޸��������ݵ�ʱ��Ӧ����NumFramesDirty = gNumFrameResources
//    // �������ã��Ӷ�ʹÿ��֡��Դ���õ�����
//	int NumFramesDirty = gNumFrameResources;
//
//	// Index into GPU constant buffer corresponding to the ObjectCB for this render item.
//    // ������ָ���GPU������������Ӧ�ڵ�ǰ��Ⱦ���е����峣��������
//	UINT ObjCBIndex = -1;
//
//    // ����Ⱦ�������Ƶļ����塣ע�⣬����һ����������ܻ��õ������Ⱦ��
//	MeshGeometry* Geo = nullptr;
//
//    // Primitive topology.
//    // ͼԪ����
//    D3D12_PRIMITIVE_TOPOLOGY PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
//
//    // DrawIndexedInstanced parameters.
//    // DrawIndexedInstanced�����Ĳ���
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
//    // ����������Ⱦ�������
//	std::vector<std::unique_ptr<RenderItem>> mAllRitems;
//
//	// Render items divided by PSO.
//    // ����PSO(��ˮ��״̬����)��������Ⱦ��
//	std::vector<RenderItem*> mOpaqueRitems;         // ��͸��
//    // std::vector<RenderItem*> mTransparentRitems; // ͸��
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
//// ���ڣ�CPU�˴����n֡���㷨�������ģ�
//void ShapesApp::Update(const GameTimer& gt)
//{
//    OnKeyboardInput(gt);
//	UpdateCamera(gt);
//
//    // Cycle through the circular frame resource array.
//    // ѭ�������ػ�ȡ֡��Դѭ�������е�Ԫ��
//    mCurrFrameResourceIndex = (mCurrFrameResourceIndex + 1) % gNumFrameResources;
//    mCurrFrameResource = mFrameResources[mCurrFrameResourceIndex].get();
//
//    // Has the GPU finished processing the commands of the current frame resource?
//    // If not, wait until the GPU has completed commands up to this fence point.
//    // GPU���Ƿ��Ѿ�ִ���괦��ǰ֡��Դ�����������أ�
//    // �����û�о���CPU�ȴ���ֱ��GPU��������ִ�в��ִ����Χ����
//    if(mCurrFrameResource->Fence != 0 && mFence->GetCompletedValue() < mCurrFrameResource->Fence)
//    {
//        HANDLE eventHandle = CreateEventEx(nullptr, false, false, EVENT_ALL_ACCESS);
//        ThrowIfFailed(mFence->SetEventOnCompletion(mCurrFrameResource->Fence, eventHandle));
//        WaitForSingleObject(eventHandle, INFINITE);
//        CloseHandle(eventHandle);
//    }
//
//    // [...] ����mCurrFrameResource�ڵ���Դ�����糣����������
//	UpdateObjectCBs(gt);
//	UpdateMainPassCB(gt);
//
//    /*  cpuʹ��ѭ��֡��gpu����:
//        ���ѿ��������ֽ�����������޷���ȫ����ȴ�����ķ�����������ִ���������֡���ٶȲ�����
//        ��ǰ���ս����ò��ȴ�������׷�ϣ���Ϊ�����󽫵���֡��Դ���ݱ�����ظ�д��
//        ���GPU����������ٶȿ���CPU�ύ�����б���ٶȣ���GPU��������״̬��
//        ͨ����������Ҫ�������쾡�µط���ϵͳͼ�δ��������������Ӧ�����������ķ�����
//        ��Ϊ�Ⲣû�г������GPU��Դ�����⣬���CPU����֡���ٶ�����ңң������GPU����CPUһ�����ڵȴ���ʱ�䡣
//        ���������������ڴ����龰����Ϊ��ʹGPU����ȫ��������������CPU������Ŀ���ʱ�����ǿ��Ա���Ϸ���������������ã�
//        ��AI���˹����ܣ�������ģ���Լ���Ϸҵ���߼��ȡ�
//        ��ˣ����˵���ö��֡��ԴҲ�޷�����ȴ�����ķ�������ô�������Ǿ����к��ô��أ�
//        ���ǣ���ʹ���ǿ��Գ�����GPU�ṩ���ݡ�Ҳ����˵����GPU�ڴ����[n]֡������ʱ��
//        CPU���Լ����������ύ���Ƶ�[n + 1]֡�͵�[n + 2]֡���õ�����⽫��������б��ַǿ�״̬���Ӷ�ʹGPU��������ȥִ�С�
//    */
//}
//
//void ShapesApp::Draw(const GameTimer& gt)
//{
//    auto cmdListAlloc = mCurrFrameResource->CmdListAlloc;
//
//    // Reuse the memory associated with command recording.
//    // We can only reset when the associated command lists have finished execution on the GPU.
//    // �������¼�����йص��ڴ�
//    // ֻ����GPU ִ��������ڴ�������������б�ʱ�����ܶԴ������б��������������
//    ThrowIfFailed(cmdListAlloc->Reset());
//
//    // A command list can be reset after it has been added to the command queue via ExecuteCommandList.
//    // Reusing the command list reuses memory.
//    // ��ͨ��ExecuteCommandList�����������б���ӵ����������֮�����ǾͿ��Զ�����������
//    // ���������б�������֮��ص��ڴ�
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
//    // ������Դ����;ָʾ��Դ״̬��ת��
//	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(),
//		D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));
//
//    // Clear the back buffer and depth buffer.
//    // �����̨����������Ȼ�����
//    mCommandList->ClearRenderTargetView(CurrentBackBufferView(), Colors::LightSteelBlue, 0, nullptr);
//    mCommandList->ClearDepthStencilView(DepthStencilView(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);
//
//    // Specify the buffers we are going to render to.
//    // ָ��Ҫ��Ⱦ��Ŀ�껺����
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
//    // ��ִ����Ҫ���ƺ���Draw�Ĺ����е���DrawRenderItems������
//    DrawRenderItems(mCommandList.Get(), mOpaqueRitems);
//
//    // Indicate a state transition on the resource usage.
//    // ������Դ����;ָʾ��Դ״̬��ת��
//	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(),
//		D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));
//
//    // Done recording commands.
//    // �������ļ�¼
//    ThrowIfFailed(mCommandList->Close());
//
//    // Add the command list to the queue for execution.
//    // �������б���뵽�������������ִ��
//    ID3D12CommandList* cmdsLists[] = { mCommandList.Get() };
//    mCommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);
//
//    // Swap the back and front buffers
//    // ����ǰ��̨������
//    ThrowIfFailed(mSwapChain->Present(0, 0));
//	mCurrBackBuffer = (mCurrBackBuffer + 1) % SwapChainBufferCount;
//
//    // [...] �������ύ��֡�������б�
//
//    // Advance the fence value to mark commands up to this fence point.
//    // ����Χ��ֵ���������ǵ���Χ����
//    mCurrFrameResource->Fence = ++mCurrentFence;
//    
//    // Add an instruction to the command queue to set a new fence point. 
//    // Because we are on the GPU timeline, the new fence point won't be 
//    // set until the GPU finishes processing all the commands prior to this Signal().
//    // ������������һ��ָ��������һ���µ�Χ����
//    // ���ڵ�ǰ��GPU����ִ�л������������GPU������Signal()����֮ǰ������������ǰ�����������ô��µ�Χ����
//    mCommandQueue->Signal(mFence.Get(), mCurrentFence);
//
//    // ֵ��ע����ǣ�GPU��ʱ������Ȼ�ڴ�����һ֡���ݣ�������Ҳûʲô���⣬��Ϊ������Щ������û��Ӱ����֮ǰ֡�������֡��Դ
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
//    ������������������˼·Ϊ��������Դ�ĸ���Ƶ�ʶԳ������ݽ��з��顣��ÿ����Ⱦ���̣�render pass���У�ֻ�轫�������õĳ�����cbPass������һ�Σ�
//    ��ÿ��ĳ�����������������ı�ʱ��ֻ����¸��������س�����cbPerObject�����ɡ�
//    �����������һ����̬���壬����һ��������ֻ����������峣������������һ�Σ����ģ�������󣬶������Ҳ���ض������и����ˡ�
//    �����ǵ���ʾ�����У���ͨ�����з�����������Ⱦ���̳����������Լ����峣����������
//    �ڻ���ÿһ֡����ʱ������������(UpdateObjectCBs(���峣��������)��UpdateMainPassCB(��Ⱦ���̳���������))������Update��������һ�Ρ�
//*/
//void ShapesApp::UpdateObjectCBs(const GameTimer& gt)
//{
//	auto currObjectCB = mCurrFrameResource->ObjectCB.get();
//	for(auto& e : mAllRitems)
//	{
//		// Only update the cbuffer data if the constants have changed.  
//		// This needs to be tracked per frame resource.
//        // ֻҪ���������˸ı�͵ø��³����������ڵ����ݡ�����Ҫ��ÿ��֡��Դ�����и���
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
//            // ����Ҫ����һ��FrameResource���и���
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
//    ֡��Դ�ͳ�����������ͼ
//    �ع�ǰ�Ŀ�֪�������Ѿ�������һ����FrameResource����Ԫ�������ɵ�������ÿ��FrameResource�ж����ϴ���������
//    ����Ϊ�����е�ÿ����Ⱦ��洢��Ⱦ���̳����Լ����峣�����ݡ�
//    std::unique_ptr<UploadBuffer<PassConstants>> PassCB = nullptr;
//    std::unique_ptr<UploadBuffer<ObjectConstants>> ObjectCB = nullptr;
//
//    �����3��֡��Դ��[n]����Ⱦ���ô��Ӧ����[3n]�����峣����������object constant buffer���Լ�3����Ⱦ���̳�����������pass constant buffer����
//    ��ˣ�����Ҳ����Ҫ����[3(n+1)]��������������ͼ��CBV��������һ�������ǻ�Ҫ�޸�CBV�������ɶ������������
//*/
//
//void ShapesApp::BuildDescriptorHeaps()
//{
//    UINT objCount = (UINT)mOpaqueRitems.size();
//
//    // Need a CBV descriptor for each object for each frame resource,
//    // +1 for the perPass CBV for each frame resource.
//    // ������ҪΪÿ��֡��Դ�е�ÿһ�����嶼����һ��CBV��������
//    // Ϊ������ÿ��֡��Դ�е���Ⱦ����CBV��+1
//    UINT numDescriptors = (objCount+1) * gNumFrameResources;
//
//    // Save an offset to the start of the pass CBVs.  These are the last 3 descriptors.
//    // ������Ⱦ����CBV����ʼƫ�������ڱ������У���������������3��������
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
//    ������BuildDescriptorHeaps() ֮��:
//    ���ڣ����ǾͿ��������д��������CBV�ѣ�����������0��������[n-1]�����˵�0��֡��Դ������CBV��
//    ������[n]��������[2n-1]�����˵�1��֡��Դ������CBV���Դ����ƣ�������[2n]��������[3n-1]�����˵�2��֡��Դ������CBV��
//    ���[3n]��[3n+1]�Լ�[3n+2]�ֱ���е�0������1���͵�2��֡��Դ����Ⱦ����CBV��
//*/
//void ShapesApp::BuildConstantBufferViews()
//{
//    UINT objCBByteSize = d3dUtil::CalcConstantBufferByteSize(sizeof(ObjectConstants));
//
//    UINT objCount = (UINT)mOpaqueRitems.size();
//
//    // Need a CBV descriptor for each object for each frame resource.
//    // ÿ��֡��Դ�е�ÿһ�����嶼��Ҫһ����Ӧ��CBV������
//    for(int frameIndex = 0; frameIndex < gNumFrameResources; ++frameIndex)
//    {
//        auto objectCB = mFrameResources[frameIndex]->ObjectCB->Resource();
//        for(UINT i = 0; i < objCount; ++i)
//        {
//            D3D12_GPU_VIRTUAL_ADDRESS cbAddress = objectCB->GetGPUVirtualAddress();
//
//            // Offset to the ith object constant buffer in the buffer.
//            // ƫ�Ƶ��������е�i������ĳ���������
//            cbAddress += i*objCBByteSize;
//
//            // Offset to the object cbv in the descriptor heap.
//            // ƫ�Ƶ������������������е�CBV
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
//    // ���3��������������ÿ��֡��Դ����Ⱦ����CBV
//    for(int frameIndex = 0; frameIndex < gNumFrameResources; ++frameIndex)
//    {
//        auto passCB = mFrameResources[frameIndex]->PassCB->Resource();
//        // ÿ��֡��Դ����Ⱦ���̻�������ֻ����һ������������
//        D3D12_GPU_VIRTUAL_ADDRESS cbAddress = passCB->GetGPUVirtualAddress();
//
//        // Offset to the pass cbv in the descriptor heap.
//        // ƫ�Ƶ����������ж�Ӧ����Ⱦ����CBV
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
//    ������
//    �ع˸�ǩ�������֪ʶ��֪����ǩ������һϵ�и�����������ɡ���ĿǰΪֹ������ֻ����������һ����������ĸ�������
//    Ȼ������������ʵ��3�����Ϳ�ѡ��
//    1����������descriptor table���������������õ������������е�һ��������Χ������ȷ��Ҫ�󶨵���Դ��
//    2������������root descriptor���ֳ�Ϊ������������inline descriptor����
//        ͨ��ֱ�����ø�����������ָʾҪ�󶨵���Դ���������轫���������������С����ǣ�ֻ�г�����������CBV��
//        �Լ���������SRV/UAV����ɫ����Դ��ͼ/���������ͼ���ſ��Ը�����������ݽ��а󶨡���Ҳ����ζ�������SRV��������Ϊ����������ʵ����Դ�󶨡�
//    3����������root constant����������������ֱ�Ӱ�һϵ��32λ�ĳ���ֵ�����ǵ��������أ��ɷ���һ����ǩ����������64 DWORDΪ�ޡ�
//    3�ָ���������ռ�ÿռ��������¡�
//        1����������ÿ����������ռ��1 DWORD��
//        2������������ÿ������������64λ��GPU�����ַ��ռ��2DWORD��
//        3����������ÿ������32λ��ռ��1 DWORD��
//    ���ǿ��Դ�����������ϵĸ�ǩ����ֻҪ��������64DWORD�����޼��ɡ���������Ȼ���������㣬�������Ŀռ���������Ѹ�١�
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
//        �ڴ�����Ҫͨ����дCD3DX12_ROOT_PARAMETER�ṹ������������������������֮ǰ������������CD3DX��Ϊǰ׺�����������ṹ��һ����
//        CD3DX12_ROOT_PARAMETER�ǶԽṹ��D3D12_ROOT_PARAMETER������չ��������һЩ������ʼ��������������[2]��
//    */
//	// Root parameter can be a table, root descriptor or root constants.
//    // �����������������������������������
//	CD3DX12_ROOT_PARAMETER slotRootParameter[2];
//
//	// Create root CBVs.
//    // ������CBV
//    slotRootParameter[0].InitAsDescriptorTable(1, &cbvTable0);
//    slotRootParameter[1].InitAsDescriptorTable(1, &cbvTable1);
//
//	// A root signature is an array of root parameters.
//    // ��ǩ����һϵ�и�����������
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
//    �㻺��������������������ͼ7.6��ʾ����Shapes��ʾ�����У�����Ҫ���Ƴ����塢դ�����壨Բ̨�������塣������ʾ����Ҫ���ƶ����������壬��ʵ��������ֻ���һ�������Բ̨���ݵĸ�����
//    ͨ�����ò�ͬ��������󣬶��������ݽ��ж�λ��ƣ����ɻ��Ƴ����Ԥ����������Բ̨������˵����Ҳ��һ��������ʵ������instancing���ķ����������˼������Լ����ڴ���Դ��ռ�á�
//
//    ͨ������ͬ����Ķ���������ϲ����������ǰ����м���������Ķ����������װ��һ�����㻺������һ�������������ڡ�
//    ����ζ�ţ��ڻ���һ������ʱ������ֻ����ƴ�����λ�ڶ�������������ֻ������е������Ӽ���Ϊ�˵���ID3D12GraphicsCommandList::DrawIndexedInstanced�����������Ƽ�������Ӽ���
//    ������Ҫ����3�����ݣ��ɲ���ͼ6.3���ع�6.3�ڵ�������ۣ������Ƿֱ��ǣ������������ںϲ������������е���ʼ����������������������������Լ���׼�����ַ�����������������һ�������ںϲ����㻺�����е�������
//    ���ع˿�֪����׼�����ַ��һ������ֵ������Ҫ�ڻ��Ƶ��ù����С���ȡ��������֮ǰ�������������ԭ����ֵ������ӣ��Դ˻�ȡ�ϲ����㻺�����������������ȷ�����Ӽ����μ���5�µ���ϰ2����
//    ���д���չʾ�˴��������建������������Ƶ����������ֵ�Լ���������ľ�����̡�
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
//	// �����еļ��������ݶ��ϲ���һ�Դ�Ķ���/������������
//    // �Դ�������ÿ�������������ڻ���������ռ�ķ�Χ
//    //
//
//	// Cache the vertex offsets to each object in the concatenated vertex buffer.
//    // �Ժϲ����㻺������ÿ������Ķ���ƫ�������л���
//	UINT boxVertexOffset = 0;
//	UINT gridVertexOffset = (UINT)box.Vertices.size();
//	UINT sphereVertexOffset = gridVertexOffset + (UINT)grid.Vertices.size();
//	UINT cylinderVertexOffset = sphereVertexOffset + (UINT)sphere.Vertices.size();
//
//	// Cache the starting index for each object in the concatenated index buffer.
//    // �Ժϲ�������������ÿ���������ʼ�������л���
//	UINT boxIndexOffset = 0;
//	UINT gridIndexOffset = (UINT)box.Indices32.size();
//	UINT sphereIndexOffset = gridIndexOffset + (UINT)grid.Indices32.size();
//	UINT cylinderIndexOffset = sphereIndexOffset + (UINT)sphere.Indices32.size();
//
//    // Define the SubmeshGeometry that cover different 
//    // regions of the vertex/index buffers.
//    // ����Ķ��SubmeshGeometry�ṹ���а����˶���/�����������ڲ�ͬ�����������������
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
//	// ��ȡ������Ķ���Ԫ�أ��ٽ���������Ķ���װ��һ�����㻺����
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
//    ��Ⱦ��
//    �������������峡���е���Ⱦ�ͨ���۲����´����֪�����е���Ⱦ���ͬһ��MeshGeometry��
//    ���ԣ�����ͨ��DrawArgs��ȡDrawIndexedInstanced�����Ĳ��������Դ������ƶ���/������������������Ҳ���ǵ��������壩��
//
//    // ShapesApp���еĳ�Ա����
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
//    // ����ͼ7.6��ʾ�����������
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
//    // ����ʾ�����е�������Ⱦ��Ƿ�͸����
//	for(auto& e : mAllRitems)
//		mOpaqueRitems.push_back(e.get());
//}
//
///*
//    ���Ƴ���
//    ���һ�����ǻ�����Ⱦ���ˣ�������ӵĲ���Ī����Ϊ�˻������������ƫ�����ҵ����ڶ�������Ӧ��CBV��
//    �������Ĺ۲�һ�£���Ⱦ����������洢һ����������ʹָ֮�������йصĳ�����������
//
//    ��ִ����Ҫ���ƺ���Draw�Ĺ����е���DrawRenderItems������
//*/
//void ShapesApp::DrawRenderItems(ID3D12GraphicsCommandList* cmdList, const std::vector<RenderItem*>& ritems)
//{
//    UINT objCBByteSize = d3dUtil::CalcConstantBufferByteSize(sizeof(ObjectConstants));
// 
//	auto objectCB = mCurrFrameResource->ObjectCB->Resource();
//
//    // For each render item...
//    // ����ÿ����Ⱦ����˵...
//    for(size_t i = 0; i < ritems.size(); ++i)
//    {
//        auto ri = ritems[i];
//
//        cmdList->IASetVertexBuffers(0, 1, &ri->Geo->VertexBufferView());
//        cmdList->IASetIndexBuffer(&ri->Geo->IndexBufferView());
//        cmdList->IASetPrimitiveTopology(ri->PrimitiveType);
//
//        // Offset to the CBV in the descriptor heap for this object and for this frame resource.
//        // Ϊ�˻��Ƶ�ǰ��֡��Դ�͵�ǰ���壬ƫ�Ƶ����������ж�Ӧ��CBV��
//        UINT cbvIndex = mCurrFrameResourceIndex*(UINT)mOpaqueRitems.size() + ri->ObjCBIndex;
//        auto cbvHandle = CD3DX12_GPU_DESCRIPTOR_HANDLE(mCbvHeap->GetGPUDescriptorHandleForHeapStart());
//        cbvHandle.Offset(cbvIndex, mCbvSrvUavDescriptorSize);
//
//        cmdList->SetGraphicsRootDescriptorTable(0, cbvHandle);
//
//        cmdList->DrawIndexedInstanced(ri->IndexCount, 1, ri->StartIndexLocation, ri->BaseVertexLocation, 0);
//    }
//}

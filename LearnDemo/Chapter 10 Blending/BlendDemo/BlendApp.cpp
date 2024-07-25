////***************************************************************************************
//// BlendApp.cpp by Frank Luna (C) 2015 All Rights Reserved.
////***************************************************************************************
//
//
///*    流水线状态对象（PSO = Pipeline State Object)
//* 混合：
//    我们在渲染此帧画面时先后绘制了地形与木板箱（构成箱子在前、地形在后的效果），使这两种材质的像素数据都位于后台缓冲区中。
//    接着再运用混合技术将水面绘制到后台缓冲区，令水的像素数据与地形以及板条箱这两种像素数据在后台缓冲区内相混合，构成可以透过水看到地形与板条箱的效果。
//    本章将研究混合（blending，也译作融合）技术，它使我们可以将当前要光栅化（又名为源像素，source pixel）的像素与之前已光栅化至后台缓冲区的像素
//    （目标像素，destination pixel）相融合。因此，该技术可用于渲染如水与玻璃之类的半透明物体。
//* 混合方程：
//    设F为像素着色器输出的当前正在光栅化的第i行、第j列像素（源像素）的颜色值，再设C(dst)为目前在后台缓冲区中与之对应的第i行、第j列像素（目标像素）的颜色值。
//    若不用混合技术，C(src)将直接覆写C(dst)（假设此像素已经通过深度/模板测试），而令后台缓冲区中第i行、第j列的像素变更为新的颜色值C(src)。
//    但是，若使用了混合技术，则C(src)与C(dst)将融合在一起得到新颜色值C后再覆写C(dst)（即将两者的混合颜色C写入后台缓冲区的第i行、第j列像素）。
//    Direct3D使用下列混合方程来使源像素颜色与目标像素颜色相融合：
//        田C(dst)⊗F(dst)
//    在第10.3节中将介绍F(src)（源混合因子）与F(dst)（目标混合因子）会使用到的具体值，通过这两种因子，我们就能够用各种数值来调整源像素与目标像素，
//    以获取各种不同的效果。运算符“⊗”(点积)表示在5.3.1节中针对颜色向量而定义的分量式乘法，而“ 田 ”则表示在10.2节中定义的二元运算符。
//    上述混合方程仅用于控制颜色的RGB分量，而alpha分量实则由类似于下面的方程来单独处理：
//        田A(dst)F(dst)
//    这两组方程本质上都是相同的，但区别在于混合因子与二元运算可能有所差异。将RGB分量与alpha分量分离开来的动机也比较简单，就是希望能独立地处理两者，来尽可能多地产生不同的混合变化效果。
//    (注意:alpha分量的混合需求远少于RGB分量的混合需求。这主要是由于我们往往并不关心后台缓冲区中的alpha值。而仅在一些对目标alpha值（destination alpha）有特定要求的算法之中，后台缓冲区内的alpha值才显得至关重要。)
//* 混合运算:
//    混合运算下列枚举项成员将用作混合方程中的二元运算符“ 田 ”：
//    enum D3D12_BLEND_OP
//    {
//        D3D12_BLEND_OP_ADD	= 1,        // C = C(src) ⊗ F(src) + C(dst) ⊗ F(dst)
//        D3D12_BLEND_OP_SUBTRACT	= 2,    // C = C(dst) ⊗ F(dst) - C(src) ⊗ F(src)
//        D3D12_BLEND_OP_REV_SUBTRACT	= 3,// C = C(src) ⊗ F(src) - C(dst) ⊗ F(dst)
//        D3D12_BLEND_OP_MIN	= 4,        // C = min(C(src), C(dst))
//        D3D12_BLEND_OP_MAX	= 5         // C = max(C(src), C(dst))
//    } D3D12_BLEND_OP;
//    (注意:在求取最小值或最大值（min/max）的运算中会忽略混合因子。)
//    这些运算符也同样适用于alpha混合运算。而且，我们还能同时为RGB和alpha这两种运算分别指定不同的运算符。
//    例如，可以像下面一样使两个RGB项相加，却令两个alpha项相减：
//    C = C(src) ⊗ F(src) + C(dst) ⊗ F(dst)
//    A = A(dst)F(dst) - A(src)F(src)
//    Direct3D从最近几版开始加入了一项新特性，通过逻辑运算符对源颜色和目标颜色进行混合，用以取代上述传统的混合方程。这些逻辑运算符如下：
//    d3d12.h中的 enum D3D12_LOGIC_OP 枚举
//* 混合因子:
//    通过为源混合因子与目标混合因子分别设置不同的混合运算符，就可以实现各式各样的混合效果。
//    我们会在10.5节中详解这些组合，但是需要先来体验一下不同的混合因子并感受它们实际的计算方式。
//    下面列举描述的是基本的混合因子，可以将它们应用于F(src)与F(dst)。关于其他更加高级的混合因子，读者可参考SDK文档中的D3D12_BLEND枚举类型。
//    设C(src)=(r(s),g(s),b(s)) A(src)=a(s)（从像素着色器输出的RGBA值），C(dst)=(r(d),g(d),b(d)) A(dst)=a(d)（已存储于渲染目标中的RGBA值），
//    F为F(src)或F(dst)，而f是f(src)或f(dst)，则我们有：
//    enum D3D12_BLEND
//    {
//        D3D12_BLEND_ZERO = 1,               // F=(0,0,0) 且 f=0
//        D3D12_BLEND_ONE = 2,                // F=(1,1,1) 且 f=1
//        D3D12_BLEND_SRC_COLOR = 3,          // F=(r(s),g(s),b(s))
//        D3D12_BLEND_INV_SRC_COLOR = 4,      // F(src)=(1-r(s),1-g(s),1-b(s))
//        D3D12_BLEND_SRC_ALPHA = 5,          // F=(a(s),a(s),a(s)) 且 f=a(s)
//        D3D12_BLEND_INV_SRC_ALPHA = 6,      // F=(1-a(s),1-a(s),1-a(s)) 且 f=1-a(s)
//        D3D12_BLEND_DEST_ALPHA = 7,         // F=(a(d),a(d),a(d)) 且 f=a(d)
//        D3D12_BLEND_INV_DEST_ALPHA = 8,     // F=(1-a(d),1-a(d),1-a(d)) 且 f=1-a(d)
//        D3D12_BLEND_DEST_COLOR = 9,         // F=(r(d),g(d),b(d))
//        D3D12_BLEND_INV_DEST_COLOR = 10,    // F=(1-r(d),1-g(d),1-b(d))
//        D3D12_BLEND_SRC_ALPHA_SAT = 11,     // F=(a(s)',a(s)',a(s)') 且 f=a(s)', 其中a(s)'=clamp(a(s), 0, 1)
//        D3D12_BLEND_BLEND_FACTOR = 14,      // F=(r,g,b)  且 f=a 其中的颜色(r,g,b,a)可用作方法ID3D12GraphicsCommandList::OMSetBlendFactor的参数。通过这种方法，我们就可以直接指定所用的混合因子值。但是在改变混合状态（blend state）之前，此值是不会生效的。
//        D3D12_BLEND_INV_BLEND_FACTOR = 15,  // F=(1-r,1-g,1-b)  且 f=1-a 这里的颜色(r,g,b,a)可用作ID3D12GraphicsCommandList::OMSetBlendFactor的参数，这使我们可以直接指定所用的混合因子值。然而，在混合状态变化之前，此值保持不变。
//        
//        D3D12_BLEND_SRC1_COLOR = 16,        //
//        D3D12_BLEND_INV_SRC1_COLOR = 17,    //
//        D3D12_BLEND_SRC1_ALPHA = 18,        //
//        D3D12_BLEND_INV_SRC1_ALPHA = 19,    //
//        D3D12_BLEND_ALPHA_FACTOR = 20,      //
//        D3D12_BLEND_INV_ALPHA_FACTOR = 21   //
//    } D3D12_BLEND;
//    我们可以用下列函数来设置混合因子：
//    void ID3D12GraphicsCommandList::OMSetBlendFactor(
//        const FLOAT BlendFactor[ 4 ]);
//    若传入nullptr，则恢复值为(1, 1, 1, 1)的默认混合因子。
//* 混合状态:
//* 
//* 透明度混合:
//    设源alpha分量a(s)为一种可用来控制源像素不透明度的百分比（例如，alpha为0表示100%透明，0.4表示40%不透明，1.0则表示100%不透明）。
//    不透明度（opacity）与透明度（transparency）的关系很简单，即T=1-A，其中的A为不透明度，T为透明度。
//    例如，若物体的不透明度达0.4，则透明度为1 − 0.4 = 0.6。现在，假设我们希望基于源像素的不透明度，将源像素与目标像素进行混合。
//    为了实现此效果，设源混合因子为D3D12_BLEND_SRC_ALPHA、目标混合因子为D3D12_BLEND_INV_SRC_ALPHA，并将混合运算符置为D3D12_BLEND_OP_ADD。
//    有了这些配置，混合方程便可化简为：
//        田C(dst) ⊗ F(dst)
//        C = C(src) ⊗ F(src)
//        C = C(src) ⊗ (a(s), a(s), a(s)) +  C(dst) ⊗ (1-a(s), 1-a(s), 1-a(s))
//        C = a(s)C(src) +  (1-a(s))C(dst)
//    例如，假设a(s)=0.25，也就是说，源像素的不透明度仅为25%。由于源像素的透明度为75%，因此，这就是说，在源像素与目标像素混合在一起的时候，
//    我们便希望最终颜色将由25%的源像素与75%的目标像素组合而成（目标像素会位于源像素的“后侧”）。根据上述方程精确地推导出下列混合计算过程：
//        C = a(s)C(src) +  (1-a(s))C(dst)
//        C = 0.25C(src) +  0.75C(dst)
//    借助此混合方法，我们就能绘制出类似于图10.1中那样的透明物体。需要注意的是，在使用此混合方法时，还应当考虑物体的绘制顺序。
//    对此，我们应遵循以下规则：
//        1.首先要绘制无需混合处理的物体。
//        2.接下来，再根据混合物体与摄像机的距离对它们进行排序。
//        3.最后，按由远及近的顺序通过混合的方式来绘制这些物体。
//    依照由后向前的顺序进行绘制的原因是，每个物体都会与其后的所有物体执行混合运算。对于一个透明的物体而言，我们应当可以透过它看到其背后的场景。
//    因此就需要将透明物体后的所有对应像素都预先写入后台缓冲区内，随后再将此透明物体的源像素与其后场景的目标像素进行混合。
//
//* 雾效:
//    实现雾化效果的流程如下：如图10.9所示，首先指明雾的颜色、由摄像机到雾气的最近距离以及雾的分散范围（即从雾到摄像机的最近距离至雾能完全覆盖物体的这段范围），
//    接下来再将网格三角形上点的颜色置为原色与雾色的加权平均值：
//    
//    foggedColor = litColor + s(fogColor - litColor) = (1 - s)cdotlitColor + s*fogColor
//
//    参数s的范围为[0, 1]，由一个以摄像机位置与被雾覆盖物体表面点之间的距离作为参数的函数来确定。随着该表面点与观察点之间距离的增加，它会被雾气遮挡得愈加朦胧。
//    参数s的定义如下：
//
//    s = saturate((dist(P,E) - fogStart) / fogRange)
//
//    其中，dist(P, E)为表面点P与摄像机位置E之间的距离。而函数saturate会将其参数限制在区间[0, 1]内：
//                   x, 0<=x<=1
//    saturate(x) =  0, x < 0
//                   1, x > 1
//                |                   |
//                |                   |
//                |                   |
// E              |        p          |
//                |                   |
//                |                   |
// <---fogStart---><-----fogRange------>
//     无雾区               雾区
//图10.9　摄像机E到某点P的距离，fogStart（摄像机到雾气的最近距离）与fogRange（雾气的范围）即是相关参数
//
//    当dist(P,E)<=fogStart时s=0，而雾的颜色则由下式给出：foggedColor = litColor换句话说，当物体表面点到摄像机的距离小于fogStart时，雾色就不会改变物体顶点的本色。
//    顾名思义，只有表面点到摄像机的距离至少为“fogStart”（雾效开始）时，其颜色才会受到雾色的影响。
//    设fogEnd = fogStart + fogRange。当dist(P,E)=>fogEnd时s=1，且雾色为：foggedColor = fogColor这便是说，当物体表面点的位置到观察点的距离大于或等于fogEnd时，
//    浓雾会将它完全遮住——————所以我们只能看到雾气的颜色。
//    当fogStart<dist(P,E)<fogEnd时，随着dist(P,E)从fogStart向fogEnd递增，变量s也呈线性地由0增加至1。
//    这表明随着距离的增加，雾色会越来越浓重，而物体原色也愈加寡淡。这是显而易见的，因为随着距离的增加，雾气势必越发浓重，以致越远的景物越迷蒙。
//    |
//    |          fogend
//1.0 |          __________
//    |         /
//    |        /
//    |       /
//    |      /
//    |     /                         dist(P,E)
//    ------------------------------>
//     fogstart fogend
//
//图10.10　上图中，根据距离函数而得到的s（雾气的颜色权值）的图像。
//*/
//
//#include "../../../Common/d3dApp.h"
//#include "../../../Common/MathHelper.h"
//#include "../../../Common/UploadBuffer.h"
//#include "../../../Common/GeometryGenerator.h"
//#include "BlendFrameResource.h"
//#include "BlendWaves.h"
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
//enum class RenderLayer : int
//{
//	Opaque = 0,
//	Transparent,
//	AlphaTested,
//	Count
//};
//
//class BlendApp : public D3DApp
//{
//public:
//    BlendApp(HINSTANCE hInstance);
//    BlendApp(const BlendApp& rhs) = delete;
//    BlendApp& operator=(const BlendApp& rhs) = delete;
//    ~BlendApp();
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
//	void UpdateWaves(const GameTimer& gt); 
//
//	void LoadTextures();
//    void BuildRootSignature();
//	void BuildDescriptorHeaps();
//    void BuildShadersAndInputLayout();
//    void BuildLandGeometry();
//    void BuildWavesGeometry();
//	void BuildBoxGeometry();
//    void BuildPSOs();
//    void BuildFrameResources();
//    void BuildMaterials();
//    void BuildRenderItems();
//    void DrawRenderItems(ID3D12GraphicsCommandList* cmdList, const std::vector<RenderItem*>& ritems);
//
//	std::array<const CD3DX12_STATIC_SAMPLER_DESC, 6> GetStaticSamplers();
//
//    float GetHillsHeight(float x, float z)const;
//    XMFLOAT3 GetHillsNormal(float x, float z)const;
//
//private:
//
//    std::vector<std::unique_ptr<BlendFrameResource>> mFrameResources;
//    BlendFrameResource* mCurrFrameResource = nullptr;
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
//	std::unique_ptr<BlendWaves> mWaves;
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
//        BlendApp theApp(hInstance);
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
//BlendApp::BlendApp(HINSTANCE hInstance)
//    : D3DApp(hInstance)
//{
//}
//
//BlendApp::~BlendApp()
//{
//    if(md3dDevice != nullptr)
//        FlushCommandQueue();
//}
//
//bool BlendApp::Initialize()
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
//    mWaves = std::make_unique<BlendWaves>(128, 128, 1.0f, 0.03f, 4.0f, 0.2f);
// 
//	LoadTextures();
//    BuildRootSignature();
//	BuildDescriptorHeaps();
//    BuildShadersAndInputLayout();
//    BuildLandGeometry();
//    BuildWavesGeometry();
//	BuildBoxGeometry();
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
//void BlendApp::OnResize()
//{
//    D3DApp::OnResize();
//
//    // The window resized, so update the aspect ratio and recompute the projection matrix.
//    XMMATRIX P = XMMatrixPerspectiveFovLH(0.25f*MathHelper::Pi, AspectRatio(), 1.0f, 1000.0f);
//    XMStoreFloat4x4(&mProj, P);
//}
//
//void BlendApp::Update(const GameTimer& gt)
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
//    UpdateWaves(gt);
//}
//
//void BlendApp::Draw(const GameTimer& gt)
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
//	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(),
//		D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));
//
//    // Clear the back buffer and depth buffer.
//    mCommandList->ClearRenderTargetView(CurrentBackBufferView(), (float*)&mMainPassCB.FogColor, 0, nullptr);
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
//    DrawRenderItems(mCommandList.Get(), mRitemLayer[(int)RenderLayer::Opaque]);
//
//	mCommandList->SetPipelineState(mPSOs["alphaTested"].Get());
//	DrawRenderItems(mCommandList.Get(), mRitemLayer[(int)RenderLayer::AlphaTested]);
//
//	mCommandList->SetPipelineState(mPSOs["transparent"].Get());
//	DrawRenderItems(mCommandList.Get(), mRitemLayer[(int)RenderLayer::Transparent]);
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
//
//    // Add an instruction to the command queue to set a new fence point. 
//    // Because we are on the GPU timeline, the new fence point won't be 
//    // set until the GPU finishes processing all the commands prior to this Signal().
//    mCommandQueue->Signal(mFence.Get(), mCurrentFence);
//}
//
//void BlendApp::OnMouseDown(WPARAM btnState, int x, int y)
//{
//    mLastMousePos.x = x;
//    mLastMousePos.y = y;
//
//    SetCapture(mhMainWnd);
//}
//
//void BlendApp::OnMouseUp(WPARAM btnState, int x, int y)
//{
//    ReleaseCapture();
//}
//
//void BlendApp::OnMouseMove(WPARAM btnState, int x, int y)
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
//void BlendApp::OnKeyboardInput(const GameTimer& gt)
//{
//}
// 
//void BlendApp::UpdateCamera(const GameTimer& gt)
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
//void BlendApp::AnimateMaterials(const GameTimer& gt)
//{
//	// Scroll the water material texture coordinates.
//	auto waterMat = mMaterials["water"].get();
//
//	float& tu = waterMat->MatTransform(3, 0);
//	float& tv = waterMat->MatTransform(3, 1);
//
//	tu += 0.1f * gt.DeltaTime();
//	tv += 0.02f * gt.DeltaTime();
//
//	if(tu >= 1.0f)
//		tu -= 1.0f;
//
//	if(tv >= 1.0f)
//		tv -= 1.0f;
//
//	waterMat->MatTransform(3, 0) = tu;
//	waterMat->MatTransform(3, 1) = tv;
//
//	// Material has changed, so need to update cbuffer.
//	waterMat->NumFramesDirty = gNumFrameResources;
//}
//
//void BlendApp::UpdateObjectCBs(const GameTimer& gt)
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
//void BlendApp::UpdateMaterialCBs(const GameTimer& gt)
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
//void BlendApp::UpdateMainPassCB(const GameTimer& gt)
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
//	mMainPassCB.Lights[0].Strength = { 0.9f, 0.9f, 0.8f };
//	mMainPassCB.Lights[1].Direction = { -0.57735f, -0.57735f, 0.57735f };
//	mMainPassCB.Lights[1].Strength = { 0.3f, 0.3f, 0.3f };
//	mMainPassCB.Lights[2].Direction = { 0.0f, -0.707f, -0.707f };
//	mMainPassCB.Lights[2].Strength = { 0.15f, 0.15f, 0.15f };
//
//	auto currPassCB = mCurrFrameResource->PassCB.get();
//	currPassCB->CopyData(0, mMainPassCB);
//}
//
//void BlendApp::UpdateWaves(const GameTimer& gt)
//{
//	// Every quarter second, generate a random wave.
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
//	mWaves->Update(gt.DeltaTime());
//
//	// Update the wave vertex buffer with the new solution.
//	auto currWavesVB = mCurrFrameResource->WavesVB.get();
//	for(int i = 0; i < mWaves->VertexCount(); ++i)
//	{
//		Vertex v;
//
//		v.Pos = mWaves->Position(i);
//		v.Normal = mWaves->Normal(i);
//		
//		// Derive tex-coords from position by 
//		// mapping [-w/2,w/2] --> [0,1]
//		v.TexC.x = 0.5f + v.Pos.x / mWaves->Width();
//		v.TexC.y = 0.5f - v.Pos.z / mWaves->Depth();
//
//		currWavesVB->CopyData(i, v);
//	}
//
//	// Set the dynamic VB of the wave renderitem to the current frame VB.
//	mWavesRitem->Geo->VertexBufferGPU = currWavesVB->Resource();
//}
//
//void BlendApp::LoadTextures()
//{
//	auto grassTex = std::make_unique<Texture>();
//	grassTex->Name = "grassTex";
//	grassTex->Filename = L"E:/DX12Book/DX12LearnProject/DX12Learn/Textures/grass.dds";
//	ThrowIfFailed(DirectX::CreateDDSTextureFromFile12(md3dDevice.Get(),
//		mCommandList.Get(), grassTex->Filename.c_str(),
//		grassTex->Resource, grassTex->UploadHeap));
//
//	auto waterTex = std::make_unique<Texture>();
//	waterTex->Name = "waterTex";
//	waterTex->Filename = L"E:/DX12Book/DX12LearnProject/DX12Learn/Textures/water1.dds";
//	ThrowIfFailed(DirectX::CreateDDSTextureFromFile12(md3dDevice.Get(),
//		mCommandList.Get(), waterTex->Filename.c_str(),
//		waterTex->Resource, waterTex->UploadHeap));
//
//	auto fenceTex = std::make_unique<Texture>();
//	fenceTex->Name = "fenceTex";
//	fenceTex->Filename = L"E:/DX12Book/DX12LearnProject/DX12Learn/Textures/WireFence.dds";
//	ThrowIfFailed(DirectX::CreateDDSTextureFromFile12(md3dDevice.Get(),
//		mCommandList.Get(), fenceTex->Filename.c_str(),
//		fenceTex->Resource, fenceTex->UploadHeap));
//
//	mTextures[grassTex->Name] = std::move(grassTex);
//	mTextures[waterTex->Name] = std::move(waterTex);
//	mTextures[fenceTex->Name] = std::move(fenceTex);
//}
//
//void BlendApp::BuildRootSignature()
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
//void BlendApp::BuildDescriptorHeaps()
//{
//	//
//	// Create the SRV heap.
//	//
//	D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {};
//	srvHeapDesc.NumDescriptors = 3;
//	srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
//	srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
//	ThrowIfFailed(md3dDevice->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&mSrvDescriptorHeap)));
//
//	//
//	// Fill out the heap with actual descriptors.
//	//
//	CD3DX12_CPU_DESCRIPTOR_HANDLE hDescriptor(mSrvDescriptorHeap->GetCPUDescriptorHandleForHeapStart());
//
//	auto grassTex = mTextures["grassTex"]->Resource;
//	auto waterTex = mTextures["waterTex"]->Resource;
//	auto fenceTex = mTextures["fenceTex"]->Resource;
//
//	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
//	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
//	srvDesc.Format = grassTex->GetDesc().Format;
//	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
//	srvDesc.Texture2D.MostDetailedMip = 0;
//	srvDesc.Texture2D.MipLevels = -1;
//	md3dDevice->CreateShaderResourceView(grassTex.Get(), &srvDesc, hDescriptor);
//
//	// next descriptor
//	hDescriptor.Offset(1, mCbvSrvDescriptorSize);
//
//	srvDesc.Format = waterTex->GetDesc().Format;
//	md3dDevice->CreateShaderResourceView(waterTex.Get(), &srvDesc, hDescriptor);
//
//	// next descriptor
//	hDescriptor.Offset(1, mCbvSrvDescriptorSize);
//
//	srvDesc.Format = fenceTex->GetDesc().Format;
//	md3dDevice->CreateShaderResourceView(fenceTex.Get(), &srvDesc, hDescriptor);
//}
//
//void BlendApp::BuildShadersAndInputLayout()
//{
//	const D3D_SHADER_MACRO defines[] =
//	{
//		"FOG", "1",
//		NULL, NULL
//	};
//
//	const D3D_SHADER_MACRO alphaTestDefines[] =
//	{
//		"FOG", "1",
//		"ALPHA_TEST", "1",
//		NULL, NULL
//	};
//
//	mShaders["standardVS"] = d3dUtil::CompileShader(L"E:\\DX12Book\\DX12LearnProject\\DX12Learn\\LearnDemo\\Chapter 10 Blending\\BlendDemo\\Shaders\\Default.hlsl", nullptr, "VS", "vs_5_0");
//	mShaders["opaquePS"] = d3dUtil::CompileShader(L"E:\\DX12Book\\DX12LearnProject\\DX12Learn\\LearnDemo\\Chapter 10 Blending\\BlendDemo\\Shaders\\Default.hlsl", defines, "PS", "ps_5_0");
//	mShaders["alphaTestedPS"] = d3dUtil::CompileShader(L"E:\\DX12Book\\DX12LearnProject\\DX12Learn\\LearnDemo\\Chapter 10 Blending\\BlendDemo\\Shaders\\Default.hlsl", alphaTestDefines, "PS", "ps_5_0");
//	
//    mInputLayout =
//    {
//        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
//        { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
//		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
//    };
//}
//
//void BlendApp::BuildLandGeometry()
//{
//    GeometryGenerator geoGen;
//    GeometryGenerator::MeshData grid = geoGen.CreateGrid(160.0f, 160.0f, 50, 50);
//
//    //
//    // Extract the vertex elements we are interested and apply the height function to
//    // each vertex.  In addition, color the vertices based on their height so we have
//    // sandy looking beaches, grassy low hills, and snow mountain peaks.
//    //
//
//    std::vector<Vertex> vertices(grid.Vertices.size());
//    for(size_t i = 0; i < grid.Vertices.size(); ++i)
//    {
//        auto& p = grid.Vertices[i].Position;
//        vertices[i].Pos = p;
//        vertices[i].Pos.y = GetHillsHeight(p.x, p.z);
//        vertices[i].Normal = GetHillsNormal(p.x, p.z);
//		vertices[i].TexC = grid.Vertices[i].TexC;
//    }
//
//    const UINT vbByteSize = (UINT)vertices.size() * sizeof(Vertex);
//
//    std::vector<std::uint16_t> indices = grid.GetIndices16();
//    const UINT ibByteSize = (UINT)indices.size() * sizeof(std::uint16_t);
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
//void BlendApp::BuildWavesGeometry()
//{
//    std::vector<std::uint16_t> indices(3 * mWaves->TriangleCount()); // 3 indices per face
//	assert(mWaves->VertexCount() < 0x0000ffff);
//
//    // Iterate over each quad.
//    int m = mWaves->RowCount();
//    int n = mWaves->ColumnCount();
//    int k = 0;
//    for(int i = 0; i < m - 1; ++i)
//    {
//        for(int j = 0; j < n - 1; ++j)
//        {
//            indices[k] = i*n + j;
//            indices[k + 1] = i*n + j + 1;
//            indices[k + 2] = (i + 1)*n + j;
//
//            indices[k + 3] = (i + 1)*n + j;
//            indices[k + 4] = i*n + j + 1;
//            indices[k + 5] = (i + 1)*n + j + 1;
//
//            k += 6; // next quad
//        }
//    }
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
//void BlendApp::BuildBoxGeometry()
//{
//	GeometryGenerator geoGen;
//	GeometryGenerator::MeshData box = geoGen.CreateBox(8.0f, 8.0f, 8.0f, 3);
//
//	std::vector<Vertex> vertices(box.Vertices.size());
//	for (size_t i = 0; i < box.Vertices.size(); ++i)
//	{
//		auto& p = box.Vertices[i].Position;
//		vertices[i].Pos = p;
//		vertices[i].Normal = box.Vertices[i].Normal;
//		vertices[i].TexC = box.Vertices[i].TexC;
//	}
//
//	const UINT vbByteSize = (UINT)vertices.size() * sizeof(Vertex);
//
//	std::vector<std::uint16_t> indices = box.GetIndices16();
//	const UINT ibByteSize = (UINT)indices.size() * sizeof(std::uint16_t);
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
//	SubmeshGeometry submesh;
//	submesh.IndexCount = (UINT)indices.size();
//	submesh.StartIndexLocation = 0;
//	submesh.BaseVertexLocation = 0;
//
//	geo->DrawArgs["box"] = submesh;
//
//	mGeometries["boxGeo"] = std::move(geo);
//}
//
//void BlendApp::BuildPSOs()
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
//    ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&opaquePsoDesc, IID_PPV_ARGS(&mPSOs["opaque"])));
//
//	//
//	// PSO for transparent objects 创建开启混合功能的PSO
//	//
//
//	D3D12_GRAPHICS_PIPELINE_STATE_DESC transparentPsoDesc = opaquePsoDesc;
//
//	D3D12_RENDER_TARGET_BLEND_DESC transparencyBlendDesc;
//	transparencyBlendDesc.BlendEnable = true;
//	transparencyBlendDesc.LogicOpEnable = false;
//	transparencyBlendDesc.SrcBlend = D3D12_BLEND_SRC_ALPHA;
//	transparencyBlendDesc.DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
//	transparencyBlendDesc.BlendOp = D3D12_BLEND_OP_ADD;
//	transparencyBlendDesc.SrcBlendAlpha = D3D12_BLEND_ONE;
//	transparencyBlendDesc.DestBlendAlpha = D3D12_BLEND_ZERO;
//	transparencyBlendDesc.BlendOpAlpha = D3D12_BLEND_OP_ADD;
//	transparencyBlendDesc.LogicOp = D3D12_LOGIC_OP_NOOP;
//	transparencyBlendDesc.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
//
//	transparentPsoDesc.BlendState.RenderTarget[0] = transparencyBlendDesc;
//	ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&transparentPsoDesc, IID_PPV_ARGS(&mPSOs["transparent"])));
//
//	//
//	// PSO for alpha tested objects 针对alpha测试物体所采用的PSO
//	//
//    // 图10.7所示的是“Blend Demo”（混合）演示程序的效果。它用透明混合的处理方法来绘制半透明的水，并通过clip测试来渲染铁丝网盒。
//    // 另一个值得一提的变化是，由于当前立方体使用的是铁丝网纹理，因此我们就应对alpha测试物体都禁用背面剔除（不然后面就穿帮了！）：
//	D3D12_GRAPHICS_PIPELINE_STATE_DESC alphaTestedPsoDesc = opaquePsoDesc;
//	alphaTestedPsoDesc.PS = 
//	{ 
//		reinterpret_cast<BYTE*>(mShaders["alphaTestedPS"]->GetBufferPointer()),
//		mShaders["alphaTestedPS"]->GetBufferSize()
//	};
//    // 剔除模式(对alpha测试物体都禁用背面剔除)
//	alphaTestedPsoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
//	ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&alphaTestedPsoDesc, IID_PPV_ARGS(&mPSOs["alphaTested"])));
//}
//
//void BlendApp::BuildFrameResources()
//{
//    for(int i = 0; i < gNumFrameResources; ++i)
//    {
//        mFrameResources.push_back(std::make_unique<BlendFrameResource>(md3dDevice.Get(),
//            1, (UINT)mAllRitems.size(), (UINT)mMaterials.size(), mWaves->VertexCount()));
//    }
//}
//
//void BlendApp::BuildMaterials()
//{
//	auto grass = std::make_unique<Material>();
//	grass->Name = "grass";
//	grass->MatCBIndex = 0;
//	grass->DiffuseSrvHeapIndex = 0;
//	grass->DiffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
//	grass->FresnelR0 = XMFLOAT3(0.01f, 0.01f, 0.01f);
//	grass->Roughness = 0.125f;
//
//	// This is not a good water material definition, but we do not have all the rendering
//	// tools we need (transparency, environment reflection), so we fake it for now.
//	auto water = std::make_unique<Material>();
//	water->Name = "water";
//	water->MatCBIndex = 1;
//	water->DiffuseSrvHeapIndex = 1;
//	water->DiffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 0.5f);
//	water->FresnelR0 = XMFLOAT3(0.1f, 0.1f, 0.1f);
//	water->Roughness = 0.0f;
//
//	auto wirefence = std::make_unique<Material>();
//	wirefence->Name = "wirefence";
//	wirefence->MatCBIndex = 2;
//	wirefence->DiffuseSrvHeapIndex = 2;
//	wirefence->DiffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
//	wirefence->FresnelR0 = XMFLOAT3(0.1f, 0.1f, 0.1f);
//	wirefence->Roughness = 0.25f;
//
//	mMaterials["grass"] = std::move(grass);
//	mMaterials["water"] = std::move(water);
//	mMaterials["wirefence"] = std::move(wirefence);
//}
//
//void BlendApp::BuildRenderItems()
//{
//    auto wavesRitem = std::make_unique<RenderItem>();
//    wavesRitem->World = MathHelper::Identity4x4();
//	XMStoreFloat4x4(&wavesRitem->TexTransform, XMMatrixScaling(5.0f, 5.0f, 1.0f));
//	wavesRitem->ObjCBIndex = 0;
//	wavesRitem->Mat = mMaterials["water"].get();
//	wavesRitem->Geo = mGeometries["waterGeo"].get();
//	wavesRitem->PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
//	wavesRitem->IndexCount = wavesRitem->Geo->DrawArgs["grid"].IndexCount;
//	wavesRitem->StartIndexLocation = wavesRitem->Geo->DrawArgs["grid"].StartIndexLocation;
//	wavesRitem->BaseVertexLocation = wavesRitem->Geo->DrawArgs["grid"].BaseVertexLocation;
//
//    mWavesRitem = wavesRitem.get();
//
//	mRitemLayer[(int)RenderLayer::Transparent].push_back(wavesRitem.get());
//
//    auto gridRitem = std::make_unique<RenderItem>();
//    gridRitem->World = MathHelper::Identity4x4();
//	XMStoreFloat4x4(&gridRitem->TexTransform, XMMatrixScaling(5.0f, 5.0f, 1.0f));
//	gridRitem->ObjCBIndex = 1;
//	gridRitem->Mat = mMaterials["grass"].get();
//	gridRitem->Geo = mGeometries["landGeo"].get();
//	gridRitem->PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
//    gridRitem->IndexCount = gridRitem->Geo->DrawArgs["grid"].IndexCount;
//    gridRitem->StartIndexLocation = gridRitem->Geo->DrawArgs["grid"].StartIndexLocation;
//    gridRitem->BaseVertexLocation = gridRitem->Geo->DrawArgs["grid"].BaseVertexLocation;
//
//	mRitemLayer[(int)RenderLayer::Opaque].push_back(gridRitem.get());
//
//	auto boxRitem = std::make_unique<RenderItem>();
//	XMStoreFloat4x4(&boxRitem->World, XMMatrixTranslation(3.0f, 2.0f, -9.0f));
//	boxRitem->ObjCBIndex = 2;
//	boxRitem->Mat = mMaterials["wirefence"].get();
//	boxRitem->Geo = mGeometries["boxGeo"].get();
//	boxRitem->PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
//	boxRitem->IndexCount = boxRitem->Geo->DrawArgs["box"].IndexCount;
//	boxRitem->StartIndexLocation = boxRitem->Geo->DrawArgs["box"].StartIndexLocation;
//	boxRitem->BaseVertexLocation = boxRitem->Geo->DrawArgs["box"].BaseVertexLocation;
//
//	mRitemLayer[(int)RenderLayer::AlphaTested].push_back(boxRitem.get());
//
//    mAllRitems.push_back(std::move(wavesRitem));
//    mAllRitems.push_back(std::move(gridRitem));
//	mAllRitems.push_back(std::move(boxRitem));
//}
//
//void BlendApp::DrawRenderItems(ID3D12GraphicsCommandList* cmdList, const std::vector<RenderItem*>& ritems)
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
//		CD3DX12_GPU_DESCRIPTOR_HANDLE tex(mSrvDescriptorHeap->GetGPUDescriptorHandleForHeapStart());
//		tex.Offset(ri->Mat->DiffuseSrvHeapIndex, mCbvSrvDescriptorSize);
//
//        D3D12_GPU_VIRTUAL_ADDRESS objCBAddress = objectCB->GetGPUVirtualAddress() + ri->ObjCBIndex*objCBByteSize;
//		D3D12_GPU_VIRTUAL_ADDRESS matCBAddress = matCB->GetGPUVirtualAddress() + ri->Mat->MatCBIndex*matCBByteSize;
//
//		cmdList->SetGraphicsRootDescriptorTable(0, tex);
//        cmdList->SetGraphicsRootConstantBufferView(1, objCBAddress);
//        cmdList->SetGraphicsRootConstantBufferView(3, matCBAddress);
//
//        cmdList->DrawIndexedInstanced(ri->IndexCount, 1, ri->StartIndexLocation, ri->BaseVertexLocation, 0);
//    }
//}
//
//std::array<const CD3DX12_STATIC_SAMPLER_DESC, 6> BlendApp::GetStaticSamplers()
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
//float BlendApp::GetHillsHeight(float x, float z)const
//{
//    return 0.3f*(z*sinf(0.1f*x) + x*cosf(0.1f*z));
//}
//
//XMFLOAT3 BlendApp::GetHillsNormal(float x, float z)const
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

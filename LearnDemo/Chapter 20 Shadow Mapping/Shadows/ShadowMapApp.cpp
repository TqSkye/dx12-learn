////***************************************************************************************
//// ShadowMapApp.cpp by Frank Luna (C) 2015 All Rights Reserved.
////***************************************************************************************
//
///*
//渲染场景深度:
//    阴影贴图（shadow mapping，也有译作阴影映射）算法依赖于以光源的视角渲染场景深度（scene depth，即场景中的深度信息）——从本质上来讲，这其实就是一种变相的“渲染到纹理”技术，本书曾在13.7.2节中对后者进行了第一次阐述。
//    从“渲染场景深度”这个名字就可以看出，构建此深度缓冲区要从光源的视角着手。待以光源视角渲染场景深度之后，我们就能知晓离光源最近的像素片段，即那些不在阴影范围之中的像素片段。
//    本节将考察一个名为ShadowMap的工具类，它所存储的是以光源为视角而得到的场景深度数据。其实此工具类就是简单地封装了渲染阴影会用到的一个深度/模板缓冲区、一个视口以及多个视图，
//    而用于阴影贴图的那个深度/模板缓冲区也被称为阴影图（shadow map）。(详情见ShadowMap.cpp和ShadowMap.h脚本。)
//    该工具类(ShadowMap.cpp和ShadowMap.h脚本)的构造函数会根据指定的尺寸和视口来创建纹理。阴影图的分辨率（resolution，也有译作解析度）会直接影响阴影效果的质量，但是在提升该分辨率的同时，渲染也将产生更多的开销并占用更多的内存。
//
//    我们即将认识到：阴影贴图算法需要执行两个渲染过程（render pass）。
//    在第一次渲染过程中，我们要以光源的视角将场景深度数据渲染至阴影图中（后文中有时称之为深度绘制过程，depth pass）；
//    而在第二次渲染过程中则像往常那样，以“玩家”的摄像机视角将场景渲染至后台缓冲区之内，但为了实现阴影算法，此时应以阴影图作为着色器的输入之一。我们为访问着色器资源及其视图而提供了以下方法。(ShadowMap.cpp中定义)
//        ID3D12Resource* ShadowMap::Resource()
//        {
//          return mShadowMap.Get();
//        }
//
//        CD3DX12_GPU_DESCRIPTOR_HANDLE ShadowMap::Srv()const
//        {
//           return mhGpuSrv;
//        }
//
//        CD3DX12_CPU_DESCRIPTOR_HANDLE ShadowMap::Dsv()const
//        {
//           return mhCpuDsv;
//        }
//
//正交投影:
//    我们此前一直使用的是透视投影（perspective projection）。透视投影的关键特性在于，物体离观察点越远，它们就会显得越小（即近大远小）。这与我们在现实生活中对物体的感官是一致的。
//    除此之外，还有一种名为正交投影（orthographic projection，也有译作正投影）的投影类型。这种投影主要运用于3D科学或工程应用之中，即需要平行线在投射之后继续保持平行的情景（而不是像透视投影那样交于灭点）。
//    因此，正交投影只适用于模拟平行光所生成的阴影。沿着观察空间z轴的正方向看去，正交投影的视景体（viewing volume）是宽度为w、高度为h、近平面为n、远平面为f的对齐于观察空间坐标轴的长方体（见图20.1）。
//    这些数据定义了相对于观察空间坐标系的长方体视景体。使用正交投影时，其投影线均平行于观察空间的z轴（见图20.2）。因此我们可以看到，顶点(x,y,z)的2D投影为(x,y)
//
//    与透视投影一样，我们既希望保留正交投影中的深度信息，又希望采用规格化设备坐标。为了将正交投影视景体从观察空间变换到NDC（规格化设备坐标）空间，
//    我们就需要通过缩放以及平移操作来将观察空间中的视景体[-w/2,w/2] x [-h/2,h/2] x [n,f]映射为NDC空间的视景体[-1,1] x [-1,1] x [0,1]。
//    把上述关键坐标逐个进行对比变换即可确定该映射。对于前两对坐标来说，通过观察区间的差别，即可借助缩放因子方便地将它们变换到NDC空间：
//        2/w · [-w/2,w/2] = [-1,1]
//        2/h · [-h/2,h/2] = [-1,1]
//    而针对第3对坐标，则需要实现[n,f]-->[0,1]这一映射。在此，我们将其表示为方程g(z) = az + b（即一次缩放与平移变换）。
//    由于g(n) = 0且g(f) = 1，我们便可以解出a与b：
//        an + b = 0
//        af + b = 1
//    通过第一个方程可知b = -an。将它代入第二个方程，则有：
//        af - an = 1
//        a = 1 / (f - n)
//    所以：
//        -(n/(f-n)) = b
//    因此，
//        g(z) = z/(f-n) - n/(f-n)
//    读者不妨绘制出当变量n与f满足f>n时，函数g(z)在[n,f]区间内的图像。
//    最后，我们可得到将观察空间坐标(x,y,z)转换到NDC空间坐标(x',y',z')的正交变换：
//        x'= 2/w · x
//        y'= 2/h · y
//        z'= z/(f-n) - n/(f-n)
//    或者用矩阵表示为：
//                                2/w   0,      0,    0
//    [x',y',z',1] = [x,y,z,1] · [0,    2/h,    0,    0]
//                                0,    0,   1/(f-n), 0
//                                0,    0,   n/(n-f), 1
//    上述等式中的4X4矩阵即为正交投影矩阵（orthographic projection matrix）。
//    回顾透视投影变换可以知道，我们不得不将这个变换过程分为两个部分：一个是通过投影矩阵描述的线性部分，另一个则是通过除以w分量来描述的非线性部分。
//    相比之下，正交投影变换则完全是一种线性变换——因为整个过程都不需要除以w分量。将原坐标乘以正交投影矩阵便可以直接将其变换为NDC坐标。
//
//阴影贴图:
//    阴影贴图的大致思路是，从光源的视角将场景深度以“渲染至纹理”的方式绘制到名为阴影图（shadow map）的深度缓冲区中。
//    上述工作完成之后，我们就会得到一张从光源视角看去，由一切可见像素的深度数据所构成的阴影图（被其他像素所遮挡的像素不在此阴影图之列，因为它们在深度测试时将会失败，
//    所以它们不是被其他像素所覆写（overwrite）就是从来没有被写入过）。为了以光源的视角渲染场景，我们需要定义一个可以将坐标从世界空间变换到光源空间的光源观察矩阵（light view matrix），
//    以及一个用于描述光源在世界空间中照射范围的光源投影矩阵（light projection matrix）。
//    光源照射的体积范围可能为平截头体（透视投影）或长方体（正交投影）。通过在平截头体内嵌入聚光灯的照明圆锥体，便可用此平截头体光源体积（frustum light volume）来模拟聚光灯光源。
//    而长方体光源体积则可用于模拟平行光光源。然而，由于平行光被限制在长方体内并且只能经过长方体体积范围，因此它只能照射到场景中的一部分区域（见图20.5）。
//    对于能够照射到整个场景的光源（例如太阳）来说，我们就可以扩展光源体积，从而使它足以照亮全部场景。
//    
//    阴影图存储的是距离光源最近的可视像素深度值，但是它的分辨率有限，以致每一个阴影图纹素都要表示场景中的一片区域。因此，阴影图只是以光源视角针对场景深度进行的离散采样，这将会导致所谓的阴影粉刺（shadow acne）等图像走样（aliasing）[1]问题（见图20.7）。
//    通过偏移阴影图中的深度值来防止出现错误的阴影效果。此时，我们便可得到[插图]与[插图]。但寻找合适的深度偏移量通常要通过尝试若干
//    偏移量过大会导致名为peter-panning（彼得·潘，即小飞侠，他曾在一次逃跑时弄丢了自己的影子）的失真效果，使阴影看起来与物体相分离。然而，并没有哪一种固定的偏移量可正确地运用于所有几何体的阴影绘制。
//    特别是图20.11中所示的那种（从光源的角度来看）有着极大斜率的三角形，这时就需要选取更大的偏移量。但是，如果企图通过一个过大的深度偏移量来处理所有的斜边，则又会造成如图20.10所示的peter-panning问题。
//        typedef struct D3D12_RASTERIZER_DESC {
//         [...]
//         INT       DepthBias;
//         FLOAT      DepthBiasClamp;
//         FLOAT      SlopeScaledDepthBias;
//         [...]
//        } D3D12_RASTERIZER_DESC;
//    1．DepthBias：一个固定的应用偏移量。对于格式为UNORM的深度缓冲区而言，可参考下列代码注释中该整数值的设置方法。
//    2．DepthBiasClamp：所允许的最大深度偏移量。以此来设置深度偏移量的上限。不难想象，极其陡峭的倾斜度会致使斜率缩放偏移量过大，继而造成peter-panning失真。
//    3．SlopeScaledDepthBias：根据多边形的斜率来控制偏移程度的缩放因子。具体配置方法可参见下列代码注释中的公式。
//    注意，在将场景渲染至阴影图时，便会应用该斜率缩放偏移量。这是由于我们希望以光源的视角基于多边形的斜率而进行偏移操作，从而避免阴影失真。
//    因此，我们就会对阴影图中的数值进行偏移计算（即由硬件将像素的深度值与偏移值相加）。在演示程序中采用的具体数值如下。
//        // [出自MSDN]
//        // 如果当前的深度缓冲区采用UNORM格式且与输出合并阶段绑定在一起，或深度缓冲区还没有执行绑定操作，
//        // 则偏移量的计算过程如下
//        //
//        // Bias = (float)DepthBias * r + SlopeScaledDepthBias * MaxDepthSlope;
//        //
//        // 这里的r是在将深度缓冲区格式转换为 float32类型后，其深度值可取到的大于0的最小可表示的值
//        // [MSDN援引部分结束]
//        // 
//        // 对于一个24位的深度缓冲区来说，r = 1 / 2^24
//        //
//        // 例如：DepthBias = 100000 ==> 实际的DepthBias = 100000/2^24 = .006
//
//        // 这些数据极其依赖于实际场景，因此我们需要对特定场景反复尝试才能找到最合适的偏移数值
//        D3D12_GRAPHICS_PIPELINE_STATE_DESC smapPsoDesc = opaquePsoDesc;
//        smapPsoDesc.RasterizerState.DepthBias = 100000;
//        smapPsoDesc.RasterizerState.DepthBiasClamp = 0.0f;
//        smapPsoDesc.RasterizerState.SlopeScaledDepthBias = 1.0f;
//
//    */
//
//#include "../../../Common/d3dApp.h"
//#include "../../../Common/MathHelper.h"
//#include "../../../Common/UploadBuffer.h"
//#include "../../../Common/GeometryGenerator.h"
//#include "../../../Common/Camera.h"
//#include "ShadowFrameResource.h"
//#include "ShadowMap.h"
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
//    RenderItem(const RenderItem& rhs) = delete;
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
//    Debug,
//	Sky,
//	Count
//};
//
//class ShadowMapApp : public D3DApp
//{
//public:
//    ShadowMapApp(HINSTANCE hInstance);
//    ShadowMapApp(const ShadowMapApp& rhs) = delete;
//    ShadowMapApp& operator=(const ShadowMapApp& rhs) = delete;
//    ~ShadowMapApp();
//
//    virtual bool Initialize()override;
//
//private:
//    virtual void CreateRtvAndDsvDescriptorHeaps()override;
//    virtual void OnResize()override;
//    virtual void Update(const GameTimer& gt)override;
//    virtual void Draw(const GameTimer& gt)override;
//
//    virtual void OnMouseDown(WPARAM btnState, int x, int y)override;
//    virtual void OnMouseUp(WPARAM btnState, int x, int y)override;
//    virtual void OnMouseMove(WPARAM btnState, int x, int y)override;
//
//    void OnKeyboardInput(const GameTimer& gt);
//	void AnimateMaterials(const GameTimer& gt);
//	void UpdateObjectCBs(const GameTimer& gt);
//	void UpdateMaterialBuffer(const GameTimer& gt);
//    void UpdateShadowTransform(const GameTimer& gt);
//	void UpdateMainPassCB(const GameTimer& gt);
//    void UpdateShadowPassCB(const GameTimer& gt);
//
//	void LoadTextures();
//    void BuildRootSignature();
//	void BuildDescriptorHeaps();
//    void BuildShadersAndInputLayout();
//    void BuildShapeGeometry();
//    void BuildSkullGeometry();
//    void BuildPSOs();
//    void BuildFrameResources();
//    void BuildMaterials();
//    void BuildRenderItems();
//    void DrawRenderItems(ID3D12GraphicsCommandList* cmdList, const std::vector<RenderItem*>& ritems);
//    void DrawSceneToShadowMap();
//
//	std::array<const CD3DX12_STATIC_SAMPLER_DESC, 7> GetStaticSamplers();
//
//private:
//
//    std::vector<std::unique_ptr<ShadowFrameResource>> mFrameResources;
//    ShadowFrameResource* mCurrFrameResource = nullptr;
//    int mCurrFrameResourceIndex = 0;
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
//	// List of all the render items.
//	std::vector<std::unique_ptr<RenderItem>> mAllRitems;
//
//	// Render items divided by PSO.
//	std::vector<RenderItem*> mRitemLayer[(int)RenderLayer::Count];
//
//	UINT mSkyTexHeapIndex = 0;
//    UINT mShadowMapHeapIndex = 0;
//
//    UINT mNullCubeSrvIndex = 0;
//    UINT mNullTexSrvIndex = 0;
//
//    CD3DX12_GPU_DESCRIPTOR_HANDLE mNullSrv;
//
//    PassConstants mMainPassCB;  // index 0 of pass cbuffer.
//    PassConstants mShadowPassCB;// index 1 of pass cbuffer.
//
//	Camera mCamera;
//
//    std::unique_ptr<ShadowMap> mShadowMap;
//
//    DirectX::BoundingSphere mSceneBounds;
//
//    float mLightNearZ = 0.0f;
//    float mLightFarZ = 0.0f;
//    XMFLOAT3 mLightPosW;
//    XMFLOAT4X4 mLightView = MathHelper::Identity4x4();
//    XMFLOAT4X4 mLightProj = MathHelper::Identity4x4();
//    XMFLOAT4X4 mShadowTransform = MathHelper::Identity4x4();
//
//    float mLightRotationAngle = 0.0f;
//    XMFLOAT3 mBaseLightDirections[3] = {
//        XMFLOAT3(0.57735f, -0.57735f, 0.57735f),
//        XMFLOAT3(-0.57735f, -0.57735f, 0.57735f),
//        XMFLOAT3(0.0f, -0.707f, -0.707f)
//    };
//    XMFLOAT3 mRotatedLightDirections[3];
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
//        ShadowMapApp theApp(hInstance);
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
//ShadowMapApp::ShadowMapApp(HINSTANCE hInstance)
//    : D3DApp(hInstance)
//{
//    // Estimate the scene bounding sphere manually since we know how the scene was constructed.
//    // The grid is the "widest object" with a width of 20 and depth of 30.0f, and centered at
//    // the world space origin.  In general, you need to loop over every world space vertex
//    // position and compute the bounding sphere.
//    // 由于我们知道当前场景是如何构建出来的，因此可手动估算场景的包围球
//    // 中心位于世界空间的原点，且宽度为20，深度为30.0f的栅格是场景中“最宽的物体”。而在实际的项
//    // 目中，我们若要计算此包围球，通常就需要遍历世界空间中的每个顶点的位置
//    mSceneBounds.Center = XMFLOAT3(0.0f, 0.0f, 0.0f);
//    mSceneBounds.Radius = sqrtf(10.0f*10.0f + 15.0f*15.0f);
//}
//
//ShadowMapApp::~ShadowMapApp()
//{
//    if(md3dDevice != nullptr)
//        FlushCommandQueue();
//}
//
//bool ShadowMapApp::Initialize()
//{
//    if(!D3DApp::Initialize())
//        return false;
//
//    // Reset the command list to prep for initialization commands.
//    ThrowIfFailed(mCommandList->Reset(mDirectCmdListAlloc.Get(), nullptr));
//
//	mCamera.SetPosition(0.0f, 2.0f, -15.0f);
// 
//    mShadowMap = std::make_unique<ShadowMap>(
//        md3dDevice.Get(), 2048, 2048);
//
//	LoadTextures();
//    BuildRootSignature();
//	BuildDescriptorHeaps();
//    BuildShadersAndInputLayout();
//    BuildShapeGeometry();
//    BuildSkullGeometry();
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
//void ShadowMapApp::CreateRtvAndDsvDescriptorHeaps()
//{
//    // Add +6 RTV for cube render target.
//    D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc;
//    rtvHeapDesc.NumDescriptors = SwapChainBufferCount;
//    rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
//    rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
//    rtvHeapDesc.NodeMask = 0;
//    ThrowIfFailed(md3dDevice->CreateDescriptorHeap(
//        &rtvHeapDesc, IID_PPV_ARGS(mRtvHeap.GetAddressOf())));
//
//    // Add +1 DSV for shadow map.
//    D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc;
//    dsvHeapDesc.NumDescriptors = 2;
//    dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
//    dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
//    dsvHeapDesc.NodeMask = 0;
//    ThrowIfFailed(md3dDevice->CreateDescriptorHeap(
//        &dsvHeapDesc, IID_PPV_ARGS(mDsvHeap.GetAddressOf())));
//}
// 
//void ShadowMapApp::OnResize()
//{
//    D3DApp::OnResize();
//
//	mCamera.SetLens(0.25f*MathHelper::Pi, AspectRatio(), 1.0f, 1000.0f);
//}
//
//void ShadowMapApp::Update(const GameTimer& gt)
//{
//    OnKeyboardInput(gt);
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
//    //
//    // Animate the lights (and hence shadows). 根据时间的流逝动态调整各光源（以及物体阴影）
//    //
//
//    mLightRotationAngle += 0.1f*gt.DeltaTime();
//
//    XMMATRIX R = XMMatrixRotationY(mLightRotationAngle);
//    for(int i = 0; i < 3; ++i)
//    {
//        XMVECTOR lightDir = XMLoadFloat3(&mBaseLightDirections[i]);
//        lightDir = XMVector3TransformNormal(lightDir, R);
//        XMStoreFloat3(&mRotatedLightDirections[i], lightDir);
//    }
//
//	AnimateMaterials(gt);
//	UpdateObjectCBs(gt);
//	UpdateMaterialBuffer(gt);
//    UpdateShadowTransform(gt);
//	UpdateMainPassCB(gt);
//    UpdateShadowPassCB(gt);
//}
//
//void ShadowMapApp::Draw(const GameTimer& gt)
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
//    ID3D12DescriptorHeap* descriptorHeaps[] = { mSrvDescriptorHeap.Get() };
//    mCommandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);
//
//    mCommandList->SetGraphicsRootSignature(mRootSignature.Get());
//
//    // Bind all the materials used in this scene.  For structured buffers, we can bypass the heap and 
//    // set as a root descriptor.
//    auto matBuffer = mCurrFrameResource->MaterialBuffer->Resource();
//    mCommandList->SetGraphicsRootShaderResourceView(2, matBuffer->GetGPUVirtualAddress());
//
//    // Bind null SRV for shadow map pass.
//    mCommandList->SetGraphicsRootDescriptorTable(3, mNullSrv);	 
//
//    // Bind all the textures used in this scene.  Observe
//    // that we only have to specify the first descriptor in the table.  
//    // The root signature knows how many descriptors are expected in the table.
//    mCommandList->SetGraphicsRootDescriptorTable(4, mSrvDescriptorHeap->GetGPUDescriptorHandleForHeapStart());
//
//    DrawSceneToShadowMap();
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
//	auto passCB = mCurrFrameResource->PassCB->Resource();
//	mCommandList->SetGraphicsRootConstantBufferView(1, passCB->GetGPUVirtualAddress());
//
//    // Bind the sky cube map.  For our demos, we just use one "world" cube map representing the environment
//    // from far away, so all objects will use the same cube map and we only need to set it once per-frame.  
//    // If we wanted to use "local" cube maps, we would have to change them per-object, or dynamically
//    // index into an array of cube maps.
//
//    CD3DX12_GPU_DESCRIPTOR_HANDLE skyTexDescriptor(mSrvDescriptorHeap->GetGPUDescriptorHandleForHeapStart());
//    skyTexDescriptor.Offset(mSkyTexHeapIndex, mCbvSrvUavDescriptorSize);
//    mCommandList->SetGraphicsRootDescriptorTable(3, skyTexDescriptor);
//
//    mCommandList->SetPipelineState(mPSOs["opaque"].Get());
//    DrawRenderItems(mCommandList.Get(), mRitemLayer[(int)RenderLayer::Opaque]);
//
//    mCommandList->SetPipelineState(mPSOs["debug"].Get());
//    DrawRenderItems(mCommandList.Get(), mRitemLayer[(int)RenderLayer::Debug]);
//
//	mCommandList->SetPipelineState(mPSOs["sky"].Get());
//	DrawRenderItems(mCommandList.Get(), mRitemLayer[(int)RenderLayer::Sky]);
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
//void ShadowMapApp::OnMouseDown(WPARAM btnState, int x, int y)
//{
//    mLastMousePos.x = x;
//    mLastMousePos.y = y;
//
//    SetCapture(mhMainWnd);
//}
//
//void ShadowMapApp::OnMouseUp(WPARAM btnState, int x, int y)
//{
//    ReleaseCapture();
//}
//
//void ShadowMapApp::OnMouseMove(WPARAM btnState, int x, int y)
//{
//    if((btnState & MK_LBUTTON) != 0)
//    {
//		// Make each pixel correspond to a quarter of a degree.
//		float dx = XMConvertToRadians(0.25f*static_cast<float>(x - mLastMousePos.x));
//		float dy = XMConvertToRadians(0.25f*static_cast<float>(y - mLastMousePos.y));
//
//		mCamera.Pitch(dy);
//		mCamera.RotateY(dx);
//    }
//
//    mLastMousePos.x = x;
//    mLastMousePos.y = y;
//}
// 
//void ShadowMapApp::OnKeyboardInput(const GameTimer& gt)
//{
//	const float dt = gt.DeltaTime();
//
//	if(GetAsyncKeyState('W') & 0x8000)
//		mCamera.Walk(10.0f*dt);
//
//	if(GetAsyncKeyState('S') & 0x8000)
//		mCamera.Walk(-10.0f*dt);
//
//	if(GetAsyncKeyState('A') & 0x8000)
//		mCamera.Strafe(-10.0f*dt);
//
//	if(GetAsyncKeyState('D') & 0x8000)
//		mCamera.Strafe(10.0f*dt);
//
//	mCamera.UpdateViewMatrix();
//}
// 
//void ShadowMapApp::AnimateMaterials(const GameTimer& gt)
//{
//	
//}
//
//void ShadowMapApp::UpdateObjectCBs(const GameTimer& gt)
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
//			objConstants.MaterialIndex = e->Mat->MatCBIndex;
//
//			currObjectCB->CopyData(e->ObjCBIndex, objConstants);
//
//			// Next FrameResource need to be updated too.
//			e->NumFramesDirty--;
//		}
//	}
//}
//
//void ShadowMapApp::UpdateMaterialBuffer(const GameTimer& gt)
//{
//	auto currMaterialBuffer = mCurrFrameResource->MaterialBuffer.get();
//	for(auto& e : mMaterials)
//	{
//		// Only update the cbuffer data if the constants have changed.  If the cbuffer
//		// data changes, it needs to be updated for each FrameResource.
//		Material* mat = e.second.get();
//		if(mat->NumFramesDirty > 0)
//		{
//			XMMATRIX matTransform = XMLoadFloat4x4(&mat->MatTransform);
//
//			MaterialData matData;
//			matData.DiffuseAlbedo = mat->DiffuseAlbedo;
//			matData.FresnelR0 = mat->FresnelR0;
//			matData.Roughness = mat->Roughness;
//			XMStoreFloat4x4(&matData.MatTransform, XMMatrixTranspose(matTransform));
//			matData.DiffuseMapIndex = mat->DiffuseSrvHeapIndex;
//			matData.NormalMapIndex = mat->NormalSrvHeapIndex;
//
//			currMaterialBuffer->CopyData(mat->MatCBIndex, matData);
//
//			// Next FrameResource need to be updated too.
//			mat->NumFramesDirty--;
//		}
//	}
//}
//
//void ShadowMapApp::UpdateShadowTransform(const GameTimer& gt)
//{
//    // Only the first "main" light casts a shadow.
//    // 只有第一个“主”光源才投射出物体的阴影
//    XMVECTOR lightDir = XMLoadFloat3(&mRotatedLightDirections[0]);
//    XMVECTOR lightPos = -2.0f*mSceneBounds.Radius*lightDir;
//    XMVECTOR targetPos = XMLoadFloat3(&mSceneBounds.Center);
//    XMVECTOR lightUp = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
//    XMMATRIX lightView = XMMatrixLookAtLH(lightPos, targetPos, lightUp);
//
//    XMStoreFloat3(&mLightPosW, lightPos);
//
//    // Transform bounding sphere to light space.
//    // 将包围球变换到光源空间
//    XMFLOAT3 sphereCenterLS;
//    XMStoreFloat3(&sphereCenterLS, XMVector3TransformCoord(targetPos, lightView));
//
//    // Ortho frustum in light space encloses scene.
//    // 位于光源空间中包围场景的正交投影视景体
//    float l = sphereCenterLS.x - mSceneBounds.Radius;
//    float b = sphereCenterLS.y - mSceneBounds.Radius;
//    float n = sphereCenterLS.z - mSceneBounds.Radius;
//    float r = sphereCenterLS.x + mSceneBounds.Radius;
//    float t = sphereCenterLS.y + mSceneBounds.Radius;
//    float f = sphereCenterLS.z + mSceneBounds.Radius;
//
//    mLightNearZ = n;
//    mLightFarZ = f;
//    XMMATRIX lightProj = XMMatrixOrthographicOffCenterLH(l, r, b, t, n, f);
//
//    // Transform NDC space [-1,+1]^2 to texture space [0,1]^2
//    // 将坐标从范围为[-1,+1]^2的NDC空间变换到范围为[0,1]^2的纹理空间
//    XMMATRIX T(
//        0.5f, 0.0f, 0.0f, 0.0f,
//        0.0f, -0.5f, 0.0f, 0.0f,
//        0.0f, 0.0f, 1.0f, 0.0f,
//        0.5f, 0.5f, 0.0f, 1.0f);
//
//    XMMATRIX S = lightView*lightProj*T;
//    XMStoreFloat4x4(&mLightView, lightView);
//    XMStoreFloat4x4(&mLightProj, lightProj);
//    XMStoreFloat4x4(&mShadowTransform, S);
//}
//
//void ShadowMapApp::UpdateMainPassCB(const GameTimer& gt)
//{
//	XMMATRIX view = mCamera.GetView();
//	XMMATRIX proj = mCamera.GetProj();
//
//	XMMATRIX viewProj = XMMatrixMultiply(view, proj);
//	XMMATRIX invView = XMMatrixInverse(&XMMatrixDeterminant(view), view);
//	XMMATRIX invProj = XMMatrixInverse(&XMMatrixDeterminant(proj), proj);
//	XMMATRIX invViewProj = XMMatrixInverse(&XMMatrixDeterminant(viewProj), viewProj);
//
//    XMMATRIX shadowTransform = XMLoadFloat4x4(&mShadowTransform);
//
//	XMStoreFloat4x4(&mMainPassCB.View, XMMatrixTranspose(view));
//	XMStoreFloat4x4(&mMainPassCB.InvView, XMMatrixTranspose(invView));
//	XMStoreFloat4x4(&mMainPassCB.Proj, XMMatrixTranspose(proj));
//	XMStoreFloat4x4(&mMainPassCB.InvProj, XMMatrixTranspose(invProj));
//	XMStoreFloat4x4(&mMainPassCB.ViewProj, XMMatrixTranspose(viewProj));
//	XMStoreFloat4x4(&mMainPassCB.InvViewProj, XMMatrixTranspose(invViewProj));
//    XMStoreFloat4x4(&mMainPassCB.ShadowTransform, XMMatrixTranspose(shadowTransform));
//	mMainPassCB.EyePosW = mCamera.GetPosition3f();
//	mMainPassCB.RenderTargetSize = XMFLOAT2((float)mClientWidth, (float)mClientHeight);
//	mMainPassCB.InvRenderTargetSize = XMFLOAT2(1.0f / mClientWidth, 1.0f / mClientHeight);
//	mMainPassCB.NearZ = 1.0f;
//	mMainPassCB.FarZ = 1000.0f;
//	mMainPassCB.TotalTime = gt.TotalTime();
//	mMainPassCB.DeltaTime = gt.DeltaTime();
//	mMainPassCB.AmbientLight = { 0.25f, 0.25f, 0.35f, 1.0f };
//	mMainPassCB.Lights[0].Direction = mRotatedLightDirections[0];
//	mMainPassCB.Lights[0].Strength = { 0.9f, 0.8f, 0.7f };
//	mMainPassCB.Lights[1].Direction = mRotatedLightDirections[1];
//	mMainPassCB.Lights[1].Strength = { 0.4f, 0.4f, 0.4f };
//	mMainPassCB.Lights[2].Direction = mRotatedLightDirections[2];
//	mMainPassCB.Lights[2].Strength = { 0.2f, 0.2f, 0.2f };
// 
//	auto currPassCB = mCurrFrameResource->PassCB.get();
//	currPassCB->CopyData(0, mMainPassCB);
//}
//
//void ShadowMapApp::UpdateShadowPassCB(const GameTimer& gt)
//{
//    XMMATRIX view = XMLoadFloat4x4(&mLightView);
//    XMMATRIX proj = XMLoadFloat4x4(&mLightProj);
//
//    XMMATRIX viewProj = XMMatrixMultiply(view, proj);
//    XMMATRIX invView = XMMatrixInverse(&XMMatrixDeterminant(view), view);
//    XMMATRIX invProj = XMMatrixInverse(&XMMatrixDeterminant(proj), proj);
//    XMMATRIX invViewProj = XMMatrixInverse(&XMMatrixDeterminant(viewProj), viewProj);
//
//    UINT w = mShadowMap->Width();
//    UINT h = mShadowMap->Height();
//
//    XMStoreFloat4x4(&mShadowPassCB.View, XMMatrixTranspose(view));
//    XMStoreFloat4x4(&mShadowPassCB.InvView, XMMatrixTranspose(invView));
//    XMStoreFloat4x4(&mShadowPassCB.Proj, XMMatrixTranspose(proj));
//    XMStoreFloat4x4(&mShadowPassCB.InvProj, XMMatrixTranspose(invProj));
//    XMStoreFloat4x4(&mShadowPassCB.ViewProj, XMMatrixTranspose(viewProj));
//    XMStoreFloat4x4(&mShadowPassCB.InvViewProj, XMMatrixTranspose(invViewProj));
//    mShadowPassCB.EyePosW = mLightPosW;
//    mShadowPassCB.RenderTargetSize = XMFLOAT2((float)w, (float)h);
//    mShadowPassCB.InvRenderTargetSize = XMFLOAT2(1.0f / w, 1.0f / h);
//    mShadowPassCB.NearZ = mLightNearZ;
//    mShadowPassCB.FarZ = mLightFarZ;
//
//    auto currPassCB = mCurrFrameResource->PassCB.get();
//    currPassCB->CopyData(1, mShadowPassCB);
//}
//
//void ShadowMapApp::LoadTextures()
//{
//	std::vector<std::string> texNames = 
//	{
//		"bricksDiffuseMap",
//		"bricksNormalMap",
//		"tileDiffuseMap",
//		"tileNormalMap",
//		"defaultDiffuseMap",
//		"defaultNormalMap",
//		"skyCubeMap"
//	};
//	
//    std::vector<std::wstring> texFilenames =
//    {
//        L"E:/DX12Book/DX12LearnProject/DX12Learn/Textures/bricks2.dds",
//        L"E:/DX12Book/DX12LearnProject/DX12Learn/Textures/bricks2_nmap.dds",
//        L"E:/DX12Book/DX12LearnProject/DX12Learn/Textures/tile.dds",
//        L"E:/DX12Book/DX12LearnProject/DX12Learn/Textures/tile_nmap.dds",
//        L"E:/DX12Book/DX12LearnProject/DX12Learn/Textures/white1x1.dds",
//        L"E:/DX12Book/DX12LearnProject/DX12Learn/Textures/default_nmap.dds",
//        L"E:/DX12Book/DX12LearnProject/DX12Learn/Textures/desertcube1024.dds"
//    };
//	
//	for(int i = 0; i < (int)texNames.size(); ++i)
//	{
//		auto texMap = std::make_unique<Texture>();
//		texMap->Name = texNames[i];
//		texMap->Filename = texFilenames[i];
//		ThrowIfFailed(DirectX::CreateDDSTextureFromFile12(md3dDevice.Get(),
//			mCommandList.Get(), texMap->Filename.c_str(),
//			texMap->Resource, texMap->UploadHeap));
//			
//		mTextures[texMap->Name] = std::move(texMap);
//	}		
//}
//
//void ShadowMapApp::BuildRootSignature()
//{
//	CD3DX12_DESCRIPTOR_RANGE texTable0;
//	texTable0.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 2, 0, 0);
//
//	CD3DX12_DESCRIPTOR_RANGE texTable1;
//	texTable1.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 10, 2, 0);
//
//    // Root parameter can be a table, root descriptor or root constants.
//    CD3DX12_ROOT_PARAMETER slotRootParameter[5];
//
//	// Perfomance TIP: Order from most frequent to least frequent.
//    slotRootParameter[0].InitAsConstantBufferView(0);
//    slotRootParameter[1].InitAsConstantBufferView(1);
//    slotRootParameter[2].InitAsShaderResourceView(0, 1);
//	slotRootParameter[3].InitAsDescriptorTable(1, &texTable0, D3D12_SHADER_VISIBILITY_PIXEL);
//	slotRootParameter[4].InitAsDescriptorTable(1, &texTable1, D3D12_SHADER_VISIBILITY_PIXEL);
//
//
//	auto staticSamplers = GetStaticSamplers();
//
//    // A root signature is an array of root parameters.
//	CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(5, slotRootParameter,
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
//void ShadowMapApp::BuildDescriptorHeaps()
//{
//	//
//	// Create the SRV heap.
//	//
//	D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {};
//	srvHeapDesc.NumDescriptors = 14;
//	srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
//	srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
//	ThrowIfFailed(md3dDevice->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&mSrvDescriptorHeap)));
//
//	//
//	// Fill out the heap with actual descriptors.
//	//
//	CD3DX12_CPU_DESCRIPTOR_HANDLE hDescriptor(mSrvDescriptorHeap->GetCPUDescriptorHandleForHeapStart());
//
//	std::vector<ComPtr<ID3D12Resource>> tex2DList = 
//	{
//		mTextures["bricksDiffuseMap"]->Resource,
//		mTextures["bricksNormalMap"]->Resource,
//		mTextures["tileDiffuseMap"]->Resource,
//		mTextures["tileNormalMap"]->Resource,
//		mTextures["defaultDiffuseMap"]->Resource,
//		mTextures["defaultNormalMap"]->Resource
//	};
//	
//	auto skyCubeMap = mTextures["skyCubeMap"]->Resource;
//
//	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
//	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
//	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
//	srvDesc.Texture2D.MostDetailedMip = 0;
//	srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;
//	
//	for(UINT i = 0; i < (UINT)tex2DList.size(); ++i)
//	{
//		srvDesc.Format = tex2DList[i]->GetDesc().Format;
//		srvDesc.Texture2D.MipLevels = tex2DList[i]->GetDesc().MipLevels;
//		md3dDevice->CreateShaderResourceView(tex2DList[i].Get(), &srvDesc, hDescriptor);
//
//		// next descriptor
//		hDescriptor.Offset(1, mCbvSrvUavDescriptorSize);
//	}
//	
//	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
//	srvDesc.TextureCube.MostDetailedMip = 0;
//	srvDesc.TextureCube.MipLevels = skyCubeMap->GetDesc().MipLevels;
//	srvDesc.TextureCube.ResourceMinLODClamp = 0.0f;
//	srvDesc.Format = skyCubeMap->GetDesc().Format;
//	md3dDevice->CreateShaderResourceView(skyCubeMap.Get(), &srvDesc, hDescriptor);
//	
//	mSkyTexHeapIndex = (UINT)tex2DList.size();
//    mShadowMapHeapIndex = mSkyTexHeapIndex + 1;
//
//    mNullCubeSrvIndex = mShadowMapHeapIndex + 1;
//    mNullTexSrvIndex = mNullCubeSrvIndex + 1;
//
//    auto srvCpuStart = mSrvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
//    auto srvGpuStart = mSrvDescriptorHeap->GetGPUDescriptorHandleForHeapStart();
//    auto dsvCpuStart = mDsvHeap->GetCPUDescriptorHandleForHeapStart();
//
//
//    auto nullSrv = CD3DX12_CPU_DESCRIPTOR_HANDLE(srvCpuStart, mNullCubeSrvIndex, mCbvSrvUavDescriptorSize);
//    mNullSrv = CD3DX12_GPU_DESCRIPTOR_HANDLE(srvGpuStart, mNullCubeSrvIndex, mCbvSrvUavDescriptorSize);
//
//    md3dDevice->CreateShaderResourceView(nullptr, &srvDesc, nullSrv);
//    nullSrv.Offset(1, mCbvSrvUavDescriptorSize);
//
//    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
//    srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
//    srvDesc.Texture2D.MostDetailedMip = 0;
//    srvDesc.Texture2D.MipLevels = 1;
//    srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;
//    md3dDevice->CreateShaderResourceView(nullptr, &srvDesc, nullSrv);
//    
//    mShadowMap->BuildDescriptors(
//        CD3DX12_CPU_DESCRIPTOR_HANDLE(srvCpuStart, mShadowMapHeapIndex, mCbvSrvUavDescriptorSize),
//        CD3DX12_GPU_DESCRIPTOR_HANDLE(srvGpuStart, mShadowMapHeapIndex, mCbvSrvUavDescriptorSize),
//        CD3DX12_CPU_DESCRIPTOR_HANDLE(dsvCpuStart, 1, mDsvDescriptorSize));
//}
//
//void ShadowMapApp::BuildShadersAndInputLayout()
//{
//	const D3D_SHADER_MACRO alphaTestDefines[] =
//	{
//		"ALPHA_TEST", "1",
//		NULL, NULL
//	};
//
//	mShaders["standardVS"] = d3dUtil::CompileShader(L"E:\\DX12Book\\DX12LearnProject\\DX12Learn\\LearnDemo\\Chapter 20 Shadow Mapping\\Shadows\\Shaders\\Default.hlsl", nullptr, "VS", "vs_5_1");
//	mShaders["opaquePS"] = d3dUtil::CompileShader(L"E:\\DX12Book\\DX12LearnProject\\DX12Learn\\LearnDemo\\Chapter 20 Shadow Mapping\\Shadows\\Shaders\\Default.hlsl", nullptr, "PS", "ps_5_1");
//
//    mShaders["shadowVS"] = d3dUtil::CompileShader(L"E:\\DX12Book\\DX12LearnProject\\DX12Learn\\LearnDemo\\Chapter 20 Shadow Mapping\\Shadows\\Shaders\\Shadows.hlsl", nullptr, "VS", "vs_5_1");
//    mShaders["shadowOpaquePS"] = d3dUtil::CompileShader(L"E:\\DX12Book\\DX12LearnProject\\DX12Learn\\LearnDemo\\Chapter 20 Shadow Mapping\\Shadows\\Shaders\\Shadows.hlsl", nullptr, "PS", "ps_5_1");
//    mShaders["shadowAlphaTestedPS"] = d3dUtil::CompileShader(L"E:\\DX12Book\\DX12LearnProject\\DX12Learn\\LearnDemo\\Chapter 20 Shadow Mapping\\Shadows\\Shaders\\Shadows.hlsl", alphaTestDefines, "PS", "ps_5_1");
//	
//    mShaders["debugVS"] = d3dUtil::CompileShader(L"E:\\DX12Book\\DX12LearnProject\\DX12Learn\\LearnDemo\\Chapter 20 Shadow Mapping\\Shadows\\Shaders\\ShadowDebug.hlsl", nullptr, "VS", "vs_5_1");
//    mShaders["debugPS"] = d3dUtil::CompileShader(L"E:\\DX12Book\\DX12LearnProject\\DX12Learn\\LearnDemo\\Chapter 20 Shadow Mapping\\Shadows\\Shaders\\ShadowDebug.hlsl", nullptr, "PS", "ps_5_1");
//
//	mShaders["skyVS"] = d3dUtil::CompileShader(L"E:\\DX12Book\\DX12LearnProject\\DX12Learn\\LearnDemo\\Chapter 20 Shadow Mapping\\Shadows\\Shaders\\Sky.hlsl", nullptr, "VS", "vs_5_1");
//	mShaders["skyPS"] = d3dUtil::CompileShader(L"E:\\DX12Book\\DX12LearnProject\\DX12Learn\\LearnDemo\\Chapter 20 Shadow Mapping\\Shadows\\Shaders\\Sky.hlsl", nullptr, "PS", "ps_5_1");
//
//    mInputLayout =
//    {
//        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
//        { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
//		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
//		{ "TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 32, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
//    };
//}
//
//void ShadowMapApp::BuildShapeGeometry()
//{
//    GeometryGenerator geoGen;
//	GeometryGenerator::MeshData box = geoGen.CreateBox(1.0f, 1.0f, 1.0f, 3);
//	GeometryGenerator::MeshData grid = geoGen.CreateGrid(20.0f, 30.0f, 60, 40);
//	GeometryGenerator::MeshData sphere = geoGen.CreateSphere(0.5f, 20, 20);
//	GeometryGenerator::MeshData cylinder = geoGen.CreateCylinder(0.5f, 0.3f, 3.0f, 20, 20);
//    GeometryGenerator::MeshData quad = geoGen.CreateQuad(0.0f, 0.0f, 1.0f, 1.0f, 0.0f);
//    
//	//
//	// We are concatenating all the geometry into one big vertex/index buffer.  So
//	// define the regions in the buffer each submesh covers.
//	//
//
//	// Cache the vertex offsets to each object in the concatenated vertex buffer.
//	UINT boxVertexOffset = 0;
//	UINT gridVertexOffset = (UINT)box.Vertices.size();
//	UINT sphereVertexOffset = gridVertexOffset + (UINT)grid.Vertices.size();
//	UINT cylinderVertexOffset = sphereVertexOffset + (UINT)sphere.Vertices.size();
//    UINT quadVertexOffset = cylinderVertexOffset + (UINT)cylinder.Vertices.size();
//
//	// Cache the starting index for each object in the concatenated index buffer.
//	UINT boxIndexOffset = 0;
//	UINT gridIndexOffset = (UINT)box.Indices32.size();
//	UINT sphereIndexOffset = gridIndexOffset + (UINT)grid.Indices32.size();
//	UINT cylinderIndexOffset = sphereIndexOffset + (UINT)sphere.Indices32.size();
//    UINT quadIndexOffset = cylinderIndexOffset + (UINT)cylinder.Indices32.size();
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
//    SubmeshGeometry quadSubmesh;
//    quadSubmesh.IndexCount = (UINT)quad.Indices32.size();
//    quadSubmesh.StartIndexLocation = quadIndexOffset;
//    quadSubmesh.BaseVertexLocation = quadVertexOffset;
//
//	//
//	// Extract the vertex elements we are interested in and pack the
//	// vertices of all the meshes into one vertex buffer.
//	//
//
//	auto totalVertexCount =
//		box.Vertices.size() +
//		grid.Vertices.size() +
//		sphere.Vertices.size() +
//		cylinder.Vertices.size() + 
//        quad.Vertices.size();
//
//	std::vector<Vertex> vertices(totalVertexCount);
//
//	UINT k = 0;
//	for(size_t i = 0; i < box.Vertices.size(); ++i, ++k)
//	{
//		vertices[k].Pos = box.Vertices[i].Position;
//		vertices[k].Normal = box.Vertices[i].Normal;
//		vertices[k].TexC = box.Vertices[i].TexC;
//		vertices[k].TangentU = box.Vertices[i].TangentU;
//	}
//
//	for(size_t i = 0; i < grid.Vertices.size(); ++i, ++k)
//	{
//		vertices[k].Pos = grid.Vertices[i].Position;
//		vertices[k].Normal = grid.Vertices[i].Normal;
//		vertices[k].TexC = grid.Vertices[i].TexC;
//		vertices[k].TangentU = grid.Vertices[i].TangentU;
//	}
//
//	for(size_t i = 0; i < sphere.Vertices.size(); ++i, ++k)
//	{
//		vertices[k].Pos = sphere.Vertices[i].Position;
//		vertices[k].Normal = sphere.Vertices[i].Normal;
//		vertices[k].TexC = sphere.Vertices[i].TexC;
//		vertices[k].TangentU = sphere.Vertices[i].TangentU;
//	}
//
//	for(size_t i = 0; i < cylinder.Vertices.size(); ++i, ++k)
//	{
//		vertices[k].Pos = cylinder.Vertices[i].Position;
//		vertices[k].Normal = cylinder.Vertices[i].Normal;
//		vertices[k].TexC = cylinder.Vertices[i].TexC;
//		vertices[k].TangentU = cylinder.Vertices[i].TangentU;
//	}
//
//    for(int i = 0; i < quad.Vertices.size(); ++i, ++k)
//    {
//        vertices[k].Pos = quad.Vertices[i].Position;
//        vertices[k].Normal = quad.Vertices[i].Normal;
//        vertices[k].TexC = quad.Vertices[i].TexC;
//        vertices[k].TangentU = quad.Vertices[i].TangentU;
//    }
//
//	std::vector<std::uint16_t> indices;
//	indices.insert(indices.end(), std::begin(box.GetIndices16()), std::end(box.GetIndices16()));
//	indices.insert(indices.end(), std::begin(grid.GetIndices16()), std::end(grid.GetIndices16()));
//	indices.insert(indices.end(), std::begin(sphere.GetIndices16()), std::end(sphere.GetIndices16()));
//	indices.insert(indices.end(), std::begin(cylinder.GetIndices16()), std::end(cylinder.GetIndices16()));
//    indices.insert(indices.end(), std::begin(quad.GetIndices16()), std::end(quad.GetIndices16()));
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
//    geo->DrawArgs["quad"] = quadSubmesh;
//
//	mGeometries[geo->Name] = std::move(geo);
//}
//
//void ShadowMapApp::BuildSkullGeometry()
//{
//    std::ifstream fin("E:/DX12Book/DX12LearnProject/DX12Learn/LearnDemo/Chapter 20 Shadow Mapping/Shadows/Models/skull.txt");
//
//    if (!fin)
//    {
//        MessageBox(0, L"E:/DX12Book/DX12LearnProject/DX12Learn/LearnDemo/Chapter 20 Shadow Mapping/Shadows/Models/skull.txt not found.", 0, 0);
//        return;
//    }
//
//    UINT vcount = 0;
//    UINT tcount = 0;
//    std::string ignore;
//
//    fin >> ignore >> vcount;
//    fin >> ignore >> tcount;
//    fin >> ignore >> ignore >> ignore >> ignore;
//
//    XMFLOAT3 vMinf3(+MathHelper::Infinity, +MathHelper::Infinity, +MathHelper::Infinity);
//    XMFLOAT3 vMaxf3(-MathHelper::Infinity, -MathHelper::Infinity, -MathHelper::Infinity);
//
//    XMVECTOR vMin = XMLoadFloat3(&vMinf3);
//    XMVECTOR vMax = XMLoadFloat3(&vMaxf3);
//
//    std::vector<Vertex> vertices(vcount);
//    for (UINT i = 0; i < vcount; ++i)
//    {
//        fin >> vertices[i].Pos.x >> vertices[i].Pos.y >> vertices[i].Pos.z;
//        fin >> vertices[i].Normal.x >> vertices[i].Normal.y >> vertices[i].Normal.z;
//
//        vertices[i].TexC = { 0.0f, 0.0f };
//
//        XMVECTOR P = XMLoadFloat3(&vertices[i].Pos);
//
//        XMVECTOR N = XMLoadFloat3(&vertices[i].Normal);
//
//        // Generate a tangent vector so normal mapping works.  We aren't applying
//        // a texture map to the skull, so we just need any tangent vector so that
//        // the math works out to give us the original interpolated vertex normal.
//        XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
//        if(fabsf(XMVectorGetX(XMVector3Dot(N, up))) < 1.0f - 0.001f)
//        {
//            XMVECTOR T = XMVector3Normalize(XMVector3Cross(up, N));
//            XMStoreFloat3(&vertices[i].TangentU, T);
//        }
//        else
//        {
//            up = XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);
//            XMVECTOR T = XMVector3Normalize(XMVector3Cross(N, up));
//            XMStoreFloat3(&vertices[i].TangentU, T);
//        }
//        
//
//        vMin = XMVectorMin(vMin, P);
//        vMax = XMVectorMax(vMax, P);
//    }
//
//    BoundingBox bounds;
//    XMStoreFloat3(&bounds.Center, 0.5f*(vMin + vMax));
//    XMStoreFloat3(&bounds.Extents, 0.5f*(vMax - vMin));
//
//    fin >> ignore;
//    fin >> ignore;
//    fin >> ignore;
//
//    std::vector<std::int32_t> indices(3 * tcount);
//    for (UINT i = 0; i < tcount; ++i)
//    {
//        fin >> indices[i * 3 + 0] >> indices[i * 3 + 1] >> indices[i * 3 + 2];
//    }
//
//    fin.close();
//
//    //
//    // Pack the indices of all the meshes into one index buffer.
//    //
//
//    const UINT vbByteSize = (UINT)vertices.size() * sizeof(Vertex);
//
//    const UINT ibByteSize = (UINT)indices.size() * sizeof(std::int32_t);
//
//    auto geo = std::make_unique<MeshGeometry>();
//    geo->Name = "skullGeo";
//
//    ThrowIfFailed(D3DCreateBlob(vbByteSize, &geo->VertexBufferCPU));
//    CopyMemory(geo->VertexBufferCPU->GetBufferPointer(), vertices.data(), vbByteSize);
//
//    ThrowIfFailed(D3DCreateBlob(ibByteSize, &geo->IndexBufferCPU));
//    CopyMemory(geo->IndexBufferCPU->GetBufferPointer(), indices.data(), ibByteSize);
//
//    geo->VertexBufferGPU = d3dUtil::CreateDefaultBuffer(md3dDevice.Get(),
//        mCommandList.Get(), vertices.data(), vbByteSize, geo->VertexBufferUploader);
//
//    geo->IndexBufferGPU = d3dUtil::CreateDefaultBuffer(md3dDevice.Get(),
//        mCommandList.Get(), indices.data(), ibByteSize, geo->IndexBufferUploader);
//
//    geo->VertexByteStride = sizeof(Vertex);
//    geo->VertexBufferByteSize = vbByteSize;
//    geo->IndexFormat = DXGI_FORMAT_R32_UINT;
//    geo->IndexBufferByteSize = ibByteSize;
//
//    SubmeshGeometry submesh;
//    submesh.IndexCount = (UINT)indices.size();
//    submesh.StartIndexLocation = 0;
//    submesh.BaseVertexLocation = 0;
//    submesh.Bounds = bounds;
//
//    geo->DrawArgs["skull"] = submesh;
//
//    mGeometries[geo->Name] = std::move(geo);
//}
//
//void ShadowMapApp::BuildPSOs()
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
//    //
//    // PSO for shadow map pass.
//    //
//    D3D12_GRAPHICS_PIPELINE_STATE_DESC smapPsoDesc = opaquePsoDesc;
//    smapPsoDesc.RasterizerState.DepthBias = 100000;
//    smapPsoDesc.RasterizerState.DepthBiasClamp = 0.0f;
//    smapPsoDesc.RasterizerState.SlopeScaledDepthBias = 1.0f;
//    smapPsoDesc.pRootSignature = mRootSignature.Get();
//    smapPsoDesc.VS =
//    {
//        reinterpret_cast<BYTE*>(mShaders["shadowVS"]->GetBufferPointer()),
//        mShaders["shadowVS"]->GetBufferSize()
//    };
//    smapPsoDesc.PS =
//    {
//        reinterpret_cast<BYTE*>(mShaders["shadowOpaquePS"]->GetBufferPointer()),
//        mShaders["shadowOpaquePS"]->GetBufferSize()
//    };
//    
//    // Shadow map pass does not have a render target.
//    smapPsoDesc.RTVFormats[0] = DXGI_FORMAT_UNKNOWN;
//    smapPsoDesc.NumRenderTargets = 0;
//    ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&smapPsoDesc, IID_PPV_ARGS(&mPSOs["shadow_opaque"])));
//
//    //
//    // PSO for debug layer.
//    //
//    D3D12_GRAPHICS_PIPELINE_STATE_DESC debugPsoDesc = opaquePsoDesc;
//    debugPsoDesc.pRootSignature = mRootSignature.Get();
//    debugPsoDesc.VS =
//    {
//        reinterpret_cast<BYTE*>(mShaders["debugVS"]->GetBufferPointer()),
//        mShaders["debugVS"]->GetBufferSize()
//    };
//    debugPsoDesc.PS =
//    {
//        reinterpret_cast<BYTE*>(mShaders["debugPS"]->GetBufferPointer()),
//        mShaders["debugPS"]->GetBufferSize()
//    };
//    ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&debugPsoDesc, IID_PPV_ARGS(&mPSOs["debug"])));
//
//	//
//	// PSO for sky.
//	//
//	D3D12_GRAPHICS_PIPELINE_STATE_DESC skyPsoDesc = opaquePsoDesc;
//
//	// The camera is inside the sky sphere, so just turn off culling.
//	skyPsoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
//
//	// Make sure the depth function is LESS_EQUAL and not just LESS.  
//	// Otherwise, the normalized depth values at z = 1 (NDC) will 
//	// fail the depth test if the depth buffer was cleared to 1.
//	skyPsoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
//	skyPsoDesc.pRootSignature = mRootSignature.Get();
//	skyPsoDesc.VS =
//	{
//		reinterpret_cast<BYTE*>(mShaders["skyVS"]->GetBufferPointer()),
//		mShaders["skyVS"]->GetBufferSize()
//	};
//	skyPsoDesc.PS =
//	{
//		reinterpret_cast<BYTE*>(mShaders["skyPS"]->GetBufferPointer()),
//		mShaders["skyPS"]->GetBufferSize()
//	};
//	ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&skyPsoDesc, IID_PPV_ARGS(&mPSOs["sky"])));
//
//}
//
//void ShadowMapApp::BuildFrameResources()
//{
//    for(int i = 0; i < gNumFrameResources; ++i)
//    {
//        mFrameResources.push_back(std::make_unique<ShadowFrameResource>(md3dDevice.Get(),
//            2, (UINT)mAllRitems.size(), (UINT)mMaterials.size()));
//    }
//}
//
//void ShadowMapApp::BuildMaterials()
//{
//    auto bricks0 = std::make_unique<Material>();
//    bricks0->Name = "bricks0";
//    bricks0->MatCBIndex = 0;
//    bricks0->DiffuseSrvHeapIndex = 0;
//    bricks0->NormalSrvHeapIndex = 1;
//    bricks0->DiffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
//    bricks0->FresnelR0 = XMFLOAT3(0.1f, 0.1f, 0.1f);
//    bricks0->Roughness = 0.3f;
//
//    auto tile0 = std::make_unique<Material>();
//    tile0->Name = "tile0";
//    tile0->MatCBIndex = 1;
//    tile0->DiffuseSrvHeapIndex = 2;
//    tile0->NormalSrvHeapIndex = 3;
//    tile0->DiffuseAlbedo = XMFLOAT4(0.9f, 0.9f, 0.9f, 1.0f);
//    tile0->FresnelR0 = XMFLOAT3(0.2f, 0.2f, 0.2f);
//    tile0->Roughness = 0.1f;
//
//    auto mirror0 = std::make_unique<Material>();
//    mirror0->Name = "mirror0";
//    mirror0->MatCBIndex = 2;
//    mirror0->DiffuseSrvHeapIndex = 4;
//    mirror0->NormalSrvHeapIndex = 5;
//    mirror0->DiffuseAlbedo = XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);
//    mirror0->FresnelR0 = XMFLOAT3(0.98f, 0.97f, 0.95f);
//    mirror0->Roughness = 0.1f;
//
//    auto skullMat = std::make_unique<Material>();
//    skullMat->Name = "skullMat";
//    skullMat->MatCBIndex = 3;
//    skullMat->DiffuseSrvHeapIndex = 4;
//    skullMat->NormalSrvHeapIndex = 5;
//    skullMat->DiffuseAlbedo = XMFLOAT4(0.3f, 0.3f, 0.3f, 1.0f);
//    skullMat->FresnelR0 = XMFLOAT3(0.6f, 0.6f, 0.6f);
//    skullMat->Roughness = 0.2f;
//
//    auto sky = std::make_unique<Material>();
//    sky->Name = "sky";
//    sky->MatCBIndex = 4;
//    sky->DiffuseSrvHeapIndex = 6;
//    sky->NormalSrvHeapIndex = 7;
//    sky->DiffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
//    sky->FresnelR0 = XMFLOAT3(0.1f, 0.1f, 0.1f);
//    sky->Roughness = 1.0f;
//
//    mMaterials["bricks0"] = std::move(bricks0);
//    mMaterials["tile0"] = std::move(tile0);
//    mMaterials["mirror0"] = std::move(mirror0);
//    mMaterials["skullMat"] = std::move(skullMat);
//    mMaterials["sky"] = std::move(sky);
//}
//
//void ShadowMapApp::BuildRenderItems()
//{
//	auto skyRitem = std::make_unique<RenderItem>();
//	XMStoreFloat4x4(&skyRitem->World, XMMatrixScaling(5000.0f, 5000.0f, 5000.0f));
//	skyRitem->TexTransform = MathHelper::Identity4x4();
//	skyRitem->ObjCBIndex = 0;
//	skyRitem->Mat = mMaterials["sky"].get();
//	skyRitem->Geo = mGeometries["shapeGeo"].get();
//	skyRitem->PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
//	skyRitem->IndexCount = skyRitem->Geo->DrawArgs["sphere"].IndexCount;
//	skyRitem->StartIndexLocation = skyRitem->Geo->DrawArgs["sphere"].StartIndexLocation;
//	skyRitem->BaseVertexLocation = skyRitem->Geo->DrawArgs["sphere"].BaseVertexLocation;
//
//	mRitemLayer[(int)RenderLayer::Sky].push_back(skyRitem.get());
//	mAllRitems.push_back(std::move(skyRitem));
//    
//    auto quadRitem = std::make_unique<RenderItem>();
//    quadRitem->World = MathHelper::Identity4x4();
//    quadRitem->TexTransform = MathHelper::Identity4x4();
//    quadRitem->ObjCBIndex = 1;
//    quadRitem->Mat = mMaterials["bricks0"].get();
//    quadRitem->Geo = mGeometries["shapeGeo"].get();
//    quadRitem->PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
//    quadRitem->IndexCount = quadRitem->Geo->DrawArgs["quad"].IndexCount;
//    quadRitem->StartIndexLocation = quadRitem->Geo->DrawArgs["quad"].StartIndexLocation;
//    quadRitem->BaseVertexLocation = quadRitem->Geo->DrawArgs["quad"].BaseVertexLocation;
//
//    mRitemLayer[(int)RenderLayer::Debug].push_back(quadRitem.get());
//    mAllRitems.push_back(std::move(quadRitem));
//    
//	auto boxRitem = std::make_unique<RenderItem>();
//	XMStoreFloat4x4(&boxRitem->World, XMMatrixScaling(2.0f, 1.0f, 2.0f)*XMMatrixTranslation(0.0f, 0.5f, 0.0f));
//	XMStoreFloat4x4(&boxRitem->TexTransform, XMMatrixScaling(1.0f, 0.5f, 1.0f));
//	boxRitem->ObjCBIndex = 2;
//	boxRitem->Mat = mMaterials["bricks0"].get();
//	boxRitem->Geo = mGeometries["shapeGeo"].get();
//	boxRitem->PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
//	boxRitem->IndexCount = boxRitem->Geo->DrawArgs["box"].IndexCount;
//	boxRitem->StartIndexLocation = boxRitem->Geo->DrawArgs["box"].StartIndexLocation;
//	boxRitem->BaseVertexLocation = boxRitem->Geo->DrawArgs["box"].BaseVertexLocation;
//
//	mRitemLayer[(int)RenderLayer::Opaque].push_back(boxRitem.get());
//	mAllRitems.push_back(std::move(boxRitem));
//
//    auto skullRitem = std::make_unique<RenderItem>();
//    XMStoreFloat4x4(&skullRitem->World, XMMatrixScaling(0.4f, 0.4f, 0.4f)*XMMatrixTranslation(0.0f, 1.0f, 0.0f));
//    skullRitem->TexTransform = MathHelper::Identity4x4();
//    skullRitem->ObjCBIndex = 3;
//    skullRitem->Mat = mMaterials["skullMat"].get();
//    skullRitem->Geo = mGeometries["skullGeo"].get();
//    skullRitem->PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
//    skullRitem->IndexCount = skullRitem->Geo->DrawArgs["skull"].IndexCount;
//    skullRitem->StartIndexLocation = skullRitem->Geo->DrawArgs["skull"].StartIndexLocation;
//    skullRitem->BaseVertexLocation = skullRitem->Geo->DrawArgs["skull"].BaseVertexLocation;
//
//    mRitemLayer[(int)RenderLayer::Opaque].push_back(skullRitem.get());
//    mAllRitems.push_back(std::move(skullRitem));
//
//    auto gridRitem = std::make_unique<RenderItem>();
//    gridRitem->World = MathHelper::Identity4x4();
//	XMStoreFloat4x4(&gridRitem->TexTransform, XMMatrixScaling(8.0f, 8.0f, 1.0f));
//	gridRitem->ObjCBIndex = 4;
//	gridRitem->Mat = mMaterials["tile0"].get();
//	gridRitem->Geo = mGeometries["shapeGeo"].get();
//	gridRitem->PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
//    gridRitem->IndexCount = gridRitem->Geo->DrawArgs["grid"].IndexCount;
//    gridRitem->StartIndexLocation = gridRitem->Geo->DrawArgs["grid"].StartIndexLocation;
//    gridRitem->BaseVertexLocation = gridRitem->Geo->DrawArgs["grid"].BaseVertexLocation;
//
//	mRitemLayer[(int)RenderLayer::Opaque].push_back(gridRitem.get());
//	mAllRitems.push_back(std::move(gridRitem));
//
//	XMMATRIX brickTexTransform = XMMatrixScaling(1.5f, 2.0f, 1.0f);
//	UINT objCBIndex = 5;
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
//		XMStoreFloat4x4(&leftCylRitem->TexTransform, brickTexTransform);
//		leftCylRitem->ObjCBIndex = objCBIndex++;
//		leftCylRitem->Mat = mMaterials["bricks0"].get();
//		leftCylRitem->Geo = mGeometries["shapeGeo"].get();
//		leftCylRitem->PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
//		leftCylRitem->IndexCount = leftCylRitem->Geo->DrawArgs["cylinder"].IndexCount;
//		leftCylRitem->StartIndexLocation = leftCylRitem->Geo->DrawArgs["cylinder"].StartIndexLocation;
//		leftCylRitem->BaseVertexLocation = leftCylRitem->Geo->DrawArgs["cylinder"].BaseVertexLocation;
//
//		XMStoreFloat4x4(&rightCylRitem->World, leftCylWorld);
//		XMStoreFloat4x4(&rightCylRitem->TexTransform, brickTexTransform);
//		rightCylRitem->ObjCBIndex = objCBIndex++;
//		rightCylRitem->Mat = mMaterials["bricks0"].get();
//		rightCylRitem->Geo = mGeometries["shapeGeo"].get();
//		rightCylRitem->PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
//		rightCylRitem->IndexCount = rightCylRitem->Geo->DrawArgs["cylinder"].IndexCount;
//		rightCylRitem->StartIndexLocation = rightCylRitem->Geo->DrawArgs["cylinder"].StartIndexLocation;
//		rightCylRitem->BaseVertexLocation = rightCylRitem->Geo->DrawArgs["cylinder"].BaseVertexLocation;
//
//		XMStoreFloat4x4(&leftSphereRitem->World, leftSphereWorld);
//		leftSphereRitem->TexTransform = MathHelper::Identity4x4();
//		leftSphereRitem->ObjCBIndex = objCBIndex++;
//		leftSphereRitem->Mat = mMaterials["mirror0"].get();
//		leftSphereRitem->Geo = mGeometries["shapeGeo"].get();
//		leftSphereRitem->PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
//		leftSphereRitem->IndexCount = leftSphereRitem->Geo->DrawArgs["sphere"].IndexCount;
//		leftSphereRitem->StartIndexLocation = leftSphereRitem->Geo->DrawArgs["sphere"].StartIndexLocation;
//		leftSphereRitem->BaseVertexLocation = leftSphereRitem->Geo->DrawArgs["sphere"].BaseVertexLocation;
//
//		XMStoreFloat4x4(&rightSphereRitem->World, rightSphereWorld);
//		rightSphereRitem->TexTransform = MathHelper::Identity4x4();
//		rightSphereRitem->ObjCBIndex = objCBIndex++;
//		rightSphereRitem->Mat = mMaterials["mirror0"].get();
//		rightSphereRitem->Geo = mGeometries["shapeGeo"].get();
//		rightSphereRitem->PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
//		rightSphereRitem->IndexCount = rightSphereRitem->Geo->DrawArgs["sphere"].IndexCount;
//		rightSphereRitem->StartIndexLocation = rightSphereRitem->Geo->DrawArgs["sphere"].StartIndexLocation;
//		rightSphereRitem->BaseVertexLocation = rightSphereRitem->Geo->DrawArgs["sphere"].BaseVertexLocation;
//
//		mRitemLayer[(int)RenderLayer::Opaque].push_back(leftCylRitem.get());
//		mRitemLayer[(int)RenderLayer::Opaque].push_back(rightCylRitem.get());
//		mRitemLayer[(int)RenderLayer::Opaque].push_back(leftSphereRitem.get());
//		mRitemLayer[(int)RenderLayer::Opaque].push_back(rightSphereRitem.get());
//
//		mAllRitems.push_back(std::move(leftCylRitem));
//		mAllRitems.push_back(std::move(rightCylRitem));
//		mAllRitems.push_back(std::move(leftSphereRitem));
//		mAllRitems.push_back(std::move(rightSphereRitem));
//	}
//}
//
//void ShadowMapApp::DrawRenderItems(ID3D12GraphicsCommandList* cmdList, const std::vector<RenderItem*>& ritems)
//{
//    UINT objCBByteSize = d3dUtil::CalcConstantBufferByteSize(sizeof(ObjectConstants));
// 
//	auto objectCB = mCurrFrameResource->ObjectCB->Resource();
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
//        D3D12_GPU_VIRTUAL_ADDRESS objCBAddress = objectCB->GetGPUVirtualAddress() + ri->ObjCBIndex*objCBByteSize;
//
//		cmdList->SetGraphicsRootConstantBufferView(0, objCBAddress);
//
//        cmdList->DrawIndexedInstanced(ri->IndexCount, 1, ri->StartIndexLocation, ri->BaseVertexLocation, 0);
//    }
//}
//
//void ShadowMapApp::DrawSceneToShadowMap()
//{
//    mCommandList->RSSetViewports(1, &mShadowMap->Viewport());
//    mCommandList->RSSetScissorRects(1, &mShadowMap->ScissorRect());
//
//    // Change to DEPTH_WRITE.
//    // 将资源状态改变为DEPTH_WRITE
//    mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mShadowMap->Resource(),
//        D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_DEPTH_WRITE));
//
//    UINT passCBByteSize = d3dUtil::CalcConstantBufferByteSize(sizeof(PassConstants));
//
//    // Clear the back buffer and depth buffer.
//    // 清理后台缓冲区以及深度缓冲区
//    mCommandList->ClearDepthStencilView(mShadowMap->Dsv(), 
//        D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);
//
//    // Set null render target because we are only going to draw to
//    // depth buffer.  Setting a null render target will disable color writes.
//    // Note the active PSO also must specify a render target count of 0.
//    // 由于仅向深度缓冲区绘制数据，因此将渲染目标设为空。这样一来将禁止颜色数据向渲染目标的写操作。
//    // 注意，此时也一定要把可用（处于启用状态）PSO中的渲染目标数量指定为0
//    mCommandList->OMSetRenderTargets(0, nullptr, false, &mShadowMap->Dsv());
//
//    // Bind the pass constant buffer for the shadow map pass.
//    // 为阴影图渲染过程绑定所需的常量缓冲区
//    auto passCB = mCurrFrameResource->PassCB->Resource();
//    D3D12_GPU_VIRTUAL_ADDRESS passCBAddress = passCB->GetGPUVirtualAddress() + 1*passCBByteSize;
//    mCommandList->SetGraphicsRootConstantBufferView(1, passCBAddress);
//
//    mCommandList->SetPipelineState(mPSOs["shadow_opaque"].Get());
//
//    DrawRenderItems(mCommandList.Get(), mRitemLayer[(int)RenderLayer::Opaque]);
//
//    // Change back to GENERIC_READ so we can read the texture in a shader.
//    // 将资源状态改变回GENERIC_READ，使我们能够从着色器中读取此纹理
//    mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mShadowMap->Resource(),
//        D3D12_RESOURCE_STATE_DEPTH_WRITE, D3D12_RESOURCE_STATE_GENERIC_READ));
//}
//
//std::array<const CD3DX12_STATIC_SAMPLER_DESC, 7> ShadowMapApp::GetStaticSamplers()
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
//    const CD3DX12_STATIC_SAMPLER_DESC shadow(
//        6, // shaderRegister
//        D3D12_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT, // filter
//        D3D12_TEXTURE_ADDRESS_MODE_BORDER,  // addressU
//        D3D12_TEXTURE_ADDRESS_MODE_BORDER,  // addressV
//        D3D12_TEXTURE_ADDRESS_MODE_BORDER,  // addressW
//        0.0f,                               // mipLODBias
//        16,                                 // maxAnisotropy
//        D3D12_COMPARISON_FUNC_LESS_EQUAL,
//        D3D12_STATIC_BORDER_COLOR_OPAQUE_BLACK);
//
//	return { 
//		pointWrap, pointClamp,
//		linearWrap, linearClamp, 
//		anisotropicWrap, anisotropicClamp,
//        shadow 
//    };
//}
//

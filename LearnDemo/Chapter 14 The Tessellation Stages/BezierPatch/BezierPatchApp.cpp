////***************************************************************************************
//// BezierPatchApp.cpp by Frank Luna (C) 2015 All Rights Reserved.
////***************************************************************************************
//
///*
//    曲面细分
//    
//    曲面细分的图元类型:
//        在进行曲面细分时，我们并不向IA（输入装配）阶段提交三角形，而是提交具有若干控制点（control point）的面片（patch）。
//        Direct3D支持具有1～32个控制点的面片，并以下列图元类型进行描述:
//        D3D_PRIMITIVE_TOPOLOGY_1_CONTROL_POINT_PATCHLIST = 33,
//        D3D_PRIMITIVE_TOPOLOGY_2_CONTROL_POINT_PATCHLIST = 34,
//        D3D_PRIMITIVE_TOPOLOGY_3_CONTROL_POINT_PATCHLIST = 35,
//        D3D_PRIMITIVE_TOPOLOGY_4_CONTROL_POINT_PATCHLIST = 36,
//        .
//        .
//        .
//        D3D_PRIMITIVE_TOPOLOGY_31_CONTROL_POINT_PATCHLIST = 63,
//        D3D_PRIMITIVE_TOPOLOGY_32_CONTROL_POINT_PATCHLIST = 64,
//        由于可以将三角形看作是拥有3个控制点的三角形面片（D3D_PRIMITIVE_3_CONTROL_POINT_PATCH），所以我们依然可以提交需要镶嵌化处理的普通三角形网格。
//        对于简单的四边形面片而言，则只需提交具有4个控制点的面片（D3D_PRIMITIVE_4_CONTROL_POINT_PATCH）即可。这些面片最终也会在曲面细分阶段经镶嵌化处理而分解为多个三角形[2]。
//    
//    曲面细分与顶点着色器:
//        在我们向渲染流水线提交了面片的控制点后，它们就会被推送至顶点着色器。这样一来，在开启曲面细分之时，顶点着色器就彻底沦陷为“处理控制点的着色器”。
//        正因如此，我们还能在曲面细分开展之前，对控制点进行一些调整。
//        一般来讲，动画与物理模拟的计算工作都会在对几何体进行镶嵌化处理之前的顶点着色器中以较低的频次进行（镶嵌化处理后，顶点增多，处理的频次也将随之增加）。
//    
//    外壳着色器:
//        在以下小节中，我们会探索外壳着色器（hull shader），它实际上是由两种着色器（phase）组成的：
//        1．常量外壳着色器。
//        2．控制点外壳着色器。
//        
//        常量外壳着色器:
//            常量外壳着色器（constant hull shader）会针对每个面片逐一进行处理（即每处理一个面片就被调用一次），
//            它的任务是输出网格的曲面细分因子（tessellation factor，也有译作细分因子、镶嵌因子等）。
//            曲面细分因子指示了在曲面细分阶段中将面片镶嵌处理后的份数。下面是一个具有4个控制点的四边形面片（quad patch）示例，
//            我们将它从各个方面均匀地镶嵌细分为3份。
//                struct PatchTess
//                {
//                  float EdgeTess[4]   : SV_TessFactor;
//                  float InsideTess[2] : SV_InsideTessFactor;
//
//                  // 可以在下面为每个面片附加所需的额外信息
//                };
//
//                PatchTess ConstantHS(InputPatch<VertexOut, 4> patch,
//                           uint patchID : SV_PrimitiveID)
//                {
//                  PatchTess pt;
//
//                  // 将该面片从各方面均匀地镶嵌处理为3等份
//
//                  pt.EdgeTess[0] = 3; // 四边形面片的左侧边缘
//                  pt.EdgeTess[1] = 3; // 四边形面片的上侧边缘
//                  pt.EdgeTess[2] = 3; // 四边形面片的右侧边缘
//                  pt.EdgeTess[3] = 3; // 四边形面片的下侧边缘
//
//                  pt.InsideTess[0] = 3; // u轴（四边形内部细分的列数）
//                  pt.InsideTess[1] = 3; // v轴（四边形内部细分的行数）
//
//                  return pt;
//                }
//            常量外壳着色器以面片的所有控制点作为输入，在此用InputPatch<VertexOut, 4>对此进行定义。前面提到，控制点首先会传至顶点着色器，
//            因此它们的类型由顶点着色器的输出类型VertexOut来确定。在此例中，我们的面片拥有4个控制点，所以就将InputPatch模板的第二个参数指定为4。
//            系统还通过SV_PrimitiveID语义提供了面片的ID值，此ID唯一地标识了绘制调用过程中的各个面片，我们可根据具体的需求来运用它。
//            常量外壳着色器必须输出曲面细分因子，该因子取决于面片的拓扑结构。
//
//            关于性能的建议。
//            1．如果曲面细分因子为1（这个数值其实意味着该面片不必细分），那么就考虑在渲染此面片时不对它进行细分处理；否则，便会在曲面细分阶段白白浪费GPU资源，因为在此阶段并不对其执行任何操作。
//            2．考虑到性能又涉及GPU对曲面细分的具体实现，所以不要对小于8个像素这种过小的三角形进行镶嵌化处理。
//            3．使用曲面细分技术时要采用批绘制调用（batch draw call，即尽量将曲线细分任务集中执行）（在绘制调用之间往复开启、关闭曲面细分功能的代价极其高昂）。
//        
//        控制点外壳着色器:
//            控制点外壳着色器（control point hull shader）以大量的控制点作为输入与输出，每输出一个控制点，此着色器都会被调用一次。
//            该外壳着色器的应用之一是改变曲面的表示方式，比如说把一个普通的三角形（向渲染流水线提交的3个控制点）转换为3次贝塞尔三角形面片
//            （cubic Bézier triangle patch，即一种具有10个控制点的面片）。例如，假设我们像平常那样利用（具有3个控制点的）三角形对网格进行建模，
//            就可以通过控制点外壳着色器，将这些三角形转换为具有10个控制点的高阶三次贝塞尔三角形面片。新增的控制点不仅会带来更丰富的细节，
//            而且能将三角形面片镶嵌细分为用户所期望的份数。这一策略被称为N-patches方法（法线—面片方法，normal-patches scheme）或PN三角形方法
//            （即（曲面）点—法线三角形方法，（curved） point-normal triangles，简记作PN triangles scheme）[Vlachos01]。
//            由于这种方案只需用曲面细分技术来改进已存在的三角形网格，且无须改动美术制作流程，所以实现起来比较方便。
//            对于本章第一个演示程序来说，控制点外壳着色器仅充当一个简单的传递着色器（pass-through shader），它不会对控制点进行任何的修改。
//                struct HullOut
//                {
//                  float3 PosL : POSITION;
//                };
//
//                [domain("quad")]
//                [partitioning("integer")]
//                [outputtopology("triangle_cw")]
//                [outputcontrolpoints(4)]
//                [patchconstantfunc("ConstantHS")]
//                [maxtessfactor(64.0f)]
//                HullOut HS(InputPatch<VertexOut, 4> p, 
//                      uint i : SV_OutputControlPointID,
//                      uint patchId : SV_PrimitiveID)
//                {
//                  HullOut hout;
//
//                  hout.PosL = p[i].PosL;
//
//                  return hout;
//                }
//                通过InputPatch参数即可将面片的所有控制点都传至外壳着色器之中。系统值SV_OutputControlPointID索引的是正在被外壳着色器所处理的输出控制点。
//                值得注意的是，输入的控制点数量与输出的控制点数量未必相同。例如，输入的面片可能仅含有4个控制点，而输出的面片却能够拥有16个控制点；
//                意即，这些多出来的控制点可由输入的4个控制点所衍生。上面的控制点外壳着色器还用到了以下几种属性。
//                1．domain：面片的类型。可选用的参数有tri（三角形面片）、quad（四边形面片）或isoline（等值线）。
//                2．partitioning：指定了曲面细分的细分模式。
//                    a．integer。新顶点的添加或移除仅取决于曲面细分因子的整数部分，而忽略它的小数部分。这样一来，在网格随着曲面细分级别而改变时，会容易发生明显的突跃（popping）的情况[5]。
//                    b．非整型曲面细分（fractional_even/fractional_odd）。新顶点的添加或移除取决于曲面细分因子的整数部分，但是细微的渐变“过渡”调整就要根据因子的小数部分。
//                    当我们希望将粗糙的网格经曲面细分而平滑地过渡到具有更佳细节的网格时，该参数就派上用场了。理解整型细分与非整型细分之间差别的最佳方式就是通过动画实际演示、比对，因此，本章末会有相应的练习来令读者比较两者的差异。
//                3．outputtopology：通过细分所创的三角形的绕序[6]。
//                    a．triangle_cw：顺时针方向的绕序。
//                    b．triangle_ccw：逆时针方向的绕序。
//                    c．line：针对线段曲面细分。
//                4．outputcontrolpoints：外壳着色器执行的次数，每次执行都输出1个控制点。系统值SV_OutputControlPointID给出的索引标明了当前正在工作的外壳着色器所输出的控制点。
//                5．patchconstantfunc：指定常量外壳着色器函数名称的字符串。
//                6．maxtessfactor：告知驱动程序，用户在着色器中所用的曲面细分因子的最大值。如果硬件知道了此上限，便可了解曲面细分所需的资源，继而就能在后台对此进行优化。
//                    Direct3D 11硬件支持的曲面细分因子最大值为64。
//        镶嵌器阶段:
//            程序员无法对镶嵌器[7]这一阶段进行任何控制，因为这一步骤的操作全权交由硬件处理。此环节会基于常量外壳着色器程序所输出的曲面细分因子，对面片进行镶嵌化处理。
//            图14.2和图14.3详细展示了根据不同的曲面细分因子，对四边形面片与三角形面片所进行的不同细分操作。
//
//        域着色器:
//            镶嵌器阶段会输出新建的所有顶点与三角形，在此阶段所创建顶点，都会逐一调用域着色器（domain shader）进行后续处理。
//            随着曲面细分功能的开启，顶点着色器便化身为“处理每个控制点的顶点着色器”，而外壳着色器[8]的本质实为“针对已经过镶嵌化的面片进行处理的顶点着色器”。
//            特别是，我们可以在此将经镶嵌化处理的面片顶点投射到齐次裁剪空间。对于四边形面片来讲，域着色器以曲面细分因子（还有一些来自常量外壳着色器所输出的每个面片的附加信息）、
//            控制点外壳着色器所输出的所有面片控制点以及镶嵌化处理后的顶点位置参数坐标(u,v)作为输入。注意，域着色器给出的并不是镶嵌化处理后的实际顶点位置，
//            而是这些点位于面片域空间（patch domain space）内的参数坐标(u,v)（见图14.4）。是否利用这些参数坐标以及控制点来求取真正的3D顶点位置，完全取决于用户自己。
//            在以下代码中，我们将通过双线性插值（bilinear interpolation，其工作原理与纹理的线性过滤相似）来实现这一点。
//                struct DomainOut
//                {
//                  float4 PosH : SV_POSITION;
//                };
//
//                // 每当镶嵌器（tessellator）创建顶点时都会调用域着色器。 
//                // 可以把它看作镶嵌处理阶段后的“顶点着色器”
//                [domain("quad")]
//                DomainOut DS(PatchTess patchTess, 
//                       float2 uv : SV_DomainLocation, 
//                       const OutputPatch<HullOut, 4> quad)
//                {
//                  DomainOut dout;
//
//                  // 双线性插值
//                  float3 v1 = lerp(quad[0].PosL, quad[1].PosL, uv.x); 
//                  float3 v2 = lerp(quad[2].PosL, quad[3].PosL, uv.x); 
//                  float3 p  = lerp(v1, v2, uv.y); 
//
//                  float4 posW = mul(float4(p, 1.0f), gWorld);
//                  dout.PosH = mul(posW, gViewProj);
//
//                  return dout;
//                }
//            域着色器处理三角形面片的方法与处理四边形面片的方法很相似，只是把顶点用类型为float3的重心坐标(u,v,w)来表示，以此作为域着色器的输入
//            （与重心坐标相关的数学内容请参见附录C.3节），从而取代参数坐标(u,v)。将三角形面片以重心坐标作为输入的原因，
//            很可能是因为贝塞尔三角形面片都是用重心坐标来定义所导致的。
//
//        */
//
//#include "../../../Common/d3dApp.h"
//#include "../../../Common/MathHelper.h"
//#include "../../../Common/UploadBuffer.h"
//#include "../../../Common/GeometryGenerator.h"
//#include "BPFrameResource.h"
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
//	Count
//};
//
//class BezierPatchApp : public D3DApp
//{
//public:
//    BezierPatchApp(HINSTANCE hInstance);
//    BezierPatchApp(const BezierPatchApp& rhs) = delete;
//    BezierPatchApp& operator=(const BezierPatchApp& rhs) = delete;
//    ~BezierPatchApp();
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
//    void BuildQuadPatchGeometry();
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
//    std::vector<std::unique_ptr<BPFrameResource>> mFrameResources;
//    BPFrameResource* mCurrFrameResource = nullptr;
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
//	// Cache render items of interest.
//	RenderItem* mSkullRitem = nullptr;
//	RenderItem* mReflectedSkullRitem = nullptr;
//	RenderItem* mShadowedSkullRitem = nullptr;
//
//	// List of all the render items.
//	std::vector<std::unique_ptr<RenderItem>> mAllRitems;
//
//	// Render items divided by PSO.
//	std::vector<RenderItem*> mRitemLayer[(int)RenderLayer::Count];
//
//    PassConstants mMainPassCB;
//	PassConstants mReflectedPassCB;
//
//	XMFLOAT3 mSkullTranslation = { 0.0f, 1.0f, -5.0f };
//
//	XMFLOAT3 mEyePos = { 0.0f, 0.0f, 0.0f };
//	XMFLOAT4X4 mView = MathHelper::Identity4x4();
//	XMFLOAT4X4 mProj = MathHelper::Identity4x4();
//
//    float mTheta = 1.24f*XM_PI;
//    float mPhi = 0.42f*XM_PI;
//    float mRadius = 12.0f;
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
//        BezierPatchApp theApp(hInstance);
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
//BezierPatchApp::BezierPatchApp(HINSTANCE hInstance)
//    : D3DApp(hInstance)
//{
//}
//
//BezierPatchApp::~BezierPatchApp()
//{
//    if(md3dDevice != nullptr)
//        FlushCommandQueue();
//}
//
//bool BezierPatchApp::Initialize()
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
//	LoadTextures();
//    BuildRootSignature();
//	BuildDescriptorHeaps();
//    BuildShadersAndInputLayout();
//	BuildQuadPatchGeometry();
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
//void BezierPatchApp::OnResize()
//{
//    D3DApp::OnResize();
//
//    // The window resized, so update the aspect ratio and recompute the projection matrix.
//    XMMATRIX P = XMMatrixPerspectiveFovLH(0.25f*MathHelper::Pi, AspectRatio(), 1.0f, 1000.0f);
//    XMStoreFloat4x4(&mProj, P);
//}
//
//void BezierPatchApp::Update(const GameTimer& gt)
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
//void BezierPatchApp::Draw(const GameTimer& gt)
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
//	UINT passCBByteSize = d3dUtil::CalcConstantBufferByteSize(sizeof(PassConstants));
//
//	auto passCB = mCurrFrameResource->PassCB->Resource();
//	mCommandList->SetGraphicsRootConstantBufferView(2, passCB->GetGPUVirtualAddress());
//	
//    DrawRenderItems(mCommandList.Get(), mRitemLayer[(int)RenderLayer::Opaque]);
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
//void BezierPatchApp::OnMouseDown(WPARAM btnState, int x, int y)
//{
//    mLastMousePos.x = x;
//    mLastMousePos.y = y;
//
//    SetCapture(mhMainWnd);
//}
//
//void BezierPatchApp::OnMouseUp(WPARAM btnState, int x, int y)
//{
//    ReleaseCapture();
//}
//
//void BezierPatchApp::OnMouseMove(WPARAM btnState, int x, int y)
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
//void BezierPatchApp::OnKeyboardInput(const GameTimer& gt)
//{
//}
// 
//void BezierPatchApp::UpdateCamera(const GameTimer& gt)
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
//void BezierPatchApp::AnimateMaterials(const GameTimer& gt)
//{
//
//}
//
//void BezierPatchApp::UpdateObjectCBs(const GameTimer& gt)
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
//void BezierPatchApp::UpdateMaterialCBs(const GameTimer& gt)
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
//void BezierPatchApp::UpdateMainPassCB(const GameTimer& gt)
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
//	// Main pass stored in index 2
//	auto currPassCB = mCurrFrameResource->PassCB.get();
//	currPassCB->CopyData(0, mMainPassCB);
//}
//
//void BezierPatchApp::LoadTextures()
//{
//	auto bricksTex = std::make_unique<Texture>();
//	bricksTex->Name = "bricksTex";
//	bricksTex->Filename = L"E:/DX12Book/DX12LearnProject/DX12Learn/Textures/bricks.dds";
//	ThrowIfFailed(DirectX::CreateDDSTextureFromFile12(md3dDevice.Get(),
//		mCommandList.Get(), bricksTex->Filename.c_str(),
//		bricksTex->Resource, bricksTex->UploadHeap));
//
//	auto checkboardTex = std::make_unique<Texture>();
//	checkboardTex->Name = "checkboardTex";
//	checkboardTex->Filename = L"E:/DX12Book/DX12LearnProject/DX12Learn/Textures/checkboard.dds";
//	ThrowIfFailed(DirectX::CreateDDSTextureFromFile12(md3dDevice.Get(),
//		mCommandList.Get(), checkboardTex->Filename.c_str(),
//		checkboardTex->Resource, checkboardTex->UploadHeap));
//
//	auto iceTex = std::make_unique<Texture>();
//	iceTex->Name = "iceTex";
//	iceTex->Filename = L"E:/DX12Book/DX12LearnProject/DX12Learn/Textures/ice.dds";
//	ThrowIfFailed(DirectX::CreateDDSTextureFromFile12(md3dDevice.Get(),
//		mCommandList.Get(), iceTex->Filename.c_str(),
//		iceTex->Resource, iceTex->UploadHeap));
//
//	auto white1x1Tex = std::make_unique<Texture>();
//	white1x1Tex->Name = "white1x1Tex";
//	white1x1Tex->Filename = L"E:/DX12Book/DX12LearnProject/DX12Learn/Textures/white1x1.dds";
//	ThrowIfFailed(DirectX::CreateDDSTextureFromFile12(md3dDevice.Get(),
//		mCommandList.Get(), white1x1Tex->Filename.c_str(),
//		white1x1Tex->Resource, white1x1Tex->UploadHeap));
//
//	mTextures[bricksTex->Name] = std::move(bricksTex);
//	mTextures[checkboardTex->Name] = std::move(checkboardTex);
//	mTextures[iceTex->Name] = std::move(iceTex);
//	mTextures[white1x1Tex->Name] = std::move(white1x1Tex);
//}
//
//void BezierPatchApp::BuildRootSignature()
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
//void BezierPatchApp::BuildDescriptorHeaps()
//{
//	//
//	// Create the SRV heap.
//	//
//	D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {};
//	srvHeapDesc.NumDescriptors = 4;
//	srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
//	srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
//	ThrowIfFailed(md3dDevice->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&mSrvDescriptorHeap)));
//
//	//
//	// Fill out the heap with actual descriptors.
//	//
//	CD3DX12_CPU_DESCRIPTOR_HANDLE hDescriptor(mSrvDescriptorHeap->GetCPUDescriptorHandleForHeapStart());
//
//	auto bricksTex = mTextures["bricksTex"]->Resource;
//	auto checkboardTex = mTextures["checkboardTex"]->Resource;
//	auto iceTex = mTextures["iceTex"]->Resource;
//	auto white1x1Tex = mTextures["white1x1Tex"]->Resource;
//
//	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
//	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
//	srvDesc.Format = bricksTex->GetDesc().Format;
//	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
//	srvDesc.Texture2D.MostDetailedMip = 0;
//	srvDesc.Texture2D.MipLevels = -1;
//	md3dDevice->CreateShaderResourceView(bricksTex.Get(), &srvDesc, hDescriptor);
//
//	// next descriptor
//	hDescriptor.Offset(1, mCbvSrvDescriptorSize);
//
//	srvDesc.Format = checkboardTex->GetDesc().Format;
//	md3dDevice->CreateShaderResourceView(checkboardTex.Get(), &srvDesc, hDescriptor);
//
//	// next descriptor
//	hDescriptor.Offset(1, mCbvSrvDescriptorSize);
//
//	srvDesc.Format = iceTex->GetDesc().Format;
//	md3dDevice->CreateShaderResourceView(iceTex.Get(), &srvDesc, hDescriptor);
//
//	// next descriptor
//	hDescriptor.Offset(1, mCbvSrvDescriptorSize);
//
//	srvDesc.Format = white1x1Tex->GetDesc().Format;
//	md3dDevice->CreateShaderResourceView(white1x1Tex.Get(), &srvDesc, hDescriptor);
//}
//
//void BezierPatchApp::BuildShadersAndInputLayout()
//{
//	mShaders["tessVS"] = d3dUtil::CompileShader(L"E:\\DX12Book\\DX12LearnProject\\DX12Learn\\LearnDemo\\Chapter 14 The Tessellation Stages\\BezierPatch\\Shaders\\BezierTessellation.hlsl", nullptr, "VS", "vs_5_0");
//	mShaders["tessHS"] = d3dUtil::CompileShader(L"E:\\DX12Book\\DX12LearnProject\\DX12Learn\\LearnDemo\\Chapter 14 The Tessellation Stages\\BezierPatch\\Shaders\\BezierTessellation.hlsl", nullptr, "HS", "hs_5_0");
//	mShaders["tessDS"] = d3dUtil::CompileShader(L"E:\\DX12Book\\DX12LearnProject\\DX12Learn\\LearnDemo\\Chapter 14 The Tessellation Stages\\BezierPatch\\Shaders\\BezierTessellation.hlsl", nullptr, "DS", "ds_5_0");
//	mShaders["tessPS"] = d3dUtil::CompileShader(L"E:\\DX12Book\\DX12LearnProject\\DX12Learn\\LearnDemo\\Chapter 14 The Tessellation Stages\\BezierPatch\\Shaders\\BezierTessellation.hlsl", nullptr, "PS", "ps_5_0");
//	
//    mInputLayout =
//    {
//        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
//    };
//}
//
//void BezierPatchApp::BuildQuadPatchGeometry()
//{
//    std::array<XMFLOAT3,16> vertices =
//	{
//		// Row 0
//		XMFLOAT3(-10.0f, -10.0f, +15.0f),
//		XMFLOAT3(-5.0f,  0.0f, +15.0f),
//		XMFLOAT3(+5.0f,  0.0f, +15.0f),
//		XMFLOAT3(+10.0f, 0.0f, +15.0f),
//
//		// Row 1
//		XMFLOAT3(-15.0f, 0.0f, +5.0f),
//		XMFLOAT3(-5.0f,  0.0f, +5.0f),
//		XMFLOAT3(+5.0f,  20.0f, +5.0f),
//		XMFLOAT3(+15.0f, 0.0f, +5.0f),
//
//		// Row 2
//		XMFLOAT3(-15.0f, 0.0f, -5.0f),
//		XMFLOAT3(-5.0f,  0.0f, -5.0f),
//		XMFLOAT3(+5.0f,  0.0f, -5.0f),
//		XMFLOAT3(+15.0f, 0.0f, -5.0f),
//
//		// Row 3
//		XMFLOAT3(-10.0f, 10.0f, -15.0f),
//		XMFLOAT3(-5.0f,  0.0f, -15.0f),
//		XMFLOAT3(+5.0f,  0.0f, -15.0f),
//		XMFLOAT3(+25.0f, 10.0f, -15.0f)
//	};
//
//	std::array<std::int16_t, 16> indices = 
//	{ 
//		0, 1, 2, 3,
//		4, 5, 6, 7,
//		8, 9, 10, 11, 
//		12, 13, 14, 15
//	};
//
//    const UINT vbByteSize = (UINT)vertices.size() * sizeof(Vertex);
//    const UINT ibByteSize = (UINT)indices.size() * sizeof(std::uint16_t);
//
//	auto geo = std::make_unique<MeshGeometry>();
//	geo->Name = "quadpatchGeo";
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
//	geo->VertexByteStride = sizeof(XMFLOAT3);
//	geo->VertexBufferByteSize = vbByteSize;
//	geo->IndexFormat = DXGI_FORMAT_R16_UINT;
//	geo->IndexBufferByteSize = ibByteSize;
//
//	SubmeshGeometry quadSubmesh;
//	quadSubmesh.IndexCount = (UINT)indices.size();
//	quadSubmesh.StartIndexLocation = 0;
//	quadSubmesh.BaseVertexLocation = 0;
//
//	geo->DrawArgs["quadpatch"] = quadSubmesh;
//
//	mGeometries[geo->Name] = std::move(geo);
//}
//
//void BezierPatchApp::BuildPSOs()
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
//		reinterpret_cast<BYTE*>(mShaders["tessVS"]->GetBufferPointer()), 
//		mShaders["tessVS"]->GetBufferSize()
//	};
//	opaquePsoDesc.HS =
//	{
//		reinterpret_cast<BYTE*>(mShaders["tessHS"]->GetBufferPointer()),
//		mShaders["tessHS"]->GetBufferSize()
//	};
//	opaquePsoDesc.DS =
//	{
//		reinterpret_cast<BYTE*>(mShaders["tessDS"]->GetBufferPointer()),
//		mShaders["tessDS"]->GetBufferSize()
//	};
//	opaquePsoDesc.PS = 
//	{ 
//		reinterpret_cast<BYTE*>(mShaders["tessPS"]->GetBufferPointer()),
//		mShaders["tessPS"]->GetBufferSize()
//	};
//	opaquePsoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
//	opaquePsoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_WIREFRAME;
//	opaquePsoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
//	opaquePsoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
//	opaquePsoDesc.SampleMask = UINT_MAX;
//	opaquePsoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_PATCH;
//	opaquePsoDesc.NumRenderTargets = 1;
//	opaquePsoDesc.RTVFormats[0] = mBackBufferFormat;
//	opaquePsoDesc.SampleDesc.Count = m4xMsaaState ? 4 : 1;
//	opaquePsoDesc.SampleDesc.Quality = m4xMsaaState ? (m4xMsaaQuality - 1) : 0;
//	opaquePsoDesc.DSVFormat = mDepthStencilFormat;
//    ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&opaquePsoDesc, IID_PPV_ARGS(&mPSOs["opaque"])));
//}
//
//void BezierPatchApp::BuildFrameResources()
//{
//    for(int i = 0; i < gNumFrameResources; ++i)
//    {
//        mFrameResources.push_back(std::make_unique<BPFrameResource>(md3dDevice.Get(),
//            2, (UINT)mAllRitems.size(), (UINT)mMaterials.size()));
//    }
//}
//
//void BezierPatchApp::BuildMaterials()
//{
//	auto whiteMat = std::make_unique<Material>();
//	whiteMat->Name = "quadMat";
//	whiteMat->MatCBIndex = 0;
//	whiteMat->DiffuseSrvHeapIndex = 3;
//	whiteMat->DiffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
//	whiteMat->FresnelR0 = XMFLOAT3(0.1f, 0.1f, 0.1f);
//	whiteMat->Roughness = 0.5f;
//
//	mMaterials["whiteMat"] = std::move(whiteMat);
//}
//
//void BezierPatchApp::BuildRenderItems()
//{
//	auto quadPatchRitem = std::make_unique<RenderItem>();
//	quadPatchRitem->World = MathHelper::Identity4x4();
//	quadPatchRitem->TexTransform = MathHelper::Identity4x4();
//	quadPatchRitem->ObjCBIndex = 0;
//	quadPatchRitem->Mat = mMaterials["whiteMat"].get();
//	quadPatchRitem->Geo = mGeometries["quadpatchGeo"].get();
//	quadPatchRitem->PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_16_CONTROL_POINT_PATCHLIST;
//	quadPatchRitem->IndexCount = quadPatchRitem->Geo->DrawArgs["quadpatch"].IndexCount;
//	quadPatchRitem->StartIndexLocation = quadPatchRitem->Geo->DrawArgs["quadpatch"].StartIndexLocation;
//	quadPatchRitem->BaseVertexLocation = quadPatchRitem->Geo->DrawArgs["quadpatch"].BaseVertexLocation;
//	mRitemLayer[(int)RenderLayer::Opaque].push_back(quadPatchRitem.get());
//	
//	mAllRitems.push_back(std::move(quadPatchRitem));
//}
//
//void BezierPatchApp::DrawRenderItems(ID3D12GraphicsCommandList* cmdList, const std::vector<RenderItem*>& ritems)
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
//std::array<const CD3DX12_STATIC_SAMPLER_DESC, 6> BezierPatchApp::GetStaticSamplers()
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

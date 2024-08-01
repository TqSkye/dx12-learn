////***************************************************************************************
//// NormalMapApp.cpp by Frank Luna (C) 2015 All Rights Reserved.
////***************************************************************************************
//
///*
//法线图:
//    法线图（normal map）本质上也是一种纹理，但其中每一个纹素所存储的并非RGB数据，红、绿、蓝3种分量依次存储的是压缩后的x、y、z坐标，
//    这些坐标定义的即是法向量。也就是说，法线图中的每个像素内都存储了一条法向量。图19.3所示为对一张法线图进行可视化处理后的效果。
//
//    存于法线图中的法线，它们位于由T（x轴）、B（y轴）、N（z轴）3个向量所定义的纹理空间坐标系之中。
//    向量T指向纹理图像的右侧水平方向，向量B指向纹理图像的下侧垂直方向，向量N则正交于纹理平面
//
//    为了便于讲解，假设以下示例中所用的是24位图像格式，即将每个颜色分量都存于1字节之中，因此，每个颜色分量的取值范围都为[0,255]
//    （至于32位的图像格式，其中的alpha分量可以保留不用，也可以用于存储其他的标量数据，如地形的高度图（heightmap）或高光图（specular map，也称为镜面反射图等）。
//    当然，若使用浮点格式就不必再对坐标进行压缩了，但是这将耗费更多的内存。）
//    如图19.3所示，其中的向量大多都近似平行于z轴。换言之，这些向量的z值取得了3种坐标分量中的最大值。
//    因此，当把法线图视作彩色图像时（也就是按每个法线的RGB值进行绘制），整体会呈现蓝色。这是因为z坐标被存于蓝色通道之中，而大部分法线又取得了较大的z值，所以蓝色会占据图像的大部分区域。
//
//    那么，又该如何将单位向量压缩为上述格式呢？首先要注意到的是“单位向量”一词，这就是说，每个坐标的范围都被限定在[−1, 1]。如果以平移及缩放的手段将其区间变换至[0, 1]，
//    并乘以255，再截断（truncate）小数部分，则最终得到的将是[0,255]的某个整数。即，如果x为一个在[−1, 1]区间的坐标，则f(x)函数得到的整数部分将处于范围[0, 255]之中。
//    在这里，f的定义为：
//        f(x) = (0.5x + 0.5)*255
//    因此，为了在24位图像中存储单位向量，仅需将每个坐标用函数f进行变换，再把变换后的坐标写入纹理图中对应的颜色通道即可。
//    接下来的问题是怎样实现压缩处理的逆操作，即给出一个在范围[0, 255]内的压缩后的纹理坐标，我们以某种方法将它复原为[−1, 1]区间内的值。
//    事实上，通过f的反函数即可方便地解决此问题，经过简单的变换，将得到：
//        f^-1(x) = 2x/255 - 1
//    即，如果x是范围在[0, 255]的一个整数，则f^-1(x)得到的结果是范围[−1, 1]区间的一个浮点数。
//
//    我们不必亲自动手参与压缩处理，因为借助一款Photoshop插件就能将图片轻松转换为法线图，所以只是在像素着色器中对法线图进行采样时，还是要实现解压缩变换过程中的一些步骤。
//    我们用类似于下列的语句在着色器中对法线图进行采样：
//        float3 normalT = gNormalMap.Sample( gTriLinearSam, pin.Tex );
//        颜色向量normalT将获取归一化分量构成的坐标(r,g,b)，其中0<=r,g,b<=1。
//    可见，坐标的解压工作现已完成一部分了（即压缩坐标已除以255，由位于[0, 255]的整数变换到[0, 1] 区间之内的浮点数）。
//    接下来，我们就要用函数[g:[0,1]=>[-1,1]]通过平移与缩放来将每个分量由范围[0, 1]变换到区间[−1, 1]，以此实现整个解压缩操作。此函数的定义为：
//        g(x) = 2x - 1
//    在代码中，我们用此函数来处理每一个颜色分量：
//        // 将每个分量从[0,1]解压缩至[-1,1]。
//        normalT = 2.0f*normalT - 1.0f;
//    由于标量1.0根据规则会被扩充为向量(1, 1, 1)，因此该表达式会按分量逐个进行计算。
//    如果用压缩纹理格式来存储法线图，那么BC7（DXGI_FORMAT_BC7_UNORM）图像格式的质量最佳，因为它大幅减少了因压缩法线图而导致的误差。
//    对于BC6与BC7格式而言，DirectX SDK给出了名为“BC6HBC7EncoderDecoder11”的相应示例。我们可通过这个程序将纹理文件转换为BC6或BC7格式。
//
//纹理空间/切线空间:
//    现在我们来考虑将纹理映射到3D三角形上的过程。为了便于讨论，假设在纹理贴图的过程中不存在纹理扭曲形变的现象，即在将纹理三角形映射至3D三角形上时，仅需执行刚体变换（旋转、平移操作）。
//    现在，我们就可以把纹理看作是一张贴纸，通过拾取、平移以及旋转等手段，将它贴在3D三角形上。图19.4展示了纹理坐标系的坐标轴相对于3D三角形的位置关系：这些坐标轴与三角形相切，并与三角形处于同一平面。
//    所以，三角形的纹理坐标势必也就相对于此纹理坐标系而定。再结合三角形的平面法线N，我们便获得了一个位于三角形所在平面内的3D TBN基（TBN-basis），
//    它常被称为纹理空间（texture space）或切线空间（tangent space，也有译作正切空间、切空间等）[1]。值得注意的是，切线空间通常会随不同的三角形而发生改变（见图19.5）。
//
//    现在把目光转回图19.3，此法线图中的法向量是相对于纹理空间而定义的。可是，我们所用的光源都被定义在世界空间之中。为了实现光照效果，法向量与光源必须位于同一空间内。
//    因此，当前的首要任务就是使三角形顶点所在的物体空间坐标系（object space coordinate system）与切线空间坐标系建立联系。一旦这些顶点位于物体空间之中，我们就能通过世界矩阵将它们从物体空间变换到世界空间（在下一节中会讨论相关细节）。
//    设纹理坐标分别为(u0,v0)、(u1,v1)、(u2,v2)的顶点v0、v1和v2在相对于纹理坐标系坐标轴（即切向量T与切向量B）构成的纹理平面内定义了一个三角形。
//    设e0=v1-v0且e1=v2-v0为3D三角形的两个边向量，它们所对应的纹理三角形边向量则分别为(Δu0,Δv0)=(u1-u0,v1-v0)与(Δu1,Δv1)=(u2-u0,v2-v0)。由图19.4可以看出它们的关系：
//        e0=Δu0T + Δv0B
//        e1=Δu1T + Δv1B
//    用相对于物体空间的坐标来表示这些向量，就能得到其矩阵方程：
//        [ e0,x  e0,y  e0,z ] = [Δu0  Δv0][Tx Ty Tz]
//        [ e1,x  e1,y  e1,z ]   [Δu1  Δv1][Bx By Bz]
//    注意，我们已经知道了三角形顶点的物体空间坐标，当然也就能求出边向量的物体空间坐标。因此，矩阵
//    [ e0,x  e0,y  e0,z ]
//    [ e1,x  e1,y  e1,z ]
//    是已知的。同理，由于我们也知道了纹理坐标，因而矩阵
//    [Δu0  Δv0]
//    [Δu1  Δv1]亦可知晓。
//    现在就来求出切向量T与切向量B的物体空间坐标：
//    [Tx Ty Tz] = ([Δu0  Δv0])^-1 [ e0,x  e0,y  e0,z ] = 1/(Δu0Δv1-Δv0Δu1)[Δv1  -Δv0][ e0,x  e0,y  e0,z ]
//    [Bx By Bz]   ([Δu1  Δv1])    [ e1,x  e1,y  e1,z ]                    [-Δu1  Δu0][ e1,x  e1,y  e1,z ]
//    在上述推导过程中，我们使用了下列关于逆矩阵的性质，假设有矩阵
//    A = [a b] 则有：A^-1=1/(ad-bc)[d -b] 要知道，切向量T与切向量B在物体空间中一般均不是单位长度。而且，如果纹理发生了扭曲形变，那么这两个向量也将不再互为正交规范化向量。
//        [c d]                    [-c a]
//    按照惯例，T、B、N三个向量通常分别被称为切线（tangent，也有为了区分副切线而称为主切线）、副法线[2]（binormal，亦有译作次法线。或bitangent，副切线）以及法线（normal）。
//顶点切线空间:
//    在上一节中，我们推导出了任意三角形所对应的切线空间。但是，如果在进行法线贴图时使用了这种纹理空间，则最终得到的效果会呈现明显的三角形划分痕迹，这是因为切线空间必定位于对应三角形的所在平面，
//    以致贴图面不够圆滑，直来直去全是棱角。因此，我们要在每个顶点处指定切向量，并重施求平均值的故技，令顶点法线更趋于平滑的表面，化粗糙为神奇（可回顾8.2节进行对比）。
//        1．通过计算网格中共用顶点v的每个三角形的切向量平均值，便可以求出网格中任意顶点v处的切向量T。
//        2．通过计算网格中共用顶点v的每个三角形的副切向量平均值，便能够求此网格中顶点v处的副切向量B。
//    一般来讲，在计算完上述平均值之后，往往还需要对TBN基进行正交规范化处理，使这3个向量相互正交且均具有单位长度。这通常是以格拉姆—施密特过程（Gram-Schmidt procedure）来实现的。可以在网络上找到为任意三角形网格构建逐顶点切线空间的相关代码[3]。
//    在我们当前的系统中，并不会把副切向量B直接存于内存中。而是在用到B时以B=N x T来求取此向量，公式中的N即顶点法线的平均值（平均顶点法线）。因此，我们所定义的顶点结构体如下：
//        struct Vertex
//        {
//          XMFLOAT3 Pos;
//          XMFLOAT3 Normal;
//          XMFLOAT2 TexC;
//          XMFLOAT3 TangentU;
//        };
//    前文曾提到，我们在程序中是通过GeometryGenerator函数计算纹理空间中坐标轴u所对应的切向量T来生成网格的（详见7.4.1节）。对于立方体和栅格这两种网格来说，
//    指定其每个顶点处切向量T的物体空间坐标并不是件难事（见图19.5）。而对于圆柱体与球体来讲，为了求出它们每个顶点处的切向量T，我们就要构建圆柱体或球体其具有两个变量的向量值函数P(u,v)并计算出∂P/∂u，
//    其中的参数u也常被用作纹理坐标
//在切线空间与物体空间之间进行转换:
//    至此，我们确定了网格中每个顶点处的正交规范化TBN基，而且也已求出了TBN三向量相对于网格物体空间的坐标。也就是说，我们已经掌握了TBN基相对于物体空间坐标系的坐标，并可通过下列矩阵将坐标由切线空间变换至物体空间：
//                 Tx Ty Tz
//    M(object) = [Bx By Bz]
//                 Nx Ny Nz
//    由于该矩阵是正交矩阵，所以其逆矩阵就是它的转置矩阵。因此，由物体空间转换到切线空间的坐标变换矩阵为：
//                                               Tx Bx Nx
//    M(tangent) = M(object)^-1 = M(object)^T = [Ty By Ny]
//                                               Tz Bz Nz
//    在我们所编写的着色器程序中，为了计算光照会把法向量从切线空间变换到世界空间。对此，一种可行方法是首先将法线自切线空间变换至物体空间，而后再以世界矩阵把它从物体空间变换到世界空间：
//    n(world) = (n(tangent)M(object))M(world)
//    然而，由于矩阵乘法满足结合律，因此可以将公式变为：
//    n(world) = n(tangent)(M(object)M(world))
//    注意到:
//                         <- T ->             <- T' ->     Tx' Ty' Tz'
//    M(object)M(world) = [<- B ->]M(world) = [<- B' ->] = [Bx' By' Bz']
//                         <- N ->             <- N' ->     Nx' Ny' Nz'
//    其中T'=TM(world)、B'=BM(world)以及N'=NM(world)。因此，要从切线空间直接变换到世界空间，我们仅需用世界坐标来表示切线基（tangent basis）即可，这可以通过将TBN基从物体空间坐标变换到世界空间坐标来实现。
//    我们所关心的仅是向量的变换（而非点的变换），因此用3x3矩阵来表示即可。本书前面曾讲过，仿射矩阵的第4行是用于执行平移变换的，但我们却不能平移向量。
//
//注: 切线空间/TBN基/TBN变换文章参考: https://zhuanlan.zhihu.com/p/565731092 https://zhuanlan.zhihu.com/p/574095416 https://zhuanlan.zhihu.com/p/412555049 https://zhuanlan.zhihu.com/p/654542488
//    
//    法线贴图的流程大致如下。
//    1．通过艺术加工工具或图像处理程序来创造预定的法线图，并将它存于图像文件之中。在应用程序初始化期间以这些图像文件来创建2D纹理。
//    2．针对每一个三角形，计算其切向量T。通过对网格中共享顶点v的所有三角形的切向量求取平均值，就可以获取此网格中每个顶点v处的切向量
//    （在演示程序中，由于使用的是简单的几何图形，因此可以直接指定出相应的切向量。但是，如果处理的是由3D建模程序创建的不规则形状的三角形网格，那么就需要按上述方法来计算切向量的平均值）。
//    3．在顶点着色器中，将顶点法线与切向量变换到世界空间，并将结果输出到像素着色器。
//    4．通过插值切向量与插值法向量来构建三角形表面每个像素点处的TBN基，再以此TBN基将从法线图中采集的法向量由切线空间变换到世界空间。
//    这样一来，我们就拥有了取自法线图的世界空间法向量，并可将它用于往常的光照计算。为便于法线贴图的实现，我们在Common.hlsl文件中加入了以下函数。
//    //--------------------------------------------------------------------
//    // 将一个法线图样本变换至世界空间
//    //--------------------------------------------------------------------
//    float3 NormalSampleToWorldSpace(float3 normalMapSample, 
//                    float3 unitNormalW, 
//                    float3 tangentW)
//    {
//        // 将每个坐标分量由范围[0,1]解压至[-1,1]区间
//        float3 normalT = 2.0f*normalMapSample - 1.0f;
//
//        // 构建正交规范基
//        float3 N = unitNormalW;
//        float3 T = normalize(tangentW - dot(tangentW, N)*N);
//        float3 B = cross(N, T);
//
//        float3x3 TBN = float3x3(T, B, N);
//
//        // 将法线图样本从切线空间变换到世界空间
//        float3 bumpedNormalW = mul(normalT, TBN);
//
//        return bumpedNormalW;
//    }
//    此函数可以在像素着色器中按下列方式调用：
//        float3 normalMapSample = gNormalMaps.Sample(samLinear,pin.Tex).rgb;
//        float3 bumpedNormalW = NormalSampleToWorldSpace(normalMapSample, pin.NormalW, pin. TangentW);
//    有两行代码可能不太容易理解：
//        float3 N = unitNormalW;
//        float3 T = normalize(tangentW - dot(tangentW, N)*N);
//    插值完成后，切向量与法向量可能会变为非正交规范向量。以上两行代码通过使T减去其N方向上的分量（投影），再对结果进行规范化处理，从而使T成为规范化向量且正交于N（见图19.6）。注意，这里假设unitNormalW为规范化向量。
//    N
//    ^
//    |(N·T)N
//    ^________
//    |        ^ T
//    |       /|
//    |      / |
//    |     /  |
//    |    /   |
//    |   /    |
//    |  /     |
//    | /      |
//    |/_______>______________________> T
//             T-(N·T)N
//    由于||N|| = 1, proj(n)(T) = (N·T)N, 所以切线T的分量T-proj(n)(T)正交于法线N
//    只要获取了法线图中的法线（也称“bumped normal”，常直译作凹凸法线，即方向不一的法线，可令光照表现出物体表面凹凸不平的效果），
//    即可将它运用于法向量所参与的一切后续计算（如光照、立方体贴图）。完整的法线贴图效果实现如下，其中与法线贴图相关的部分都已用黑体字标出==>(Default.hlsl)
//    */
//
//#include "../../../Common/d3dApp.h"
//#include "../../../Common/MathHelper.h"
//#include "../../../Common/UploadBuffer.h"
//#include "../../../Common/GeometryGenerator.h"
//#include "../../../Common/Camera.h"
//#include "NormalFrameResource.h"
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
//	Sky,
//	Count
//};
//
//class NormalMapApp : public D3DApp
//{
//public:
//    NormalMapApp(HINSTANCE hInstance);
//    NormalMapApp(const NormalMapApp& rhs) = delete;
//    NormalMapApp& operator=(const NormalMapApp& rhs) = delete;
//    ~NormalMapApp();
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
//	void AnimateMaterials(const GameTimer& gt);
//	void UpdateObjectCBs(const GameTimer& gt);
//	void UpdateMaterialBuffer(const GameTimer& gt);
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
//    std::vector<std::unique_ptr<NormalFrameResource>> mFrameResources;
//    NormalFrameResource* mCurrFrameResource = nullptr;
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
//	// List of all the render items.
//	std::vector<std::unique_ptr<RenderItem>> mAllRitems;
//
//	// Render items divided by PSO.
//	std::vector<RenderItem*> mRitemLayer[(int)RenderLayer::Count];
//
//	UINT mSkyTexHeapIndex = 0;
//
//    PassConstants mMainPassCB;
//
//	Camera mCamera;
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
//        NormalMapApp theApp(hInstance);
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
//NormalMapApp::NormalMapApp(HINSTANCE hInstance)
//    : D3DApp(hInstance)
//{
//}
//
//NormalMapApp::~NormalMapApp()
//{
//    if(md3dDevice != nullptr)
//        FlushCommandQueue();
//}
//
//bool NormalMapApp::Initialize()
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
//	mCamera.SetPosition(0.0f, 2.0f, -15.0f);
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
//void NormalMapApp::OnResize()
//{
//    D3DApp::OnResize();
//
//	mCamera.SetLens(0.25f*MathHelper::Pi, AspectRatio(), 1.0f, 1000.0f);
//}
//
//void NormalMapApp::Update(const GameTimer& gt)
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
//	AnimateMaterials(gt);
//	UpdateObjectCBs(gt);
//	UpdateMaterialBuffer(gt);
//	UpdateMainPassCB(gt);
//}
//
//void NormalMapApp::Draw(const GameTimer& gt)
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
//	mCommandList->SetGraphicsRootConstantBufferView(1, passCB->GetGPUVirtualAddress());
//
//	// Bind all the materials used in this scene.  For structured buffers, we can bypass the heap and 
//	// set as a root descriptor.
//	auto matBuffer = mCurrFrameResource->MaterialBuffer->Resource();
//	mCommandList->SetGraphicsRootShaderResourceView(2, matBuffer->GetGPUVirtualAddress());
//
//	// Bind the sky cube map.  For our demos, we just use one "world" cube map representing the environment
//	// from far away, so all objects will use the same cube map and we only need to set it once per-frame.  
//	// If we wanted to use "local" cube maps, we would have to change them per-object, or dynamically
//	// index into an array of cube maps.
//
//	CD3DX12_GPU_DESCRIPTOR_HANDLE skyTexDescriptor(mSrvDescriptorHeap->GetGPUDescriptorHandleForHeapStart());
//	skyTexDescriptor.Offset(mSkyTexHeapIndex, mCbvSrvDescriptorSize);
//	mCommandList->SetGraphicsRootDescriptorTable(3, skyTexDescriptor);
//
//	// Bind all the textures used in this scene.  Observe
//    // that we only have to specify the first descriptor in the table.  
//    // The root signature knows how many descriptors are expected in the table.
//	mCommandList->SetGraphicsRootDescriptorTable(4, mSrvDescriptorHeap->GetGPUDescriptorHandleForHeapStart());
//
//    DrawRenderItems(mCommandList.Get(), mRitemLayer[(int)RenderLayer::Opaque]);
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
//void NormalMapApp::OnMouseDown(WPARAM btnState, int x, int y)
//{
//    mLastMousePos.x = x;
//    mLastMousePos.y = y;
//
//    SetCapture(mhMainWnd);
//}
//
//void NormalMapApp::OnMouseUp(WPARAM btnState, int x, int y)
//{
//    ReleaseCapture();
//}
//
//void NormalMapApp::OnMouseMove(WPARAM btnState, int x, int y)
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
//void NormalMapApp::OnKeyboardInput(const GameTimer& gt)
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
//void NormalMapApp::AnimateMaterials(const GameTimer& gt)
//{
//	
//}
//
//void NormalMapApp::UpdateObjectCBs(const GameTimer& gt)
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
//void NormalMapApp::UpdateMaterialBuffer(const GameTimer& gt)
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
//void NormalMapApp::UpdateMainPassCB(const GameTimer& gt)
//{
//	XMMATRIX view = mCamera.GetView();
//	XMMATRIX proj = mCamera.GetProj();
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
//	mMainPassCB.EyePosW = mCamera.GetPosition3f();
//	mMainPassCB.RenderTargetSize = XMFLOAT2((float)mClientWidth, (float)mClientHeight);
//	mMainPassCB.InvRenderTargetSize = XMFLOAT2(1.0f / mClientWidth, 1.0f / mClientHeight);
//	mMainPassCB.NearZ = 1.0f;
//	mMainPassCB.FarZ = 1000.0f;
//	mMainPassCB.TotalTime = gt.TotalTime();
//	mMainPassCB.DeltaTime = gt.DeltaTime();
//	mMainPassCB.AmbientLight = { 0.25f, 0.25f, 0.35f, 1.0f };
//	mMainPassCB.Lights[0].Direction = { 0.57735f, -0.57735f, 0.57735f };
//	mMainPassCB.Lights[0].Strength = { 0.8f, 0.8f, 0.8f };
//	mMainPassCB.Lights[1].Direction = { -0.57735f, -0.57735f, 0.57735f };
//	mMainPassCB.Lights[1].Strength = { 0.4f, 0.4f, 0.4f };
//	mMainPassCB.Lights[2].Direction = { 0.0f, -0.707f, -0.707f };
//	mMainPassCB.Lights[2].Strength = { 0.2f, 0.2f, 0.2f };
//
//	auto currPassCB = mCurrFrameResource->PassCB.get();
//	currPassCB->CopyData(0, mMainPassCB);
//}
//
//void NormalMapApp::LoadTextures()
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
//	std::vector<std::wstring> texFilenames = 
//	{
//		L"E:/DX12Book/DX12LearnProject/DX12Learn/Textures/bricks2.dds",
//		L"E:/DX12Book/DX12LearnProject/DX12Learn/Textures/bricks2_nmap.dds",
//		L"E:/DX12Book/DX12LearnProject/DX12Learn/Textures/tile.dds",
//		L"E:/DX12Book/DX12LearnProject/DX12Learn/Textures/tile_nmap.dds",
//		L"E:/DX12Book/DX12LearnProject/DX12Learn/Textures/white1x1.dds",
//		L"E:/DX12Book/DX12LearnProject/DX12Learn/Textures/default_nmap.dds",
//		L"E:/DX12Book/DX12LearnProject/DX12Learn/Textures/snowcube1024.dds"
//	};
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
//void NormalMapApp::BuildRootSignature()
//{
//	CD3DX12_DESCRIPTOR_RANGE texTable0;
//	texTable0.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0);
//
//	CD3DX12_DESCRIPTOR_RANGE texTable1;
//	texTable1.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 10, 1, 0);
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
//void NormalMapApp::BuildDescriptorHeaps()
//{
//	//
//	// Create the SRV heap.
//	//
//	D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {};
//	srvHeapDesc.NumDescriptors = 10;
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
//		hDescriptor.Offset(1, mCbvSrvDescriptorSize);
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
//}
//
//void NormalMapApp::BuildShadersAndInputLayout()
//{
//	const D3D_SHADER_MACRO alphaTestDefines[] =
//	{
//		"ALPHA_TEST", "1",
//		NULL, NULL
//	};
//
//	mShaders["standardVS"] = d3dUtil::CompileShader(L"E:\\DX12Book\\DX12LearnProject\\DX12Learn\\LearnDemo\\Chapter 19 Normal Mapping\\NormalMap\\Shaders\\Default.hlsl", nullptr, "VS", "vs_5_1");
//	mShaders["opaquePS"] = d3dUtil::CompileShader(L"E:\\DX12Book\\DX12LearnProject\\DX12Learn\\LearnDemo\\Chapter 19 Normal Mapping\\NormalMap\\Shaders\\Default.hlsl", nullptr, "PS", "ps_5_1");
//	
//	mShaders["skyVS"] = d3dUtil::CompileShader(L"E:\\DX12Book\\DX12LearnProject\\DX12Learn\\LearnDemo\\Chapter 19 Normal Mapping\\NormalMap\\Shaders\\Sky.hlsl", nullptr, "VS", "vs_5_1");
//	mShaders["skyPS"] = d3dUtil::CompileShader(L"E:\\DX12Book\\DX12LearnProject\\DX12Learn\\LearnDemo\\Chapter 19 Normal Mapping\\NormalMap\\Shaders\\Sky.hlsl", nullptr, "PS", "ps_5_1");
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
//void NormalMapApp::BuildShapeGeometry()
//{
//    GeometryGenerator geoGen;
//	GeometryGenerator::MeshData box = geoGen.CreateBox(1.0f, 1.0f, 1.0f, 3);
//	GeometryGenerator::MeshData grid = geoGen.CreateGrid(20.0f, 30.0f, 60, 40);
//	GeometryGenerator::MeshData sphere = geoGen.CreateSphere(0.5f, 20, 20);
//	GeometryGenerator::MeshData cylinder = geoGen.CreateCylinder(0.5f, 0.3f, 3.0f, 20, 20);
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
//
//	// Cache the starting index for each object in the concatenated index buffer.
//	UINT boxIndexOffset = 0;
//	UINT gridIndexOffset = (UINT)box.Indices32.size();
//	UINT sphereIndexOffset = gridIndexOffset + (UINT)grid.Indices32.size();
//	UINT cylinderIndexOffset = sphereIndexOffset + (UINT)sphere.Indices32.size();
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
//	//
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
//void NormalMapApp::BuildPSOs()
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
//void NormalMapApp::BuildFrameResources()
//{
//    for(int i = 0; i < gNumFrameResources; ++i)
//    {
//        mFrameResources.push_back(std::make_unique<NormalFrameResource>(md3dDevice.Get(),
//            1, (UINT)mAllRitems.size(), (UINT)mMaterials.size()));
//    }
//}
//
//void NormalMapApp::BuildMaterials()
//{
//	auto bricks0 = std::make_unique<Material>();
//	bricks0->Name = "bricks0";
//	bricks0->MatCBIndex = 0;
//	bricks0->DiffuseSrvHeapIndex = 0;
//	bricks0->NormalSrvHeapIndex = 1;
//	bricks0->DiffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
//    bricks0->FresnelR0 = XMFLOAT3(0.1f, 0.1f, 0.1f);
//    bricks0->Roughness = 0.3f;
//
//	auto tile0 = std::make_unique<Material>();
//	tile0->Name = "tile0";
//	tile0->MatCBIndex = 2;
//	tile0->DiffuseSrvHeapIndex = 2;
//	tile0->NormalSrvHeapIndex = 3;
//	tile0->DiffuseAlbedo = XMFLOAT4(0.9f, 0.9f, 0.9f, 1.0f);
//	tile0->FresnelR0 = XMFLOAT3(0.2f, 0.2f, 0.2f);
//    tile0->Roughness = 0.1f;
//
//	auto mirror0 = std::make_unique<Material>();
//	mirror0->Name = "mirror0";
//	mirror0->MatCBIndex = 3;
//	mirror0->DiffuseSrvHeapIndex = 4;
//	mirror0->NormalSrvHeapIndex = 5;
//	mirror0->DiffuseAlbedo = XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);
//	mirror0->FresnelR0 = XMFLOAT3(0.98f, 0.97f, 0.95f);
//	mirror0->Roughness = 0.1f;
//
//	auto sky = std::make_unique<Material>();
//	sky->Name = "sky";
//	sky->MatCBIndex = 4;
//	sky->DiffuseSrvHeapIndex = 6;
//	sky->NormalSrvHeapIndex = 7;
//	sky->DiffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
//	sky->FresnelR0 = XMFLOAT3(0.1f, 0.1f, 0.1f);
//	sky->Roughness = 1.0f;
//	
//	mMaterials["bricks0"] = std::move(bricks0);
//	mMaterials["tile0"] = std::move(tile0);
//	mMaterials["mirror0"] = std::move(mirror0);
//	mMaterials["sky"] = std::move(sky);
//}
//
//void NormalMapApp::BuildRenderItems()
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
//	auto boxRitem = std::make_unique<RenderItem>();
//	XMStoreFloat4x4(&boxRitem->World, XMMatrixScaling(2.0f, 1.0f, 2.0f)*XMMatrixTranslation(0.0f, 0.5f, 0.0f));
//	XMStoreFloat4x4(&boxRitem->TexTransform, XMMatrixScaling(1.0f, 0.5f, 1.0f));
//	boxRitem->ObjCBIndex = 1;
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
//	auto globeRitem = std::make_unique<RenderItem>();
//	XMStoreFloat4x4(&globeRitem->World, XMMatrixScaling(2.0f, 2.0f, 2.0f)*XMMatrixTranslation(0.0f, 2.0f, 0.0f));
//	XMStoreFloat4x4(&globeRitem->TexTransform, XMMatrixScaling(1.0f, 1.0f, 1.0f));
//	globeRitem->ObjCBIndex = 2;
//	globeRitem->Mat = mMaterials["mirror0"].get();
//	globeRitem->Geo = mGeometries["shapeGeo"].get();
//	globeRitem->PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
//	globeRitem->IndexCount = globeRitem->Geo->DrawArgs["sphere"].IndexCount;
//	globeRitem->StartIndexLocation = globeRitem->Geo->DrawArgs["sphere"].StartIndexLocation;
//	globeRitem->BaseVertexLocation = globeRitem->Geo->DrawArgs["sphere"].BaseVertexLocation;
//
//	mRitemLayer[(int)RenderLayer::Opaque].push_back(globeRitem.get());
//	mAllRitems.push_back(std::move(globeRitem));
//
//    auto gridRitem = std::make_unique<RenderItem>();
//    gridRitem->World = MathHelper::Identity4x4();
//	XMStoreFloat4x4(&gridRitem->TexTransform, XMMatrixScaling(8.0f, 8.0f, 1.0f));
//	gridRitem->ObjCBIndex = 3;
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
//	UINT objCBIndex = 4;
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
//void NormalMapApp::DrawRenderItems(ID3D12GraphicsCommandList* cmdList, const std::vector<RenderItem*>& ritems)
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
//std::array<const CD3DX12_STATIC_SAMPLER_DESC, 6> NormalMapApp::GetStaticSamplers()
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

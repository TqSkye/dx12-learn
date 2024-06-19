////***************************************************************************************
//// Init Direct3D.cpp by Frank Luna (C) 2015 All Rights Reserved.
////
//// Demonstrates the sample framework by initializing Direct3D, clearing 
//// the screen, and displaying frame stats.
////
////***************************************************************************************
//
//#include "../../Common/d3dApp.h"
//#include <DirectXColors.h>
//
//using namespace DirectX;
//
//class InitDirect3DApp : public D3DApp
//{
//public:
//	InitDirect3DApp(HINSTANCE hInstance);
//	~InitDirect3DApp();
//
//	virtual bool Initialize()override;
//
//private:
//    virtual void OnResize()override;
//    virtual void Update(const GameTimer& gt)override;
//    virtual void Draw(const GameTimer& gt)override;
//
//};
//
//int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE prevInstance,
//				   PSTR cmdLine, int showCmd)
//{
//	// Enable run-time memory check for debug builds.
//    // 为调试版本开启运行时内存检测，方便监督内存泄露的情况
//#if defined(DEBUG) | defined(_DEBUG)
//	_CrtSetDbgFlag( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF );
//#endif
//
//    try
//    {
//        InitDirect3DApp theApp(hInstance);
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
//InitDirect3DApp::InitDirect3DApp(HINSTANCE hInstance)
//: D3DApp(hInstance) 
//{
//}
//
//InitDirect3DApp::~InitDirect3DApp()
//{
//}
//
//bool InitDirect3DApp::Initialize()
//{
//    if(!D3DApp::Initialize())
//		return false;
//		
//	return true;
//}
//
//void InitDirect3DApp::OnResize()
//{
//	D3DApp::OnResize();
//}
//
//void InitDirect3DApp::Update(const GameTimer& gt)
//{
//
//}
//
//void InitDirect3DApp::Draw(const GameTimer& gt)
//{
//    // Reuse the memory associated with command recording.
//    // We can only reset when the associated command lists have finished execution on the GPU.
//    // 重复使用记录命令的相关内存
//    // 只有当与GPU关联的命令列表执行完成时，我们才能将其重置
//	ThrowIfFailed(mDirectCmdListAlloc->Reset());
//
//	// A command list can be reset after it has been added to the command queue via ExecuteCommandList.
//    // Reusing the command list reuses memory.
//    // 在通过ExecuteCommandList方法将某个命令列表加入命令队列后，我们便可以重置该命令列表。以
//    // 此来复用命令列表及其内存
//    ThrowIfFailed(mCommandList->Reset(mDirectCmdListAlloc.Get(), nullptr));
//
//	// Indicate a state transition on the resource usage.
//    // 对资源的状态进行转换，将资源从呈现状态转换为渲染目标状态
//	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(),
//		D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));
//
//    // Set the viewport and scissor rect.  This needs to be reset whenever the command list is reset.
//    // 设置视口和裁剪矩形。它们需要随着命令列表的重置而重置
//    mCommandList->RSSetViewports(1, &mScreenViewport);
//    mCommandList->RSSetScissorRects(1, &mScissorRect);
//
//    /*
//        ClearRenderTargetView方法会将指定的渲染目标清理为给定的颜色，ClearDepthStencilView方法则用于清理指定的深度/模板缓冲区。
//        在每帧为了刷新场景而开始绘制之前，我们总是要清除后台缓冲区渲染目标和深度/模板缓冲区。这两个方法的声明如下:
//
//        void ID3D12GraphicsCommandList::ClearRenderTargetView(
//          D3D12_CPU_DESCRIPTOR_HANDLE RenderTargetView,
//          const FLOAT ColorRGBA[ 4 ],
//          UINT NumRects,
//          const D3D12_RECT *pRects);
//        1．RenderTargetView：待清除的资源RTV。
//        2．ColorRGBA：定义即将为渲染目标填充的颜色。
//        3．NumRects：pRects数组中的元素数量。此值可以为0。
//        4．pRects：一个D3D12_RECT类型的数组，指定了渲染目标将要被清除的多个矩形区域。
//            若设定此参数为nullptr，则表示清除整个渲染目标。
//
//        void ID3D12GraphicsCommandList::ClearDepthStencilView( 
//          D3D12_CPU_DESCRIPTOR_HANDLE DepthStencilView,
//          D3D12_CLEAR_FLAGS ClearFlags,
//          FLOAT Depth,
//          UINT8 Stencil,
//          UINT NumRects,
//          const D3D12_RECT *pRects);
//        1．DepthStencilView：待清除的深度/模板缓冲区DSV。
//        2．ClearFlags：该标志用于指定即将清除的是深度缓冲区还是模板缓冲区。
//            我们可以将此参数设置为D3D12_CLEAR_FLAG_DEPTH或D3D12_CLEAR_FLAG_STENCIL，也可以用按位或运算符连接两者，表示同时清除这两种缓冲区。
//        3．Depth：以此值来清除深度缓冲区。
//        4．Stencil：以此值来清除模板缓冲区。
//        5．NumRects：pRects数组内的元素数量。可以将此值设置为0。
//        6．pRects：一个D3D12_RECT类型的数组，用以指定资源视图将要被清除的多个矩形区域。将此值设置为nullptr，则表示清除整个渲染目标。
//    */
//    // Clear the back buffer and depth buffer.
//    // 清除后台缓冲区和深度缓冲区
//	mCommandList->ClearRenderTargetView(CurrentBackBufferView(), Colors::LightSteelBlue, 0, nullptr);
//	mCommandList->ClearDepthStencilView(DepthStencilView(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);
//	
//    /*
//        OMSetRenderTargets:通过此方法即可设置我们希望在渲染流水线上使用的渲染目标和深度/模板缓冲区。
//        （到目前为止，我们仅是把当前的后台缓冲区作为渲染目标，并只设置了一个主深度/模板缓冲区。但在本书的后续章节里，
//        我们还将运用多渲染目标技术）。此方法的原型如下:
//        void ID3D12GraphicsCommandList::OMSetRenderTargets(
//          UINT NumRenderTargetDescriptors,
//          const D3D12_CPU_DESCRIPTOR_HANDLE *pRenderTargetDescriptors,
//          BOOL RTsSingleHandleToDescriptorRange,
//          const D3D12_CPU_DESCRIPTOR_HANDLE *pDepthStencilDescriptor);
//        1．NumRenderTargetDescriptors：待绑定的RTV数量，即pRenderTargetDescriptors数组中的元素个数。
//            在使用多渲染目标这种高级技术时会涉及此参数。就目前来说，我们总是使用一个RTV。
//        2．pRenderTargetDescriptors：指向RTV数组的指针，用于指定我们希望绑定到渲染流水线上的渲染目标。
//        3．RTsSingleHandleToDescriptorRange：如果pRenderTargetDescriptors数组中的所有RTV对象在描述符堆中都是连续存放的，
//            就将此值设为true，否则设为false。
//        4．pDepthStencilDescriptor：指向一个DSV的指针，用于指定我们希望绑定到渲染流水线上的深度/模板缓冲区。
//    */
//    // Specify the buffers we are going to render to.
//    // 指定将要渲染的缓冲区
//	mCommandList->OMSetRenderTargets(1, &CurrentBackBufferView(), true, &DepthStencilView());
//	
//    // Indicate a state transition on the resource usage.
//    // 再次对资源状态进行转换，将资源从渲染目标状态转换回呈现状态
//	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(),
//		D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));
//
//    // Done recording commands.
//    // 完成命令的记录
//	ThrowIfFailed(mCommandList->Close());
// 
//    // Add the command list to the queue for execution.
//    // 将待执行的命令列表加入命令队列
//	ID3D12CommandList* cmdsLists[] = { mCommandList.Get() };
//	mCommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);
//	
//    /*
//        最后要通过IDXGISwapChain::Present方法来交换前、后台缓冲区。与此同时，我们也必须对索引进行更新，使之一直指向交换后的当前后台缓冲区。
//        这样一来，我们才可以正确地将下一帧场景渲染到新的后台缓冲区。
//    */
//	// swap the back and front buffers
//    // 交换后台缓冲区和前台缓冲区
//	ThrowIfFailed(mSwapChain->Present(0, 0));
//	mCurrBackBuffer = (mCurrBackBuffer + 1) % SwapChainBufferCount;
//
//	// Wait until frame commands are complete.  This waiting is inefficient and is
//	// done for simplicity.  Later we will show how to organize our rendering code
//	// so we do not have to wait per frame.
//    // 等待此帧的命令执行完毕。当前的实现没有什么效率，也过于简单
//    // 我们在后面将重新组织渲染部分的代码，以免在每一帧都要等待
//	FlushCommandQueue();
//}
//
///*
//    小结:
//    1．可以把Direct3D 看作是一种介于程序员和图形硬件之间的“桥梁”。借此，程序员便可以通过调用Direct3D函数来实现把资源视图绑定到硬件渲染流水线、配置渲染流水线的输出以及绘制3D几何体等操作。
//    2．组件对象模型（COM）是一种可以令DirectX不依赖于特定语言且向后兼容的技术。Direct3D程序员不需知道COM的具体实现细节，也无需了解其工作原理，只需知晓如何获取和释放COM接口即可。
//    3．1D、2D、3D纹理分别类似于由数据元素所构成的1D、2D、3D数组。纹理元素的格式必定为DXGI_FORMAT枚举类型中的成员之一。除了常见的图像数据，纹理也能存储像深度信息等其他类型的数据（如深度缓冲区就是一种存储深度值的纹理）。
//        GPU可以对纹理进行特殊的操作，比如运用过滤器和进行多重采样。
//    4．为了避免动画中发生闪烁的问题，最好将动画帧完全绘制到一种称为后台缓冲区的离屏纹理中。只要依此行事，显示在屏幕上的就会是一个完整的动画帧，观者也就不会察觉到帧的绘制过程。当动画帧被绘制在后台缓冲区后，
//        前台缓冲区与后台缓冲区的角色也就该互换了：为了显示下一帧动画，此前的后台缓冲区将变为前台缓冲区，而此前的前台缓冲区亦会变成后台缓冲区。后台和前台缓冲区交换角色的行为称为呈现（present）。
//        前台和后台缓冲区构成了交换链，在代码中通过IDXGISwapChain接口来表示。使用两个缓冲区（前台和后台）的情况称作双缓冲。
//    5．假设场景中有一些不透明的物体，那么离摄像机最近的物体上的点便会遮挡住它后面一切物体上的对应点。深度缓冲就是一种用于确定在场景中离摄像机最近点的技术。
//        通过这种技术，我们就不必再担心场景中物体的绘制顺序了。
//    6．在Direct3D中，资源不能直接与渲染流水线相绑定。为此，我们需要为绘制调用时所引用的资源指定描述符。我们可将描述符对象看作是GPU识别以及描述资源的一种轻量级结构体。
//        而且，我们还可以为同一种资源创建不同的描述符。如此一来，一种资源就可以具有多种用途。例如，我们可以借此将同一种资源绑定到渲染流水线的不同阶段，或者用不同的DXGI_FORMAT成员将它描述为不同的格式。
//        应用程序可通过创建描述符堆来为描述符分配所需的内存。
//    7．ID3D12Device是Direct3D中最重要的接口，我们可以把它看作是图形硬件设备的软件控制器。我们能够通过它来创建GPU资源以及其他用于控制图形硬件的特定接口。
//    8．每个GPU中都至少有一个命令队列。CPU可通过Direct3D API用命令列表向该队列提交命令，而这些命令则指挥GPU执行某些操作。在命令没有到达队列首部以前，用户所提交的命令是无法被执行的。
//        如果命令队列内为空，则GPU会因为没有任务要去处理而处于空闲状态；但若命令队列被装得太满，则CPU将在某个时刻因提交命令的速度追上GPU执行命令的速度而进入空闲状态。值得一提的是，这两种情景其实都没有充分地利用系统资源。
//    9．GPU是系统中与CPU一起并行工作的第二种处理器。有时，我们需要对CPU与GPU进行同步。例如，若GPU命令队列中有一条引用某资源的命令，那么在GPU完成此命令的处理之前，CPU就不能修改或销毁这一资源。
//        任何同步方法都会导致其中的一种处理器处于一段等待和空闲的状态，这意味着两种处理器并没有被充分利用，因此，我们应尽量减少同步的次数，并缩短同步的时间。
//    10．性能计数器是一种高精度的计时器，它是测量微小时间差的一种有效工具。例如，我们可以用它来测量两帧之间的间隔时间。性能计时器使用的时间单位称为计数（count）。QueryPerformanceFrequency函数输出的是性能计时器每秒的计数，
//        可用它将计数单位转换为秒。性能计时器的当前时间值（以计数为单位测量）可用QueryPerformanceCounter函数获得。
//    11．通过统计时间段[插图]内处理的帧数即可计算出每秒的平均帧数（FPS）。设[插图]为时间[插图]内处理的帧数，那么该时间段内每秒的平均帧数为[插图]。采用帧率进行考量可能会对性能造成一些误判，相对而言，
//        “处理一帧所花费时间”这个统计信息可能更加精准、直观。以秒为单位表示的每帧平均处理时间可以用帧率的倒数来计算，即[插图]。
//    12．示例框架为本书的全部例程都提供了统一的接口。d3dUtil.h、d3dUtil.cpp、d3dApp.h和d3dApp.cpp文件封装了所有应用程序必须实现的标准初始化代码。封装这些代码便隐藏了相关的细节，这样的话，我们就可以将精力集中在不同示例的特定主题之上。
//    13．为了开启调试模式需要启用调试层（debugController->EnableDebugLayer()）。如此一来，Direct3D就会把调试信息发往VC++的输出窗口。
//*/
//
//
//


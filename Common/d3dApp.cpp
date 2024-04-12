//***************************************************************************************
// d3dApp.cpp by Frank Luna (C) 2015 All Rights Reserved.
//***************************************************************************************

#include "d3dApp.h"
#include <WindowsX.h>

using Microsoft::WRL::ComPtr;
using namespace std;
using namespace DirectX;

LRESULT CALLBACK
MainWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	// Forward hwnd on because we can get messages (e.g., WM_CREATE)
	// before CreateWindow returns, and thus before mhMainWnd is valid.
    return D3DApp::GetApp()->MsgProc(hwnd, msg, wParam, lParam);
}

D3DApp* D3DApp::mApp = nullptr;
D3DApp* D3DApp::GetApp()
{
    return mApp;
}

D3DApp::D3DApp(HINSTANCE hInstance)
:	mhAppInst(hInstance)
{
    // Only one D3DApp can be constructed.
    assert(mApp == nullptr);
    mApp = this;
}

D3DApp::~D3DApp()
{
	if(md3dDevice != nullptr)
		FlushCommandQueue();
}

HINSTANCE D3DApp::AppInst()const
{
	return mhAppInst;
}

HWND D3DApp::MainWnd()const
{
	return mhMainWnd;
}

float D3DApp::AspectRatio()const
{
	return static_cast<float>(mClientWidth) / mClientHeight;
}

bool D3DApp::Get4xMsaaState()const
{
    return m4xMsaaState;
}

void D3DApp::Set4xMsaaState(bool value)
{
    if(m4xMsaaState != value)
    {
        m4xMsaaState = value;

        // Recreate the swapchain and buffers with new multisample settings.
        CreateSwapChain();
        OnResize();
    }
}

int D3DApp::Run()
{
	MSG msg = {0};
 
	mTimer.Reset();

	while(msg.message != WM_QUIT)
	{
		// If there are Window messages then process them.
		if(PeekMessage( &msg, 0, 0, 0, PM_REMOVE ))
		{
            TranslateMessage( &msg );
            DispatchMessage( &msg );
		}
		// Otherwise, do animation/game stuff.
		else
        {	
			mTimer.Tick();

			if( !mAppPaused )
			{
				CalculateFrameStats();
				Update(mTimer);	
                Draw(mTimer);
			}
			else
			{
				Sleep(100);
			}
        }
    }

	return (int)msg.wParam;
}

bool D3DApp::Initialize()
{
	if(!InitMainWindow())
		return false;

	if(!InitDirect3D())
		return false;

    // Do the initial resize code.
    OnResize();

	return true;
}
 
void D3DApp::CreateRtvAndDsvDescriptorHeaps()
{
    D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc;
    rtvHeapDesc.NumDescriptors = SwapChainBufferCount;
    rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	rtvHeapDesc.NodeMask = 0;
    ThrowIfFailed(md3dDevice->CreateDescriptorHeap(
        &rtvHeapDesc, IID_PPV_ARGS(mRtvHeap.GetAddressOf())));


    D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc;
    dsvHeapDesc.NumDescriptors = 1;
    dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
    dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	dsvHeapDesc.NodeMask = 0;
    ThrowIfFailed(md3dDevice->CreateDescriptorHeap(
        &dsvHeapDesc, IID_PPV_ARGS(mDsvHeap.GetAddressOf())));
}

void D3DApp::OnResize()
{
	assert(md3dDevice);
	assert(mSwapChain);
    assert(mDirectCmdListAlloc);

	// Flush before changing any resources.
	FlushCommandQueue();

    /*
        我们可以创建出多个关联于同一命令分配器的命令列表，但是不能同时用它们来记录命令。
        因此，当其中的一个命令列表在记录命令时，必须关闭同一命令分配器的其他命令列表。
        换句话说，要保证命令列表中的所有命令都会按顺序连续地添加到命令分配器内。
        还要注意的一点是，当创建或重置一个命令列表的时候，它会处于一种“打开”的状态。
        所以，当尝试为同一个命令分配器连续创建两个命令列表时，我们会得到这样的一个错误消息：
        "D3D12 ERROR: ID3D12CommandList::{Create,Reset}CommandList: The command allocator is currently in-use by another command list.
        （D3D    12 错误: ID3D12CommandList::{Create,Reset}CommandList:此命令分配器正在被另一个命令列表占用）"

        在调用 ID3D12CommandQueue::ExecuteCommandList(C) 方法之后，我们就可以通过ID3D12GraphicsCommandList::Reset方法，
        安全地复用命令列表C占用的相关底层内存来记录新的命令集。
        Reset方法中的参数对应于以ID3D12Device::CreateCommandList方法创建命令列表时所用的参数。
        HRESULT ID3D12GraphicsCommandList::Reset( 
          ID3D12CommandAllocator *pAllocator,
          ID3D12PipelineState *pInitialState);
        此方法将命令列表恢复为刚创建时的初始状态，我们可以借此继续复用其低层内存，也可以避免释放旧列表再创建新列表这一系列的烦琐操作。
        注意，重置命令列表并不会影响命令队列中的命令，因为相关的命令分配器仍在维护着其内存中被命令队列引用的系列命令。
    */
    ThrowIfFailed(mCommandList->Reset(mDirectCmdListAlloc.Get(), nullptr));

	// Release the previous resources we will be recreating.
	for (int i = 0; i < SwapChainBufferCount; ++i)
		mSwapChainBuffer[i].Reset();
    mDepthStencilBuffer.Reset();
	
	// Resize the swap chain.
    ThrowIfFailed(mSwapChain->ResizeBuffers(
		SwapChainBufferCount, 
		mClientWidth, mClientHeight, 
		mBackBufferFormat, 
		DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH));

	mCurrBackBuffer = 0;
 
	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHeapHandle(mRtvHeap->GetCPUDescriptorHandleForHeapStart());
	for (UINT i = 0; i < SwapChainBufferCount; i++)
	{
		ThrowIfFailed(mSwapChain->GetBuffer(i, IID_PPV_ARGS(&mSwapChainBuffer[i])));
		md3dDevice->CreateRenderTargetView(mSwapChainBuffer[i].Get(), nullptr, rtvHeapHandle);
		rtvHeapHandle.Offset(1, mRtvDescriptorSize);
	}

    // Create the depth/stencil buffer and view.
    D3D12_RESOURCE_DESC depthStencilDesc;
    depthStencilDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    depthStencilDesc.Alignment = 0;
    depthStencilDesc.Width = mClientWidth;
    depthStencilDesc.Height = mClientHeight;
    depthStencilDesc.DepthOrArraySize = 1;
    depthStencilDesc.MipLevels = 1;

	// Correction 11/12/2016: SSAO chapter requires an SRV to the depth buffer to read from 
	// the depth buffer.  Therefore, because we need to create two views to the same resource:
	//   1. SRV format: DXGI_FORMAT_R24_UNORM_X8_TYPELESS
	//   2. DSV Format: DXGI_FORMAT_D24_UNORM_S8_UINT
	// we need to create the depth buffer resource with a typeless format.  
	depthStencilDesc.Format = DXGI_FORMAT_R24G8_TYPELESS;

    depthStencilDesc.SampleDesc.Count = m4xMsaaState ? 4 : 1;
    depthStencilDesc.SampleDesc.Quality = m4xMsaaState ? (m4xMsaaQuality - 1) : 0;
    depthStencilDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    depthStencilDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

    D3D12_CLEAR_VALUE optClear;
    optClear.Format = mDepthStencilFormat;
    optClear.DepthStencil.Depth = 1.0f;
    optClear.DepthStencil.Stencil = 0;
    ThrowIfFailed(md3dDevice->CreateCommittedResource(
        &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
        &depthStencilDesc,
		D3D12_RESOURCE_STATE_COMMON,
        &optClear,
        IID_PPV_ARGS(mDepthStencilBuffer.GetAddressOf())));

    // Create descriptor to mip level 0 of entire resource using the format of the resource.
	D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc;
	dsvDesc.Flags = D3D12_DSV_FLAG_NONE;
	dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
	dsvDesc.Format = mDepthStencilFormat;
	dsvDesc.Texture2D.MipSlice = 0;
    md3dDevice->CreateDepthStencilView(mDepthStencilBuffer.Get(), &dsvDesc, DepthStencilView());

    // Transition the resource from its initial state to be used as a depth buffer.
	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mDepthStencilBuffer.Get(),
		D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_DEPTH_WRITE));
	
    // Execute the resize commands.
    ThrowIfFailed(mCommandList->Close());
    ID3D12CommandList* cmdsLists[] = { mCommandList.Get() };
    // ExecuteCommandLists是一种常用的ID3D12CommandQueue接口方法，利用它可将命令列表里的命令添加到命令队列之中：
    // 这里才是真的执行command
    mCommandQueue->ExecuteCommandLists(
        _countof(cmdsLists), // 第二个参数里命令列表数组中命令列表的数量
        cmdsLists            // 待执行的命令列表数组，指向命令列表数组中第一个元素的指针
    );
  
	// Wait until resize is complete.
	FlushCommandQueue();

	// Update the viewport transform to cover the client area.
	mScreenViewport.TopLeftX = 0;
	mScreenViewport.TopLeftY = 0;
	mScreenViewport.Width    = static_cast<float>(mClientWidth);
	mScreenViewport.Height   = static_cast<float>(mClientHeight);
	mScreenViewport.MinDepth = 0.0f;
	mScreenViewport.MaxDepth = 1.0f;

    mScissorRect = { 0, 0, mClientWidth, mClientHeight };
}
 
LRESULT D3DApp::MsgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch( msg )
	{
	// WM_ACTIVATE is sent when the window is activated or deactivated.  
	// We pause the game when the window is deactivated and unpause it 
	// when it becomes active.  
	case WM_ACTIVATE:
		if( LOWORD(wParam) == WA_INACTIVE )
		{
			mAppPaused = true;
			mTimer.Stop();
		}
		else
		{
			mAppPaused = false;
			mTimer.Start();
		}
		return 0;

	// WM_SIZE is sent when the user resizes the window.  
	case WM_SIZE:
		// Save the new client area dimensions.
		mClientWidth  = LOWORD(lParam);
		mClientHeight = HIWORD(lParam);
		if( md3dDevice )
		{
			if( wParam == SIZE_MINIMIZED )
			{
				mAppPaused = true;
				mMinimized = true;
				mMaximized = false;
			}
			else if( wParam == SIZE_MAXIMIZED )
			{
				mAppPaused = false;
				mMinimized = false;
				mMaximized = true;
				OnResize();
			}
			else if( wParam == SIZE_RESTORED )
			{
				
				// Restoring from minimized state?
				if( mMinimized )
				{
					mAppPaused = false;
					mMinimized = false;
					OnResize();
				}

				// Restoring from maximized state?
				else if( mMaximized )
				{
					mAppPaused = false;
					mMaximized = false;
					OnResize();
				}
				else if( mResizing )
				{
					// If user is dragging the resize bars, we do not resize 
					// the buffers here because as the user continuously 
					// drags the resize bars, a stream of WM_SIZE messages are
					// sent to the window, and it would be pointless (and slow)
					// to resize for each WM_SIZE message received from dragging
					// the resize bars.  So instead, we reset after the user is 
					// done resizing the window and releases the resize bars, which 
					// sends a WM_EXITSIZEMOVE message.
				}
				else // API call such as SetWindowPos or mSwapChain->SetFullscreenState.
				{
					OnResize();
				}
			}
		}
		return 0;

	// WM_EXITSIZEMOVE is sent when the user grabs the resize bars.
	case WM_ENTERSIZEMOVE:
		mAppPaused = true;
		mResizing  = true;
		mTimer.Stop();
		return 0;

	// WM_EXITSIZEMOVE is sent when the user releases the resize bars.
	// Here we reset everything based on the new window dimensions.
	case WM_EXITSIZEMOVE:
		mAppPaused = false;
		mResizing  = false;
		mTimer.Start();
		OnResize();
		return 0;
 
	// WM_DESTROY is sent when the window is being destroyed.
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;

	// The WM_MENUCHAR message is sent when a menu is active and the user presses 
	// a key that does not correspond to any mnemonic or accelerator key. 
	case WM_MENUCHAR:
        // Don't beep when we alt-enter.
        return MAKELRESULT(0, MNC_CLOSE);

	// Catch this message so to prevent the window from becoming too small.
	case WM_GETMINMAXINFO:
		((MINMAXINFO*)lParam)->ptMinTrackSize.x = 200;
		((MINMAXINFO*)lParam)->ptMinTrackSize.y = 200; 
		return 0;

	case WM_LBUTTONDOWN:
	case WM_MBUTTONDOWN:
	case WM_RBUTTONDOWN:
		OnMouseDown(wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
		return 0;
	case WM_LBUTTONUP:
	case WM_MBUTTONUP:
	case WM_RBUTTONUP:
		OnMouseUp(wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
		return 0;
	case WM_MOUSEMOVE:
		OnMouseMove(wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
		return 0;
    case WM_KEYUP:
        if(wParam == VK_ESCAPE)
        {
            PostQuitMessage(0);
        }
        else if((int)wParam == VK_F2)
            Set4xMsaaState(!m4xMsaaState);

        return 0;
	}

	return DefWindowProc(hwnd, msg, wParam, lParam);
}

bool D3DApp::InitMainWindow()
{
	WNDCLASS wc;
	wc.style         = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc   = MainWndProc; 
	wc.cbClsExtra    = 0;
	wc.cbWndExtra    = 0;
	wc.hInstance     = mhAppInst;
	wc.hIcon         = LoadIcon(0, IDI_APPLICATION);
	wc.hCursor       = LoadCursor(0, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)GetStockObject(NULL_BRUSH);
	wc.lpszMenuName  = 0;
	wc.lpszClassName = L"MainWnd";

	if( !RegisterClass(&wc) )
	{
		MessageBox(0, L"RegisterClass Failed.", 0, 0);
		return false;
	}

	// Compute window rectangle dimensions based on requested client area dimensions.
	RECT R = { 0, 0, mClientWidth, mClientHeight };
    AdjustWindowRect(&R, WS_OVERLAPPEDWINDOW, false);
	int width  = R.right - R.left;
	int height = R.bottom - R.top;

	mhMainWnd = CreateWindow(L"MainWnd", mMainWndCaption.c_str(), 
		WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, width, height, 0, 0, mhAppInst, 0); 
	if( !mhMainWnd )
	{
		MessageBox(0, L"CreateWindow Failed.", 0, 0);
		return false;
	}

	ShowWindow(mhMainWnd, SW_SHOW);
	UpdateWindow(mhMainWnd);

	return true;
}

/// <summary>
/// 初始化Direct3D入口
/// 这一节我们会利用自己编写的演示框架来展示Direct3D的初始化过程。
/// 这是一个比较冗长的流程，但每个程序只需执行一次即可。
/// 我们对Direct3D进行初始化的过程可以分为以下几个步骤：
/// 1．用D3D12CreateDevice函数创建ID3D12Device接口实例。
/// 2．创建一个ID3D12Fence对象，并查询描述符的大小。
/// 3．检测用户设备对4X MSAA质量级别的支持情况。
/// 4．依次创建命令队列、命令列表分配器和主命令列表。
/// 5．描述并创建交换链。
/// 6．创建应用程序所需的描述符堆。
/// 7．调整后台缓冲区的大小，并为它创建渲染目标视图。
/// 8．创建深度/模板缓冲区及与之关联的深度/模板视图。
/// 9．设置视口（viewport）和裁剪矩形（scissor rectangle）
/// </summary>
/// <returns></returns>
bool D3DApp::InitDirect3D()
{
#pragma region 创建设备
#if defined(DEBUG) || defined(_DEBUG) 
	// Enable the D3D12 debug layer.
    // 创建设备-启用D3D12的调试层
{
	ComPtr<ID3D12Debug> debugController;
	ThrowIfFailed(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)));
	debugController->EnableDebugLayer();
}
#endif

	ThrowIfFailed(CreateDXGIFactory1(IID_PPV_ARGS(&mdxgiFactory)));

	// Try to create hardware device.
    // 创建设备-尝试创建硬件设备
    /*
    D3D12CreateDevice原型:
    HRESULT WINAPI D3D12CreateDevice(
      IUnknown* pAdapter,
      D3D_FEATURE_LEVEL MinimumFeatureLevel,
      REFIID riid, // ID3D12Device的COM ID
      void** ppDevice );
      1．pAdapter：指定在创建设备时所用的显示适配器。若将此参数设定为空指针，则使用主显示适配器。我们在本书的示例中总是采用主适配器。在4.1.10节中，我们已展示了怎样枚举系统中所有的显示适配器。
      2．MinimumFeatureLevel：应用程序需要硬件所支持的最低功能级别。如果适配器不支持此功能级别，则设备创建失败。在我们的框架中指定的是D3D_FEATURE_LEVEL_11_0（即支持Direct3D 11的特性）。
      3．riid：所建ID3D12Device接口的COM ID。
      4．ppDevice：返回所创建的Direct3D 12设备。
    */
	HRESULT hardwareResult = D3D12CreateDevice(
		nullptr,             // default adapter(默认适配器)
		D3D_FEATURE_LEVEL_11_0,
		IID_PPV_ARGS(&md3dDevice));

	// Fallback to WARP device.
    // 创建设备-回退至WARP设备
    /*
    当调用D3D12CreateDevice失败后，程序将回退到一种软件适配器：WARP设备。
    WARP意为Windows Advanced Rasterization Platform（Windows高级光栅化平台）。
    */
	if(FAILED(hardwareResult))
	{
		ComPtr<IDXGIAdapter> pWarpAdapter;
		ThrowIfFailed(mdxgiFactory->EnumWarpAdapter(IID_PPV_ARGS(&pWarpAdapter)));

		ThrowIfFailed(D3D12CreateDevice(
			pWarpAdapter.Get(),
			D3D_FEATURE_LEVEL_11_0,
			IID_PPV_ARGS(&md3dDevice)));
	}
#pragma endregion

#pragma region 创建围栏并获取描述符的大小
    /*
    一旦创建好设备，便可以为CPU/GPU的同步而创建围栏了。
    另外，若用描述符进行工作，还需要了解它们的大小。
    但描述符在不同的GPU平台上大小各异，这就需要我们去查询相关的信息。
    随后，我们会把描述符的大小缓存起来，需要时即可直接引用:
    */
	ThrowIfFailed(md3dDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE,
		IID_PPV_ARGS(&mFence)));

	mRtvDescriptorSize = md3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	mDsvDescriptorSize = md3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
	mCbvSrvUavDescriptorSize = md3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
#pragma endregion

#pragma region 检测对4X MSAA质量级别的支持
    // Check 4X MSAA quality support for our back buffer format.
    // All Direct3D 11 capable devices support 4X MSAA for all render 
    // target formats, so we only need to check quality support.

	D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS msQualityLevels;
	msQualityLevels.Format = mBackBufferFormat;
	msQualityLevels.SampleCount = 4;
	msQualityLevels.Flags = D3D12_MULTISAMPLE_QUALITY_LEVELS_FLAG_NONE;
	msQualityLevels.NumQualityLevels = 0;
    // CheckFeatureSupport：查找采样质量级别
    /*
    注意，此方法的第二个参数兼具输入和输出的属性。当它作为输入参数时，我们必须指定纹理格式、
    采样数量以及希望查询的多重采样所支持的标志（即立flag，或作旗标）。
    接着，待函数执行后便会填写图像质量级别作为输出。对于某种纹理格式和采样数量的组合来讲，
    其质量级别的有效范围为 0 至NumQualityLevels–1.

    每个像素的最大采样数量被定义为：#define   D3D12_MAX_MULTISAMPLE_SAMPLE_COUNT ( 32 )

    但是，考虑到多重采样会占用内存资源，又为了保证程序性能等原因，通常会把采样数量设定为4或8。
    如果不希望使用多重采样，则可将采样数量设置为1，并令质量级别为0。
    其实在所有支持Direct3D 11的设备上，就已经可以对所有的渲染目标格式采用4X多重采样了。

****功能支持的检测:
    我们已经通过ID3D12Device::CheckFeatureSupport方法，检测了当前图形驱动对多重采样的支持。
    然而，这只是此函数对功能支持检测的冰山一角。这个方法的原型为：
    HRESULT ID3D12Device::CheckFeatureSupport(
      D3D12_FEATURE Feature,
      void *pFeatureSupportData,
      UINT FeatureSupportDataSize);
      1．Feature：枚举类型D3D12_FEATURE中的成员之一，用于指定我们希望检测的功能支持类型。
        a）D3D12_FEATURE_D3D12_OPTIONS：检测当前图形驱动对Direct3D 12各种功能的支持情况。
        b）D3D12_FEATURE_ARCHITECTURE：检测图形适配器中GPU的硬件体系架构特性。
        c）D3D12_FEATURE_FEATURE_LEVELS：检测对功能级别的支持情况。
        d）D3D12_FEATURE_FORMAT_SUPPORT：检测对给定纹理格式的支持情况（例如，指定的格式能否用于渲染目标？或，指定的格式能否用于混合技术？）。
        e）D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS：检测对多重采样功能的支持情况。
      2．pFeatureSupportData：指向某种数据结构的指针，该结构中存有检索到的特定功能支持的信息。此结构体的具体类型取决于Feature参数。
        a）如果将Feature参数指定为D3D12_FEATURE_D3D12_OPTIONS，则传回的是一个D3D12_FEATURE_DATA_D3D12_OPTIONS实例。
        b）如果将Feature参数指定为D3D12_FEATURE_ARCHITECTURE，则传回的是一个D3D12_FEATURE_DATA_ARCHITECTURE实例。
        c）如果将Feature参数指定为D3D12_FEATURE_FEATURE_LEVELS，则传回的是一个D3D12_FEATURE_DATA_FEATURE_LEVELS实例。
        d）如果将Feature参数指定为D3D12_FEATURE_FORMAT_SUPPORT，则传回的是一个D3D12_FEATURE_DATA_FORMAT_SUPPORT实例。
        e）如果将Feature参数指定为D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS，则传回的是一个D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS实例。
      3．FeatureSupportDataSize：传回pFeatureSupportData参数中的数据结构的大小。
    */
	ThrowIfFailed(md3dDevice->CheckFeatureSupport(
		D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS,
		&msQualityLevels,
		sizeof(msQualityLevels)));

    m4xMsaaQuality = msQualityLevels.NumQualityLevels;
    // 由于我们所用的平台必能支持4X MSAA这一功能，其返回值应该也总是大于0，所以对此而做出上述断言
	assert(m4xMsaaQuality > 0 && "Unexpected MSAA quality level.");
#pragma endregion

#ifdef _DEBUG
    LogAdapters();
#endif

    // 创建命令队列和命令列表
	CreateCommandObjects();
    // 描述并创建交换链
    CreateSwapChain();
    // 创建描述符堆
    CreateRtvAndDsvDescriptorHeaps();

	return true;
}

/// <summary>
/// 创建命令队列和命令列表
/// </summary>
void D3DApp::CreateCommandObjects()
{
    /*
      GPU将从数组里的第一个命令列表开始顺序执行.
      ID3D12GraphicsCommandList[23]接口封装了一系列图形渲染命令，它实际上继承于ID3D12CommandList接口。
      ID3D12GraphicsCommandList接口有数种方法向命令列表添加命令。
      例如，下面的代码依次就向命令列表中添加了设置视口、清除渲染目标视图和发起绘制调用的命令：
      // mCommandList为一个指向ID3D12CommandList接口的指针
      mCommandList->RSSetViewports(1, &mScreenViewport);
      mCommandList->ClearRenderTargetView(mBackBufferView,
        Colors::LightSteelBlue, 0, nullptr);
      mCommandList->DrawIndexedInstanced(36, 1, 0, 0, 0);

      虽然这些方法的名字看起来像是会使对应的命令立即执行，但事实却并非如此，上面的代码仅仅是将命令加入命令列表而已。
      调用ExecuteCommandLists方法才会将命令真正地送入命令队列，供GPU在合适的时机处理。
    */
    /*
        在Direct3D 12中，命令队列被抽象为ID3D12CommandQueue接口来表示。
        要通过填写D3D12_COMMAND_QUEUE_DESC结构体来描述队列，再调用ID3D12Device::CreateCommandQueue方法创建队列。
        我们在本书中将实际采用以下流程：
    */
	D3D12_COMMAND_QUEUE_DESC queueDesc = {};
	queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    /*
        IID_PPV_ARGS辅助宏的定义如下：
        #define IID_PPV_ARGS(ppType) __uuidof(**(ppType)), IID_PPV_ARGS_Helper(ppType)
        其中，__uuidof(**(ppType))将获取(**(ppType))的COM接口ID（globally unique identifier，
        全局唯一标识符，GUID），在上述代码段中得到的即为ID3D12CommandQueue接口的COM ID。
        IID_PPV_ARGS辅助函数的本质是将ppType强制转换为void**类型。
        我们在全书中都会见到此宏的身影，这是因为在调用Direct3D 12中创建接口实例的API时，
        大多都有一个参数是类型为void**的待创接口COM ID。
    */
	ThrowIfFailed(md3dDevice->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&mCommandQueue)));
    
    /*
        还有一种与命令列表有关的名为ID3D12CommandAllocator的内存管理类接口。
        记录在命令列表内的命令，实际上是存储在与之关联的命令分配器（command allocator）上。
        当通过ID3D12CommandQueue::ExecuteCommandLists方法执行命令列表的时候，
        命令队列就会引用分配器里的命令。而命令分配器则由ID3D12Device接口来创建：
        HRESULT ID3D12Device::CreateCommandAllocator(
          D3D12_COMMAND_LIST_TYPE type,
          REFIID riid,
          void **ppCommandAllocator);
          1．type：指定与此命令分配器相关联的命令列表类型。以下是本书常用的两种命令列表类型[24]。
            a）D3D12_COMMAND_LIST_TYPE_DIRECT。存储的是一系列可供GPU直接执行的命令（这种类型的命令列表我们之前曾提到过）。
            b）D3D12_COMMAND_LIST_TYPE_BUNDLE。将命令列表打包（bundle，也有译作集合）。
               构建命令列表时会产生一定的CPU开销，为此，Direct3D 12提供了一种优化的方法，允许我们将一系列命令打成所谓的包。
               当打包完成（命令记录完毕）之后，驱动就会对其中的命令进行预处理，以使它们在渲染期间的执行过程中得到优化。
               因此，我们应当在初始化期间就用包记录命令。如果经过分析，发现构造某些命令列表会花费大量的时间，就可以考虑使用打包技术对其进行优化。
               Direct3D 12中的绘制API的效率很高，所以一般不会用到打包技术。因此，也许在证明其确实可以带来性能的显著提升时才会用到它。
               这就是说，在大多数情况下，我们往往会将其束之高阁。本书中不会使用打包技术，关于它的详情可参见DirectX 12文档[25]。
          2．riid：待创建ID3D12CommandAllocator接口的COM ID。
          3．ppCommandAllocator：输出指向所建命令分配器的指针。
    
    */
    // 命令分配器
	ThrowIfFailed(md3dDevice->CreateCommandAllocator(
		D3D12_COMMAND_LIST_TYPE_DIRECT,
		IID_PPV_ARGS(mDirectCmdListAlloc.GetAddressOf())));

    /*
        命令列表同样由ID3D12Device接口创建：
        HRESULT ID3D12Device::CreateCommandList(
          UINT nodeMask,
          D3D12_COMMAND_LIST_TYPE type,
          ID3D12CommandAllocator *pCommandAllocator,
          ID3D12PipelineState *pInitialState,
          REFIID riid,
          void **ppCommandList);
          1．nodeMask：对于仅有一个GPU的系统而言，要将此值设为0；对于具有多GPU的系统而言，此节点掩码（node mask）指定的是与所建命令列表相关联的物理GPU。
             本书中假设我们使用的是单GPU系统。(我们可以通过ID3D12Device::GetNodeCount方法来查询系统中GPU适配器节点（物理GPU）的数量。)
          2．type：命令列表的类型，常用的选项为D3D12_COMMAND_LIST_TYPE_DIRECT和D3D12_COMMAND_LIST_TYPE_BUNDLE。
          3．pCommandAllocator：与所建命令列表相关联的命令分配器。它的类型必须与所创命令列表的类型相匹配。
          4．pInitialState：指定命令列表的渲染流水线初始状态。对于打包技术来说可将此值设为nullptr，
             另外，此法同样适用于执行命令列表中不含有任何绘制命令，即执行命令列表是为了达到初始化的目的的特殊情况。
             我们将在第6章中详细讨论ID3D12PipelineState接口。
          5．riid：待创建ID3D12CommandList接口的COM ID。
          6．ppCommandList：输出指向所建命令列表的指针。
    */
	ThrowIfFailed(md3dDevice->CreateCommandList(
		0,
		D3D12_COMMAND_LIST_TYPE_DIRECT,
		mDirectCmdListAlloc.Get(), // Associated command allocator(关联命令分配器)
		nullptr,                   // Initial PipelineStateObject(初始化流水线状态对象)
		IID_PPV_ARGS(mCommandList.GetAddressOf())));

	// Start off in a closed state.  This is because the first time we refer 
	// to the command list we will Reset it, and it needs to be closed before
	// calling Reset.
    // 首先要将命令列表置于关闭状态。这是因为在第一次引用命令列表时，我们要对它进行重置，而在调用
    // 重置方法之前又需先将其关闭
    
    // 结束记录命令
    // 当命令都被加入命令列表之后，我们必须调用ID3D12GraphicsCommandList::Close方法来结束命令的记录
	// 在调用ID3D12CommandQueue::ExecuteCommandLists方法提交命令列表之前，一定要将其关闭。
    mCommandList->Close();
}

/// <summary>
/// 描述并创建交换链
/// </summary>
void D3DApp::CreateSwapChain()
{
    // Release the previous swapchain we will be recreating.
    mSwapChain.Reset();

    // sd.BufferDesc == DXGI_SWAP_CHAIN_DESC.DXGI_MODE_DESC
    DXGI_SWAP_CHAIN_DESC sd;
    sd.BufferDesc.Width = mClientWidth;
    sd.BufferDesc.Height = mClientHeight;
    sd.BufferDesc.RefreshRate.Numerator = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.BufferDesc.Format = mBackBufferFormat;
    sd.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
    sd.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;

    sd.SampleDesc.Count = m4xMsaaState ? 4 : 1;
    sd.SampleDesc.Quality = m4xMsaaState ? (m4xMsaaQuality - 1) : 0;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.BufferCount = SwapChainBufferCount;
    sd.OutputWindow = mhMainWnd;
    sd.Windowed = true;
	sd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

	// Note: Swap chain uses queue to perform flush.
    ThrowIfFailed(mdxgiFactory->CreateSwapChain(
		mCommandQueue.Get(),
		&sd, 
		mSwapChain.GetAddressOf()));
}

/*
    如果不加围栏，cpu不阻塞，会将之前提交的命令覆盖（之前提交的这个命令好美被GPU处理）导致渲染错误
    解决此问题的一种办法是：强制CPU等待，直到GPU完成所有命令的处理，达到某个指定的围栏点（fence point）为止。我们将这种方法称为刷新命令队列（flushing the command queue），可以通过围栏（fence）来实现这一点。
    围栏用ID3D12Fence接口来表示[26]，此技术能用于实现GPU和CPU间的同步。
    创建一个围栏对象的方法如下：

    HRESULT ID3D12Device::CreateFence(
      UINT64 InitialValue,
      D3D12_FENCE_FLAGS Flags,
      REFIID riid,
      void **ppFence);

    // 示例
    ThrowIfFailed(md3dDevice->CreateFence(
      0,
      D3D12_FENCE_FLAG_NONE,
      IID_PPV_ARGS(&mFence)));

    每个围栏对象都维护着一个UINT64类型的值，此为用来标识围栏点的整数。
    起初，我们将此值设为0，每当需要标记一个新的围栏点时就将它加1。
    现在，我们用代码和注释进行展示，看看如何用一个围栏来刷新命令队列。
*/
void D3DApp::FlushCommandQueue()
{
	// Advance the fence value to mark commands up to this fence point.
    // 增加围栏值，接下来将命令标记到此围栏点
    mCurrentFence++;

    // Add an instruction to the command queue to set a new fence point.  Because we 
	// are on the GPU timeline, the new fence point won't be set until the GPU finishes
	// processing all the commands prior to this Signal().
    // 向命令队列中添加一条用来设置新围栏点的命令
    // 由于这条命令要由GPU处理(即由GPU端来修改围栏值)，所以在GPU处理完命令队列中此Signal()
    // 以前的所有命令之前，他并不会设置新的围栏点
    ThrowIfFailed(mCommandQueue->Signal(mFence.Get(), mCurrentFence));

	// Wait until the GPU has completed commands up to this fence point.
    // 在CPU端等待GPU，直到后者执行完这个围栏点之前的所有命令
    if(mFence->GetCompletedValue() < mCurrentFence)
	{
		HANDLE eventHandle = CreateEventEx(nullptr, false, false, EVENT_ALL_ACCESS);

        // Fire event when GPU hits current fence.  
        // 若GPU命中当前的围栏(即执行到Signal()指令，修改了围栏值)，则激发预定事件
        ThrowIfFailed(mFence->SetEventOnCompletion(mCurrentFence, eventHandle));

        // Wait until the GPU hits current fence event is fired.
        // 等待GPU命中围栏，激发事件
		WaitForSingleObject(eventHandle, INFINITE);
        CloseHandle(eventHandle);
	}
}

ID3D12Resource* D3DApp::CurrentBackBuffer()const
{
	return mSwapChainBuffer[mCurrBackBuffer].Get();
}

D3D12_CPU_DESCRIPTOR_HANDLE D3DApp::CurrentBackBufferView()const
{
	return CD3DX12_CPU_DESCRIPTOR_HANDLE(
		mRtvHeap->GetCPUDescriptorHandleForHeapStart(),
		mCurrBackBuffer,
		mRtvDescriptorSize);
}

D3D12_CPU_DESCRIPTOR_HANDLE D3DApp::DepthStencilView()const
{
	return mDsvHeap->GetCPUDescriptorHandleForHeapStart();
}

void D3DApp::CalculateFrameStats()
{
	// Code computes the average frames per second, and also the 
	// average time it takes to render one frame.  These stats 
	// are appended to the window caption bar.
    
	static int frameCnt = 0;
	static float timeElapsed = 0.0f;

	frameCnt++;

	// Compute averages over one second period.
	if( (mTimer.TotalTime() - timeElapsed) >= 1.0f )
	{
		float fps = (float)frameCnt; // fps = frameCnt / 1
		float mspf = 1000.0f / fps;

        wstring fpsStr = to_wstring(fps);
        wstring mspfStr = to_wstring(mspf);

        wstring windowText = mMainWndCaption +
            L"    fps: " + fpsStr +
            L"   mspf: " + mspfStr;

        SetWindowText(mhMainWnd, windowText.c_str());
		
		// Reset for next average.
		frameCnt = 0;
		timeElapsed += 1.0f;
	}
}

/*
    我们刚刚简单地叙述了DXGI的概念，下面来介绍一些在Direct3D初始化时会用到的相关接口。
    IDXGIFactory 是DXGI中的关键接口之一，主要用于创建IDXGISwapChain接口以及枚举显示适配器。
    而显示适配器则真正实现了图形处理能力。
    通常来说，显示适配器（display adapter）是一种硬件设备（例如独立显卡），
    然而系统也可以用软件显示适配器来模拟硬件的图形处理功能。一个系统中可能会存在数个适配器（比如装有数块显卡）。
    适配器用接口IDXGIAdapter来表示。我们可以用下面的代码来枚举一个系统中的所有适配器：

    下段代码输出日志:
    ***Adapter: NVIDIA GeForce GTX 760
    ***Adapter: Microsoft Basic Render Driver
*/
void D3DApp::LogAdapters()
{
    UINT i = 0;
    IDXGIAdapter* adapter = nullptr;
    std::vector<IDXGIAdapter*> adapterList;
    while(mdxgiFactory->EnumAdapters(i, &adapter) != DXGI_ERROR_NOT_FOUND)
    {
        DXGI_ADAPTER_DESC desc;
        adapter->GetDesc(&desc);

        std::wstring text = L"***Adapter: ";
        text += desc.Description;
        text += L"\n";

        OutputDebugString(text.c_str());

        adapterList.push_back(adapter);
        
        ++i;
    }

    for(size_t i = 0; i < adapterList.size(); ++i)
    {
        LogAdapterOutputs(adapterList[i]);
        ReleaseCom(adapterList[i]);
    }
}

/*
    另外，一个系统也可能装有数个显示设备。
    我们称每一台显示设备都是一个显示输出（display output，有的文档也作adapter output，适配器输出）实例，
    用IDXGIOutput接口来表示。每个适配器都与一组显示输出相关联。
    举个例子，考虑这样一个系统，该系统共有两块显卡和3台显示器，
    其中一块显卡与两台显示器相连，第三台显示器则与另一块显卡相连。
    在这种情况下，一块适配器与两个显示输出相关联，而另一块则仅有一个显示输出与之关联。
    通过以下代码，我们就可以枚举出与某块适配器关联的所有显示输出：
*/
void D3DApp::LogAdapterOutputs(IDXGIAdapter* adapter)
{
    UINT i = 0;
    IDXGIOutput* output = nullptr;
    while(adapter->EnumOutputs(i, &output) != DXGI_ERROR_NOT_FOUND)
    {
        DXGI_OUTPUT_DESC desc;
        output->GetDesc(&desc);
        
        std::wstring text = L"***Output: ";
        text += desc.DeviceName;
        text += L"\n";
        OutputDebugString(text.c_str());

        LogOutputDisplayModes(output, mBackBufferFormat);

        ReleaseCom(output);

        ++i;
    }
}

/*
每种显示设备都有一系列它所支持的显示模式，可以用下列DXGI_MODE_DESC结构体中的数据成员来加以表示：
typedef struct DXGI_MODE_DESC
{
  UINT Width;                   // 分辨率宽度
  UINT Height;                  // 分辨率高度
  DXGI_RATIONAL RefreshRate;    // 刷新率，单位为赫兹Hz
  DXGI_FORMAT Format;           // 显示格式
  DXGI_MODE_SCANLINE_ORDER ScanlineOrdering; // 逐行扫描vs.隔行扫描
  DXGI_MODE_SCALING Scaling;    // 图像如何相对于屏幕进行拉伸
} DXGI_MODE_DESC;

typedef struct DXGI_RATIONAL
{
  UINT Numerator;
  UINT Denominator;
} DXGI_RATIONAL;

typedef enum DXGI_MODE_SCANLINE_ORDER
{
  DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED    = 0,
  DXGI_MODE_SCANLINE_ORDER_PROGRESSIVE    = 1,
  DXGI_MODE_SCANLINE_ORDER_UPPER_FIELD_FIRST = 2,
  DXGI_MODE_SCANLINE_ORDER_LOWER_FIELD_FIRST = 3
} DXGI_MODE_SCANLINE_ORDER;[19]

typedef enum DXGI_MODE_SCALING
{
  DXGI_MODE_SCALING_UNSPECIFIED  = 0,
  DXGI_MODE_SCALING_CENTERED   = 1,        // 不做缩放，将图像显示在屏幕正中
  DXGI_MODE_SCALING_STRETCHED   = 2        // 根据屏幕的分辨率对图像进行拉伸缩放
} DXGI_MODE_SCALING;

一旦确定了显示模式的具体格式（DXGI_FORMAT），
我们就能通过下列代码，获得某个显示输出对此格式所支持的全部显示模式：

下面代码会输出下列相似的结果：
***Output: \\.\DISPLAY2
...
Width = 1920 Height = 1080 Refresh = 59950/1000
Width = 1920 Height = 1200 Refresh = 59950/1000
*/
void D3DApp::LogOutputDisplayModes(IDXGIOutput* output, DXGI_FORMAT format)
{
    UINT count = 0;
    UINT flags = 0;

    // Call with nullptr to get list count.
    output->GetDisplayModeList(format, flags, &count, nullptr);

    std::vector<DXGI_MODE_DESC> modeList(count);
    output->GetDisplayModeList(format, flags, &count, &modeList[0]);

    for(auto& x : modeList)
    {
        UINT n = x.RefreshRate.Numerator;
        UINT d = x.RefreshRate.Denominator;
        std::wstring text =
            L"Width = " + std::to_wstring(x.Width) + L" " +
            L"Height = " + std::to_wstring(x.Height) + L" " +
            L"Refresh = " + std::to_wstring(n) + L"/" + std::to_wstring(d) +
            L"\n";

        ::OutputDebugString(text.c_str());
    }
}




/*
****纹理格式:
    不是任意类型的数据元素都能用于组成纹理，它只能存储DXGI_FORMAT枚举类型中描述的特定格式的数据元素。
    下面是一些相关的格式示例：
    1．DXGI_FORMAT_R32G32B32_FLOAT：每个元素由3个32位浮点数分量构成。
    2．DXGI_FORMAT_R16G16B16A16_UNORM：每个元素由4个16位分量构成，每个分量都被映射到 [0, 1] 区间。
    3．DXGI_FORMAT_R32G32_UINT：每个元素由2个32位无符号整数分量构成。
    4．DXGI_FORMAT_R8G8B8A8_UNORM：每个元素由4个8位无符号分量构成，每个分量都被映射到 [0, 1] 区间。
    5．DXGI_FORMAT_R8G8B8A8_SNORM：每个元素由4个8位有符号分量构成，每个分量都被映射到 [−1, 1] 区间。
    6．DXGI_FORMAT_R8G8B8A8_SINT：每个元素由4个8位有符号整数分量构成，每个分量都被映射到 [−128, 127] 区间。
    7．DXGI_FORMAT_R8G8B8A8_UINT：每个元素由4个8位无符号整数分量构成，每个分量都被映射到 [0, 255] 区间。

****深度缓存:
    深度缓冲区也是一种纹理，所以一定要用明确的数据格式来创建它。深度缓冲可用的格式包括以下几种。
    1．DXGI_FORMAT_D32_FLOAT_S8X24_UINT：
        该格式共占用64位，取其中的32位指定一个浮点型深度缓冲区，另有8位（无符号整数）分配给模板缓冲区（stencil buffer），
        并将该元素映射到[0, 255]区间，剩下的24位仅用于填充对齐（padding）不作他用。
    2．DXGI_FORMAT_D32_FLOAT：
        指定一个32位浮点型深度缓冲区。
    3．DXGI_FORMAT_D24_UNORM_S8_UINT：
        指定一个无符号24位深度缓冲区，并将该元素映射到[0, 1]区间。
        另有8位（无符号整型）分配给模板缓冲区，将此元素映射到[0, 255]区间。
    4．DXGI_FORMAT_D16_UNORM：
        指定一个无符号16位深度缓冲区，把该元素映射到[0, 1]区间。
    *一个应用程序不一定要用到模板缓冲区。但一经使用，则深度缓冲区将总是与模板缓冲区如影随形，共同进退。
    例如，32位格式: DXGI_FORMAT_D24_UNORM_S8_UINT 使用24位作为深度缓冲区，其他8位作为模板缓冲区。
    出于这个原因，深度缓冲区叫作深度/模板缓冲区更为得体。模板缓冲区的运用是更高级的主题，在第11章中会有相应的介绍。

****资源与描述符：
    每个描述符都有一种具体类型，此类型指明了资源的具体作用。本书常用的描述符如下。
    1．CBV/SRV/UAV描述符分别表示的是
        常量缓冲区视图（constant buffer view）、
        着色器资源视图（shader resource view）
        和无序访问视图（unordered access view）这3种资源。
    2．采样器（sampler，亦有译为取样器）描述符表示的是采样器资源（用于纹理贴图）。
    3．RTV描述符表示的是渲染目标视图资源（render target view）。
    4．DSV描述符表示的是深度/模板视图资源（depth/stencil view）。

****多重采样技术的原理：
    1．超级采样技术（supersampling，可简记作SSAA，即Super Sample Anti-Aliasing）:
    它使用4倍于屏幕分辨率大小的后台缓冲区和深度缓冲区。3D场景将以这种更大的分辨率渲染到后台缓冲区中。
    当数据要从后台缓冲区调往屏幕显示的时候，会将后台缓冲区按4个像素一组进行解析
    （resolve，或称降采样，downsample。把放大的采样点数降低回原采样点数）：
    每组用求平均值的方法得到一种相对平滑的像素颜色。因此，超级采样实际上是通过软件的方式提升了画面的分辨率。
    超级采样是一种开销高昂的操作，因为它将像素的处理数量和占用的内存大小都增加到之前的4倍。
    2．多重采样技术（multisampling，可简记作MSAA，即MultiSample Anti-Aliasing）:
    这种技术通过跨子像素[12]共享一些计算信息，从而使它比超级采样的开销更低。
    现假设采用4X多重采样（即每个像素中都有4个子像素），并同样使用4倍于屏幕分辨率的后台缓冲区和深度缓冲区。
    值得注意的是，这种技术并不需要对每一个子像素都进行计算，而是仅计算一次像素中心处的颜色，
    再基于可视性（每个子像素经深度/模板测试的结果）和覆盖性（子像素的中心在多边形的里面还是外面？）
    将得到的颜色信息分享给其子像素[13]

****DirectX图形基础结构:
    DirectX图形基础结构（DirectX Graphics Infrastructure，DXGI，也有译作DirectX图形基础设施）
    是一种与Direct3D配合使用的API。设计DXGI的基本理念是使多种图形API中所共有的底层任务能借助一组通用API来进行处理。
*/
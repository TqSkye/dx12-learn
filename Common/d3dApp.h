//***************************************************************************************
// d3dApp.h by Frank Luna (C) 2015 All Rights Reserved.
//***************************************************************************************
/*
    应用程序框架示例:
    全书的演示程序都使用了d3dUtil.h、d3dUtil.cpp、d3dApp.h和d3dApp.cpp中的框架代码，可以从本书的官方网站下载到这些文件。
    d3dUtil.h和d3dUtil.cpp文件中含有程序所需的实用工具代码，d3dApp.h和d3dApp.cpp文件内包含用于封装Direct3D示例程序的Direct3D应用程序类核心代码。
    由于在书中不能细述这些文件里的每一行代码（例如，我们不会展示如何创建一个窗口，基本的Win32编程是阅读本书的必备知识[44]），
    所以我们鼓励读者在阅读完本章后，仔细研究这些文件。构建此框架的目标是：隐去窗口创建和Direct3D初始化的具体细节；通过对这些代码进行封装，
    那些细枝末节就不会分散我们的注意力，继而使我们把精力集中在重点代码上。
 
    D3DApp类是一种基础的Direct3D应用程序类，它提供了创建应用程序主窗口、运行程序消息循环、处理窗口消息以及初始化Direct3D等多种功能的函数。
    此外，该类还为应用程序例程定义了一组框架函数。我们可以根据需求通过实例化一个继承自D3DApp的类，重写（override）框架的虚函数，
    以此从D3DApp类中派生出自定义的用户代码。D3DApp类的定义如下：
*/
#pragma once

#if defined(DEBUG) || defined(_DEBUG)
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif

#include "d3dUtil.h"
#include "GameTimer.h"

// Link necessary d3d12 libraries.
#pragma comment(lib,"d3dcompiler.lib")
#pragma comment(lib, "D3D12.lib")
#pragma comment(lib, "dxgi.lib")

class D3DApp
{
protected:

    D3DApp(HINSTANCE hInstance);
    D3DApp(const D3DApp& rhs) = delete;
    D3DApp& operator=(const D3DApp& rhs) = delete;
    virtual ~D3DApp();

public:

    static D3DApp* GetApp();
    
	HINSTANCE AppInst()const;
	HWND      MainWnd()const;
	float     AspectRatio()const;

    bool Get4xMsaaState()const;
    void Set4xMsaaState(bool value);

	int Run();
 
    /// <summary>
    /// 框架方法:需要重写
    /// Initialize：通过此方法为程序编写初始化代码，例如分配资源、初始化对象和建立3D场景等。
    /// D3DApp类实现的初始化方法会调用InitMainWindow和InitDirect3D，因此，我们在自己实现的初始化派生方法中，
    /// 应当首先像下面那样来调用D3DApp类中的初始化方法：
    /// bool TestApp::Initialize()
    /// {
    ///     if (!D3DApp::Initialize())
    ///         return false;
    /// 
    ///     /* 其他的初始化代码请置于此 */
    /// }
    /// </summary>
    /// <returns></returns>
    virtual bool Initialize();
    /// <summary>
    /// 框架方法:需要重写
    /// MsgProc：该方法用于实现应用程序主窗口的窗口过程函数（procedure function）。
    /// 一般来说，如果需要处理在D3DApp::MsgProc中没有得到处理（或者不能如我们所愿进行处理）的消息，只要重写此方法即可。
    /// 该方法的实现在4.5.5节中有相应的讲解。此外，如果对该方法进行了重写，那么其中并未处理的消息都应当转交至D3DApp::MsgProc。
    /// </summary>
    /// <param name="hwnd"></param>
    /// <param name="msg"></param>
    /// <param name="wParam"></param>
    /// <param name="lParam"></param>
    /// <returns></returns>
    virtual LRESULT MsgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

protected:
    /// <summary>
    /// 框架方法:需要重写
    /// CreateRtvAndDsvDescriptorHeaps：此虚函数用于创建应用程序所需的RTV和DSV描述符堆。
    /// 默认的实现是创建一个含有SwapChainBufferCount个RTV描述符的RTV堆（为交换链中的缓冲区而创建），
    /// 以及具有一个DSV描述符的DSV堆（为深度/模板缓冲区而创建）。该方法的默认实现足以满足大多数的示例，但是，
    /// 为了使用多渲染目标（multiple render targets）这种高级技术，届时仍将重写此方法。
    /// </summary>
    virtual void CreateRtvAndDsvDescriptorHeaps();

	/// <summary>
    /// 框架方法:需要重写
	/// OnResize：当D3DApp::MsgProc函数接收到WM_SIZE消息时便会调用此方法。若窗口的大小发生了改变，
    /// 一些与工作区大小有关的Direct3D属性也需要随之调整。特别是后台缓冲区以及深度/模板缓冲区，为了匹配窗口工作区调整后的大小需要对其重新创建。
    /// 我们可以通过调用IDXGISwapChain::ResizeBuffers方法来调整后台缓冲区的尺寸。对于深度/模板缓冲区而言，
    /// 则需要在销毁后根据新的工作区大小进行重建。另外，渲染目标和深度/模板的视图也应重新创建。
    /// D3DApp类中OnResize方法实现的功能即为调整后台缓冲区和深度/模板缓冲区的尺寸，我们可直接查阅其源代码来研究相关细节。
    /// 除了这些缓冲区以外，依赖于工作区大小的其他属性（如投影矩阵，projection matrix）也要在此做相应的修改。由于在调整窗口大小时，
    /// 客户端代码可能还需执行一些它自己的逻辑代码，因此该方法亦属于框架的一部分。
	/// </summary>
	virtual void OnResize(); 

    /// <summary>
    /// 框架方法:需要重写
    /// Update：在绘制每一帧时都会调用该抽象方法，我们通过它来随着时间的推移而更新3D应用程序（
    /// 如呈现动画、移动摄像机、做碰撞检测以及检查用户的输入等）。
    /// </summary>
    /// <param name="gt"></param>
	virtual void Update(const GameTimer& gt)=0;
    /// <summary>
    /// 框架方法:需要重写
    /// Draw：在绘制每一帧时都会调用的抽象方法。我们在该方法中发出渲染命令，将当前帧真正地绘制到后台缓冲区中。
    /// 当完成帧的绘制后，再调用IDXGISwapChain::Present方法将后台缓冲区的内容显示在屏幕上。
    /// </summary>
    /// <param name="gt"></param>
    virtual void Draw(const GameTimer& gt)=0;

	// Convenience overrides for handling mouse input.
    // 便于重写鼠标输入消息的处理流程
	virtual void OnMouseDown(WPARAM btnState, int x, int y){ }
	virtual void OnMouseUp(WPARAM btnState, int x, int y)  { }
	virtual void OnMouseMove(WPARAM btnState, int x, int y){ }

protected:

	bool InitMainWindow();
	bool InitDirect3D();
	void CreateCommandObjects();
    void CreateSwapChain();

	void FlushCommandQueue();

	ID3D12Resource* CurrentBackBuffer()const;
	D3D12_CPU_DESCRIPTOR_HANDLE CurrentBackBufferView()const;
	D3D12_CPU_DESCRIPTOR_HANDLE DepthStencilView()const;

	void CalculateFrameStats();

    void LogAdapters();
    void LogAdapterOutputs(IDXGIAdapter* adapter);
    void LogOutputDisplayModes(IDXGIOutput* output, DXGI_FORMAT format);

protected:

    static D3DApp* mApp;

    HINSTANCE mhAppInst = nullptr; // application instance handle           // 应用程序实例句柄
    HWND      mhMainWnd = nullptr; // main window handle                    // 主窗口句柄
	bool      mAppPaused = false;  // is the application paused?            // 应用程序是否暂停
	bool      mMinimized = false;  // is the application minimized?         // 应用程序是否最小化
	bool      mMaximized = false;  // is the application maximized?         // 应用程序是否最大化
	bool      mResizing = false;   // are the resize bars being dragged?    // 大小调整栏是否受到拖拽
    bool      mFullscreenState = false;// fullscreen enabled                // 是否开启全屏模式

	// Set true to use 4X MSAA (?.1.8).  The default is false.
    // 若将该选项设置为true，则使用4X MSAA技术(参见4.1.8节)。默认值为false
    bool      m4xMsaaState = false;    // 4X MSAA enabled                   // 是否开启4X MSAA
    UINT      m4xMsaaQuality = 0;      // quality level of 4X MSAA          // 4X MSAA的质量级别

	// Used to keep track of the 揹elta-time?and game time (?.4).
    // 用于记录"delta-time"（帧之间的时间间隔）和游戏总时间(参见4.4节)
	GameTimer mTimer;
	
    Microsoft::WRL::ComPtr<IDXGIFactory4> mdxgiFactory;
    /*
    前台缓冲区和后台缓冲区构成了交换链（swap chain），在Direct3D中用IDXGISwapChain接口来表示。
    这个接口不仅存储了前台缓冲区和后台缓冲区两种纹理，而且还提供了修改缓冲区大小（IDXGISwapChain::ResizeBuffers）
    和呈现缓冲区内容（IDXGISwapChain::Present）的方法。使用两个缓冲区（前台和后台）
    的情况称为双缓冲（double buffering，亦有译作双重缓冲、双倍缓冲等）。
    当然，也可以运用更多的缓冲区。例如，使用3个缓冲区就叫作三重缓冲（triple buffering，亦有译作三倍缓冲等[7]）。
    对于一般的应用来说，使用两个缓冲区就足够了。
    */
    // 交换链接口
    Microsoft::WRL::ComPtr<IDXGISwapChain> mSwapChain;
    Microsoft::WRL::ComPtr<ID3D12Device> md3dDevice;

    Microsoft::WRL::ComPtr<ID3D12Fence> mFence;
    UINT64 mCurrentFence = 0;
	
    Microsoft::WRL::ComPtr<ID3D12CommandQueue> mCommandQueue;
    Microsoft::WRL::ComPtr<ID3D12CommandAllocator> mDirectCmdListAlloc;
    Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> mCommandList;

	static const int SwapChainBufferCount = 2;
	int mCurrBackBuffer = 0;
    Microsoft::WRL::ComPtr<ID3D12Resource> mSwapChainBuffer[SwapChainBufferCount];
    Microsoft::WRL::ComPtr<ID3D12Resource> mDepthStencilBuffer;

    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> mRtvHeap;
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> mDsvHeap;

    D3D12_VIEWPORT mScreenViewport; 
    D3D12_RECT mScissorRect;

	UINT mRtvDescriptorSize = 0;
	UINT mDsvDescriptorSize = 0;
	UINT mCbvSrvUavDescriptorSize = 0;

	// Derived class should set these in derived constructor to customize starting values.
    // 用户应该在派生类的派生构造函数中自定义这些初始值
	std::wstring mMainWndCaption = L"d3d App";
	D3D_DRIVER_TYPE md3dDriverType = D3D_DRIVER_TYPE_HARDWARE;
    DXGI_FORMAT mBackBufferFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
    DXGI_FORMAT mDepthStencilFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
	int mClientWidth = 800;
	int mClientHeight = 600;
};


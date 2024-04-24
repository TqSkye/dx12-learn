//***************************************************************************************
// d3dApp.h by Frank Luna (C) 2015 All Rights Reserved.
//***************************************************************************************
/*
    Ӧ�ó�����ʾ��:
    ȫ�����ʾ����ʹ����d3dUtil.h��d3dUtil.cpp��d3dApp.h��d3dApp.cpp�еĿ�ܴ��룬���Դӱ���Ĺٷ���վ���ص���Щ�ļ���
    d3dUtil.h��d3dUtil.cpp�ļ��к��г��������ʵ�ù��ߴ��룬d3dApp.h��d3dApp.cpp�ļ��ڰ������ڷ�װDirect3Dʾ�������Direct3DӦ�ó�������Ĵ��롣
    ���������в���ϸ����Щ�ļ����ÿһ�д��루���磬���ǲ���չʾ��δ���һ�����ڣ�������Win32������Ķ�����ıر�֪ʶ[44]����
    �������ǹ����������Ķ��걾�º���ϸ�о���Щ�ļ��������˿�ܵ�Ŀ���ǣ���ȥ���ڴ�����Direct3D��ʼ���ľ���ϸ�ڣ�ͨ������Щ������з�װ��
    ��Щϸ֦ĩ�ھͲ����ɢ���ǵ�ע�������̶�ʹ���ǰѾ����������ص�����ϡ�
 
    D3DApp����һ�ֻ�����Direct3DӦ�ó����࣬���ṩ�˴���Ӧ�ó��������ڡ����г�����Ϣѭ������������Ϣ�Լ���ʼ��Direct3D�ȶ��ֹ��ܵĺ�����
    ���⣬���໹ΪӦ�ó������̶�����һ���ܺ��������ǿ��Ը�������ͨ��ʵ����һ���̳���D3DApp���࣬��д��override����ܵ��麯����
    �Դ˴�D3DApp�����������Զ�����û����롣D3DApp��Ķ������£�
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
    /// ��ܷ���:��Ҫ��д
    /// Initialize��ͨ���˷���Ϊ�����д��ʼ�����룬���������Դ����ʼ������ͽ���3D�����ȡ�
    /// D3DApp��ʵ�ֵĳ�ʼ�����������InitMainWindow��InitDirect3D����ˣ��������Լ�ʵ�ֵĳ�ʼ�����������У�
    /// Ӧ����������������������D3DApp���еĳ�ʼ��������
    /// bool TestApp::Initialize()
    /// {
    ///     if (!D3DApp::Initialize())
    ///         return false;
    /// 
    ///     /* �����ĳ�ʼ�����������ڴ� */
    /// }
    /// </summary>
    /// <returns></returns>
    virtual bool Initialize();
    /// <summary>
    /// ��ܷ���:��Ҫ��д
    /// MsgProc���÷�������ʵ��Ӧ�ó��������ڵĴ��ڹ��̺�����procedure function����
    /// һ����˵�������Ҫ������D3DApp::MsgProc��û�еõ��������߲�����������Ը���д�������Ϣ��ֻҪ��д�˷������ɡ�
    /// �÷�����ʵ����4.5.5��������Ӧ�Ľ��⡣���⣬����Ը÷�����������д����ô���в�δ�������Ϣ��Ӧ��ת����D3DApp::MsgProc��
    /// </summary>
    /// <param name="hwnd"></param>
    /// <param name="msg"></param>
    /// <param name="wParam"></param>
    /// <param name="lParam"></param>
    /// <returns></returns>
    virtual LRESULT MsgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

protected:
    /// <summary>
    /// ��ܷ���:��Ҫ��д
    /// CreateRtvAndDsvDescriptorHeaps�����麯�����ڴ���Ӧ�ó��������RTV��DSV�������ѡ�
    /// Ĭ�ϵ�ʵ���Ǵ���һ������SwapChainBufferCount��RTV��������RTV�ѣ�Ϊ�������еĻ���������������
    /// �Լ�����һ��DSV��������DSV�ѣ�Ϊ���/ģ�建���������������÷�����Ĭ��ʵ����������������ʾ�������ǣ�
    /// Ϊ��ʹ�ö���ȾĿ�꣨multiple render targets�����ָ߼���������ʱ�Խ���д�˷�����
    /// </summary>
    virtual void CreateRtvAndDsvDescriptorHeaps();

	/// <summary>
    /// ��ܷ���:��Ҫ��д
	/// OnResize����D3DApp::MsgProc�������յ�WM_SIZE��Ϣʱ�����ô˷����������ڵĴ�С�����˸ı䣬
    /// һЩ�빤������С�йص�Direct3D����Ҳ��Ҫ��֮�������ر��Ǻ�̨�������Լ����/ģ�建������Ϊ��ƥ�䴰�ڹ�����������Ĵ�С��Ҫ�������´�����
    /// ���ǿ���ͨ������IDXGISwapChain::ResizeBuffers������������̨�������ĳߴ硣�������/ģ�建�������ԣ�
    /// ����Ҫ�����ٺ�����µĹ�������С�����ؽ������⣬��ȾĿ������/ģ�����ͼҲӦ���´�����
    /// D3DApp����OnResize����ʵ�ֵĹ��ܼ�Ϊ������̨�����������/ģ�建�����ĳߴ磬���ǿ�ֱ�Ӳ�����Դ�������о����ϸ�ڡ�
    /// ������Щ���������⣬�����ڹ�������С���������ԣ���ͶӰ����projection matrix��ҲҪ�ڴ�����Ӧ���޸ġ������ڵ������ڴ�Сʱ��
    /// �ͻ��˴�����ܻ���ִ��һЩ���Լ����߼����룬��˸÷��������ڿ�ܵ�һ���֡�
	/// </summary>
	virtual void OnResize(); 

    /// <summary>
    /// ��ܷ���:��Ҫ��д
    /// Update���ڻ���ÿһ֡ʱ������øó��󷽷�������ͨ����������ʱ������ƶ�����3DӦ�ó���
    /// ����ֶ������ƶ������������ײ����Լ�����û�������ȣ���
    /// </summary>
    /// <param name="gt"></param>
	virtual void Update(const GameTimer& gt)=0;
    /// <summary>
    /// ��ܷ���:��Ҫ��д
    /// Draw���ڻ���ÿһ֡ʱ������õĳ��󷽷��������ڸ÷����з�����Ⱦ�������ǰ֡�����ػ��Ƶ���̨�������С�
    /// �����֡�Ļ��ƺ��ٵ���IDXGISwapChain::Present��������̨��������������ʾ����Ļ�ϡ�
    /// </summary>
    /// <param name="gt"></param>
    virtual void Draw(const GameTimer& gt)=0;

	// Convenience overrides for handling mouse input.
    // ������д���������Ϣ�Ĵ�������
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

    HINSTANCE mhAppInst = nullptr; // application instance handle           // Ӧ�ó���ʵ�����
    HWND      mhMainWnd = nullptr; // main window handle                    // �����ھ��
	bool      mAppPaused = false;  // is the application paused?            // Ӧ�ó����Ƿ���ͣ
	bool      mMinimized = false;  // is the application minimized?         // Ӧ�ó����Ƿ���С��
	bool      mMaximized = false;  // is the application maximized?         // Ӧ�ó����Ƿ����
	bool      mResizing = false;   // are the resize bars being dragged?    // ��С�������Ƿ��ܵ���ק
    bool      mFullscreenState = false;// fullscreen enabled                // �Ƿ���ȫ��ģʽ

	// Set true to use 4X MSAA (?.1.8).  The default is false.
    // ������ѡ������Ϊtrue����ʹ��4X MSAA����(�μ�4.1.8��)��Ĭ��ֵΪfalse
    bool      m4xMsaaState = false;    // 4X MSAA enabled                   // �Ƿ���4X MSAA
    UINT      m4xMsaaQuality = 0;      // quality level of 4X MSAA          // 4X MSAA����������

	// Used to keep track of the �delta-time?and game time (?.4).
    // ���ڼ�¼"delta-time"��֮֡���ʱ����������Ϸ��ʱ��(�μ�4.4��)
	GameTimer mTimer;
	
    Microsoft::WRL::ComPtr<IDXGIFactory4> mdxgiFactory;
    /*
    ǰ̨�������ͺ�̨�����������˽�������swap chain������Direct3D����IDXGISwapChain�ӿ�����ʾ��
    ����ӿڲ����洢��ǰ̨�������ͺ�̨�����������������һ��ṩ���޸Ļ�������С��IDXGISwapChain::ResizeBuffers��
    �ͳ��ֻ��������ݣ�IDXGISwapChain::Present���ķ�����ʹ��������������ǰ̨�ͺ�̨��
    �������Ϊ˫���壨double buffering����������˫�ػ��塢˫������ȣ���
    ��Ȼ��Ҳ�������ø���Ļ����������磬ʹ��3���������ͽ������ػ��壨triple buffering�������������������[7]����
    ����һ���Ӧ����˵��ʹ���������������㹻�ˡ�
    */
    // �������ӿ�
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
    // �û�Ӧ������������������캯�����Զ�����Щ��ʼֵ
	std::wstring mMainWndCaption = L"d3d App";
	D3D_DRIVER_TYPE md3dDriverType = D3D_DRIVER_TYPE_HARDWARE;
    DXGI_FORMAT mBackBufferFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
    DXGI_FORMAT mDepthStencilFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
	int mClientWidth = 800;
	int mClientHeight = 600;
};


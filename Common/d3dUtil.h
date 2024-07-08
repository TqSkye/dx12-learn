//***************************************************************************************
// d3dUtil.h by Frank Luna (C) 2015 All Rights Reserved.
//
// General helper code.
//***************************************************************************************

#pragma once

#include <windows.h>
#include <wrl.h>
#include <dxgi1_4.h>
#include <d3d12.h>
#include <D3Dcompiler.h>
#include <DirectXMath.h>
#include <DirectXPackedVector.h>
#include <DirectXColors.h>
#include <DirectXCollision.h>
#include <string>
#include <memory>
#include <algorithm>
#include <vector>
#include <array>
#include <unordered_map>
#include <cstdint>
#include <fstream>
#include <sstream>
#include <cassert>
#include "d3dx12.h"
#include "DDSTextureLoader.h"
#include "MathHelper.h"

extern const int gNumFrameResources;

inline void d3dSetDebugName(IDXGIObject* obj, const char* name)
{
    if(obj)
    {
        obj->SetPrivateData(WKPDID_D3DDebugObjectName, lstrlenA(name), name);
    }
}
inline void d3dSetDebugName(ID3D12Device* obj, const char* name)
{
    if(obj)
    {
        obj->SetPrivateData(WKPDID_D3DDebugObjectName, lstrlenA(name), name);
    }
}
inline void d3dSetDebugName(ID3D12DeviceChild* obj, const char* name)
{
    if(obj)
    {
        obj->SetPrivateData(WKPDID_D3DDebugObjectName, lstrlenA(name), name);
    }
}

inline std::wstring AnsiToWString(const std::string& str)
{
    WCHAR buffer[512];
    MultiByteToWideChar(CP_ACP, 0, str.c_str(), -1, buffer, 512);
    return std::wstring(buffer);
}

/*
#if defined(_DEBUG)
    #ifndef Assert
    #define Assert(x, description)                                  \
    {                                                               \
        static bool ignoreAssert = false;                           \
        if(!ignoreAssert && !(x))                                   \
        {                                                           \
            Debug::AssertResult result = Debug::ShowAssertDialog(   \
            (L#x), description, AnsiToWString(__FILE__), __LINE__); \
        if(result == Debug::AssertIgnore)                           \
        {                                                           \
            ignoreAssert = true;                                    \
        }                                                           \
                    else if(result == Debug::AssertBreak)           \
        {                                                           \
            __debugbreak();                                         \
        }                                                           \
        }                                                           \
    }
    #endif
#else
    #ifndef Assert
    #define Assert(x, description) 
    #endif
#endif 		
    */

class d3dUtil
{
public:

    static bool IsKeyDown(int vkeyCode);

    static std::string ToString(HRESULT hr);

    static UINT CalcConstantBufferByteSize(UINT byteSize)
    {
        // Constant buffers must be a multiple of the minimum hardware
        // allocation size (usually 256 bytes).  So round up to nearest
        // multiple of 256.  We do this by adding 255 and then masking off
        // the lower 2 bytes which store all bits < 256.
        // Example: Suppose byteSize = 300.
        // (300 + 255) & ~255
        // 555 & ~255
        // 0x022B & ~0x00ff
        // 0x022B & 0xff00
        // 0x0200
        // 512

        // �����������Ĵ�С������Ӳ����С����ռ䣨ͨ����256B����������
        // Ϊ�ˣ�Ҫ�������Ϊ�����������С��256������������������ͨ��Ϊ����ֵbyteSize����255��
        // ��������ͽ���ĵ�2�ֽڣ�����������С��256�����ݲ��֣���ʵ����һ��
        // ���磺����byteSize = 300
        // (300 + 255) & ~255
        // 555 & ~255
        // 0x022B & ~0x00ff
        // 0x022B & 0xff00
        // 0x0200
        // 512
        return (byteSize + 255) & ~255;
    }

    static Microsoft::WRL::ComPtr<ID3DBlob> LoadBinary(const std::wstring& filename);

    static Microsoft::WRL::ComPtr<ID3D12Resource> CreateDefaultBuffer(
        ID3D12Device* device,
        ID3D12GraphicsCommandList* cmdList,
        const void* initData,
        UINT64 byteSize,
        Microsoft::WRL::ComPtr<ID3D12Resource>& uploadBuffer);

	static Microsoft::WRL::ComPtr<ID3DBlob> CompileShader(
		const std::wstring& filename,
		const D3D_SHADER_MACRO* defines,
		const std::string& entrypoint,
		const std::string& target);
};

/*
    �������Direct3D�����᷵��HRESULT�����롣���ǵ�ʾ������ʵ����ü򵥵Ĵ������������ⷵ�ص�HRESULTֵ��
    ������ʧ�ܣ����׳��쳣����ʾ���ó���Ĵ����롢���������ļ����Լ�����������кš���Щ����������d3dUtil.h�еĴ���ʵ�֣�
*/
class DxException
{
public:
    DxException() = default;
    DxException(HRESULT hr, const std::wstring& functionName, const std::wstring& filename, int lineNumber);

    std::wstring ToString()const;

    HRESULT ErrorCode = S_OK;
    std::wstring FunctionName;
    std::wstring Filename;
    int LineNumber = -1;
};

/*
    ����ͼ�θ����ṹ��:
    ����ͨ������һ��ͬʱ���ж��㻺�����������������Ľṹ��������ض����������塣���⣬��˽ṹ�弴�ɽ������������������ϵͳ�ڴ�֮��
    ���Թ�CPU��ȡ�����磬ִ��ʰȡ��picking������ײ��⣨collision detection�������Ĺ�������ҪCPU�����ʼ��������ݡ�
    ���ߣ��ýṹ�廹�����˶��㻺������������������һЩ��Ҫ���ԣ������ʽ��ÿ����������ռ�õ��ֽ�������
    ���ṩ�˷��ػ�������ͼ�ķ���������Ҫ������������ʱ�����Ǿ�ʹ�������MeshGeometry��������d3dUtil.hͷ�ļ��У��ṹ�塣
*/

// Defines a subrange of geometry in a MeshGeometry.  This is for when multiple
// geometries are stored in one vertex and index buffer.  It provides the offsets
// and data needed to draw a subset of geometry stores in the vertex and index 
// buffers so that we can implement the technique described by Figure 6.3.
// ����SubmeshGeometry������MeshGeometry�д洢�ĵ���������
// �˽ṹ�������ڽ�������������ݴ���һ�����㻺������һ�����������������
// ���ṩ�˶Դ��ڶ��㻺�����������������еĵ�����������л�����������ݺ�ƫ���������ǿ��Ծݴ���
// ʵ��ͼ6.3�������ļ���
struct SubmeshGeometry
{
	UINT IndexCount = 0;
	UINT StartIndexLocation = 0;
	INT BaseVertexLocation = 0;

    // Bounding box of the geometry defined by this submesh. 
    // This is used in later chapters of the book.
    // ͨ���������������嵱ǰSubmeshGeometry�ṹ�������漸����İ�Χ�У�bounding box�������ǽ��ڱ���ĺ����½���ʹ�ô�����
	DirectX::BoundingBox Bounds;
};

struct MeshGeometry
{
	// Give it a name so we can look it up by name.
    // ָ���˼��������񼯺ϵ����ƣ��������Ǿ��ܸ��ݴ����ҵ���
	std::string Name;

	// System memory copies.  Use Blobs because the vertex/index format can be generic.
	// It is up to the client to cast appropriately.  
    // ϵͳ�ڴ��еĸ��������ڶ���/���������Ƿ��͸�ʽ�������ʽ���û���������������Blob��������ʾ
    // ���û���ʹ��ʱ�ٽ���ת��Ϊ�ʵ�������
	Microsoft::WRL::ComPtr<ID3DBlob> VertexBufferCPU = nullptr;
	Microsoft::WRL::ComPtr<ID3DBlob> IndexBufferCPU  = nullptr;

	Microsoft::WRL::ComPtr<ID3D12Resource> VertexBufferGPU = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> IndexBufferGPU = nullptr;

	Microsoft::WRL::ComPtr<ID3D12Resource> VertexBufferUploader = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> IndexBufferUploader = nullptr;

    // Data about the buffers.
    // �뻺������ص�����
	UINT VertexByteStride = 0;
	UINT VertexBufferByteSize = 0;
	DXGI_FORMAT IndexFormat = DXGI_FORMAT_R16_UINT;
	UINT IndexBufferByteSize = 0;

	// A MeshGeometry may store multiple geometries in one vertex/index buffer.
	// Use this container to define the Submesh geometries so we can draw
	// the Submeshes individually.
    // һ��MeshGeometry�ṹ���ܹ��洢һ�鶥��/�����������еĶ��������
    // �������������������������񼸺��壬���Ǿ��ܵ����ػ��Ƴ����е������񣨵��������壩
	std::unordered_map<std::string, SubmeshGeometry> DrawArgs;

	D3D12_VERTEX_BUFFER_VIEW VertexBufferView()const
	{
		D3D12_VERTEX_BUFFER_VIEW vbv;
		vbv.BufferLocation = VertexBufferGPU->GetGPUVirtualAddress();
		vbv.StrideInBytes = VertexByteStride;
		vbv.SizeInBytes = VertexBufferByteSize;

		return vbv;
	}

	D3D12_INDEX_BUFFER_VIEW IndexBufferView()const
	{
		D3D12_INDEX_BUFFER_VIEW ibv;
		ibv.BufferLocation = IndexBufferGPU->GetGPUVirtualAddress();
		ibv.Format = IndexFormat;
		ibv.SizeInBytes = IndexBufferByteSize;

		return ibv;
	}

	// We can free this memory after we finish upload to the GPU.
    // �������ϴ���GPU�����Ǿ����ͷ���Щ�ڴ���
	void DisposeUploaders()
	{
		VertexBufferUploader = nullptr;
		IndexBufferUploader = nullptr;
	}
};

struct Light
{
    DirectX::XMFLOAT3 Strength = { 0.5f, 0.5f, 0.5f };  //                              ��Դ����ɫ
    float FalloffStart = 1.0f;                          // point/spot light only        �������Դ/�۹�ƹ�Դʹ��
    DirectX::XMFLOAT3 Direction = { 0.0f, -1.0f, 0.0f };// directional/spot light only  ���������Դ/�۹�ƹ�Դʹ��
    float FalloffEnd = 10.0f;                           // point/spot light only        �������Դ/�۹�ƹ�Դʹ��
    DirectX::XMFLOAT3 Position = { 0.0f, 0.0f, 0.0f };  // point/spot light only        �������Դ/�۹�ƹ�Դʹ��
    float SpotPower = 64.0f;                            // spot light only              �����۹�ƹ�Դʹ��
};

#define MaxLights 16

struct MaterialConstants
{
	DirectX::XMFLOAT4 DiffuseAlbedo = { 1.0f, 1.0f, 1.0f, 1.0f };
	DirectX::XMFLOAT3 FresnelR0 = { 0.01f, 0.01f, 0.01f };
	float Roughness = 0.25f;

	// Used in texture mapping.
	DirectX::XMFLOAT4X4 MatTransform = MathHelper::Identity4x4();
};

// Simple struct to represent a material for our demos.  A production 3D engine
// would likely create a class hierarchy of Materials.
// �����ǵ���ʾ�����б�ʾ���ʵļ򵥽ṹ�� 
struct Material
{
	// Unique material name for lookup.
    // ���ڲ��Ҳ��ʵ�Ψһ��Ӧ����
	std::string Name;

	// Index into constant buffer corresponding to this material.
    // �����ʵĳ�������������
	int MatCBIndex = -1;

	// Index into SRV heap for diffuse texture.
    // ������������SRV���е��������ڵ�9��������ͼʱ���õ�
	int DiffuseSrvHeapIndex = -1;

	// Index into SRV heap for normal texture.
	int NormalSrvHeapIndex = -1;

	// Dirty flag indicating the material has changed and we need to update the constant buffer.
	// Because we have a material constant buffer for each FrameResource, we have to apply the
	// update to each FrameResource.  Thus, when we modify a material we should set 
	// NumFramesDirty = gNumFrameResources so that each frame resource gets the update.
    // �Ѹ��±�־��dirty flag��Ҳ�����־����ʾ���������б䶯��������Ҳ����Ҫ���³����������ˡ�
    // ����ÿ��֡��ԴFrameResource����һ�����ʳ��������������Ա����ÿ��FrameResource����
    // �и��¡���ˣ����޸�ĳ������ʱ��Ӧ������NumFramesDirty = gNumFrameResources����ʹÿ
    // ��֡��Դ���ܵõ�����
	int NumFramesDirty = gNumFrameResources;

	// Material constant buffer data used for shading.
    // ������ɫ�Ĳ��ʳ�������������
	DirectX::XMFLOAT4 DiffuseAlbedo = { 1.0f, 1.0f, 1.0f, 1.0f };
	DirectX::XMFLOAT3 FresnelR0 = { 0.01f, 0.01f, 0.01f };
	float Roughness = .25f;
	DirectX::XMFLOAT4X4 MatTransform = MathHelper::Identity4x4();
};

struct Texture
{
	// Unique material name for lookup.
	std::string Name;

	std::wstring Filename;

	Microsoft::WRL::ComPtr<ID3D12Resource> Resource = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> UploadHeap = nullptr;
};

/*
    ���ѿ���ThrowIfFailed�ض���һ���꣬������һ��������������ˣ�__FILE__��__LINE__����λ��ThrowIfFailed���ڵ��ļ����У�
    ���ǳ��������ļ����С�L#x�Ὣ��ThrowIfFailed�Ĳ���ת��ΪUnicode�ַ�����
    ����һ�������Ǿ��ܽ����������������Ĵ�����Ϣ�������Ϣ���С�

    ����Direct3D�������ص�HRESULTֵ������������ʹ�ú������м���[47]��
    ThrowIfFailed(md3dDevice->CreateCommittedResource(
      &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
      D3D12_HEAP_FLAG_NONE,
      &depthStencilDesc,
      D3D12_RESOURCE_STATE_COMMON,
      &optClear,
      IID_PPV_ARGS(mDepthStencilBuffer.GetAddressOf())));

      ���������߼���λ��һ��try/catch��֮�У�
        try
        {
        InitDirect3DApp theApp(hInstance);
        if(!theApp.Initialize())
            return 0;

        return theApp.Run();
        }
        catch(DxException& e)
        {
        MessageBox(nullptr, e.ToString().c_str(), L"HR Failed", MB_OK);
        return 0;
        }
      ������ص�HRESULT�Ǹ�����ֵ�����׳��쳣��ͨ��MessageBox������������Ϣ�����˳�����
      ���磬����CreateCommittedResource����������һ����Ч����ʱ�����Ǳ�ῴ��ͼ4.13��ʾ����Ϣ��
*/
#ifndef ThrowIfFailed
#define ThrowIfFailed(x)                                              \
{                                                                     \
    HRESULT hr__ = (x);                                               \
    std::wstring wfn = AnsiToWString(__FILE__);                       \
    if(FAILED(hr__)) { throw DxException(hr__, L#x, wfn, __LINE__); } \
}
#endif

#ifndef ReleaseCom
#define ReleaseCom(x) { if(x){ x->Release(); x = 0; } }
#endif
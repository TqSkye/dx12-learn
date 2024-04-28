
#include "d3dUtil.h"
#include <comdef.h>
#include <fstream>

using Microsoft::WRL::ComPtr;

DxException::DxException(HRESULT hr, const std::wstring& functionName, const std::wstring& filename, int lineNumber) :
    ErrorCode(hr),
    FunctionName(functionName),
    Filename(filename),
    LineNumber(lineNumber)
{
}

bool d3dUtil::IsKeyDown(int vkeyCode)
{
    return (GetAsyncKeyState(vkeyCode) & 0x8000) != 0;
}

ComPtr<ID3DBlob> d3dUtil::LoadBinary(const std::wstring& filename)
{
    std::ifstream fin(filename, std::ios::binary);

    fin.seekg(0, std::ios_base::end);
    std::ifstream::pos_type size = (int)fin.tellg();
    fin.seekg(0, std::ios_base::beg);

    ComPtr<ID3DBlob> blob;
    ThrowIfFailed(D3DCreateBlob(size, blob.GetAddressOf()));

    fin.read((char*)blob->GetBufferPointer(), size);
    fin.close();

    return blob;
}

/*
    ���ھ�̬�����壨static geometry����ÿһ֡�����ᷢ���ı�ļ����壩���ԣ����ǻὫ�䶥�㻺��������Ĭ�϶ѣ�D3D12_HEAP_TYPE_DEFAULT�������Ż����ܡ�
    һ��˵������Ϸ�еĴ���������壨����ľ����������κͶ�����ɫ��������˴�������������£����㻺������ʼ����Ϻ�ֻ��GPU��Ҫ�����ж�ȡ���������Ƽ����壬
    ����ʹ��Ĭ�϶��Ǻ����ǵ�������Ȼ�������CPU������Ĭ�϶��еĶ��㻺����д�����ݣ���ô���Ǹ���γ�ʼ���˶��㻺�����أ�
    
    ��ˣ����˴������㻺������Դ����֮�⣬���ǻ�����D3D12_HEAP_TYPE_UPLOAD���ֶ�����������һ�������н�λ�õ��ϴ���������upload buffer����Դ��
    ��4.3.8������Ǿ���ͨ������Դ�ύ���ϴ��ѣ��ŵ��Խ����ݴ�CPU���Ƶ�GPU�Դ��С��ڴ������ϴ�������֮�����ǾͿ��Խ��������ݴ�ϵͳ�ڴ渴�Ƶ��ϴ���������
    �����ٰѶ������ݴ��ϴ����������Ƶ������Ķ��㻺�����С�
    
    ����������Ҫ������Ϊ�н���ϴ�����������ʼ��Ĭ�ϻ����������ö�����D3D12_HEAP_TYPE_DEFAULT�����Ļ��������е����ݣ�
    ��ˣ����Ǿ���d3dUtil.h/.cpp�ļ��й��������й��ߺ������Ա�����ÿ��ʹ��Ĭ�ϻ�����ʱ������Щ�ظ��Ĺ�����
*/
Microsoft::WRL::ComPtr<ID3D12Resource> d3dUtil::CreateDefaultBuffer(
    ID3D12Device* device,
    ID3D12GraphicsCommandList* cmdList,
    const void* initData,
    UINT64 byteSize,
    Microsoft::WRL::ComPtr<ID3D12Resource>& uploadBuffer)
{
    ComPtr<ID3D12Resource> defaultBuffer;

    // Create the actual default buffer resource.
    // ����ʵ�ʵ�Ĭ�ϻ�������Դ
    ThrowIfFailed(device->CreateCommittedResource(
        &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
        D3D12_HEAP_FLAG_NONE,
        &CD3DX12_RESOURCE_DESC::Buffer(byteSize),
		D3D12_RESOURCE_STATE_COMMON,
        nullptr,
        IID_PPV_ARGS(defaultBuffer.GetAddressOf())));

    // In order to copy CPU memory data into our default buffer, we need to create
    // an intermediate upload heap. 
    // Ϊ�˽�CPU���ڴ��е����ݸ��Ƶ�Ĭ�ϻ����������ǻ���Ҫ����һ�������н�λ�õ��ϴ���
    ThrowIfFailed(device->CreateCommittedResource(
        &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
        &CD3DX12_RESOURCE_DESC::Buffer(byteSize),
		D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(uploadBuffer.GetAddressOf())));


    // Describe the data we want to copy into the default buffer.
    // ��������ϣ�����Ƶ�Ĭ�ϻ������е�����
    D3D12_SUBRESOURCE_DATA subResourceData = {};
    subResourceData.pData = initData;
    subResourceData.RowPitch = byteSize;
    subResourceData.SlicePitch = subResourceData.RowPitch;
    /*
        D3D12_SUBRESOURCE_DATA�ṹ��Ķ���Ϊ��
        typedef struct D3D12_SUBRESOURCE_DATA
        {
          const void *pData;
          LONG_PTR RowPitch;
          LONG_PTR SlicePitch;
        } D3D12_SUBRESOURCE_DATA;
        1��pData��ָ��ĳ��ϵͳ�ڴ���ָ�룬�����г�ʼ�����������õ����ݡ��������ʼ���Ļ������ܹ��洢[��ͼ]���������ݣ����ϵͳ�ڴ��ض�����������[��ͼ]���������ݣ��Դ�����ʼ��������������
        2��RowPitch�����ڻ��������ԣ��˲���Ϊ���������ݵ��ֽ�����
        3��SlicePitch�����ڻ��������ԣ��˲�����Ϊ���������ݵ��ֽ���������Ĵ�����ʾ�˴��ཫ��δ�������������8�������Ĭ�ϻ���������Ϊ���е�ÿ�����㶼�ֱ����˲�ͬ����ɫ��
            Vertex vertices[] =
            {
              { XMFLOAT3(-1.0f, -1.0f, -1.0f), XMFLOAT4(Colors::White) },
              { XMFLOAT3(-1.0f, +1.0f, -1.0f), XMFLOAT4(Colors::Black) },
              { XMFLOAT3(+1.0f, +1.0f, -1.0f), XMFLOAT4(Colors::Red) },
              { XMFLOAT3(+1.0f, -1.0f, -1.0f), XMFLOAT4(Colors::Green) },
              { XMFLOAT3(-1.0f, -1.0f, +1.0f), XMFLOAT4(Colors::Blue) },
              { XMFLOAT3(-1.0f, +1.0f, +1.0f), XMFLOAT4(Colors::Yellow) },
              { XMFLOAT3(+1.0f, +1.0f, +1.0f), XMFLOAT4(Colors::Cyan) },
              { XMFLOAT3(+1.0f, -1.0f, +1.0f), XMFLOAT4(Colors::Magenta) }
            };

            const UINT64 vbByteSize = 8 * sizeof(Vertex);

            ComPtr<ID3D12Resource> VertexBufferGPU = nullptr;
            ComPtr<ID3D12Resource> VertexBufferUploader = nullptr;
            VertexBufferGPU = d3dUtil::CreateDefaultBuffer(md3dDevice.Get(),
            mCommandList.Get(), vertices, vbByteSize, VertexBufferUploader);
    */


    // Schedule to copy the data to the default buffer resource.  At a high level, the helper function UpdateSubresources
    // will copy the CPU memory into the intermediate upload heap.  Then, using ID3D12CommandList::CopySubresourceRegion,
    // the intermediate upload heap data will be copied to mBuffer.
    // �����ݸ��Ƶ�Ĭ�ϻ�������Դ������
    // UpdateSubresources�����������Ƚ����ݴ�CPU�˵��ڴ��и��Ƶ�λ���н�λ�õ��ϴ�������ţ�
    // ��ͨ������ID3D12CommandList::CopySubresourceRegion���������ϴ����ڵ����ݸ��Ƶ�mBuffer��[4]
	cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(defaultBuffer.Get(), 
		D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST));
    UpdateSubresources<1>(cmdList, defaultBuffer.Get(), uploadBuffer.Get(), 0, 0, 1, &subResourceData);
	cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(defaultBuffer.Get(),
		D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_GENERIC_READ));

    // Note: uploadBuffer has to be kept alive after the above function calls because
    // the command list has not been executed yet that performs the actual copy.
    // The caller can Release the uploadBuffer after it knows the copy has been executed.
    // ע�⣺�ڵ������������󣬱��뱣֤uploadBuffer��Ȼ���ڣ������ܶ��������������١�������Ϊ
    // �����б��еĸ��Ʋ���������δִ�С��������ߵ�֪������ɵ���Ϣ�󣬷����ͷ�uploadBuffer

    return defaultBuffer;
}

ComPtr<ID3DBlob> d3dUtil::CompileShader(
	const std::wstring& filename,
	const D3D_SHADER_MACRO* defines,
	const std::string& entrypoint,
	const std::string& target)
{
	UINT compileFlags = 0;
#if defined(DEBUG) || defined(_DEBUG)  
	compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

	HRESULT hr = S_OK;

	ComPtr<ID3DBlob> byteCode = nullptr;
	ComPtr<ID3DBlob> errors;
	hr = D3DCompileFromFile(filename.c_str(), defines, D3D_COMPILE_STANDARD_FILE_INCLUDE,
		entrypoint.c_str(), target.c_str(), compileFlags, 0, &byteCode, &errors);

	if(errors != nullptr)
		OutputDebugStringA((char*)errors->GetBufferPointer());

	ThrowIfFailed(hr);

	return byteCode;
}

std::wstring DxException::ToString()const
{
    // Get the string description of the error code.
    _com_error err(ErrorCode);
    std::wstring msg = err.ErrorMessage();

    return FunctionName + L" failed in " + Filename + L"; line " + std::to_wstring(LineNumber) + L"; error: " + msg;
}



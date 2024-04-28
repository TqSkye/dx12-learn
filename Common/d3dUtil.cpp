
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
    对于静态几何体（static geometry，即每一帧都不会发生改变的几何体）而言，我们会将其顶点缓冲区置于默认堆（D3D12_HEAP_TYPE_DEFAULT）中来优化性能。
    一般说来，游戏中的大多数几何体（如树木、建筑物、地形和动画角色）都是如此处理。在这种情况下，顶点缓冲区初始化完毕后，只有GPU需要从其中读取数据来绘制几何体，
    所以使用默认堆是很明智的做法。然而，如果CPU不能向默认堆中的顶点缓冲区写入数据，那么我们该如何初始化此顶点缓冲区呢？
    
    因此，除了创建顶点缓冲区资源本身之外，我们还需用D3D12_HEAP_TYPE_UPLOAD这种堆类型来创建一个处于中介位置的上传缓冲区（upload buffer）资源。
    在4.3.8节里，我们就是通过把资源提交至上传堆，才得以将数据从CPU复制到GPU显存中。在创建了上传缓冲区之后，我们就可以将顶点数据从系统内存复制到上传缓冲区，
    而后再把顶点数据从上传缓冲区复制到真正的顶点缓冲区中。
    
    由于我们需要利用作为中介的上传缓冲区来初始化默认缓冲区（即用堆类型D3D12_HEAP_TYPE_DEFAULT创建的缓冲区）中的数据，
    因此，我们就在d3dUtil.h/.cpp文件中构建了下列工具函数，以避免在每次使用默认缓冲区时再做这些重复的工作。
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
    // 创建实际的默认缓冲区资源
    ThrowIfFailed(device->CreateCommittedResource(
        &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
        D3D12_HEAP_FLAG_NONE,
        &CD3DX12_RESOURCE_DESC::Buffer(byteSize),
		D3D12_RESOURCE_STATE_COMMON,
        nullptr,
        IID_PPV_ARGS(defaultBuffer.GetAddressOf())));

    // In order to copy CPU memory data into our default buffer, we need to create
    // an intermediate upload heap. 
    // 为了将CPU端内存中的数据复制到默认缓冲区，我们还需要创建一个处于中介位置的上传堆
    ThrowIfFailed(device->CreateCommittedResource(
        &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
        &CD3DX12_RESOURCE_DESC::Buffer(byteSize),
		D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(uploadBuffer.GetAddressOf())));


    // Describe the data we want to copy into the default buffer.
    // 描述我们希望复制到默认缓冲区中的数据
    D3D12_SUBRESOURCE_DATA subResourceData = {};
    subResourceData.pData = initData;
    subResourceData.RowPitch = byteSize;
    subResourceData.SlicePitch = subResourceData.RowPitch;
    /*
        D3D12_SUBRESOURCE_DATA结构体的定义为：
        typedef struct D3D12_SUBRESOURCE_DATA
        {
          const void *pData;
          LONG_PTR RowPitch;
          LONG_PTR SlicePitch;
        } D3D12_SUBRESOURCE_DATA;
        1．pData：指向某个系统内存块的指针，其中有初始化缓冲区所用的数据。如果欲初始化的缓冲区能够存储[插图]个顶点数据，则该系统内存块必定可容纳至少[插图]个顶点数据，以此来初始化整个缓冲区。
        2．RowPitch：对于缓冲区而言，此参数为欲复制数据的字节数。
        3．SlicePitch：对于缓冲区而言，此参数亦为欲复制数据的字节数。下面的代码演示了此类将如何创建存有立方体8个顶点的默认缓冲区，并为其中的每个顶点都分别赋予了不同的颜色。
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
    // 将数据复制到默认缓冲区资源的流程
    // UpdateSubresources辅助函数会先将数据从CPU端的内存中复制到位于中介位置的上传堆里接着，
    // 再通过调用ID3D12CommandList::CopySubresourceRegion函数，把上传堆内的数据复制到mBuffer中[4]
	cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(defaultBuffer.Get(), 
		D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST));
    UpdateSubresources<1>(cmdList, defaultBuffer.Get(), uploadBuffer.Get(), 0, 0, 1, &subResourceData);
	cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(defaultBuffer.Get(),
		D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_GENERIC_READ));

    // Note: uploadBuffer has to be kept alive after the above function calls because
    // the command list has not been executed yet that performs the actual copy.
    // The caller can Release the uploadBuffer after it knows the copy has been executed.
    // 注意：在调用上述函数后，必须保证uploadBuffer依然存在，而不能对它立即进行销毁。这是因为
    // 命令列表中的复制操作可能尚未执行。待调用者得知复制完成的消息后，方可释放uploadBuffer

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



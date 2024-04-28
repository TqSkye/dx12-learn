/*
    上传缓冲区辅助函数脚本:

    将上传缓冲区的相关操作简单地封装一下，使用起来会更加方便。我们在UploadBuffer.h文件中定义了下面这个类，令上传缓冲区的相关处理工作更加轻松。
    它替我们实现了上传缓冲区资源的构造与析构函数、处理资源的映射和取消映射操作，还提供了CopyData方法来更新缓冲区内的特定元素。
    在需要通过CPU修改上传缓冲区中数据的时候（例如，当观察矩阵有了变化），便可以使用CopyData。注意，此类可用于各种类型的上传缓冲区，而并非只针对常量缓冲区。
    当用此类管理常量缓冲区时，我们就需要通过构造函数参数isConstantBuffer来对此加以描述。
    另外，如果此类中存储的是常量缓冲区，那么其中的构造函数将自动填充内存，使每个常量缓冲区的大小都成为256B的整数倍。
*/

#pragma once

#include "d3dUtil.h"

template<typename T>
class UploadBuffer
{
public:
    UploadBuffer(ID3D12Device* device, UINT elementCount, bool isConstantBuffer) :
        mIsConstantBuffer(isConstantBuffer)
    {
        mElementByteSize = sizeof(T);

        // Constant buffer elements need to be multiples of 256 bytes.
        // This is because the hardware can only view constant data 
        // at m*256 byte offsets and of n*256 byte lengths. 
        // typedef struct D3D12_CONSTANT_BUFFER_VIEW_DESC {
        // UINT64 OffsetInBytes; // multiple of 256
        // UINT   SizeInBytes;   // multiple of 256
        // } D3D12_CONSTANT_BUFFER_VIEW_DESC;

        // 常量缓冲区的大小为256B的整数倍。这是因为硬件只能按m*256B的偏移量和n*256B的数据
        // 长度这两种规格来查看常量数据
        // typedef struct D3D12_CONSTANT_BUFFER_VIEW_DESC {
        // D3D12_GPU_VIRTUAL_ADDRESS BufferLocation; // 256的整数倍
        // UINT  SizeInBytes;     // 256的整数倍
        // } D3D12_CONSTANT_BUFFER_VIEW_DESC;

        /*
            常量缓冲区对硬件也有特别的要求，即常量缓冲区的大小必为硬件最小分配空间（256B）的整数倍。我们经常需要用到多个相同类型的常量缓冲区。
            例如，假设常量缓冲区cbPerObject内存储的是随不同物体而异的常量数据，因此，如果我们要绘制[插图]个物体，则需要[插图]个该类型的常量缓冲区。
            下列代码展示了我们是如何创建一个缓冲区资源，并利用它来存储NumElements个常量缓冲区。
            struct ObjectConstants
            {
              DirectX::XMFLOAT4X4 WorldViewProj = MathHelper::Identity4x4();
            };

            UINT mElementByteSize = d3dUtil::CalcConstantBufferByteSize(sizeof
                 (ObjectConstants));

            ComPtr<ID3D12Resource> mUploadCBuffer;
                device->CreateCommittedResource(
                  &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
                  D3D12_HEAP_FLAG_NONE,
                  &CD3DX12_RESOURCE_DESC::Buffer(mElementByteSize * NumElements),
                  D3D12_RESOURCE_STATE_GENERIC_READ,
                  nullptr,
                  IID_PPV_ARGS(&mUploadCBuffer));
            * 上面ObjectConstants定义在BoxApp.cpp中
        */
        /*
             我们可以认为mUploadCBuffer中存储了一个ObjectConstants类型的常量缓冲区数组（同时按256字节的整数倍来为之填充数据）。
             待到绘制物体的时候，只要将常量缓冲区视图（Constant Buffer View，CBV）绑定到存有物体相应常量数据的缓冲区子区域即可。
             由于mUploadCBuffer缓冲区存储的是一个常量缓冲区数组，因此，我们把它称之为常量缓冲区。
             工具函数d3dUtil::CalcConstantBufferByteSize会做适当的运算，使缓冲区的大小凑整为硬件最小分配空间（256B）的整数倍。
         */
        if (isConstantBuffer)
            mElementByteSize = d3dUtil::CalcConstantBufferByteSize(sizeof(T));

        ThrowIfFailed(device->CreateCommittedResource(
            &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
            D3D12_HEAP_FLAG_NONE,
            &CD3DX12_RESOURCE_DESC::Buffer(mElementByteSize * elementCount),
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            IID_PPV_ARGS(&mUploadBuffer)));
     
        /*
            更新常量缓冲区
            由于常量缓冲区是用D3D12_HEAP_TYPE_UPLOAD这种堆类型来创建的，所以我们就能通过CPU为常量缓冲区资源更新数据。
            为此，我们首先要获得指向欲更新资源数据的指针，可用Map方法来做到这一点：

            第一个参数是子资源（subresource）的索引[12]，指定了欲映射的子资源。对于缓冲区来说，它自身就是唯一的子资源，
            所以我们将此参数设置为0。第二个参数是一个可选项，是个指向D3D12_RANGE结构体的指针，此结构体描述了内存的映射范围，
            若将该参数指定为空指针，则对整个资源进行映射。第三个参数则借助双重指针，返回待映射资源数据的目标内存块。
            我们利用memcpy函数将数据从系统内存（system memory，也就是CPU端控制的内存）复制到常量缓冲区：
            memcpy(mMappedData, &data, dataSizeInBytes);

        */
        ThrowIfFailed(mUploadBuffer->Map(0, nullptr, reinterpret_cast<void**>(&mMappedData)));

        // We do not need to unmap until we are done with the resource.  However, we must not write to
        // the resource while it is in use by the GPU (so we must use synchronization techniques).
        // 只要还会修改当前的资源，我们就无须取消映射
        // 但是，在资源被GPU使用期间，我们千万不可向该资源进行写操作（所以必须借助于同步技术）
    }

    UploadBuffer(const UploadBuffer& rhs) = delete;
    UploadBuffer& operator=(const UploadBuffer& rhs) = delete;
    // 当常量缓冲区更新完成后，我们应在释放映射内存之前对其进行Unmap（取消映射）操作:
    // Unmap的第一个参数是子资源索引，指定了将被取消映射的子资源。若取消映射的是缓冲区，则将其置为0。
    // 第二个参数是个可选项，是一个指向D3D12_RANGE结构体的指针，用于描述取消映射的内存范围，若将它指定为空指针，则取消整个资源的映射。
    ~UploadBuffer()
    {
        if (mUploadBuffer != nullptr)
            mUploadBuffer->Unmap(0, nullptr);

        mMappedData = nullptr;
    }

    ID3D12Resource* Resource()const
    {
        return mUploadBuffer.Get();
    }

    void CopyData(int elementIndex, const T& data)
    {
        memcpy(&mMappedData[elementIndex * mElementByteSize], &data, sizeof(T));
    }

private:
    Microsoft::WRL::ComPtr<ID3D12Resource> mUploadBuffer;
    BYTE* mMappedData = nullptr;

    UINT mElementByteSize = 0;
    bool mIsConstantBuffer = false;
};
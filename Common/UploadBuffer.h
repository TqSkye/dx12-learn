/*
    �ϴ����������������ű�:

    ���ϴ�����������ز����򵥵ط�װһ�£�ʹ����������ӷ��㡣������UploadBuffer.h�ļ��ж�������������࣬���ϴ�����������ش������������ɡ�
    ��������ʵ�����ϴ���������Դ�Ĺ���������������������Դ��ӳ���ȡ��ӳ����������ṩ��CopyData���������»������ڵ��ض�Ԫ�ء�
    ����Ҫͨ��CPU�޸��ϴ������������ݵ�ʱ�����磬���۲�������˱仯���������ʹ��CopyData��ע�⣬��������ڸ������͵��ϴ���������������ֻ��Գ�����������
    ���ô��������������ʱ�����Ǿ���Ҫͨ�����캯������isConstantBuffer���Դ˼���������
    ���⣬��������д洢���ǳ�������������ô���еĹ��캯�����Զ�����ڴ棬ʹÿ�������������Ĵ�С����Ϊ256B����������
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

        // �����������Ĵ�СΪ256B����������������ΪӲ��ֻ�ܰ�m*256B��ƫ������n*256B������
        // ���������ֹ�����鿴��������
        // typedef struct D3D12_CONSTANT_BUFFER_VIEW_DESC {
        // D3D12_GPU_VIRTUAL_ADDRESS BufferLocation; // 256��������
        // UINT  SizeInBytes;     // 256��������
        // } D3D12_CONSTANT_BUFFER_VIEW_DESC;

        /*
            ������������Ӳ��Ҳ���ر��Ҫ�󣬼������������Ĵ�С��ΪӲ����С����ռ䣨256B���������������Ǿ�����Ҫ�õ������ͬ���͵ĳ�����������
            ���磬���賣��������cbPerObject�ڴ洢�����治ͬ�������ĳ������ݣ���ˣ��������Ҫ����[��ͼ]�����壬����Ҫ[��ͼ]�������͵ĳ�����������
            ���д���չʾ����������δ���һ����������Դ�������������洢NumElements��������������
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
            * ����ObjectConstants������BoxApp.cpp��
        */
        /*
             ���ǿ�����ΪmUploadCBuffer�д洢��һ��ObjectConstants���͵ĳ������������飨ͬʱ��256�ֽڵ���������Ϊ֮������ݣ���
             �������������ʱ��ֻҪ��������������ͼ��Constant Buffer View��CBV���󶨵�����������Ӧ�������ݵĻ����������򼴿ɡ�
             ����mUploadCBuffer�������洢����һ���������������飬��ˣ����ǰ�����֮Ϊ������������
             ���ߺ���d3dUtil::CalcConstantBufferByteSize�����ʵ������㣬ʹ�������Ĵ�С����ΪӲ����С����ռ䣨256B������������
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
            ���³���������
            ���ڳ�������������D3D12_HEAP_TYPE_UPLOAD���ֶ������������ģ��������Ǿ���ͨ��CPUΪ������������Դ�������ݡ�
            Ϊ�ˣ���������Ҫ���ָ����������Դ���ݵ�ָ�룬����Map������������һ�㣺

            ��һ������������Դ��subresource��������[12]��ָ������ӳ�������Դ�����ڻ�������˵�����������Ψһ������Դ��
            �������ǽ��˲�������Ϊ0���ڶ���������һ����ѡ��Ǹ�ָ��D3D12_RANGE�ṹ���ָ�룬�˽ṹ���������ڴ��ӳ�䷶Χ��
            �����ò���ָ��Ϊ��ָ�룬���������Դ����ӳ�䡣���������������˫��ָ�룬���ش�ӳ����Դ���ݵ�Ŀ���ڴ�顣
            ��������memcpy���������ݴ�ϵͳ�ڴ棨system memory��Ҳ����CPU�˿��Ƶ��ڴ棩���Ƶ�������������
            memcpy(mMappedData, &data, dataSizeInBytes);

        */
        ThrowIfFailed(mUploadBuffer->Map(0, nullptr, reinterpret_cast<void**>(&mMappedData)));

        // We do not need to unmap until we are done with the resource.  However, we must not write to
        // the resource while it is in use by the GPU (so we must use synchronization techniques).
        // ֻҪ�����޸ĵ�ǰ����Դ�����Ǿ�����ȡ��ӳ��
        // ���ǣ�����Դ��GPUʹ���ڼ䣬����ǧ�򲻿������Դ����д���������Ա��������ͬ��������
    }

    UploadBuffer(const UploadBuffer& rhs) = delete;
    UploadBuffer& operator=(const UploadBuffer& rhs) = delete;
    // ������������������ɺ�����Ӧ���ͷ�ӳ���ڴ�֮ǰ�������Unmap��ȡ��ӳ�䣩����:
    // Unmap�ĵ�һ������������Դ������ָ���˽���ȡ��ӳ�������Դ����ȡ��ӳ����ǻ�������������Ϊ0��
    // �ڶ��������Ǹ���ѡ���һ��ָ��D3D12_RANGE�ṹ���ָ�룬��������ȡ��ӳ����ڴ淶Χ��������ָ��Ϊ��ָ�룬��ȡ��������Դ��ӳ�䡣
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

/*
    结构化缓冲区资源以下示例展示了如何通过HLSL来定义结构化缓冲区（structured buffer）：
        struct Data
        {
            float3 v1;
            float2 v2;
        };

        StructuredBuffer<Data> gInputA : register(t0);
        StructuredBuffer<Data> gInputB : register(t1);
        RWStructuredBuffer<Data> gOutput : register(u0);

    结构化缓冲区是一种由相同类型元素所构成的简单缓冲区——其本质上是一种数组。正如我们所看到的，该元素类型可以是用户以HLSL定义的结构体。
    我们可以把为顶点缓冲区与索引缓冲区创建SRV的方法同样用于创建结构化缓冲区的SRV。
    除了“必须指定D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS标志”这一条之外，将结构化缓冲区用作UAV也与之前的操作基本一致。
    设置此标志的目的是用于把资源转换为D3D12_RESOURCE_STATE_UNORDERED_ACCESS状态。

将计算着色器的执行结果复制到系统内存:
    一般来说，在用计算着色器对纹理进行处理之后，我们就会将结果在屏幕上显示出来，并根据呈现的效果来验证计算着色器的准确性（accuracy）。
    但是，如果使用结构化缓冲区参与运算，或使用GPGPU进行通用计算，则运算结果可能根本就无法显示出来。所以当前的燃眉之急是如何将GPU端显存
    （您是否还记得，在通过UAV向结构化缓冲区写入数据时，缓冲区其实是位于显存之中）里的运算结果回传至系统内存中。
    首先，应以堆属性D3D12_HEAP_TYPE_READBACK来创建系统内存缓冲区，再通过ID3D12GraphicsCommandList::CopyResource方法将GPU资源复制到系统内存资源之中。
    其次，系统内存资源必须与待复制的资源有着相同的类型与大小。
    最后，还需用映射API函数对系统内存缓冲区进行映射，使CPU可以顺利地读取其中的数据。
    至此，我们就能将数据复制到系统内存块中了，可令CPU端对其开展后续的处理，或存数据于文件，或执行所需的各种操作。

    本章包含了一个名为“VecAdd”的结构化缓冲区演示程序，它的功能比较简单，就是将分别存于两个结构化缓冲区中向量的对应分量进行求和运算：
*/

struct Data
{
	float3 v1;
	float2 v2;
};

StructuredBuffer<Data> gInputA : register(t0);
StructuredBuffer<Data> gInputB : register(t1);
RWStructuredBuffer<Data> gOutput : register(u0);

// 为了方便起见，我们使每个结构化缓冲区中仅含有32个元素。因此，只需分派一个线程组即可（因为一个线程组即可同时处理32个数据元素)
[numthreads(32, 1, 1)]
void CS(int3 dtid : SV_DispatchThreadID)
{
	gOutput[dtid.x].v1 = gInputA[dtid.x].v1 + gInputB[dtid.x].v1;
	gOutput[dtid.x].v2 = gInputA[dtid.x].v2 + gInputB[dtid.x].v2;
}

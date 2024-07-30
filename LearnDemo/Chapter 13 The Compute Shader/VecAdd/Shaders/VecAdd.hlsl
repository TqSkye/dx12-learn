
/*
    �ṹ����������Դ����ʾ��չʾ�����ͨ��HLSL������ṹ����������structured buffer����
        struct Data
        {
            float3 v1;
            float2 v2;
        };

        StructuredBuffer<Data> gInputA : register(t0);
        StructuredBuffer<Data> gInputB : register(t1);
        RWStructuredBuffer<Data> gOutput : register(u0);

    �ṹ����������һ������ͬ����Ԫ�������ɵļ򵥻����������䱾������һ�����顣���������������ģ���Ԫ�����Ϳ������û���HLSL����Ľṹ�塣
    ���ǿ��԰�Ϊ���㻺��������������������SRV�ķ���ͬ�����ڴ����ṹ����������SRV��
    ���ˡ�����ָ��D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS��־����һ��֮�⣬���ṹ������������UAVҲ��֮ǰ�Ĳ�������һ�¡�
    ���ô˱�־��Ŀ�������ڰ���Դת��ΪD3D12_RESOURCE_STATE_UNORDERED_ACCESS״̬��

��������ɫ����ִ�н�����Ƶ�ϵͳ�ڴ�:
    һ����˵�����ü�����ɫ����������д���֮�����ǾͻὫ�������Ļ����ʾ�����������ݳ��ֵ�Ч������֤������ɫ����׼ȷ�ԣ�accuracy����
    ���ǣ����ʹ�ýṹ���������������㣬��ʹ��GPGPU����ͨ�ü��㣬�����������ܸ������޷���ʾ���������Ե�ǰ��ȼü֮������ν�GPU���Դ�
    �����Ƿ񻹼ǵã���ͨ��UAV��ṹ��������д������ʱ����������ʵ��λ���Դ�֮�У�����������ش���ϵͳ�ڴ��С�
    ���ȣ�Ӧ�Զ�����D3D12_HEAP_TYPE_READBACK������ϵͳ�ڴ滺��������ͨ��ID3D12GraphicsCommandList::CopyResource������GPU��Դ���Ƶ�ϵͳ�ڴ���Դ֮�С�
    ��Σ�ϵͳ�ڴ���Դ����������Ƶ���Դ������ͬ���������С��
    ��󣬻�����ӳ��API������ϵͳ�ڴ滺��������ӳ�䣬ʹCPU����˳���ض�ȡ���е����ݡ�
    ���ˣ����Ǿ��ܽ����ݸ��Ƶ�ϵͳ�ڴ�����ˣ�����CPU�˶��俪չ�����Ĵ�������������ļ�����ִ������ĸ��ֲ�����

    ���°�����һ����Ϊ��VecAdd���Ľṹ����������ʾ�������Ĺ��ܱȽϼ򵥣����ǽ��ֱ���������ṹ���������������Ķ�Ӧ��������������㣺
*/

struct Data
{
	float3 v1;
	float2 v2;
};

StructuredBuffer<Data> gInputA : register(t0);
StructuredBuffer<Data> gInputB : register(t1);
RWStructuredBuffer<Data> gOutput : register(u0);

// Ϊ�˷������������ʹÿ���ṹ���������н�����32��Ԫ�ء���ˣ�ֻ�����һ���߳��鼴�ɣ���Ϊһ���߳��鼴��ͬʱ����32������Ԫ��)
[numthreads(32, 1, 1)]
void CS(int3 dtid : SV_DispatchThreadID)
{
	gOutput[dtid.x].v1 = gInputA[dtid.x].v1 + gInputB[dtid.x].v1;
	gOutput[dtid.x].v2 = gInputA[dtid.x].v2 + gInputB[dtid.x].v2;
}

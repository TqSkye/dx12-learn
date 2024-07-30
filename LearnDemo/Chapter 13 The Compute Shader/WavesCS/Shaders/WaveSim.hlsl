//=============================================================================
// WaveSim.hlsl by Frank Luna (C) 2011 All Rights Reserved.
//
// UpdateWavesCS(): Solves 2D wave equation using the compute shader.
//
// DisturbWavesCS(): Runs one thread to disturb a grid height and its
//     neighbors to generate a wave. 
//=============================================================================

/*
    ����������������в�������Ԫ�ؿ��Խ���2D�������Է��ʡ���13.2�ڶ��������ɫ���У����ǻ��ڷ��ɵ��߳�ID������������13.4���н����߳�ID�������ۣ���
    ��ÿ���̶߳�Ҫ��ָ��һ��Ψһ�ĵ���ID�����ȱ�ʶ������
        [numthreads(16, 16, 1)]
        void CS(int3 dispatchThreadID : SV_DispatchThreadID)
        {
          // �����������к�������ֱ�Ϊx��y����������ͣ��������������Ӧ��gOutput������
          gOutput[dispatchThreadID.xy] = 
            gInputA[dispatchThreadID.xy] +
            gInputB[dispatchThreadID.xy];
        }
    ��������Ϊ����������ַ����㹻����߳��飨������һ���߳�������һ�����������أ�����ô��δ���Ὣ��������ͼ��Ķ�Ӧ���ݽ����ۼӣ��ٽ������������gOutput�С�
    (ע��:ϵͳ�Լ�����ɫ���е�����Խ����Ϊ������ȷ�Ķ��塣Խ��Ķ��������Ƿ���0������Խ�紦д������ʱȴ����ʵ��ִ���κβ�����no-ops��[Boyd08]��)
    ���ڼ�����ɫ��������GPU�ϣ���˱���Խ�����Ϊ����GPU��һ�㹤�ߣ��ر�����ͨ�������������������в�����ʱ�򡣵��ǣ���������л������������⡣
        ��һ�������ǣ����ǲ���ʹ��Sample���������������SampleLevel��������Sample��ȣ�SampleLevel��Ҫ��ȡ����������Ĳ�������ָ�������mipmap�㼶��
            0��ʾmipmap����߼���1�ǵڶ��������Դ����ơ����˲�������С�����֣����С���������ڿ���mipmap���Թ��˵�����mipmap�㼶֮����в�ֵ��
            ����Sample���������������Ļ�����������������������Զ�ѡ����ѵ�mipmap�㼶����Ϊ������ɫ������ֱ�Ӳ�����Ⱦ�������޷�֪��Sample��������ѡ���mipmap�㼶��
            �������Ǳ����ڼ�����ɫ������SampleLevel��������ʽ���ֶ���ָ��mipmap�Ĳ㼶��
        �ڶ��������ǣ������Ƕ�������в���ʱ����ʹ�÷�ΧΪ[0,1]^2�Ĺ�һ���������꣬����������������ʱ�����Ǳ���Խ�����Ĵ�С��width��height����������Ŀ����߶ȣ�����Ϊһ������������������
            ��������������(x, y)����ȡ��һ���������꣺
                u = x/width,    v = y/height
        ���д���չʾ��һ��ʹ�����������ļ�����ɫ�������ڶ���������ͬ�İ汾�����������������SampleLevel�������������Ǽ�������Ĵ�СΪ[512X512]���ҽ�ʹ����ߵ�mipmap�㼶��
            //
            // �汾1��ʹ����������
            //

            cbuffer cbUpdateSettings
            {
                float gWaveConstant0;
                float gWaveConstant1;
                float gWaveConstant2;

                float gDisturbMag;
                int2 gDisturbIndex;
            };

            RWTexture2D<float> gPrevSolInput : register(u0);
            RWTexture2D<float> gCurrSolInput : register(u1);
            RWTexture2D<float> gOutput       : register(u2);

            [numthreads(16, 16, 1)]
            void CS(int3 dispatchThreadID : SV_DispatchThreadID)
            {
                int x = dispatchThreadID.x;
                int y = dispatchThreadID.y;

                gOutput[int2(x,y)] = 
                    gWaveConstant0 * gPrevSolInput[int2(x,y)].r +
                    gWaveConstant1 * gCurrSolInput[int2(x,y)].r +
                    gWaveConstant2 * (
                        gCurrSolInput[int2(x,y+1)].r + 
                        gCurrSolInput[int2(x,y-1)].r + 
                        gCurrSolInput[int2(x+1,y)].r + 
                        gCurrSolInput[int2(x-1,y)].r);
            }

            //
            // �汾2��ʹ�ú���SampleLevel����������
            //

            cbuffer cbUpdateSettings
            {
                float gWaveConstant0;
                float gWaveConstant1;
                float gWaveConstant2;

                float gDisturbMag;
                int2 gDisturbIndex;
            };

            SamplerState samPoint : register(s0);

            RWTexture2D<float> gPrevSolInput : register(u0);
            RWTexture2D<float> gCurrSolInput : register(u1);
            RWTexture2D<float> gOutput    : register(u2);

            [numthreads(16, 16, 1)]
            void CS(int3 dispatchThreadID : SV_DispatchThreadID)
            {
             // �൱����SampleLevel()ȡ�������[]
             int x = dispatchThreadID.x;
             int y = dispatchThreadID.y;

             float2 c = float2(x,y)/512.0f;
             float2 t = float2(x,y-1)/512.0;
             float2 b = float2(x,y+1)/512.0;
             float2 l = float2(x-1,y)/512.0;
             float2 r = float2(x+1,y)/512.0;

             gNextSolOutput[int2(x,y)] = 
                gWaveConstants0*gPrevSolInput.SampleLevel(samPoint, c, 0.0f).r +
                gWaveConstants1*gCurrSolInput.SampleLevel(samPoint, c, 0.0f).r +
                gWaveConstants2*(
                    gCurrSolInput.SampleLevel(samPoint, b, 0.0f).r + 
                    gCurrSolInput.SampleLevel(samPoint, t, 0.0f).r + 
                    gCurrSolInput.SampleLevel(samPoint, r, 0.0f).r + 
                    gCurrSolInput.SampleLevel(samPoint, l, 0.0f).r);
            }
*/

// For updating the simulation.
cbuffer cbUpdateSettings
{
	float gWaveConstant0;
	float gWaveConstant1;
	float gWaveConstant2;
	
	float gDisturbMag;
	int2 gDisturbIndex;
};
 
RWTexture2D<float> gPrevSolInput : register(u0);
RWTexture2D<float> gCurrSolInput : register(u1);
RWTexture2D<float> gOutput       : register(u2);
 
[numthreads(16, 16, 1)]
void UpdateWavesCS(int3 dispatchThreadID : SV_DispatchThreadID)
{
	// We do not need to do bounds checking because:
	//	 *out-of-bounds reads return 0, which works for us--it just means the boundary of 
	//    our water simulation is clamped to 0 in local space.
	//   *out-of-bounds writes are a no-op.
	
	int x = dispatchThreadID.x;
	int y = dispatchThreadID.y;

	gOutput[int2(x,y)] = 
		gWaveConstant0 * gPrevSolInput[int2(x,y)].r +
		gWaveConstant1 * gCurrSolInput[int2(x,y)].r +
		gWaveConstant2 *(
			gCurrSolInput[int2(x,y+1)].r + 
			gCurrSolInput[int2(x,y-1)].r + 
			gCurrSolInput[int2(x+1,y)].r + 
			gCurrSolInput[int2(x-1,y)].r);
}

[numthreads(1, 1, 1)]
void DisturbWavesCS(int3 groupThreadID : SV_GroupThreadID,
                    int3 dispatchThreadID : SV_DispatchThreadID)
{
	// We do not need to do bounds checking because:
	//	 *out-of-bounds reads return 0, which works for us--it just means the boundary of 
	//    our water simulation is clamped to 0 in local space.
	//   *out-of-bounds writes are a no-op.
	
	int x = gDisturbIndex.x;
	int y = gDisturbIndex.y;

	float halfMag = 0.5f*gDisturbMag;

	// Buffer is RW so operator += is well defined.
	gOutput[int2(x,y)]   += gDisturbMag;
	gOutput[int2(x+1,y)] += halfMag;
	gOutput[int2(x-1,y)] += halfMag;
	gOutput[int2(x,y+1)] += halfMag;
	gOutput[int2(x,y-1)] += halfMag;
}

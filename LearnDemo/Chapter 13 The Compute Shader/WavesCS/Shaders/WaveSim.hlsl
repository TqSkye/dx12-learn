//=============================================================================
// WaveSim.hlsl by Frank Luna (C) 2011 All Rights Reserved.
//
// UpdateWavesCS(): Solves 2D wave equation using the compute shader.
//
// DisturbWavesCS(): Runs one thread to disturb a grid height and its
//     neighbors to generate a wave. 
//=============================================================================

/*
    利用索引对纹理进行采样纹理元素可以借助2D索引加以访问。在13.2节定义计算着色器中，我们基于分派的线程ID来索引纹理（在13.4节中将对线程ID进行讨论）。
    而每个线程都要被指定一个唯一的调度ID（调度标识符）。
        [numthreads(16, 16, 1)]
        void CS(int3 dispatchThreadID : SV_DispatchThreadID)
        {
          // 对两个纹理中横纵坐标分别为x、y处的纹素求和，并将结果存至相应的gOutput纹素中
          gOutput[dispatchThreadID.xy] = 
            gInputA[dispatchThreadID.xy] +
            gInputB[dispatchThreadID.xy];
        }
    假设我们为处理纹理而分发了足够多的线程组（即利用一个线程来处理一个单独的纹素），那么这段代码会将两个纹理图像的对应数据进行累加，再将结果存于纹理gOutput中。
    (注意:系统对计算着色器中的索引越界行为有着明确的定义。越界的读操作总是返回0，而向越界处写入数据时却不会实际执行任何操作（no-ops）[Boyd08]。)
    由于计算着色器运行在GPU上，因此便可以将它作为访问GPU的一般工具，特别是在通过纹理过滤来对纹理进行采样的时候。但是，这个过程中还存在两点问题。
        第一个问题是，我们不能使用Sample方法，而必须采用SampleLevel方法。与Sample相比，SampleLevel需要获取第三个额外的参数，以指定纹理的mipmap层级。
            0表示mipmap的最高级别，1是第二级，并以此类推。若此参数存在小数部分，则该小数将用于在开启mipmap线性过滤的两个mipmap层级之间进行插值。
            至于Sample方法，它会根据屏幕上纹理所覆的像素数量而自动选择最佳的mipmap层级。因为计算着色器不可直接参与渲染，它便无法知道Sample方法自行选择的mipmap层级，
            所以我们必须在计算着色器中以SampleLevel方法来显式（手动）指定mipmap的层级。
        第二个问题是，当我们对纹理进行采样时，会使用范围为[0,1]^2的归一化纹理坐标，而非整数索引。此时，我们便可以将纹理的大小（width，height）（即纹理的宽度与高度）设置为一个常量缓冲区变量，
            再利用整数索引(x, y)来求取归一化纹理坐标：
                u = x/width,    v = y/height
        下列代码展示了一个使用整数索引的计算着色器，而第二个功能相同的版本则采用了纹理坐标与SampleLevel函数。这里我们假设纹理的大小为[512X512]，且仅使用最高的mipmap层级：
            //
            // 版本1：使用整数索引
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
            // 版本2：使用函数SampleLevel与纹理坐标
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
             // 相当于以SampleLevel()取代运算符[]
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

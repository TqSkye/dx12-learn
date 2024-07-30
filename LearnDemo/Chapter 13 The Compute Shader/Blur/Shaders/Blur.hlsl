//=============================================================================
// Performs a separable Guassian blur with a blur radius up to 5 pixels.
// ����ģ���뾶���Ϊ5�����صĿɷ����˹ģ��
//=============================================================================

cbuffer cbSettings : register(b0)
{
	// We cannot have an array entry in a constant buffer that gets mapped onto
	// root constants, so list each element.  
	// ���ǲ��ܰѸ�����ӳ�䵽λ�ڳ����������е�����Ԫ�أ����Ҫ��ÿһ��Ԫ�ض�һһ�г�
	int gBlurRadius;

	// Support up to 11 blur weights.
     // ���֧��11��ģ��Ȩֵ
	float w0;
	float w1;
	float w2;
	float w3;
	float w4;
	float w5;
	float w6;
	float w7;
	float w8;
	float w9;
	float w10;
};

static const int gMaxBlurRadius = 5;


Texture2D gInput            : register(t0);
RWTexture2D<float4> gOutput : register(u0);

#define N 256
#define CacheSize (N + 2*gMaxBlurRadius)
groupshared float4 gCache[CacheSize];

[numthreads(N, 1, 1)]
void HorzBlurCS(int3 groupThreadID : SV_GroupThreadID,
				int3 dispatchThreadID : SV_DispatchThreadID)
{
	// Put in an array for each indexing.
    // ���������б�������
	float weights[11] = { w0, w1, w2, w3, w4, w5, w6, w7, w8, w9, w10 };

	//
	// Fill local thread storage to reduce bandwidth.  To blur 
	// N pixels, we will need to load N + 2*BlurRadius pixels
	// due to the blur radius.
	//
	//
    // ͨ����д�����̴߳洢�������ٴ���ĸ��ء���Ҫ��N�����ؽ���ģ����������ģ���뾶��������Ҫ
    // ����N + 2*BlurRadius������
    // 
    
	// This thread group runs N threads.  To get the extra 2*BlurRadius pixels, 
	// have 2*BlurRadius threads sample an extra pixel.
    // ���߳���������N���̡߳�Ϊ�˻�ȡ�����2*BlurRadius�����أ�����Ҫ��2*BlurRadius���̶߳���ɼ�һ����������
	if(groupThreadID.x < gBlurRadius)
	{
		// Clamp out of bound samples that occur at image borders.
        // ����ͼ�����߽����Խ��������������ǯλ����
		int x = max(dispatchThreadID.x - gBlurRadius, 0);
		gCache[groupThreadID.x] = gInput[int2(x, dispatchThreadID.y)];
	}
	if(groupThreadID.x >= N-gBlurRadius)
	{
		// Clamp out of bound samples that occur at image borders.
        // ����ͼ���Ҳ�߽紦��Խ������������ǯλ����
		int x = min(dispatchThreadID.x + gBlurRadius, gInput.Length.x-1);
		gCache[groupThreadID.x+2*gBlurRadius] = gInput[int2(x, dispatchThreadID.y)];
	}

	// Clamp out of bound samples that occur at image borders.
    // ���ͼ��߽紦��Խ������������ǯλ����
	gCache[groupThreadID.x+gBlurRadius] = gInput[min(dispatchThreadID.xy, gInput.Length.xy-1)];

	// Wait for all threads to finish.
    // �ȴ����е��߳��������
	GroupMemoryBarrierWithGroupSync();
	
	//
	// Now blur each pixel. ���ڶ�ÿ�����ؽ���ģ������
	//

	float4 blurColor = float4(0, 0, 0, 0);
	
	for(int i = -gBlurRadius; i <= gBlurRadius; ++i)
	{
		int k = groupThreadID.x + gBlurRadius + i;
		
		blurColor += weights[i+gBlurRadius]*gCache[k];
	}
	
	gOutput[dispatchThreadID.xy] = blurColor;
}

[numthreads(1, N, 1)]
void VertBlurCS(int3 groupThreadID : SV_GroupThreadID,
				int3 dispatchThreadID : SV_DispatchThreadID)
{
	// Put in an array for each indexing.
    // ���������б�������
	float weights[11] = { w0, w1, w2, w3, w4, w5, w6, w7, w8, w9, w10 };

	//
	// Fill local thread storage to reduce bandwidth.  To blur 
	// N pixels, we will need to load N + 2*BlurRadius pixels
	// due to the blur radius.
	//
    //
    // ��д�����̴߳洢�������ٴ���ĸ��ɡ����Ҫ��N�����ؽ���ģ�������ټ���ģ���뾶�����Ǿ���
    // Ҫ���ع�N + 2*BlurRadius������
    //
	
	// This thread group runs N threads.  To get the extra 2*BlurRadius pixels, 
	// have 2*BlurRadius threads sample an extra pixel.
    // ���߳���������N���̡߳�Ҫȡ������2*BlurRadius�����صĻ�������Ҫ��2*BlurRadius���߳�Ҫ�����ɼ�һ������
	if(groupThreadID.y < gBlurRadius)
	{
		// Clamp out of bound samples that occur at image borders.
        // ����ͼ���ϲ�߽紦��Խ������������ǯλ����
		int y = max(dispatchThreadID.y - gBlurRadius, 0);
		gCache[groupThreadID.y] = gInput[int2(dispatchThreadID.x, y)];
	}
	if(groupThreadID.y >= N-gBlurRadius)
	{
		// Clamp out of bound samples that occur at image borders.
        // ���ͼ���²�߽紦��Խ������������ǯλ����
		int y = min(dispatchThreadID.y + gBlurRadius, gInput.Length.y-1);
		gCache[groupThreadID.y+2*gBlurRadius] = gInput[int2(dispatchThreadID.x, y)];
	}
	
	// Clamp out of bound samples that occur at image borders.
    // ����ͼ��߽紦��Խ������������ǯλ����
	gCache[groupThreadID.y+gBlurRadius] = gInput[min(dispatchThreadID.xy, gInput.Length.xy-1)];


	// Wait for all threads to finish.
    // �ȴ����е��̶߳���ɸ��Ե�����
	GroupMemoryBarrierWithGroupSync();
	
	//
	// Now blur each pixel.���ڶ�ÿһ�����ض�����ģ������
	//

	float4 blurColor = float4(0, 0, 0, 0);
	
	for(int i = -gBlurRadius; i <= gBlurRadius; ++i)
	{
		int k = groupThreadID.y + gBlurRadius + i;
		
		blurColor += weights[i+gBlurRadius]*gCache[k];
	}
	
	gOutput[dispatchThreadID.xy] = blurColor;
}
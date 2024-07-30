//=============================================================================
// Performs edge detection using Sobel operator.ͨ������������������ͼ��ı�Ե���
//=============================================================================

Texture2D gInput            : register(t0);
RWTexture2D<float4> gOutput : register(u0);


// Approximates luminance ("brightness") from an RGB value.  These weights are derived from
// experiment based on eye sensitivity to different wavelengths of light.
// ����RGB ���ݼ������Ӧ����(luminance����������(brightness)��) �Ľ���ֵ����ЩȨ��������
// �۶Բ�ͬ��Ĳ������жȵ�ʵ�����֮����������
float CalcLuminance(float3 color)
{
    return dot(color, float3(0.299f, 0.587f, 0.114f));
}

[numthreads(16, 16, 1)]
void SobelCS(int3 dispatchThreadID : SV_DispatchThreadID)
{
    // Sample the pixels in the neighborhood of this pixel.
    // �ɼ��뵱ǰ�������������ڵ�������
	float4 c[3][3];
	for(int i = 0; i < 3; ++i)
	{
		for(int j = 0; j < 3; ++j)
		{
			int2 xy = dispatchThreadID.xy + int2(-1 + j, -1 + i);
			c[i][j] = gInput[xy]; 
		}
	}

	// For each color channel, estimate partial x derivative using Sobel scheme.
    // ���ÿ����ɫͨ����������������ʽ���������x��ƫ��������ֵ
	float4 Gx = -1.0f*c[0][0] - 2.0f*c[1][0] - 1.0f*c[2][0] + 1.0f*c[0][2] + 2.0f*c[1][2] + 1.0f*c[2][2];

	// For each color channel, estimate partial y derivative using Sobel scheme.
    // ����ÿ����ɫͨ����������������ʽ���������y��ƫ��������ֵ
	float4 Gy = -1.0f*c[2][0] - 2.0f*c[2][1] - 1.0f*c[2][1] + 1.0f*c[0][0] + 2.0f*c[0][1] + 1.0f*c[0][2];

	// Gradient is (Gx, Gy).  For each color channel, compute magnitude to get maximum rate of change.
    // �ݶȼ�Ϊ(Gx, Gy)�����ÿ����ɫͨ����������ݶȵĴ�С���ݶȵ�ģ�����ҵ����ı仯��
	float4 mag = sqrt(Gx*Gx + Gy*Gy);

	// Make edges black, and nonedges white.
    // ���ݶȶ��͵ı�Ե������Ϊ��ɫ���ݶ�ƽ̹�ķǱ�Ե������Ϊ��ɫ
	mag = 1.0f - saturate(CalcLuminance(mag.rgb));

	gOutput[dispatchThreadID.xy] = mag;
}

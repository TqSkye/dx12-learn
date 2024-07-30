//=============================================================================
// Performs edge detection using Sobel operator.通过索贝尔算子来进行图像的边缘检测
//=============================================================================

Texture2D gInput            : register(t0);
RWTexture2D<float4> gOutput : register(u0);


// Approximates luminance ("brightness") from an RGB value.  These weights are derived from
// experiment based on eye sensitivity to different wavelengths of light.
// 根据RGB 数据计算出对应亮度(luminance，即“亮度(brightness)”) 的近似值。这些权重是在人
// 眼对不同光的波长敏感度的实验基础之上所得来的
float CalcLuminance(float3 color)
{
    return dot(color, float3(0.299f, 0.587f, 0.114f));
}

[numthreads(16, 16, 1)]
void SobelCS(int3 dispatchThreadID : SV_DispatchThreadID)
{
    // Sample the pixels in the neighborhood of this pixel.
    // 采集与当前欲处理像素相邻的众像素
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
    // 针对每个颜色通道，运用索贝尔公式估算出关于x的偏导数近似值
	float4 Gx = -1.0f*c[0][0] - 2.0f*c[1][0] - 1.0f*c[2][0] + 1.0f*c[0][2] + 2.0f*c[1][2] + 1.0f*c[2][2];

	// For each color channel, estimate partial y derivative using Sobel scheme.
    // 对于每个颜色通道，利用索贝尔公式估算出关于y的偏导数近似值
	float4 Gy = -1.0f*c[2][0] - 2.0f*c[2][1] - 1.0f*c[2][1] + 1.0f*c[0][0] + 2.0f*c[0][1] + 1.0f*c[0][2];

	// Gradient is (Gx, Gy).  For each color channel, compute magnitude to get maximum rate of change.
    // 梯度即为(Gx, Gy)。针对每个颜色通道，计算出梯度的大小（梯度的模）以找到最大的变化率
	float4 mag = sqrt(Gx*Gx + Gy*Gy);

	// Make edges black, and nonedges white.
    // 将梯度陡峭的边缘处绘制为黑色，梯度平坦的非边缘处绘制为白色
	mag = 1.0f - saturate(CalcLuminance(mag.rgb));

	gOutput[dispatchThreadID.xy] = mag;
}

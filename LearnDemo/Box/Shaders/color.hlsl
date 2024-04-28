//***************************************************************************************
// color.hlsl by Frank Luna (C) 2015 All Rights Reserved.
//
// Transforms and colors geometry.
//***************************************************************************************

cbuffer cbPerObject : register(b0)
{
	float4x4 gWorldViewProj; 
};

/*
    以下代码实现的是一个简单的顶点着色器（vertex shader，回顾5.6节）：
    cbuffer cbPerObject : register(b0)
    {
      float4x4 gWorldViewProj; 
    };

    void VS(float3 iPosL : POSITION, 
        float4 iColor : COLOR, 
        out float4 oPosH : SV_POSITION,
        out float4 oColor : COLOR)
    {
      // 把顶点变换到齐次裁剪空间
      oPosH = mul(float4(iPosL, 1.0f), gWorldViewProj);

      // 直接将顶点的颜色信息传至像素着色器
      oColor = iColor;
    }

    顶点着色器就是上例中名为VS的函数。值得注意的是，我们可以给顶点着色器起任意合法的函数名。上述顶点着色器共有4个参数，
    前两个为输入参数，后两个为输出参数（通过关键字out来表示）。HLSL没有引用（reference）和指针（pointer）的概念，
    所以需要借助结构体或多个输出参数才能够从函数中返回多个数值。而且，在HLSL中，所有的函数都是内联（inline）函数。

    前两个输入参数分别对应于绘制立方体所自定义顶点结构体中的两个数据成员，也构成了顶点着色器的输入签名（input signature）。
    参数语义“:POSITION”和“:COLOR”用于将顶点结构体中的元素映射到顶点着色器的相应输入参数，如图6.4所示。
    
    输出参数也附有各自的语义（“:SV_POSITION”和“:COLOR”），并以此作为纽带，将顶点着色器的输出参数映射到下个处理阶段（几何着色器或像素着色器）中所对应的输入参数。
    注意，SV_POSITION语义比较特殊（SV代表系统值，即system value），它所修饰的顶点着色器输出元素存有齐次裁剪空间中的顶点位置信息。
    因此，我们必须为输出位置信息的参数附上SV_POSITION语义，使GPU可以在进行例如裁剪、深度测试和光栅化等处理之时，借此实现其他属性所无法介入的有关运算。
    值得注意的是，对于任何不具有系统值的输出参数而言，我们都可以根据需求以合法的语义名修饰它[8]。
*/
struct VertexIn
{
	float3 PosL  : POSITION;
    float4 Color : COLOR;
};

struct VertexOut
{
	float4 PosH  : SV_POSITION;
    float4 Color : COLOR;
};

/*
    注意:
    1.如果没有使用几何着色器（我们会在第12章中介绍这种着色器），那么顶点着色器必须用SV_POSITION语义来输出顶点在齐次裁剪空间中的位置，
        因为（在没有使用几何着色器的情况下）执行完顶点着色器之后，硬件期望获取顶点位于齐次裁剪空间之中的坐标。
        如果使用了几何着色器，则可以把输出顶点在齐次裁剪空间中位置的工作交给它来处理。
    2.在顶点着色器（或几何着色器）中是无法进行透视除法的，此阶段只能实现投影矩阵这一环节的运算。而透视除法将在后面交由硬件执行。
*/
VertexOut VS(VertexIn vin)
{
	VertexOut vout;
	
	// Transform to homogeneous clip space.
    // 把顶点变换到齐次裁剪空间(homogeneous clip space = PosH中'H'), PosL中'L'=局部空间
    /*
        顶点的位置是一个点而非向量，所以将向量的第4个分量设置为1(w=1)。
        矩阵变量gWorldViewProj存于常量缓冲区（constant buffer）内，我们会在6.6节中对它进行相关讨论。
        内置函数（built-in function,也译作内建函数、内部函数等）mul则用于计算向量与矩阵之间的乘法。
        顺便提一下，mul函数可以根据不同规模的矩阵乘法而重载。
        例如，我们可以用mul函数进行两个4X4矩阵的乘法、两个3X3矩阵的乘法或者一个1X3向量与3X3矩阵的乘法等。
        着色器函数的最后一行代码把输入的颜色直接复制给输出参数，继而将该颜色传递到渲染流水线的下个阶段：
        oColor = iColor;
    */
	vout.PosH = mul(float4(vin.PosL, 1.0f), gWorldViewProj);
	
	// Just pass vertex color into the pixel shader.
    // 直接将顶点的颜色信息传至像素着色器
    vout.Color = vin.Color;
    
    return vout;
}

/*
    像素着色器的输入与顶点着色器的输出可以准确匹配，这也是必须满足的一点。像素着色器返回一个4D颜色值，
    而位于此函数参数列表后的SV_TARGET语义则表示该返回值的类型应当与渲染目标格式（render target format）相匹配
    （该输出值会被存于渲染目标之中）。
*/
float4 PS(VertexOut pin) : SV_Target
{
    return pin.Color;
}



//***************************************************************************************
// Default.hlsl by Frank Luna (C) 2015 All Rights Reserved.
//***************************************************************************************
/*
    实例数据:
        在本书的前一版中，实例数据都是自输入装配阶段获取的。在创建输入布局（input layout）时，可以通过枚举项D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA替代D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA
        来指定输入的数据为逐实例（per-instance）数据流，而非逐顶点（per-vertex）据流。随后，再将第二个顶点缓冲区与含有实例数据的输入流相绑定。
        [1]Direct3D 12仍然支持这种向流水线传递实例数据的方式，但是我们要另择一种更为现代化的方法。

        这所谓的“摩登”方法就是为所有实例都创建一个存有其实例数据的结构化缓冲区。例如，若要将某个对象实例化100次，就应当创建一个具有100个实例数据元素的结构化缓冲区。
        接着把此结构化缓冲区资源绑定到渲染流水线上，并根据要绘制的实例在顶点着色器中索引相应的数据。那么，怎样才能在顶点着色器中确定要绘制的实例呢？
        为此，Direct3D提供了系统值标识符SV_InstanceID，可供用户在顶点着色器中方便地实现上述目的。例如，将构成第一个实例所用的各顶点统一编号为0，把组成第二个实例所需的诸顶点统一编号为1，并依此类推。
        据此，我们便能在顶点着色器中对结构化缓冲区进行索引来获取所需的实例数据。下列着色器代码展示了这一系列的工作流程：
            (即整个当前脚本文件)

        可以发现，代码中已不见了物体常量缓冲区（cbPerObject）的身影。此时，每个物体的数据将由实例缓冲区提供。
        从代码中还能看出，我们是如何通过动态索引来为每个实例关联不同的材质与纹理的。最重要的是，我们可以在单次绘制调用中获取大量的（逐）实例数据！
        为了完整性起见，现将上述着色器程序所对应的根签名代码列举如下。
            CD3DX12_DESCRIPTOR_RANGE texTable;
            texTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 7, 0, 0);

            // 根参数可以是描述符表、根描述符或根常量
            CD3DX12_ROOT_PARAMETER slotRootParameter[4];

            // 性能小提示：按变更频率由高到低进行排列，效果更佳
            slotRootParameter[0].InitAsShaderResourceView(0, 1);
            slotRootParameter[1].InitAsShaderResourceView(1, 1);
            slotRootParameter[2].InitAsConstantBufferView(0);
            slotRootParameter[3].InitAsDescriptorTable(1, &texTable, D3D12_SHADER_
                VISIBILITY_PIXEL);

            auto staticSamplers = GetStaticSamplers();

            // 根签名由一系列根参数所组成
            CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(4, slotRootParameter,
              (UINT)staticSamplers.size(), staticSamplers.data(),
              D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
        与上一章中所做的工作一样，我们在渲染每一帧画面时都要绑定一次场景中的全部材质与纹理。因而在每次绘制调用时，我们只需再设置存有相应实例数据的结构化缓冲区即可。
            void InstancingAndCullingApp::Draw(const GameTimer& gt)
            {
              ...
              // 绑定此场景所需的全部材质。对于结构化缓冲区而言，我们可以绕过描述符堆的使用而将其直接设置
              // 为根描述符

              auto matBuffer = mCurrFrameResource->MaterialBuffer->Resource();
              mCommandList->SetGraphicsRootShaderResourceView(1, matBuffer-
                >GetGPUVirtualAddress());

              auto passCB = mCurrFrameResource->PassCB->Resource();
              mCommandList->SetGraphicsRootConstantBufferView(2, passCB-
                >GetGPUVirtualAddress());

              // 绑定渲染此场景所用的一切纹理
              mCommandList->SetGraphicsRootDescriptorTable(3, 
                mSrvDescriptorHeap->GetGPUDescriptorHandleForHeapStart());

              DrawRenderItems(mCommandList.Get(), mOpaqueRitems);
              ...
            }

            void InstancingAndCullingApp::DrawRenderItems(
              ID3D12GraphicsCommandList* cmdList, 
              const std::vector<RenderItem*>& ritems)
            {
              // 针对每个渲染项……
              for(size_t i = 0; i < ritems.size(); ++i)
              {
                auto ri = ritems[i];

                cmdList->IASetVertexBuffers(0, 1, &ri->Geo->VertexBufferView());
                cmdList->IASetIndexBuffer(&ri->Geo->IndexBufferView());
                cmdList->IASetPrimitiveTopology(ri->PrimitiveType);

                // 设置此渲染项要用到的实例缓冲区
                // 对于结构化缓冲区来讲，我们可以绕过描述符堆而将其直接设置为根描述符
                auto instanceBuffer = mCurrFrameResource->InstanceBuffer-
                >Resource();
                mCommandList->SetGraphicsRootShaderResourceView(
                  0, instanceBuffer->GetGPUVirtualAddress());

                cmdList->DrawIndexedInstanced(ri->IndexCount, 
                  ri->InstanceCount, ri->StartIndexLocation, 
                  ri->BaseVertexLocation, 0);
              }
            }
*/

// Defaults for number of lights.
// 光源数量的默认值
#ifndef NUM_DIR_LIGHTS
    #define NUM_DIR_LIGHTS 3
#endif

#ifndef NUM_POINT_LIGHTS
    #define NUM_POINT_LIGHTS 0
#endif

#ifndef NUM_SPOT_LIGHTS
    #define NUM_SPOT_LIGHTS 0
#endif

// Include structures and functions for lighting.
// 包含光照所需的结构体与函数
#include "LightingUtil.hlsl"

struct InstanceData
{
	float4x4 World;
	float4x4 TexTransform;
	uint     MaterialIndex;
	uint     InstPad0;
	uint     InstPad1;
	uint     InstPad2;
};

struct MaterialData
{
	float4   DiffuseAlbedo;
    float3   FresnelR0;
    float    Roughness;
	float4x4 MatTransform;
	uint     DiffuseMapIndex;
	uint     MatPad0;
	uint     MatPad1;
	uint     MatPad2;
};

// An array of textures, which is only supported in shader model 5.1+.  Unlike Texture2DArray, the textures
// in this array can be different sizes and formats, making it more flexible than texture arrays.
// 只有着色器模型5.1+才支持的纹理数组。与Texture2Darray类型数组不同的是，此数组可由不同大小
// 及不同格式的纹理构成，因此它比纹理数组更为灵活
Texture2D gDiffuseMap[7] : register(t0);

// Put in space1, so the texture array does not overlap with these resources.  
// The texture array will occupy registers t0, t1, ..., t6 in space0. 
// 将下列这两个结构化缓冲区置于space1中，从而避免纹理数组与之重叠而上述纹理数组则将占用t0，
// t1，…，t6寄存器的space0
StructuredBuffer<InstanceData> gInstanceData : register(t0, space1);
StructuredBuffer<MaterialData> gMaterialData : register(t1, space1);

SamplerState gsamPointWrap        : register(s0);
SamplerState gsamPointClamp       : register(s1);
SamplerState gsamLinearWrap       : register(s2);
SamplerState gsamLinearClamp      : register(s3);
SamplerState gsamAnisotropicWrap  : register(s4);
SamplerState gsamAnisotropicClamp : register(s5);

// Constant data that varies per pass.
// 每趟绘制过程中都可能会有所变化的常量数据
cbuffer cbPass : register(b0)
{
    float4x4 gView;
    float4x4 gInvView;
    float4x4 gProj;
    float4x4 gInvProj;
    float4x4 gViewProj;
    float4x4 gInvViewProj;
    float3 gEyePosW;
    float cbPerObjectPad1;
    float2 gRenderTargetSize;
    float2 gInvRenderTargetSize;
    float gNearZ;
    float gFarZ;
    float gTotalTime;
    float gDeltaTime;
    float4 gAmbientLight;

    // Indices [0, NUM_DIR_LIGHTS) are directional lights;
    // indices [NUM_DIR_LIGHTS, NUM_DIR_LIGHTS+NUM_POINT_LIGHTS) are point lights;
    // indices [NUM_DIR_LIGHTS+NUM_POINT_LIGHTS, NUM_DIR_LIGHTS+NUM_POINT_LIGHT+NUM_SPOT_LIGHTS)
    // are spot lights for a maximum of MaxLights per object.
    // 对于每个以MaxLights为光源数量最大值的对象来讲，索引[0, NUM_DIR_LIGHTS)表示的是方向光源；
    // 索引[NUM_DIR_LIGHTS, NUM_DIR_LIGHTS+NUM_POINT_LIGHTS)表示的是点光源；
    // 索引[NUM_DIR_LIGHTS+NUM_POINT_LIGHTS+NUM_SPOT_ LIGHTS]表示的是聚光灯光源
    Light gLights[MaxLights];
};

struct VertexIn
{
	float3 PosL    : POSITION;
    float3 NormalL : NORMAL;
	float2 TexC    : TEXCOORD;
};

struct VertexOut
{
	float4 PosH    : SV_POSITION;
    float3 PosW    : POSITION;
    float3 NormalW : NORMAL;
	float2 TexC    : TEXCOORD;
	
	// nointerpolation is used so the index is not interpolated 
	// across the triangle.
    // 由于此处使用的修饰符是nointerpolation，因此该索引指向的都是未经插值的三角形
	nointerpolation uint MatIndex  : MATINDEX;
};

VertexOut VS(VertexIn vin, uint instanceID : SV_InstanceID)
{
	VertexOut vout = (VertexOut)0.0f;
	
	// Fetch the instance data.
    // 获取实例数据
	InstanceData instData = gInstanceData[instanceID];
	float4x4 world = instData.World;
	float4x4 texTransform = instData.TexTransform;
	uint matIndex = instData.MaterialIndex;

	vout.MatIndex = matIndex;
	
	// Fetch the material data.
    // 获取材质数据
	MaterialData matData = gMaterialData[matIndex];
	
    // Transform to world space.
    // 将顶点变换到世界空间
    float4 posW = mul(float4(vin.PosL, 1.0f), world);
    vout.PosW = posW.xyz;

    // Assumes nonuniform scaling; otherwise, need to use inverse-transpose of world matrix.
    // 假设要执行的是等比缩放，否则就需要使用世界矩阵的逆转置矩阵进行计算
    vout.NormalW = mul(vin.NormalL, (float3x3)world);

    // Transform to homogeneous clip space.
    // 把顶点变换到齐次裁剪空间
    vout.PosH = mul(posW, gViewProj);
	
	// Output vertex attributes for interpolation across triangle.
    // 为了对三角形进行插值而输出顶点属性
	float4 texC = mul(float4(vin.TexC, 0.0f, 1.0f), texTransform);
	vout.TexC = mul(texC, matData.MatTransform).xy;
	
    return vout;
}

float4 PS(VertexOut pin) : SV_Target
{
	// Fetch the material data.
    // 获取材质数据
	MaterialData matData = gMaterialData[pin.MatIndex];
	float4 diffuseAlbedo = matData.DiffuseAlbedo;
    float3 fresnelR0 = matData.FresnelR0;
    float  roughness = matData.Roughness;
	uint diffuseTexIndex = matData.DiffuseMapIndex;
	
	// Dynamically look up the texture in the array.
    // 在数组中动态地查找纹理
    diffuseAlbedo *= gDiffuseMap[diffuseTexIndex].Sample(gsamLinearWrap, pin.TexC);
	
    // Interpolating normal can unnormalize it, so renormalize it.
    // 对法线插值可能使它非规范化，因此需要再次对它进行规范化处理
    pin.NormalW = normalize(pin.NormalW);

    // Vector from point being lit to eye. 
    // 从表面上一点指向观察点的向量
    float3 toEyeW = normalize(gEyePosW - pin.PosW);

    // Light terms.
    // 光照项
    float4 ambient = gAmbientLight*diffuseAlbedo;

    const float shininess = 1.0f - roughness;
    Material mat = { diffuseAlbedo, fresnelR0, shininess };
    float3 shadowFactor = 1.0f;
    float4 directLight = ComputeLighting(gLights, mat, pin.PosW,
        pin.NormalW, toEyeW, shadowFactor);

    float4 litColor = ambient + directLight;

    // Common convention to take alpha from diffuse albedo.
    // 从漫反射反照率获取alpha值的常用方法
    litColor.a = diffuseAlbedo.a;

    return litColor;
}



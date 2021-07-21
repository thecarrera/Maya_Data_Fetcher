cbuffer ViewBuffer : register(b0)
{
    float4x4 viewMat;
};
cbuffer ProjBuffer : register(b1)
{
    float4x4 projMat;
};
cbuffer TransformBuffer : register(b2)
{
    float4x4 worldMat;
}

struct VS_IN {
    float3 Pos          : POSITION;
    float3 Norm         : NORMAL;
    float3 Tangent      : TANGENT;
    float3 Bitangent    : BINORMAL;
    float2 UV           : TEXCOORD;
};

struct VS_OUT {
    float4 Pos          : SV_POSITION;
    float4 Norm         : NORMAL;
    float4 oTangent     : TANGENT;
    float4 oBitangent   : BINORMAL;
    float2 UV           : TEXCOORD;
    float4 wPos         : WPOSITION;
};

VS_OUT VS_main(VS_IN input)
{
    VS_OUT output = (VS_OUT)0;
    
    //output.Pos = float4(mul(mul(mul(float4(input.Pos, 1.0f), worldMat), viewMat), projMat).xyz, 1.0f);
    output.Pos = float4(input.Pos, 1.0f);
    output.Norm = float4(input.Norm, 1.0f);
    output.oTangent = float4(input.Tangent, 1.0f);
    output.oBitangent = float4(input.Bitangent, 1.0f);
    output.wPos = float4(mul(float4(input.Pos, 1.0f), worldMat).xyz, 1.0f);
    output.UV = input.UV;
    
    return output;
}
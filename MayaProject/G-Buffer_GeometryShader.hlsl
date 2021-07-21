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

struct GS_IN
{
	float4 Pos	: SV_POSITION;
    float3 Norm : NORMAL;
	float2 UV	: TEXCOORD;
};

struct GS_OUT
{
    float4 Pos : SV_POSITION;
    float4 Norm : NORMAL;
	float2 UV	: TEXCOORD;
    float4 wPos : WPOSITION;
    float3 oTangent : TANGIENT;
    float3 oBTangent : BITANGIENT;
};

float2x3 faceEdgeCalc(float3 Pos0, float3 Pos1, float3 Pos2)
{
    float3 edge1 = Pos1 - Pos0;
    float3 edge2 = Pos2 - Pos0;
    return float2x3(edge1, edge2);
}
float2x2 texUVCalc(float2 uv0, float2 uv1, float2 uv2)
{
    float2 edge1 = uv1 - uv0;
    float2 edge2 = uv2 - uv0;
    return float2x2(edge1, edge2);
}
float2x2 inverseMat(float det, float2x2 texUVs)
{
    float2x2 tempTexUVs = { texUVs._m11, -texUVs._m01, -texUVs._m10, texUVs._m00 };
    tempTexUVs = det * tempTexUVs;
    return tempTexUVs;
}
float2x3 matriceMult(float2x2 UV, float2x3 Edges)
{
    float2x3 objTB =
    {
        UV._m00 * Edges._m00 + UV._m01 * Edges._m10,
		UV._m00 * Edges._m01 + UV._m01 * Edges._m11,
		UV._m00 * Edges._m02 + UV._m01 * Edges._m12,

		UV._m10 * Edges._m00 + UV._m11 * Edges._m10,
		UV._m10 * Edges._m01 + UV._m11 * Edges._m11,
		UV._m10 * Edges._m02 + UV._m11 * Edges._m12,
    };

    return objTB;
}

[maxvertexcount(3)]
void GS_main( triangle GS_IN input[3], inout TriangleStream<GS_OUT> OutputStream)
{
    GS_OUT output = (GS_OUT)0;
	
	//Normal Map
    float2x3 faceEdges = faceEdgeCalc(input[0].Pos.xyz, input[1].Pos.xyz, input[2].Pos.xyz);
    float2x2 texUVs = texUVCalc(input[0].UV, input[1].UV, input[2].UV);
    
    float det = 1.0f / (texUVs._m00 * texUVs._m11 - texUVs._m01 * texUVs._m10);
    
    texUVs = inverseMat(det, texUVs);
	
    float2x3 objTB = matriceMult(texUVs, faceEdges);
    float3 objTan = { objTB._m00, objTB._m01, objTB._m02 };
    float3 objBTan = { objTB._m10, objTB._m11, objTB._m12 };

    for (int i = 0; i < 3; ++i)
    {
        output.Pos = mul(mul(mul(input[i].Pos, worldMat), viewMat), projMat);
        output.Norm = float4(input[i].Norm, 1.0f);
        output.UV = input[i].UV;
        output.wPos = float4(mul(input[i].Pos, worldMat).xyz, 1.0f);
        output.oTangent = objTan;
        output.oBTangent = objBTan;
    
        OutputStream.Append(output);
    }
}
struct VS_IN
{
    float3 pos : SV_POSITION;
    float3 norm : NORMAL;
    float2 uv : UV;
};

struct VS_OUT
{
    float4 pos : SV_POSITION;
    float3 norm : NORMAL;
    float2 uv : UV;
};

VS_OUT VS_main( VS_IN input)
{
    VS_OUT output = (VS_OUT)0;
    
    output.pos = float4(input.pos, 1.f);
    output.norm = input.norm;
    output.uv = input.uv;
    
	return output;
}
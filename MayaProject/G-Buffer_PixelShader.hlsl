Texture2D txDiffuse		: register(t0);
Texture2D normalMap		: register(t1);

SamplerState sampAni	: register(s0);

struct PS_IN
{
    float4 Pos       : SV_POSITION;
    float4 Norm      : NORMAL;
    float4 oTangent  : TANGENT;
    float4 oBTangent : BINORMAL;
	float2 UV	     : TEXCOORD;
    float4 wPos      : WPOSITION;
};

struct PS_OUT
{
    float4 DiffuseAlbedo    : SV_Target0;
    float4 Normal           : SV_Target1;
    float4 Position         : SV_Target2;
};

PS_OUT PS_main(in PS_IN input)
{
    PS_OUT output = (PS_OUT) 0;
    
    float2 uv = input.UV;
    uv.y = 1 - uv.y;
    float3 diffuseAlbedo = txDiffuse.Sample(sampAni, uv).rgb;
    //Normal Map
    float3 normMap = normalMap.Sample(sampAni, uv).xyz;
    normMap = 2.0f * normMap - 1.0f;
    //float3 N = normalize(input.Norm.xyz);
    //float3 T = normalize(input.oTangent.xyz - dot(input.oTangent.xyz, N) * N);
    //float3 B = cross(N, T);
    //float3x3 TBN = { T, B, N };
    float3x3 TBN = { normalize(input.oTangent.xyz), normalize(input.oBTangent.xyz), normalize(input.Norm.xyz)};
    //float3 newNormal = mul(normMap, TBN);
    float3 newNormal = input.Norm;
    
    output.DiffuseAlbedo    = float4(diffuseAlbedo, 1.0f);
    output.Normal           = float4(newNormal, 1.0f);
    output.Position         = input.wPos;
    
    //output.DiffuseAlbedo = float4(1.f, 0.f, 0.f, 1.f);
    //output.Normal = float4(0.f, 1.f, 0.f, 1.f);
    //output.Position = float4(0.f, 0.f, 1.f, 1.f);
    
    return output;
}
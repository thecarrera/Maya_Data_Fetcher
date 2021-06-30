Texture2D NormalTexture			: register(t0);
Texture2D DiffuseAlbedoTexture	: register(t1);
Texture2D PositionTexture		: register(t2);

struct PointLightData
{
    float intensity;
    float3 color;
    float4x4 worldMat;
};

cbuffer cameraViewBuffer : register(b0)
{
    float4x4 viewMat;
}
cbuffer LightBuffer : register(b1)
{
    uint pointLightCount;
    PointLightData pointLights[20];
};

float4 CalcLights(  in float3 cameraPos,
                    in float3 normal,
                    in float3 position,
                    in float3 diffuseAlbedo)
{
    float3 finalColor = float3(1.0f, 1.0f, 1.0f);
    
    for (uint i = 0; i < pointLightCount; ++i)
    {
        float3 lightDirection = normalize(pointLights[i].worldMat._14_24_34 - position.xyz);
        float  distance = length(lightDirection);
        float3 attinuation = max(0, 1.f - (distance / pointLights[i].intensity));
        
        float angleFromLight = saturate(dot(normal, (lightDirection / distance)));
        float3 diffuse = angleFromLight * pointLights[i].color * diffuseAlbedo;
        
        //Specular
        float3 reflection = reflect(-lightDirection, normal);
        float specAngl = dot(reflection, normalize(-position));
        float specular = dot(lightDirection, normalize(cameraPos));
        specular = saturate(specular);
        
        finalColor *= (diffuse + specular) * attinuation;
    }
    return float4(finalColor, 1.0f);
}
float4 PS_main(in float4 screenPos : SV_Position) : SV_Target0
{
    int3 sampleIndecies = int3(screenPos.xy, 0);
    
    float3 cameraPos = viewMat._14_24_34;
    float3 normal = NormalTexture.Load(sampleIndecies).xyz;
    float3 position = PositionTexture.Load(sampleIndecies).xyz;
    float3 diffuseAlbedo = DiffuseAlbedoTexture.Load(sampleIndecies).xyz;
   
    //return CalcLights(cameraPos, normal, position, diffuseAlbedo);
    return float4(0.f, 0.f, 1.f, 1.f);
}
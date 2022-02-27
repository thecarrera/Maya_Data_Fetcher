Texture2D DiffuseAlbedoTexture	: register(t0);
Texture2D NormalTexture			: register(t1);
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
    float3 finalColor = float3(0.0f, 0.0f, 0.0f);
    
    for (uint i = 0; i < pointLightCount; ++i)
    {
        float3 lightDirection = 0;
        lightDirection.xy = pointLights[i].worldMat._41_42 - position.xy;
        lightDirection.z = pointLights[i].worldMat._43 - position.z;
        
        float distance = length(lightDirection);
        lightDirection = normalize(lightDirection);
        float attinuation = 10.0f / (1.0f + distance * 0.1f + pow(distance, 2) * 0.5f);
        
        float angleFromLight = saturate(dot(normal, (lightDirection / distance)));
        float3 diffuse = angleFromLight * pointLights[i].color * diffuseAlbedo;
        
        //Specular
        float3 reflection = reflect(lightDirection, normal);
        saturate(reflection);
        //float specAngl = saturate(dot(reflection, normalize(position)));
        float specular = max(dot(normalize(cameraPos - position), reflection), 0.0f);
        specular = saturate(specular);
        specular = pow(specular, 60.f);
        
        finalColor += (diffuse + specular) * (attinuation * 2.f);
        saturate(finalColor);
    }
    return float4(finalColor, 1.0f);
}

float4 PS_main(in float4 screenPos : SV_Position) : SV_Target0
{
    int3 sampleIndecies = int3(screenPos.xy, 0);
    
    float3 cameraPos = viewMat._41_42_43;
    float3 diffuseAlbedo = DiffuseAlbedoTexture.Load(sampleIndecies).xyz; 
    float3 normal = NormalTexture.Load(sampleIndecies).xyz;
    float3 position = PositionTexture.Load(sampleIndecies).xyz;
   
    return CalcLights(cameraPos, normal, position, diffuseAlbedo);
    //return float4(diffuseAlbedo, 1.0f);
    //return float4(position, 1.0f);
    //return float4(normal, 1.0f);
    //return float4(0.f, 0.f, 1.f, 1.f);
}
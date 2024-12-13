#include "ShaderIncludes.hlsli"

cbuffer ExternalData : register(b0) {
	float4 colorTint;
    float3 cameraPos;
    float totalTime;
    float roughness;
    Light lights[5];
}

Texture2D Albedo : register(t0);
Texture2D NormalMap : register(t1);
Texture2D RoughnessMap : register(t2);
Texture2D MetalnessMap : register(t3);
Texture2D ShadowMap : register(t4);
SamplerState LerpSampler : register(s0);
SamplerComparisonState ShadowSampler : register(s1);



float Attenuate(Light light, float3 worldPosition)
{
    float dist = distance(light.position, worldPosition);
    float attenuation = saturate(1.0f - (dist * dist / (light.range * light.range)));
    return attenuation * attenuation;
}

float3 constructLight(VertexToPixel input, Light light, float3 surfaceColor, float3 specularColor, float roughness, float metalness)
{
    float3 diffuseTerm = DiffusePBR(input.normal, normalize(light.direction));
    float3 fresnel;
    float specular = MicrofacetBRDF(input.normal, normalize(light.direction), input.worldPosition - cameraPos, roughness, specularColor, fresnel);
    
    float3 balancedDiffuse = DiffuseEnergyConserve(diffuseTerm, fresnel, metalness);
    
    float3 finalLight = (balancedDiffuse * surfaceColor + specular) * light.intensity * light.color;
    if (light.type == LIGHT_TYPE_POINT)
        finalLight *= Attenuate(light, input.worldPosition);
    return finalLight;
}

float3 transformNormal(float3 normal, float3 tangent, float3 unpackedNormal)
{
    float3 gsTangent = normalize(tangent - normal * dot(tangent, normal));
    float3 biTangent = cross(gsTangent, normal);
    float3x3 TBN = float3x3(tangent, biTangent, normal);
    return mul(unpackedNormal, TBN);
}

// --------------------------------------------------------
// The entry point (main method) for our pixel shader
// 
// - Input is the data coming down the pipeline (defined by the struct)
// - Output is a single color (float4)
// - Has a special semantic (SV_TARGET), which means 
//    "put the output of this into the current render target"
// - Named "main" because that's the default the shader compiler looks for
// --------------------------------------------------------
float4 main(VertexToPixel input) : SV_TARGET
{
    input.shadowMapPos /= input.shadowMapPos.w;
    
    float2 shadowUV = input.shadowMapPos.xy * 0.5f + 0.5f;
    shadowUV.y = 1 - shadowUV.y; // Flip the Y
    
    // Grab the distances we need: light-to-pixel and closest-surface
    float distToLight = input.shadowMapPos.z;
    float shadowAmount = ShadowMap.SampleCmpLevelZero(ShadowSampler, shadowUV, distToLight).r;
    
    input.normal = normalize(input.normal);
    input.tangent = normalize(input.tangent);
    
    float3 unpackedNormal = normalize(NormalMap.Sample(LerpSampler, input.uv).rgb * 2 - 1);
    input.normal = transformNormal(input.normal, input.tangent, unpackedNormal);
    
    float3 finalLight;
    
    float3 surfaceColor = pow(Albedo.Sample(LerpSampler, input.uv).rgb, 2.2f);
    float roughness = RoughnessMap.Sample(LerpSampler, input.uv).r;
    float metalness = MetalnessMap.Sample(LerpSampler, input.uv).r;
    float3 specularColor = lerp(F0_NON_METAL, surfaceColor, metalness);
    
    for (int i = 0; i < 5; i++)
    {
        if (lights[i].type == LIGHT_TYPE_POINT)
        {
            Light editableLight = lights[i];
            editableLight.direction = input.worldPosition - editableLight.position;
            finalLight += constructLight(input, editableLight, surfaceColor, specularColor, roughness, metalness);
        }
        else
        {
            float3 lightResult = constructLight(input, lights[i], surfaceColor, specularColor, roughness, metalness);
            if (i == 0)
            {
                lightResult *= shadowAmount;
            }
            finalLight += lightResult;
        }
    }
    
	// Just return the input color
	// - This color (like most values passing through the rasterizer) is 
	//   interpolated for each pixel between the corresponding vertices 
	//   of the triangle we're rendering
	//return colorTint * float4(input.tangent, 1);
    return float4(pow(finalLight, 1.0f/2.2f), 1);
}
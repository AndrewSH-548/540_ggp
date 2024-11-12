#include "ShaderIncludes.hlsli"

cbuffer ExternalData : register(b0) {
	float4 colorTint;
    float3 cameraPos;
	float3 ambient;
    float totalTime;
    float roughness;
    Light lights[5];
}

Texture2D CushionTexture : register(t0);
Texture2D NormalMap : register(t1);
SamplerState LerpSampler : register(s0);

// Calculates the diffuse term.
float3 calculateDiffuseTerm(VertexToPixel input, Light light, float3 surfaceColor)
{
    return saturate(dot(input.normal, -normalize(light.direction))) * light.color * light.intensity * surfaceColor;
}

float phongSpecular(VertexToPixel input, Light light)
{
    float3 view = normalize(cameraPos - input.worldPosition);
    float3 reflection = reflect(normalize(light.direction), input.normal);
    float specularExponent = (1 - roughness) * MAX_SPECULAR_EXPONENT;
	
    float specular = pow(saturate(dot(reflection, view)), specularExponent);
    
    return specular;
}

float Attenuate(Light light, float3 worldPosition)
{
    float dist = distance(light.position, worldPosition);
    float attenuation = saturate(1.0f - (dist * dist / (light.range * light.range)));
    return attenuation * attenuation;
}

float3 constructLight(VertexToPixel input, Light light)
{
    float3 diffuseTerm = calculateDiffuseTerm(input, light, (float3) colorTint);
    float specular = phongSpecular(input, light);
    float3 finalLight = diffuseTerm + specular;
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
    input.normal = normalize(input.normal);
    input.tangent = normalize(input.tangent);
    
    float3 unpackedNormal = normalize(NormalMap.Sample(LerpSampler, input.uv).rgb * 2 - 1);
    input.normal = transformNormal(input.normal, input.tangent, unpackedNormal);
    
    float3 finalLight;
    
    float3 surfaceColor = CushionTexture.Sample(LerpSampler, input.uv).rgb;
    
    for (int i = 0; i < 5; i++)
    {
        if (lights[i].type == LIGHT_TYPE_POINT)
        {
            Light editableLight = lights[i];
            editableLight.direction = input.worldPosition - editableLight.position;
            finalLight += constructLight(input, editableLight);
        }
        else
        {
            finalLight += constructLight(input, lights[i]);
        }
    }
    
    // Apply ambientTerm here
    finalLight *= normalize((float3) colorTint * ambient);
    
	
	// Just return the input color
	// - This color (like most values passing through the rasterizer) is 
	//   interpolated for each pixel between the corresponding vertices 
	//   of the triangle we're rendering
	//return colorTint * float4(input.tangent, 1);
    return float4(surfaceColor * finalLight, 1);
}
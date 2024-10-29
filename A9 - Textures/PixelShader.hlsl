#include "ShaderIncludes.hlsli"

cbuffer ExternalData : register(b0) {
	float4 colorTint;
    float3 cameraPos;
	float3 ambient;
    float roughness;
    Light lights[5];
}

Texture2D IronBlock : register(t0);
Texture2D Cobblestone : register(t1);
SamplerState PointSampler : register(s0);

// Calculates the diffuse term.
float3 calculateDiffuseTerm(VertexToPixel input, Light light, float3 surfaceColor)
{
    return saturate(dot(input.normal, -light.direction)) * light.color * light.intensity * surfaceColor;
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
    float3 ambientTerm = normalize((float3) colorTint * ambient);
    float3 diffuseTerm = calculateDiffuseTerm(input, light, (float3) colorTint);
    float specular = phongSpecular(input, light);
    float3 finalLight = ambientTerm * diffuseTerm + specular;
    if (light.type == LIGHT_TYPE_POINT)
        finalLight *= Attenuate(light, input.worldPosition);
    return finalLight;
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
    float3 finalLight;
    
    float3 surfaceColor = IronBlock.Sample(PointSampler, input.uv).rgb;
    float3 overlayColor = Cobblestone.Sample(PointSampler, input.uv).rgb / 4;
    
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
	
	// Just return the input color
	// - This color (like most values passing through the rasterizer) is 
	//   interpolated for each pixel between the corresponding vertices 
	//   of the triangle we're rendering
	//return colorTint * float4(ambient, 1);
    return float4(surfaceColor * finalLight - overlayColor, 1);
}
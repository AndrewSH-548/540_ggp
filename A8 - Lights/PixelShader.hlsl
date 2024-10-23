#include "ShaderIncludes.hlsli"

cbuffer ExternalData : register(b0) {
	float4 colorTint;
    float3 cameraPos;
	float3 ambient;
    float roughness;
    Light yellowLightD;
}

// Calculates the diffuse term.
float3 calculateDiffuseTerm(VertexToPixel input, Light light, float3 surfaceColor)
{
    return saturate(dot(input.normal, -light.direction)) * light.color * light.intensity * surfaceColor;
}

float phongSpecular(VertexToPixel input, Light light)
{
    float3 view = normalize(cameraPos - input.worldPosition);
    float3 reflection = reflect(light.direction, input.normal);
    float specularExponent = (1 - roughness) * MAX_SPECULAR_EXPONENT;
	
    float specular = pow(saturate(dot(reflection, view)), specularExponent);
    
    return specular;
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
	
    float3 ambientTerm = normalize((float3)colorTint * ambient);
    float3 diffuseTerm = calculateDiffuseTerm(input, yellowLightD, (float3)colorTint);	
    float specular = phongSpecular(input, yellowLightD);
    
    float3 finalLight = ambientTerm * diffuseTerm + specular;
	
	// Just return the input color
	// - This color (like most values passing through the rasterizer) is 
	//   interpolated for each pixel between the corresponding vertices 
	//   of the triangle we're rendering
	//return colorTint * float4(ambient, 1);
    return float4(finalLight, 1);
}
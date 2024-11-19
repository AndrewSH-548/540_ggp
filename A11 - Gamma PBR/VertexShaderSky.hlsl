#include "ShaderIncludes.hlsli"

cbuffer ExternalData : register(b0)
{
    matrix view;
    matrix projection;
}

VertexToPixel_Sky main( VertexShaderInput input)
{
    VertexToPixel_Sky output;
    
    //Copy the view matrix and remove any translations
    matrix viewMatrix = view;
    viewMatrix._14 = 0;
    viewMatrix._24 = 0;
    viewMatrix._34 = 0;
    
    //Create the output position and sample direction
    output.position = mul(mul(projection, viewMatrix), float4(input.localPosition, 1.0f));
    output.position.z = output.position.w;                      //Ensure depth is 1
    output.sampleDirection = input.localPosition;               //Always points to the origin   
    
	return output;
}
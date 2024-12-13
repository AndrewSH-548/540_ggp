#include "ShaderIncludes.hlsli"

TextureCube SkyBoxTexture : register(t0);
SamplerState LerpSampler : register(s0);

float4 main(VertexToPixel_Sky input) : SV_TARGET
{
	
    return SkyBoxTexture.Sample(LerpSampler, input.sampleDirection);
}
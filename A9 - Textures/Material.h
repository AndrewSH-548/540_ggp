#pragma once
#include <memory>
#include <DirectXMath.h>

#include "SimpleShader.h"
class Material
{
public:
	Material(DirectX::XMFLOAT4 colorTint, std::shared_ptr<SimpleVertexShader> vertexShader, std::shared_ptr<SimplePixelShader> pixelShader, float roughness);

	void SetColorTint(DirectX::XMFLOAT4 colorTint);
	void SetVertexShader(SimpleVertexShader vertexShader);
	void SetPixelShader(SimplePixelShader pixelShader);

	DirectX::XMFLOAT4 GetColorTint();
	std::shared_ptr<SimpleVertexShader> GetVertexShader();
	std::shared_ptr<SimplePixelShader> GetPixelShader();
	float GetRoughness();
private:
	DirectX::XMFLOAT4 colorTint;
	std::shared_ptr<SimpleVertexShader> vertexShader;
	std::shared_ptr<SimplePixelShader> pixelShader;
	float roughness;
};


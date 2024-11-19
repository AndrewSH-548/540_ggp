#include "Material.h"

using namespace std;
using namespace DirectX;

Material::Material(XMFLOAT4 colorTint, shared_ptr<SimpleVertexShader> vertexShader, shared_ptr<SimplePixelShader> pixelShader, float roughness) {
	this->colorTint = colorTint;
	this->vertexShader = vertexShader;
	this->pixelShader = pixelShader;
	this->roughness = roughness;
}

void Material::SetColorTint(XMFLOAT4 colorTint) {
	this->colorTint = colorTint;
}

void Material::SetVertexShader(SimpleVertexShader vertexShader) {
	this->vertexShader.reset(&vertexShader);
}

void Material::SetPixelShader(SimplePixelShader pixelShader) {
	this->pixelShader.reset(&pixelShader);
};

XMFLOAT4 Material::GetColorTint() {
	return colorTint;
}

shared_ptr<SimpleVertexShader> Material::GetVertexShader() {
	return vertexShader;
}

shared_ptr<SimplePixelShader> Material::GetPixelShader() {
	return pixelShader;
}

float Material::GetRoughness() {
	return roughness;
}
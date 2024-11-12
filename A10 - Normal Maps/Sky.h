#pragma once
#include "Mesh.h"
#include "SimpleShader.h"
#include "Camera.h"

#include <d3d11.h>
#include <wrl/client.h>
#include <memory>

class Sky
{
public:
	Sky(Mesh mesh, Microsoft::WRL::ComPtr<ID3D11SamplerState> samplerState, const wchar_t* vertexShaderPath, const wchar_t* pixelShaderPath, std::wstring skyTexturePath);
	void Draw(Camera camera);
	~Sky();
private:
	Microsoft::WRL::ComPtr<ID3D11SamplerState> samplerState;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> skyTexture;
	Microsoft::WRL::ComPtr<ID3D11DepthStencilState> stencilState;
	Microsoft::WRL::ComPtr<ID3D11RasterizerState> rasterizer;

	std::shared_ptr<Mesh> mesh;
	std::shared_ptr<SimplePixelShader> pixelShader;
	std::shared_ptr<SimpleVertexShader> vertexShader;

	// Helper for creating a cubemap from 6 individual textures
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> CreateCubemap(
		const wchar_t* right,
		const wchar_t* left,
		const wchar_t* up,
		const wchar_t* down,
		const wchar_t* front,
		const wchar_t* back);
};
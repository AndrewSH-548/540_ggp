#pragma once
#include "Mesh.h"
#include "Buffer.h"
#include "Entity.h"

#include <d3d11.h>
#include <wrl/client.h>
#include <DirectXMath.h>
#include <vector>

using namespace std;

class Game
{
public:
	// Basic OOP setup
	Game() = default;
	~Game();
	Game(const Game&) = delete; // Remove copy constructor
	Game& operator=(const Game&) = delete; // Remove copy-assignment operator

	// Primary functions
	void Initialize();
	void Update(float deltaTime, float totalTime);
	void Draw(float deltaTime, float totalTime);
	void OnResize();


private:
	int number;
	float displayColor[4] = { 0.2f, 0.0f, 0.2f, 0.0f };
	float colorTint[4] = { 1.0f, 1.0f, 0.5f, 1.0f };
	bool isDemoVisible = true;
	vector<Entity> entities;
	float movementSpeed = 0.1;

	// Initialization helper methods - feel free to customize, combine, remove, etc.
	void LoadShaders();
	void LoadConstantBuffer();
	void CreateGeometry();
	void UpdateImGui(float deltaTime);
	void BuildUI();
	void ConstructShaderData(DirectX::XMFLOAT4X4 worldMatrix);

	// Note the usage of ComPtr below
	//  - This is a smart pointer for objects that abide by the
	//     Component Object Model, which DirectX objects do
	//  - More info here: https://github.com/Microsoft/DirectXTK/wiki/ComPtr

	// Buffers to hold actual geometry data
	Microsoft::WRL::ComPtr<ID3D11Buffer> vertexBuffer;
	Microsoft::WRL::ComPtr<ID3D11Buffer> indexBuffer;
	Microsoft::WRL::ComPtr<ID3D11Buffer> constBuffer;

	// Shaders and shader-related constructs
	Microsoft::WRL::ComPtr<ID3D11PixelShader> pixelShader;
	Microsoft::WRL::ComPtr<ID3D11VertexShader> vertexShader;
	Microsoft::WRL::ComPtr<ID3D11InputLayout> inputLayout;
};


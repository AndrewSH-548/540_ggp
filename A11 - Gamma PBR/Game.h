#pragma once
#include "Mesh.h"
#include "Entity.h"
#include "Camera.h"
#include "SimpleShader.h"
#include "Light.h"
#include "WICTextureLoader.h"
#include "Sky.h"

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
	vector<Light> lights;
	vector<const char*> lightNames;
	float movementSpeed = 0.1f;
	std::shared_ptr<Sky> skyBox;

	// Initialization helper methods - feel free to customize, combine, remove, etc.
	void LoadShaders();
	void CreateGeometry();
	void UpdateImGui(float deltaTime);
	void BuildUI();
	void ConstructShaderData(Entity currentEntity, float totalTime);

	// Note the usage of ComPtr below
	//  - This is a smart pointer for objects that abide by the
	//     Component Object Model, which DirectX objects do
	//  - More info here: https://github.com/Microsoft/DirectXTK/wiki/ComPtr

	// Buffers to hold actual geometry data
	Microsoft::WRL::ComPtr<ID3D11Buffer> vertexBuffer;
	Microsoft::WRL::ComPtr<ID3D11Buffer> indexBuffer;

	// Textures
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> albedo;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> normalMap;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> roughnessMap;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> metalnessMap;
	Microsoft::WRL::ComPtr<ID3D11SamplerState> samplerState;

	std::unordered_map<std::string, Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>> textures;
	std::unordered_map<std::string, Microsoft::WRL::ComPtr<ID3D11SamplerState>> samplers;

	// Shaders and shader-related constructs
	std::shared_ptr<SimplePixelShader> pixelShader;
	std::shared_ptr<SimplePixelShader> uvPixelShader;
	std::shared_ptr<SimplePixelShader> normalPixelShader;
	std::shared_ptr<SimplePixelShader> customPixelShader;
	std::shared_ptr<SimpleVertexShader> vertexShader;

	vector<std::shared_ptr<Camera>> cameras;
	int activeCamera;
	Light yellowLightD;
	Light cyanLightD;
	Light magentaLightD;
	Light redLightP;
	Light blueLightP;
};


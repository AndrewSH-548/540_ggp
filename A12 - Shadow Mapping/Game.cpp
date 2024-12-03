#include "Game.h"
#include "Graphics.h"
#include "Vertex.h"
#include "Input.h"
#include "PathHelpers.h"
#include "Window.h"
#include "Mesh.h"
#include "Material.h"

#include <DirectXMath.h>
#include <memory>
#include <math.h>

// This code assumes files are in "ImGui" subfolder!
// Adjust as necessary for your own folder structure and project setup
#include "ImGui/imgui.h"
#include "ImGui/imgui_impl_dx11.h"
#include "ImGui/imgui_impl_win32.h"

// Needed for a helper function to load pre-compiled shader files
#pragma comment(lib, "d3dcompiler.lib")
#include <d3dcompiler.h>

// For the DirectX Math library
using namespace DirectX;
using namespace std;

// --------------------------------------------------------
// Called once per program, after the window and graphics API
// are initialized but before the game loop begins
// --------------------------------------------------------
void Game::Initialize()
{
	// Helper methods for loading shaders, creating some basic
	// geometry to draw and some simple camera matrices.
	//  - You'll be expanding and/or replacing these later
	LoadShaders();
	CreateGeometry();
	

	// Set initial graphics API state
	//  - These settings persist until we change them
	//  - Some of these, like the primitive topology & input layout, probably won't change
	//  - Others, like setting shaders, will need to be moved elsewhere later
	{
		// Tell the input assembler (IA) stage of the pipeline what kind of
		// geometric primitives (points, lines or triangles) we want to draw.  
		// Essentially: "What kind of shape should the GPU draw with our vertices?"
		Graphics::Context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	}

	// Initialize ImGui itself & platform/renderer backends
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGui_ImplWin32_Init(Window::Handle());
	ImGui_ImplDX11_Init(Graphics::Device.Get(), Graphics::Context.Get());
	ImGui::StyleColorsClassic();
	
	CreateWICTextureFromFile(Graphics::Device.Get(), Graphics::Context.Get(), FixPath(L"../../Assets/Textures/cobblestone/albedo.png").c_str(), 0, &albedo);
	CreateWICTextureFromFile(Graphics::Device.Get(), Graphics::Context.Get(), FixPath(L"../../Assets/Textures/cobblestone/normas.png").c_str(), 0, &normalMap);
	CreateWICTextureFromFile(Graphics::Device.Get(), Graphics::Context.Get(), FixPath(L"../../Assets/Textures/cobblestone/roughness.png").c_str(), 0, &roughnessMap);
	CreateWICTextureFromFile(Graphics::Device.Get(), Graphics::Context.Get(), FixPath(L"../../Assets/Textures/cobblestone/metal.png").c_str(), 0, &metalnessMap);

	textures.insert({ "Albedo", albedo });
	textures.insert({ "NormalMap", normalMap });
	textures.insert({ "RoughnessMap", roughnessMap});
	textures.insert({ "MetalnessMap", metalnessMap });

	D3D11_SAMPLER_DESC samplerDesc = {};
	samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;								//Lerp
	samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;

	Graphics::Device->CreateSamplerState(&samplerDesc, &samplerState);
	samplers.insert({ "LerpSampler", samplerState });

	cameras.push_back(make_shared<Camera>(Camera(
		Window::AspectRatio(),
		XMFLOAT3(0, -5, -30), XMFLOAT3(0.02f, 0, 0),										//Starting position and rotation vectors
		0.4f, false)));
	cameras.push_back(make_shared<Camera>(Camera(
		Window::AspectRatio(),
		XMFLOAT3(-20.4f, 0, -20), XMFLOAT3(0.145f, XM_PIDIV4, 0),
		0.4f, false)));
	cameras.push_back(make_shared<Camera>(Camera(
		Window::AspectRatio(),
		XMFLOAT3(0, -0.8f, -1), XMFLOAT3(-XM_PIDIV4, 0, 0),							//Starting position and rotation vectors
		1.2f, true)));

	activeCamera = 0;

	//Skybox construction
	skyBox = make_shared<Sky>(Mesh(FixPath(L"../../Assets/Models/cube.obj").c_str()), samplerState, 
		FixPath(L"VertexShaderSky.cso").c_str(),
		FixPath(L"PixelShaderSky.cso").c_str(),
		FixPath(L"../../Assets/Textures/Clouds Pink/"));
	
	ConstructShadowMap();
};


// --------------------------------------------------------
// Clean up memory or objects created by this class
// 
// Note: Using smart pointers means there probably won't
//       be much to manually clean up here!
// --------------------------------------------------------
Game::~Game()
{
	// ImGui clean up
	ImGui_ImplDX11_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();
}

// --------------------------------------------------------
// Loads shaders from compiled shader object (.cso) files
// and also created the Input Layout that describes our 
// vertex data to the rendering pipeline. 
// - Input Layout creation is done here because it must 
//    be verified against vertex shader byte code
// - We'll have that byte code already loaded below
// --------------------------------------------------------
void Game::LoadShaders()
{
	vertexShader = make_shared<SimpleVertexShader>(Graphics::Device, Graphics::Context, FixPath(L"VertexShader.cso").c_str());
	pixelShader = make_shared<SimplePixelShader>(Graphics::Device, Graphics::Context, FixPath(L"PixelShader.cso").c_str());

	shadowVS = make_shared<SimpleVertexShader>(Graphics::Device, Graphics::Context, FixPath(L"VertexShaderShadow.cso").c_str());
}

// --------------------------------------------------------
// Creates the geometry we're going to draw
// --------------------------------------------------------
void Game::CreateGeometry()
{
	//Create the materials to pass into the entities
	Material lightFilter = Material(XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f), vertexShader, pixelShader, 0.1f);

	entities.push_back(Entity("Fancy Donut", Mesh(FixPath(L"../../Assets/Models/torus.obj").c_str()), lightFilter));
	entities.push_back(Entity("Fancy Cube", Mesh(FixPath(L"../../Assets/Models/cube.obj").c_str()), lightFilter));
	entities.push_back(Entity("Red-Green Sphere", Mesh(FixPath(L"../../Assets/Models/sphere.obj").c_str()), lightFilter));
	entities.push_back(Entity("Red-Green Helix", Mesh(FixPath(L"../../Assets/Models/helix.obj").c_str()), lightFilter));
	entities.push_back(Entity("Floor Cube", Mesh(FixPath(L"../../Assets/Models/cube.obj").c_str()), lightFilter));

	//Position all the objects.
	for (int i = 0; i < entities.size() - 1; i++) {
		entities[i].GetTransform()->MoveAbsolute(3.0f * i, -8.0f, 0.0f);
	}

	entities[4].GetTransform()->MoveAbsolute(0.0f, -12.0f, 0.0f);
	entities[4].GetTransform()->Scale(20.0f, 1.0f, 20.0f);

	for (int i = 0; i < 3; i++) {
		
	}
	//First Light: Yellow direction light
	Light dirLight = {};
	dirLight.type = LIGHT_TYPE_DIRECTIONAL;
	dirLight.direction = XMFLOAT3(0.0f, 1.0f, 3.0f);
	dirLight.color = XMFLOAT3(1.0f, 1.0f, 0.3f);
	dirLight.intensity = 6.3f;
	lights.push_back(dirLight);
	lightNames.push_back("Yellow");
	
	//Second Light: Blue point light
	Light light = {};
	light.type = LIGHT_TYPE_POINT;
	light.range = 8.0f;
	light.intensity = 8;
	light.position = XMFLOAT3(0.0f, -8.0f, 0.0f);
	light.color = XMFLOAT3(0.0f, 0.0f, 1.0f);
	lights.push_back(light);
	lightNames.push_back("Blue");
}

void Game::ConstructShadowMap() {
	// Create the actual texture that will be the shadow map
	D3D11_TEXTURE2D_DESC shadowDesc = {};
	shadowDesc.Width = shadowMapResolution; // Ideally a power of 2 (like 1024)
	shadowDesc.Height = shadowMapResolution; // Ideally a power of 2 (like 1024)
	shadowDesc.ArraySize = 1;
	shadowDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;
	shadowDesc.CPUAccessFlags = 0;
	shadowDesc.Format = DXGI_FORMAT_R32_TYPELESS;
	shadowDesc.MipLevels = 1;
	shadowDesc.MiscFlags = 0;
	shadowDesc.SampleDesc.Count = 1;
	shadowDesc.SampleDesc.Quality = 0;
	shadowDesc.Usage = D3D11_USAGE_DEFAULT;
	Microsoft::WRL::ComPtr<ID3D11Texture2D> shadowTexture;
	Graphics::Device->CreateTexture2D(&shadowDesc, 0, shadowTexture.GetAddressOf());

	// Create the depth/stencil view
	D3D11_DEPTH_STENCIL_VIEW_DESC shadowDSDesc = {};
	shadowDSDesc.Format = DXGI_FORMAT_D32_FLOAT;
	shadowDSDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
	shadowDSDesc.Texture2D.MipSlice = 0;
	Graphics::Device->CreateDepthStencilView(
		shadowTexture.Get(),
		&shadowDSDesc,
		shadowDSV.GetAddressOf());

	// Create the SRV for the shadow map
	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Format = DXGI_FORMAT_R32_FLOAT;
	srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = 1;
	srvDesc.Texture2D.MostDetailedMip = 0;
	Graphics::Device->CreateShaderResourceView(
		shadowTexture.Get(),
		&srvDesc,
		shadowSRV.GetAddressOf());

	XMMATRIX lightView = XMMatrixLookToLH(
		XMLoadFloat3(&lights[0].direction) * -20,
		XMLoadFloat3(&lights[0].direction),					//Shadows will be straight down
		XMVECTOR{ 0, 1, 0 });

	float lightProjectionSize = 15.0f;
	XMMATRIX lightProjection = XMMatrixOrthographicLH(
		lightProjectionSize,
		lightProjectionSize,
		1.0f,
		100.0f);

}


// --------------------------------------------------------
// Handle resizing to match the new window size
//  - Eventually, we'll want to update our 3D camera
// --------------------------------------------------------
void Game::OnResize()
{
	//This is called before initialization, so ensure the camera exists before calling anything it has.
	if (cameras.size() > 0) {
		for (shared_ptr<Camera> c : cameras) {
			c->UpdateProjectionMatrix(Window::AspectRatio());
		}
	}
}


// --------------------------------------------------------
// Update your game here - user input, move objects, AI, etc.
// --------------------------------------------------------
void Game::Update(float deltaTime, float totalTime)
{
	//Update the UI
	UpdateImGui(deltaTime);
	BuildUI();
	cameras[activeCamera]->Update(deltaTime);
	entities[0].GetTransform()->Rotate(0, deltaTime, 0);
	entities[3].GetTransform()->Rotate(0, deltaTime, 0);

	for (int i = 0; i < entities.size(); i++) {
		entities[i].GetTransform()->SetWorldMatrices();
	}

	// Example input checking: Quit if the escape key is pressed
	if (Input::KeyDown(VK_ESCAPE))
		Window::Quit();
}


// --------------------------------------------------------
// Clear the screen, redraw everything, present to the user
// --------------------------------------------------------
void Game::Draw(float deltaTime, float totalTime)
{
	// Frame START
	// - These things should happen ONCE PER FRAME
	// - At the beginning of Game::Draw() before drawing *anything*
	{
		// Clear the back buffer (erase what's on screen) and depth buffer
		Graphics::Context->ClearRenderTargetView(Graphics::BackBufferRTV.Get(), displayColor);
		Graphics::Context->ClearDepthStencilView(Graphics::DepthBufferDSV.Get(), D3D11_CLEAR_DEPTH, 1.0f, 0);
		
		// Render Shadow Map
		{
			//Clear the map first
			Graphics::Context->ClearDepthStencilView(shadowDSV.Get(), D3D11_CLEAR_DEPTH, 1.0f, 0);

			//Set merger state and clear pixel shader
			ID3D11RenderTargetView* nullRTV{};
			Graphics::Context->OMSetRenderTargets(1, &nullRTV, shadowDSV.Get());
			Graphics::Context->PSSetShader(0, 0, 0);
			
			//Change the viewport
			D3D11_VIEWPORT viewport = {};
			viewport.Width = (float)shadowMapResolution;
			viewport.Height = (float)shadowMapResolution;
			viewport.MaxDepth = 1.0f;
			Graphics::Context->RSSetViewports(1, &viewport);

			shadowVS->SetShader();
			shadowVS->SetMatrix4x4("view", shadowViewMatrix);
			shadowVS->SetMatrix4x4("projection", shadowProjectionMatrix);
			// Loop and draw all entities
			for (auto& e : entities)
			{
				shadowVS->SetMatrix4x4("world", e.GetTransform()->GetWorldMatrix());
				shadowVS->CopyAllBufferData();
				// Draw the mesh directly to avoid the entity's material
				// Note: Your code may differ significantly here!
				e.GetMesh()->Draw();
			}

			viewport.Width = (float)Window::Width();
			viewport.Height = (float)Window::Height();
			Graphics::Context->RSSetViewports(1, &viewport);
			Graphics::Context->OMSetRenderTargets(
				1,
				Graphics::BackBufferRTV.GetAddressOf(),
				Graphics::DepthBufferDSV.Get());
		}
	}

	// DRAW geometry
	// - These steps are generally repeated for EACH object you draw
	// - Other Direct3D calls will also be necessary to do more complex things
	{
		for (int i = 0; i < entities.size(); i++) {
			entities[i].GetMaterial()->GetVertexShader()->SetShader();
			entities[i].GetMaterial()->GetPixelShader()->SetShader();
			ConstructShaderData(entities[i], totalTime);
			entities[i].Draw();
		}

		skyBox->Draw(*cameras[activeCamera]);
		ImGui::Render(); // Turns this frame’s UI into renderable triangles
		ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData()); // Draws it to the screen
	}

	// Frame END
	// - These should happen exactly ONCE PER FRAME
	// - At the very end of the frame (after drawing *everything*)
	{
		// Present at the end of the frame
		bool vsync = Graphics::VsyncState();
		Graphics::SwapChain->Present(
			vsync ? 1 : 0,
			vsync ? 0 : DXGI_PRESENT_ALLOW_TEARING);

		// Re-bind back buffer and depth buffer after presenting
		Graphics::Context->OMSetRenderTargets(
			1,
			Graphics::BackBufferRTV.GetAddressOf(),
			Graphics::DepthBufferDSV.Get());
	}
}

void Game::ConstructShaderData(Entity currentEntity, float totalTime) {
	std::shared_ptr<SimpleVertexShader> vertexShader = currentEntity.GetMaterial()->GetVertexShader();
	vertexShader->SetMatrix4x4("world", currentEntity.GetTransform()->GetWorldMatrix());
	vertexShader->SetMatrix4x4("worldInverseTranspose", currentEntity.GetTransform()->GetWorldInverseTransposeMatrix());
	vertexShader->SetMatrix4x4("view", cameras[activeCamera]->GetViewMatrix());
	vertexShader->SetMatrix4x4("projection", cameras[activeCamera]->GetProjectionMatrix());

	std::shared_ptr<SimplePixelShader> pixelShader = currentEntity.GetMaterial()->GetPixelShader();
	pixelShader->SetFloat4("colorTint", currentEntity.GetMaterial()->GetColorTint());
	pixelShader->SetFloat3("cameraPos", cameras[activeCamera]->GetTransform().GetPosition());
	pixelShader->SetFloat("totalTime", totalTime);
	pixelShader->SetFloat("roughness", currentEntity.GetMaterial()->GetRoughness());
	pixelShader->SetData("lights", &lights[0], sizeof(Light) * (int)lights.size());
	for (auto& t : textures) {
		pixelShader->SetShaderResourceView(t.first.c_str(), t.second);
	}
	for (auto& s : samplers) {
		pixelShader->SetSamplerState(s.first.c_str(), s.second);
	}

	//Copy the data to the buffer at the start of the frame
	vertexShader->CopyAllBufferData();
	pixelShader->CopyAllBufferData();
}

#pragma region ImGui Code
//Handles all of ImGui, called during Game::Update
//Set aside since Update will be crowded later
void Game::UpdateImGui(float deltaTime) {
	// Feed fresh data to ImGui
	ImGuiIO& io = ImGui::GetIO();
	io.DeltaTime = deltaTime;
	io.DisplaySize.x = (float)Window::Width();
	io.DisplaySize.y = (float)Window::Height();
	// Reset the frame
	ImGui_ImplDX11_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();
	// Determine new input capture
	Input::SetKeyboardCapture(io.WantCaptureKeyboard);
	Input::SetMouseCapture(io.WantCaptureMouse);
	// Show the demo window, only if it's toggled
	if (isDemoVisible) ImGui::ShowDemoWindow();
}

//Handle all UI-related updating logic here
void Game::BuildUI() {
	//Framerate: Uses a float placeholder to display the value accurately
	ImGui::Text("Framerate: %f FPS", ImGui::GetIO().Framerate);

	//Window Resolution: Display as 2 decimal integers.
	ImGui::Text("Window Resolution: %dx%d", Window::Width(), Window::Height());

	//Window for Mesh Data
	ImGui::Begin("Mesh Data");
	
	for (int i = 0; i < entities.size(); i++) {
		ImGui::PushID(i);
		XMFLOAT3 position = entities[i].GetTransform()->GetPosition();
		XMFLOAT3 rotation = entities[i].GetTransform()->GetRotation();
		XMFLOAT3 scale = entities[i].GetTransform()->GetScale();
		ImGui::Text("	%d", i);
		ImGui::SliderFloat3("Position", &position.x, -0.5f, 0.5f);
		ImGui::SliderFloat3("Rotation (Radians)", &rotation.x, -XM_PI, XM_PI);
		ImGui::SliderFloat3("Scale", &scale.x, 0.5f, 3);
		entities[i].GetTransform()->SetPosition(position);
		entities[i].GetTransform()->SetRotation(rotation);
		entities[i].GetTransform()->SetScale(scale);
		ImGui::PopID();
	}

	ImGui::End();

	ImGui::Begin("Directional Light Control");
	for (int i = 0; i < 1; i++) {
		ImGui::PushID(i);
		ImGui::Text(lightNames[i]);
		ImGui::SliderFloat3("Direction", &lights[i].direction.x, -10.0f, 10.0f);
		ImGui::PopID();
	}
	ImGui::End();

	ImGui::Begin("Point Light Control");
	for (int i = 1; i < 2; i++) {
		ImGui::PushID(i);
		ImGui::Text(lightNames[i]);
		ImGui::SliderFloat3("Position", &lights[i].position.x, -10.0f, 10.0f);
		ImGui::PopID();
	}
	ImGui::End();

	ImGui::Image(shadowSRV.Get(), ImVec2(512, 512));

	ImGui::Begin("Camera Control");
	ImGui::Text("Camera %d", activeCamera);
	ImGui::Text("Position\nX: %f\nY: %f\nZ: %f",
		cameras[activeCamera]->GetTransform().GetPosition().x,
		cameras[activeCamera]->GetTransform().GetPosition().y,
		cameras[activeCamera]->GetTransform().GetPosition().z);
	ImGui::Text("Rotation\nPitch: %f\nYaw: %f\nRoll: %f",
		cameras[activeCamera]->GetTransform().GetRotation().x,
		cameras[activeCamera]->GetTransform().GetRotation().y,
		cameras[activeCamera]->GetTransform().GetRotation().z);
	ImGui::Text("FOV (Radians): %f", cameras[activeCamera]->GetFov());
	if (ImGui::Button("Previous")) {
		if (activeCamera == 0) activeCamera = (int)cameras.size() - 1;
		else activeCamera--;
	}
	ImGui::SameLine();
	if (ImGui::Button("Next")) {
		if (activeCamera == cameras.size() - 1) activeCamera = 0;
		else activeCamera++;
	}
	ImGui::End();

	if (ImGui::Button("Toggle Demo Window")) {
		isDemoVisible = !isDemoVisible;
	}
}
#pragma endregion
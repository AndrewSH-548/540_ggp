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
	
	cameras.push_back(make_shared<Camera>(Camera(
		Window::AspectRatio(),
		XMFLOAT3(0, -5, -30), XMFLOAT3(-0.02f, 0, 0),										//Starting position and rotation vectors
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
	uvPixelShader = make_shared<SimplePixelShader>(Graphics::Device, Graphics::Context, FixPath(L"psUV.cso").c_str());
	normalPixelShader = make_shared<SimplePixelShader>(Graphics::Device, Graphics::Context, FixPath(L"psNormal.cso").c_str());
	customPixelShader = make_shared<SimplePixelShader>(Graphics::Device, Graphics::Context, FixPath(L"psCustom.cso").c_str());
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
	entities.push_back(Entity("Red Cube", Mesh(FixPath(L"../../Assets/Models/cube.obj").c_str()), lightFilter));
	entities.push_back(Entity("Red Plane", Mesh(FixPath(L"../../Assets/Models/quad_double_sided.obj").c_str()), lightFilter));
	entities.push_back(Entity("RGB Donut", Mesh(FixPath(L"../../Assets/Models/torus.obj").c_str()), lightFilter));
	entities.push_back(Entity("RGB Helix", Mesh(FixPath(L"../../Assets/Models/helix.obj").c_str()), lightFilter));

	//Position all the objects.
	for (int i = 0; i < entities.size(); i++) {
		if (i % 2 == 1) entities[i].GetTransform()->MoveAbsolute(3.0f, 0.0f, 0.0f);
		if (i / 2 >= 1) {
			for (int j = 0; j < i / 2; j++) {
				entities[i].GetTransform()->MoveAbsolute(0.0f, -3.0f, 0.0f);
			}
		}
	}

	//Make the plane upright
	entities[5].GetTransform()->Rotate(XM_PIDIV2, 0.0f, 0.0f);

	//Add 3 directional lights
	for (int i = 0; i < 3; i++) {
		Light light = {};
		light.type = LIGHT_TYPE_DIRECTIONAL;
		light.intensity = 1.3f;
		lights.push_back(light);
	}
	//First Light: Yellow, lower left source
	lightNames.push_back("Yellow");
	lights[0].direction = XMFLOAT3(1.0f, -1.0f, 0.0f);
	lights[0].color = XMFLOAT3(0.8f, 0.8f, 0.3f);
	//Second Light: Cyan, lower right source farther front
	lightNames.push_back("Cyan");
	lights[1].direction = XMFLOAT3(-1.0f, -1.0f, 1.0f);
	lights[1].color = XMFLOAT3(0.3f, 0.8f, 0.8f);
	//Third Light: Magenta, above and behind source
	lightNames.push_back("Magenta");
	lights[2].direction = XMFLOAT3(0.0f, 1.0f, -1.0f);
	lights[2].color = XMFLOAT3(0.8f, 0.3f, 0.8f);

	//Add 2 point lights
	for (int i = 0; i < 2; i++) {
		Light light = {};
		light.type = LIGHT_TYPE_POINT;
		light.range = 8.0f;
		light.intensity = 4;
		lights.push_back(light);
	}
	//Fourth Light: Red point light
	lightNames.push_back("Red");
	lights[3].position = XMFLOAT3(0.0f, 0.0f, 0.0f);
	lights[3].color = XMFLOAT3(1.0f, 0.0f, 0.0f);
	//Fifth Light: Blue point light
	lightNames.push_back("Blue");
	lights[4].position = XMFLOAT3(0.0f, -8.0f, 0.0f);
	lights[4].color = XMFLOAT3(0.0f, 0.0f, 1.0f);
}


// --------------------------------------------------------
// Handle resizing to match the new window size
//  - Eventually, we'll want to update our 3D camera
// --------------------------------------------------------
void Game::OnResize()
{
	//This is called before initialization, so ensure the camera exists before calling anything it has.
	if (cameras.size() > 0) {
		for (shared_ptr<Camera> c : cameras){
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
	entities[7].GetTransform()->Rotate(0, deltaTime, 0);

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
	}

	
	// DRAW geometry
	// - These steps are generally repeated for EACH object you draw
	// - Other Direct3D calls will also be necessary to do more complex things
	{	
		for (int i = 0; i < entities.size(); i++) {
			entities[i].GetMaterial()->GetVertexShader()->SetShader();
			entities[i].GetMaterial()->GetPixelShader()->SetShader();
			ConstructShaderData(entities[i], XMFLOAT3(1, 1, 1), totalTime);
			entities[i].Draw();
		}
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

void Game::ConstructShaderData(Entity currentEntity, XMFLOAT3 ambientColor, float totalTime) {
	std::shared_ptr<SimpleVertexShader> vertexShader = currentEntity.GetMaterial()->GetVertexShader();
	vertexShader->SetMatrix4x4("world", currentEntity.GetTransform()->GetWorldMatrix()); 
	vertexShader->SetMatrix4x4("worldInverseTranspose", currentEntity.GetTransform()->GetWorldInverseTransposeMatrix());
	vertexShader->SetMatrix4x4("view", cameras[activeCamera]->GetViewMatrix());
	vertexShader->SetMatrix4x4("projection", cameras[activeCamera]->GetProjectionMatrix()); 
	
	std::shared_ptr<SimplePixelShader> pixelShader = currentEntity.GetMaterial()->GetPixelShader();
	pixelShader->SetFloat4("colorTint", currentEntity.GetMaterial()->GetColorTint());
	pixelShader->SetFloat3("cameraPos", cameras[activeCamera]->GetTransform().GetPosition());
	pixelShader->SetFloat3("ambient", ambientColor);
	pixelShader->SetFloat("totalTime", totalTime);
	pixelShader->SetFloat("roughness", currentEntity.GetMaterial()->GetRoughness());
	pixelShader->SetData("lights", &lights[0], sizeof(Light) * (int)lights.size());

	//Copy the data to the buffer at the start of the frame
	vertexShader->CopyAllBufferData();
	pixelShader->CopyAllBufferData();
}

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

	//Separate Window for the color editor
	ImGui::Begin("Background Color Editor");

	ImGui::ColorPicker4("Background", &displayColor[0]);

	ImGui::End();

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
	for (int i = 0; i < 3; i++) {
		ImGui::PushID(i);
		ImGui::Text(lightNames[i]);
		ImGui::SliderFloat3("Direction", &lights[i].direction.x, -10.0f, 10.0f);
		ImGui::PopID();
	}
	ImGui::End();

	ImGui::Begin("Point Light Control");
	for (int i = 3; i < 5; i++) {
		ImGui::PushID(i);
		ImGui::Text(lightNames[i]);
		ImGui::SliderFloat3("Position", &lights[i].position.x, -10.0f, 10.0f);
		ImGui::PopID();
	}
	ImGui::End();

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
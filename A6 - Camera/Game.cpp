#include "Game.h"
#include "Graphics.h"
#include "Vertex.h"
#include "Input.h"
#include "PathHelpers.h"
#include "Window.h"
#include "Mesh.h"
#include "Buffer.h"

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
	LoadConstantBuffer();

	// Set initial graphics API state
	//  - These settings persist until we change them
	//  - Some of these, like the primitive topology & input layout, probably won't change
	//  - Others, like setting shaders, will need to be moved elsewhere later
	{
		// Tell the input assembler (IA) stage of the pipeline what kind of
		// geometric primitives (points, lines or triangles) we want to draw.  
		// Essentially: "What kind of shape should the GPU draw with our vertices?"
		Graphics::Context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		// Ensure the pipeline knows how to interpret all the numbers stored in
		// the vertex buffer. For this course, all of your vertices will probably
		// have the same layout, so we can just set this once at startup.
		Graphics::Context->IASetInputLayout(inputLayout.Get());

		// Set the active vertex and pixel shaders
		//  - Once you start applying different shaders to different objects,
		//    these calls will need to happen multiple times per frame
		Graphics::Context->VSSetShader(vertexShader.Get(), 0, 0);
		Graphics::Context->PSSetShader(pixelShader.Get(), 0, 0);

		//Set the constant buffer, so the program can see it
		Graphics::Context->VSSetConstantBuffers(0, 1, constBuffer.GetAddressOf());
	}

	// Initialize ImGui itself & platform/renderer backends
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGui_ImplWin32_Init(Window::Handle());
	ImGui_ImplDX11_Init(Graphics::Device.Get(), Graphics::Context.Get());
	ImGui::StyleColorsClassic();
	
	cameras.push_back(make_shared<Camera>(Camera(
		Window::AspectRatio(), 
		XMFLOAT3(0, 0, -4), XMFLOAT3(0, 0, 0),										//Starting position and rotation vectors
		0.4f, false)));
	cameras.push_back(make_shared<Camera>(Camera(
		Window::AspectRatio(),
		XMFLOAT3(-2.4f, 0, -2), XMFLOAT3(0, XM_PIDIV4, 0),
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
	// BLOBs (or Binary Large OBjects) for reading raw data from external files
	// - This is a simplified way of handling big chunks of external data
	// - Literally just a big array of bytes read from a file
	ID3DBlob* pixelShaderBlob;
	ID3DBlob* vertexShaderBlob;

	// Loading shaders
	//  - Visual Studio will compile our shaders at build time
	//  - They are saved as .cso (Compiled Shader Object) files
	//  - We need to load them when the application starts
	{
		// Read our compiled shader code files into blobs
		// - Essentially just "open the file and plop its contents here"
		// - Uses the custom FixPath() helper from Helpers.h to ensure relative paths
		// - Note the "L" before the string - this tells the compiler the string uses wide characters
		D3DReadFileToBlob(FixPath(L"PixelShader.cso").c_str(), &pixelShaderBlob);
		D3DReadFileToBlob(FixPath(L"VertexShader.cso").c_str(), &vertexShaderBlob);

		// Create the actual Direct3D shaders on the GPU
		Graphics::Device->CreatePixelShader(
			pixelShaderBlob->GetBufferPointer(),	// Pointer to blob's contents
			pixelShaderBlob->GetBufferSize(),		// How big is that data?
			0,										// No classes in this shader
			pixelShader.GetAddressOf());			// Address of the ID3D11PixelShader pointer

		Graphics::Device->CreateVertexShader(
			vertexShaderBlob->GetBufferPointer(),	// Get a pointer to the blob's contents
			vertexShaderBlob->GetBufferSize(),		// How big is that data?
			0,										// No classes in this shader
			vertexShader.GetAddressOf());			// The address of the ID3D11VertexShader pointer
	}

	// Create an input layout 
	//  - This describes the layout of data sent to a vertex shader
	//  - In other words, it describes how to interpret data (numbers) in a vertex buffer
	//  - Doing this NOW because it requires a vertex shader's byte code to verify against!
	//  - Luckily, we already have that loaded (the vertex shader blob above)
	{
		D3D11_INPUT_ELEMENT_DESC inputElements[2] = {};

		// Set up the first element - a position, which is 3 float values
		inputElements[0].Format = DXGI_FORMAT_R32G32B32_FLOAT;				// Most formats are described as color channels; really it just means "Three 32-bit floats"
		inputElements[0].SemanticName = "POSITION";							// This is "POSITION" - needs to match the semantics in our vertex shader input!
		inputElements[0].AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;	// How far into the vertex is this?  Assume it's after the previous element

		// Set up the second element - a color, which is 4 more float values
		inputElements[1].Format = DXGI_FORMAT_R32G32B32A32_FLOAT;			// 4x 32-bit floats
		inputElements[1].SemanticName = "COLOR";							// Match our vertex shader input!
		inputElements[1].AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;	// After the previous element

		// Create the input layout, verifying our description against actual shader code
		Graphics::Device->CreateInputLayout(
			inputElements,							// An array of descriptions
			2,										// How many elements in that array?
			vertexShaderBlob->GetBufferPointer(),	// Pointer to the code of a shader that uses this layout
			vertexShaderBlob->GetBufferSize(),		// Size of the shader code that uses this layout
			inputLayout.GetAddressOf());			// Address of the resulting ID3D11InputLayout pointer
	}
}

// --------------------------------------------------------
// Loads the constant buffer. Size i
// --------------------------------------------------------
void Game::LoadConstantBuffer() {
	//Calculate the size using the buffer struct
	unsigned int size = sizeof(Buffer);
	size = (size + 15) / 16 * 16; // Must be a multiple of 16

	// Describe the constant buffer
	D3D11_BUFFER_DESC cbDesc = {}; // Sets struct to all zeros
	cbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	cbDesc.ByteWidth = size;
	cbDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	cbDesc.Usage = D3D11_USAGE_DYNAMIC;
	Graphics::Device->CreateBuffer(&cbDesc, 0, constBuffer.GetAddressOf());
}

// --------------------------------------------------------
// Creates the geometry we're going to draw
// --------------------------------------------------------
void Game::CreateGeometry()
{
	//Predefine the array of triangles
	Vertex triangleVertices[] = {
		{ XMFLOAT3(+0.0f, +0.5f, -0.1f), XMFLOAT4(1.0f, 1.0f, 0.0f, 1.0f) },
		{ XMFLOAT3(+0.5f, -0.5f, -0.1f), XMFLOAT4(0.0f, 1.0f, 1.0f, 1.0f) },
		{ XMFLOAT3(-0.5f, -0.5f, -0.1f), XMFLOAT4(1.0f, 0.0f, 1.0f, 1.0f) },
	};
	
	Vertex diamondVertices[] = {
		{ XMFLOAT3(-0.35f, +0.4f, +0.0f), XMFLOAT4(0.0f, 1.0f, 1.0f, 1.0f) },
		{ XMFLOAT3(-0.27f, +0.6f, +0.0f), XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f) },
		{ XMFLOAT3(-0.19f, +0.4f, +0.0f), XMFLOAT4(0.3f, 0.8f, 1.0f, 1.0f) },
		{ XMFLOAT3(-0.27f, +0.2f, +0.0f), XMFLOAT4(0.0f, 1.0f, 1.0f, 1.0f) },	
	};

	Vertex hexagonVertices[] = {
		{ XMFLOAT3(+0.3f, +0.6f, +0.0f), XMFLOAT4(1.0f, 0.4f, 0.4f, 1.0f) },
		{ XMFLOAT3(+0.4f, +0.5f, +0.0f), XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f) },
		{ XMFLOAT3(+0.4f, +0.3f, +0.0f), XMFLOAT4(0.8f, 0.0f, 0.0f, 1.0f) },
		{ XMFLOAT3(+0.3f, +0.2f, +0.0f), XMFLOAT4(0.6f, 0.0f, 0.0f, 1.0f) },
		{ XMFLOAT3(+0.2f, +0.3f, +0.0f), XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f) },
		{ XMFLOAT3(+0.2f, +0.5f, +0.0f), XMFLOAT4(1.0f, 0.2f, 0.2f, 1.0f) },
	};

	int triangleIndices[] = { 0, 1, 2 };
	int diamondIndices[] = { 0, 1, 2, 0, 2, 3};
	int hexagonIndices[] = { 0, 1, 2, 0, 2, 3, 0, 3, 4, 0, 4, 5};

	Mesh starterTriangle = Mesh(triangleVertices, 3, triangleIndices, 3);
	Mesh diamond = Mesh(diamondVertices, 4, diamondIndices, 6);
	Mesh hexagon = Mesh(hexagonVertices, 6, hexagonIndices, 12);

	entities.push_back(Entity("Starter Triangle", starterTriangle));
	entities.push_back(Entity("Diamond 1", diamond));
	entities.push_back(Entity("Diamond 2", diamond));
	entities.push_back(Entity("Diamond 3", diamond));
	entities.push_back(Entity("Hexagon", hexagon));
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
	if (entities[2].GetTransform()->GetPosition().x > 0.2f || entities[2].GetTransform()->GetPosition().x < -0.2f) {
		movementSpeed *= -1;
	}
	float direction = movementSpeed * deltaTime;
	//Update the UI
	UpdateImGui(deltaTime);
	BuildUI();
	cameras[activeCamera]->Update(deltaTime);
	entities[0].GetTransform()->Rotate(0, 0, deltaTime);
	entities[2].GetTransform()->MoveAbsolute(direction, 0, 0);
	entities[3].GetTransform()->MoveAbsolute(0, direction, 0);

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
			ConstructShaderData(entities[i].GetTransform()->GetWorldMatrix(), cameras[activeCamera]->GetViewMatrix(), cameras[activeCamera]->GetProjectionMatrix());
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

void Game::ConstructShaderData(XMFLOAT4X4 worldMatrix, XMFLOAT4X4 viewMatrix, XMFLOAT4X4 projectionMatrix) {
	// Set shader effect
	// Uses the predefined color tint and offset.
	Buffer shaderData = Buffer();
	shaderData.world = worldMatrix;
	shaderData.view = viewMatrix;
	shaderData.projection = projectionMatrix;
	shaderData.colorTint = XMFLOAT4(colorTint);

	//Copy the data to the buffer at the start of the frame
	D3D11_MAPPED_SUBRESOURCE mappedBuffer = {};
	Graphics::Context->Map(constBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedBuffer);
	memcpy(mappedBuffer.pData, &shaderData, sizeof(shaderData));
	Graphics::Context->Unmap(constBuffer.Get(), 0);
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

	ImGui::Begin("Shader Editor");
	ImGui::ColorPicker4("Color Tint", &colorTint[0]);
	ImGui::End();

	ImGui::Begin("Camera Control");
	ImGui::Text("Camera %d", activeCamera);
	ImGui::Text("Position: %f %f %f",
		cameras[activeCamera]->GetTransform().GetPosition().x,
		cameras[activeCamera]->GetTransform().GetPosition().y,
		cameras[activeCamera]->GetTransform().GetPosition().z);
	ImGui::Text("Rotation\nPitch: %f\nYaw: %f\nRoll: %f",
		cameras[activeCamera]->GetTransform().GetRotation().x,
		cameras[activeCamera]->GetTransform().GetRotation().y,
		cameras[activeCamera]->GetTransform().GetRotation().z);
	ImGui::Text("FOV (Radians): %f", cameras[activeCamera]->GetFov());
	if (ImGui::Button("Previous")) {
		if (activeCamera == 0) activeCamera = cameras.size() - 1;
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
#pragma once
#include "Vertex.h"

#include <d3d11.h>
#include <wrl/client.h>
#include <DirectXMath.h>
#include <d3dcompiler.h>

class Mesh
{
public:
	Microsoft::WRL::ComPtr<ID3D11Buffer> GetVertexBuffer();
	Microsoft::WRL::ComPtr<ID3D11Buffer> GetIndexBuffer();
	int GetVertexCount();
	int GetIndexCount();
	void Draw();
	Mesh(Vertex vertices[], int vertexCount, int indices[], int indexCount);
	~Mesh();

private:
	Microsoft::WRL::ComPtr<ID3D11Buffer> vertexBuffer;
	Microsoft::WRL::ComPtr<ID3D11Buffer> indexBuffer;
	int vertexCount;
	int indexCount;
};


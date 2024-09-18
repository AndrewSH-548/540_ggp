#pragma once
#include <d3d11.h>
#include <DirectXMath.h>
#include <wrl/client.h>


struct Buffer {
	DirectX::XMFLOAT4 colorTint;
	DirectX::XMFLOAT3 offset;
};
#pragma once
#include "Transform.h"
#include <DirectXMath.h>

class Camera
{
public:
	Camera(float aspectRatio, DirectX::XMFLOAT3 startPos, DirectX::XMFLOAT3 orientation, float fovRads, bool isOrthographic);

	DirectX::XMFLOAT4X4 GetViewMatrix();
	DirectX::XMFLOAT4X4 GetProjectionMatrix();
	Transform GetTransform();
	float GetFov();

	void UpdateViewMatrix();
	void UpdateProjectionMatrix(float aspectRatio);
	void Update(float deltaTime);
private:
	Transform transform;

	DirectX::XMFLOAT4X4 viewMatrix;
	DirectX::XMFLOAT4X4 projectionMatrix;

	float fovRadians;

	float nearPlane;
	float farPlane;
	
	float moveSpeed;
	float mouseSpeed;

	bool isOrthographic;
};


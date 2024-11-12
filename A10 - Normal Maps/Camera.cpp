#include "Camera.h"
#include "Input.h"
#include <DirectXMath.h>

using namespace DirectX;

/// <summary>
/// Camera constructor. Allows custom orientation and fov. 
/// </summary>
/// <param name="windowWidth">Width of the window at startup.</param>
/// <param name="windowHeight">Height of the window at startup.</param>
/// <param name="startPos">Camera's initial position.</param>
/// <param name="orientation">Camera's initial orientation.</param>
/// <param name="fovRads">FOV angle in radians.</param>
/// <param name="isOrthographic">True if orthographic, false if perspective.</param>
Camera::Camera(float aspectRatio, XMFLOAT3 startPos, XMFLOAT3 orientation, float fovRads, bool isOrthographic) {
	transform = Transform();
	transform.SetPosition(startPos);
	transform.SetRotation(orientation);
	
	fovRadians = fovRads;

	moveSpeed = 10;
	mouseSpeed = 0.005f;

	this->isOrthographic = isOrthographic;
	
	UpdateViewMatrix();
	UpdateProjectionMatrix(aspectRatio);
}

/// <summary>
/// Returns the camera's view matrix.
/// </summary>
XMFLOAT4X4 Camera::GetViewMatrix() {
	return viewMatrix;
}

/// <summary>
/// Returns the camera's projection matrix.
/// </summary>
XMFLOAT4X4 Camera::GetProjectionMatrix() {
	return projectionMatrix;
}

/// <summary>
/// Returns the camera's transform.
/// </summary>
Transform Camera::GetTransform() {
	return transform;
}

/// <summary>
/// Returns the camera's FOV.
/// </summary>
float Camera::GetFov() {
	return fovRadians;
}
 
/// <summary>
/// Uses the transform to update the view matrix.
/// </summary>
void Camera::UpdateViewMatrix() {
	XMFLOAT3 localPos = transform.GetPosition();
	XMFLOAT3 direction = transform.GetForward();

	XMMATRIX view = XMMatrixLookToLH(
		XMLoadFloat3(&localPos),							//Eye Position - Loaded from transform's position
		XMLoadFloat3(&direction),							//Eye Direction - Loaded from transform's rotation
		XMVECTOR{0, 1, 0});									//World Up Vector
	XMStoreFloat4x4(&viewMatrix, view);
}

/// <summary>
/// Updates the projection matrix. Method differs if perspective or orthographic.
/// </summary>
/// <param name="aspectRatio">Current aspect ratio of the window.</param>
void Camera::UpdateProjectionMatrix(float aspectRatio) {
	XMMATRIX projection = isOrthographic ? XMMatrixOrthographicLH(aspectRatio, 1, 0.1f, 100) : XMMatrixPerspectiveFovLH(fovRadians, aspectRatio, 0.1f, 100);
	XMStoreFloat4x4(&projectionMatrix, projection);
}

/// <summary>
/// Updates the camera every frame. Includes 
/// </summary>
/// <param name="deltaTime">Change in time, used to standardize update rates.</param>
void Camera::Update(float deltaTime) {
	//Forward and backward movement
	if (Input::KeyDown('W')) transform.MoveRelative(XMFLOAT3(0, 0, moveSpeed * deltaTime));
	if (Input::KeyDown('S')) transform.MoveRelative(XMFLOAT3(0, 0, -moveSpeed * deltaTime));

	//Horizontal movement. -X is left, +X is right.
	if (Input::KeyDown('A')) transform.MoveRelative(XMFLOAT3(-moveSpeed * deltaTime, 0, 0));
	if (Input::KeyDown('D')) transform.MoveRelative(XMFLOAT3(moveSpeed * deltaTime, 0, 0));

	//Move along the world Y-axis.
	if (Input::KeyDown('R')) transform.MoveAbsolute(XMFLOAT3(0, moveSpeed * deltaTime * 2, 0));
	if (Input::KeyDown('F')) transform.MoveAbsolute(XMFLOAT3(0, -moveSpeed * deltaTime * 2, 0));

	if (Input::MouseLeftDown()) {
		float cursorMovementX = (float)Input::GetMouseXDelta();
		float cursorMovementY = (float)Input::GetMouseYDelta();

		transform.Rotate(cursorMovementY * mouseSpeed, cursorMovementX * mouseSpeed, 0);
	}

	//Clamp the X rotation.
	if (transform.GetRotation().x > XM_PIDIV2) transform.SetRotation(XM_PIDIV2, transform.GetRotation().y, transform.GetRotation().z);
	if (transform.GetRotation().x < -XM_PIDIV2) transform.SetRotation(-XM_PIDIV2, transform.GetRotation().y, transform.GetRotation().z);

	UpdateViewMatrix();
}
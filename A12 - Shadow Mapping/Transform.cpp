#include "Transform.h"
#include <DirectXMath.h>

using namespace DirectX;

/// <summary>
/// Constructor, sets defaults for the vectors and matrices.
/// </summary>
Transform::Transform() {
	position = XMFLOAT3(0, 0, 0);
	rotation = XMFLOAT3(0, 0, 0);
	scale = XMFLOAT3(1, 1, 1);

	XMStoreFloat4x4(&world, XMMatrixIdentity());
	XMStoreFloat4x4(&worldInverseTranspose, XMMatrixIdentity());

}

/// <summary>
/// Set position vector using separated float values.
/// </summary>
/// <param name="x">X-offset.</param>
/// <param name="y">Y-offset.</param>
/// <param name="z">Z-offset.</param>
void Transform::SetPosition(float x, float y, float z) {
	position.x = x;
	position.y = y;
	position.z = z;
}

/// <summary>
/// Set position vector using combined xmfloat3 value.
/// </summary>
/// <param name="position">New position value.</param>
void Transform::SetPosition(XMFLOAT3 position) {
	this->position = position;
}

/// <summary>
/// Set rotation vector using separated float values.
/// </summary>
/// <param name="pitch">rotation around X-axis.</param>
/// <param name="yaw">rotation around Y-axis.</param>
/// <param name="roll">rotation around Z-axis.</param>
void Transform::SetRotation(float pitch, float yaw, float roll) {
	rotation.x = pitch;
	rotation.y = yaw;
	rotation.z = roll;
}

/// <summary>
/// Set rotation vector using combined xmfloat3 value.
/// </summary>
/// <param name="rotation">New rotation value.</param>
void Transform::SetRotation(XMFLOAT3 rotation) {
	this->rotation = rotation;
}

/// <summary>
/// Set scale vector using separated float values.
/// </summary>
/// <param name="x">x-factor.</param>
/// <param name="y">y-factor.</param>
/// <param name="z">z-factor.</param>
void Transform::SetScale(float x, float y, float z) {
	scale.x = x;
	scale.y = y;
	scale.z = z;
}

/// <summary>
/// Set scale vector using combined xmfloat3 value.
/// </summary>
/// <param name="rotation">New scale value.</param>
void Transform::SetScale(XMFLOAT3 scale) {
	this->scale = scale;
}

/// <summary>
/// Uses the current transform vectors to set the world matrix and its inverse transpose.
/// </summary>
void Transform::SetWorldMatrices() {
	XMMATRIX translation = XMMatrixTranslation(position.x, position.y, position.z);
	XMMATRIX rotation = XMMatrixRotationRollPitchYaw(this->rotation.x, this->rotation.y, this->rotation.z);
	XMMATRIX scale = XMMatrixScaling(this->scale.x, this->scale.y, this->scale.z);

	XMMATRIX world = scale * rotation * translation;
	XMStoreFloat4x4(&this->world, world);
	XMStoreFloat4x4(&this->worldInverseTranspose, XMMatrixInverse(0, XMMatrixTranspose(world)));
}

XMFLOAT3 Transform::GetPosition() {
	return position;
}

XMFLOAT3 Transform::GetRotation() {
	return rotation;
}

XMFLOAT3 Transform::GetScale() {
	return scale;
}

XMFLOAT4X4 Transform::GetWorldMatrix() {
	return world;
}

XMFLOAT4X4 Transform::GetWorldInverseTransposeMatrix() {
	return worldInverseTranspose;
}

XMFLOAT3 Transform::GetRight() {
	XMVECTOR orientation = XMQuaternionRotationRollPitchYaw(rotation.x, rotation.y, rotation.z);
	XMVECTOR right = XMVector3Rotate(XMVECTOR{ 1, 0, 0 }, orientation);
	return XMFLOAT3(XMVectorGetX(right), XMVectorGetY(right), XMVectorGetZ(right));
}

XMFLOAT3 Transform::GetUp() {
	XMVECTOR orientation = XMQuaternionRotationRollPitchYaw(rotation.x, rotation.y, rotation.z);
	XMVECTOR up = XMVector3Rotate(XMVECTOR{ 0, 1, 0 }, orientation);
	return XMFLOAT3(XMVectorGetX(up), XMVectorGetY(up), XMVectorGetZ(up));
}

XMFLOAT3 Transform::GetForward() {
	XMVECTOR orientation = XMQuaternionRotationRollPitchYaw(rotation.x, rotation.y, rotation.z);
	XMVECTOR forward = XMVector3Rotate(XMVECTOR{ 0, 0, 1 }, orientation);
	return XMFLOAT3(XMVectorGetX(forward), XMVectorGetY(forward), XMVectorGetZ(forward));
}

/// <summary>
/// Adjust world position vector using separated float values.
/// </summary>
/// <param name="x">X-offset.</param>
/// <param name="y">Y-offset.</param>
/// <param name="z">Z-offset.</param>
void Transform::MoveAbsolute(float x, float y, float z) {
	position.x += x;
	position.y += y;
	position.z += z;
}

/// <summary>
/// Adjust local position vector using separated float values.
/// </summary>
/// <param name="x">X-offset.</param>
/// <param name="y">Y-offset.</param>
/// <param name="z">Z-offset.</param>
void Transform::MoveRelative(float x, float y, float z) {
	XMVECTOR localPos = XMVECTOR{ x, y, z };
	XMVECTOR orientation = XMQuaternionRotationRollPitchYaw(rotation.x, rotation.y, rotation.z);
	XMVECTOR movement = XMVector3Rotate(localPos, orientation);
	position.x += XMVectorGetX(movement);
	position.y += XMVectorGetY(movement);
	position.z += XMVectorGetZ(movement);

}

/// <summary>
/// Adjust world position vector using combined xmfloat3 value.
/// </summary>
/// <param name="position">Offset value.</param>
void Transform::MoveAbsolute(XMFLOAT3 position) {
	this->position.x += position.x;
	this->position.y += position.y;
	this->position.z += position.z;
}

/// <summary>
/// Adjust local position vector using combined xmfloat3 value.
/// </summary>
/// <param name="position">Offset value.</param>
void Transform::MoveRelative(XMFLOAT3 position) {
	XMVECTOR localPos = XMVECTOR{ position.x, position.y, position.z };
	XMVECTOR orientation = XMQuaternionRotationRollPitchYaw(rotation.x, rotation.y, rotation.z);
	XMVECTOR movement = XMVector3Rotate(localPos, orientation);
	this->position.x += XMVectorGetX(movement);
	this->position.y += XMVectorGetY(movement);
	this->position.z += XMVectorGetZ(movement);
}

/// <summary>
/// Adjust rotation vector using separated float values.
/// </summary>
/// <param name="pitch">rotation around X-axis.</param>
/// <param name="yaw">rotation around Y-axis.</param>
/// <param name="roll">rotation around Z-axis.</param>
void Transform::Rotate(float pitch, float yaw, float roll) {
	rotation.x += pitch;
	rotation.y += yaw;
	rotation.z += roll;
}

/// <summary>
/// Adjust rotation vector using combined xmfloat3 value.
/// </summary>
/// <param name="rotation">Offset value.</param>
void Transform::Rotate(XMFLOAT3 rotation) {
	this->rotation.x += rotation.x;
	this->rotation.y += rotation.y;
	this->rotation.z += rotation.z;
}

/// <summary>
/// Adjust scale vector using separated float values.
/// </summary>
/// <param name="x">x-factor.</param>
/// <param name="y">y-factor.</param>
/// <param name="z">z-factor.</param>
void Transform::Scale(float x, float y, float z) {
	scale.x *= x;
	scale.y *= y;
	scale.z *= z;
}

/// <summary>
/// Adjust scale vector using combined xmfloat3 value.
/// </summary>
/// <param name="rotation">Scale factor.</param>
void Transform::Scale(XMFLOAT3 scale) {
	this->scale.x *= scale.x;
	this->scale.y *= scale.y;
	this->scale.z *= scale.z;
}
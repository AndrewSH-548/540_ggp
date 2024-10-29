#include "Entity.h"

using namespace std;

Entity::Entity(const char* name, Mesh mesh, Material material) {
	transform = Transform();
	this->mesh = make_shared<Mesh>(mesh);
	this->material = make_shared<Material>(material);
}

void Entity::Draw() {
	mesh->Draw();
}

Entity::~Entity() {
}

Transform* Entity::GetTransform() {
	return &transform;
}

shared_ptr<Mesh> Entity::GetMesh() {
	return mesh;
}

std::shared_ptr<Material> Entity::GetMaterial() {
	return material;
}
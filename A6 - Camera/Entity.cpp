#include "Entity.h"
#include "Transform.h"
#include "Mesh.h"

#include <memory>

using namespace std;

Entity::Entity(const char* name, Mesh mesh) {
	transform = Transform();
	this->mesh = make_shared<Mesh>(mesh);

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
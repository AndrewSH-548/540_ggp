#pragma once
#include <memory>
#include "Transform.h"
#include "Mesh.h"

class Entity
{
public:
	Entity(const char* name, Mesh mesh);
	~Entity();

	Transform* GetTransform();
	std::shared_ptr<Mesh> GetMesh();
	const char* name;

	void Draw();
private:
	Transform transform;
	std::shared_ptr<Mesh> mesh;
	
};


#pragma once
#include "IComponent.h"
#include "Gameplay/Physics/RigidBody.h"
#include "Gameplay/Physics/TriggerVolume.h"
#include "Utils/TypeHelpers.h"

struct GLFWwindow;

/// <summary>
/// A simple behaviour that allows movement of a gameobject with WASD, mouse,
/// and ctrl + space
/// </summary>
class SimpleCameraControl : public Gameplay::IComponent {
public:
	typedef std::shared_ptr<SimpleCameraControl> Sptr;

	SimpleCameraControl();
	virtual ~SimpleCameraControl();

	virtual void Update(float deltaTime) override;

public:
	virtual void RenderImGui() override;
	MAKE_TYPENAME(SimpleCameraControl);
	virtual nlohmann::json ToJson() const override;
	static SimpleCameraControl::Sptr FromJson(const nlohmann::json& blob);
	bool canMove = true;

protected:
	float _shiftMultipler;
	glm::vec2 _mouseSensitivity;
	glm::vec3 _moveSpeeds;
	glm::dvec2 _prevMousePos;
	glm::vec2 _currentRot;

	bool _isMousePressed = false;
	

	float _impulse;

	bool _isPressed = false;
	Gameplay::Physics::RigidBody::Sptr _body;
};
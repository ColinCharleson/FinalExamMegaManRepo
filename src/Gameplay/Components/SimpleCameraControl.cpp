#include "Gameplay/Components/SimpleCameraControl.h"
#include <GLFW/glfw3.h>
#define  GLM_SWIZZLE
#include <GLM/gtc/quaternion.hpp>

#include "Gameplay/GameObject.h"
#include "Gameplay/Scene.h"
#include "Utils/JsonGlmHelpers.h"
#include "Gameplay/Physics/RigidBody.h"
#include "Gameplay/Physics/TriggerVolume.h"
#include "Utils/ImGuiHelper.h"
#include "Gameplay/Components/EnemyControl.h"
#include "Gameplay/Physics/RigidBody.h"
#include "Gameplay/InputEngine.h"
#include "Application/Application.h"
#include "Gameplay/Components/GUI/GuiPanel.h"

SimpleCameraControl::SimpleCameraControl() :
	IComponent(),
	_mouseSensitivity({ 0.5f, 0.3f }),
	_moveSpeeds(glm::vec3(1.0f)),
	_shiftMultipler(2.0f),
	_currentRot(glm::vec2(0.0f)),
	_isMousePressed(false)
{
}

SimpleCameraControl::~SimpleCameraControl() = default;
using namespace Gameplay::Physics;

void SimpleCameraControl::Update(float deltaTime)
{
	GetGameObject()->SetRotation(glm::vec3(0.0f, 0.0f, 180.0f));
	if (InputEngine::IsKeyDown(GLFW_KEY_A) && canMove)
	{
		GetGameObject()->SetPostion(glm::vec3((GetGameObject()->GetPosition().x - 2 * deltaTime),6, GetGameObject()->GetPosition().z));
	}
	if (InputEngine::IsKeyDown(GLFW_KEY_D) && canMove)
	{
		GetGameObject()->SetPostion(glm::vec3((GetGameObject()->GetPosition().x + 2 * deltaTime), 6, GetGameObject()->GetPosition().z));
	}
}



void SimpleCameraControl::RenderImGui()
{
	LABEL_LEFT(ImGui::DragFloat2, "Mouse Sensitivity", &_mouseSensitivity.x, 0.01f);
	LABEL_LEFT(ImGui::DragFloat3, "Move Speed       ", &_moveSpeeds.x, 0.01f, 0.01f);
	LABEL_LEFT(ImGui::DragFloat, "Shift Multiplier ", &_shiftMultipler, 0.01f, 1.0f);
	ImGui::Text((~InputEngine::GetMouseState(GLFW_MOUSE_BUTTON_LEFT)).c_str());
	glm::dvec2 delta = InputEngine::GetMousePos() - _prevMousePos;
	ImGui::Text("%d, %d", delta.x, delta.y);
}

nlohmann::json SimpleCameraControl::ToJson() const
{
	return {
		{ "mouse_sensitivity", _mouseSensitivity },
		{ "move_speed", _moveSpeeds },
		{ "shift_mult", _shiftMultipler }
	};
}

SimpleCameraControl::Sptr SimpleCameraControl::FromJson(const nlohmann::json & blob)
{
	SimpleCameraControl::Sptr result = std::make_shared<SimpleCameraControl>();
	result->_mouseSensitivity = JsonGet(blob, "mouse_sensitivity", result->_mouseSensitivity);
	result->_moveSpeeds = JsonGet(blob, "move_speed", result->_moveSpeeds);
	result->_shiftMultipler = JsonGet(blob, "shift_mult", 2.0f);
	return result;
}

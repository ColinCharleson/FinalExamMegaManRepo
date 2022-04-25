#include "Gameplay/Physics/TriggerVolume.h"

#include <BulletCollision/CollisionDispatch/btGhostObject.h>

#include "Utils/GlmBulletConversions.h"

#include <GLFW/glfw3.h>
#include "Gameplay/GameObject.h"
#include "Gameplay/Scene.h"
#include "Utils/ImGuiHelper.h"
#include "Gameplay/Components/SimpleCameraControl.h"
#include "Gameplay/Components/TriggerVolumeEnterBehaviour.h"

#include "Gameplay/Components/EnemyControl.h"
#include "Gameplay/Physics/RigidBody.h"
#include "NOU/Mesh.h"
#include "NOU/CCamera.h"
#include "Gameplay/Material.h"
#include "Utils/TypeHelpers.h"

#include "Gameplay/InputEngine.h"
#include "Application/Application.h"
#include "Gameplay/Components/GUI/GuiPanel.h"

void EnemyControl::RenderImGui()
{
	LABEL_LEFT(ImGui::DragFloat, "Impulse", &_impulse, 1.0f);
}

nlohmann::json EnemyControl::ToJson() const
{
	return {
		{ "impulse", _impulse }
	};
}

using namespace Gameplay::Physics;

EnemyControl::EnemyControl() :
	IComponent(),
	_impulse(10.0f)
{
}

EnemyControl::Sptr EnemyControl::FromJson(const nlohmann::json& blob)
{
	EnemyControl::Sptr result = std::make_shared<EnemyControl>();
	result->_impulse = blob["impulse"];
	return result;
}

EnemyControl::~EnemyControl() = default;

template<typename T>
T Lerp(const T & a, const T & b, float t)
{
	return (1.0f - t) * a + t * b;
}

void EnemyControl::Update(float deltaTime)
{
	std::vector<glm::vec3> _points = { glm::vec3(8.0f, 6.0f, 1.0f), glm::vec3(3.0f, 6.0f, 1.0f), };

	_timer += deltaTime;

	while (_timer >= m_segmentTravelTime)
	{
		_timer -= m_segmentTravelTime;

		m_segmentIndex += 1;

		if (m_segmentIndex == _points.size())
		{
			m_segmentIndex = 0;
		}
	}

	float t = _timer / m_segmentTravelTime;

	glm::vec3 a = _points[((m_segmentIndex - 1) + _points.size()) % _points.size()];
	glm::vec3 b = _points[((m_segmentIndex)+_points.size()) % _points.size()];

	GetGameObject()->SetPostion(Lerp(a, b, t));
}

void EnemyControl::OnTriggerVolumeEntered(const std::shared_ptr<RigidBody>& body)
{

	if (body->GetGameObject()->GetParent())
	{
		_CollideName = body->GetGameObject()->GetParent()->Name;
	}
	_CollideName = body->GetGameObject()->Name;

	if (_CollideName == "Player")
	{
		if (GetGameObject()->GetScene()->FindObjectByName("Player")->GetPosition().z <= 2.8f)
		{
			GetGameObject()->GetScene()->FindObjectByName("Player")->Get<SimpleCameraControl>()->canMove = false;
			GetGameObject()->GetScene()->FindObjectByName("You Lose Text")->Get<GuiPanel>()->IsEnabled = true;
		}
		else
		{
			GetGameObject()->GetScene()->RemoveGameObject(GetGameObject()->SelfRef());
		}
	}
}



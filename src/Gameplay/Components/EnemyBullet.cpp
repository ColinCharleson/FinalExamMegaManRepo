#include "Gameplay/Physics/TriggerVolume.h"

#include <BulletCollision/CollisionDispatch/btGhostObject.h>

#include "Utils/GlmBulletConversions.h"

#include <GLFW/glfw3.h>
#include "Gameplay/GameObject.h"
#include "Gameplay/Scene.h"
#include "Utils/ImGuiHelper.h"
#include "Gameplay/Components/SimpleCameraControl.h"
#include "Gameplay/Components/TriggerVolumeEnterBehaviour.h"

#include "Gameplay/Components/EnemyBullet.h"
#include "Gameplay/Physics/RigidBody.h"
#include "NOU/Mesh.h"
#include "NOU/CCamera.h"
#include "Gameplay/Material.h"
#include "Utils/TypeHelpers.h"

#include "Gameplay/InputEngine.h"
#include "Application/Application.h"
#include "Gameplay/Components/GUI/GuiPanel.h"

void EnemyBullet::RenderImGui()
{
	LABEL_LEFT(ImGui::DragFloat, "Impulse", &_impulse, 1.0f);
}

nlohmann::json EnemyBullet::ToJson() const
{
	return {
		{ "impulse", _impulse }
	};
}

using namespace Gameplay::Physics;

EnemyBullet::EnemyBullet() :
	IComponent(),
	_impulse(10.0f)
{
}

EnemyBullet::Sptr EnemyBullet::FromJson(const nlohmann::json& blob)
{
	EnemyBullet::Sptr result = std::make_shared<EnemyBullet>();
	result->_impulse = blob["impulse"];
	return result;
}

EnemyBullet::~EnemyBullet() = default;

template<typename T>
T Lerp(const T & a, const T & b, float t)
{
	return (1.0f - t) * a + t * b;
}
int score = 0;
float fireTime = 2;
bool readyFire = false;

glm::vec3 a = glm::vec3(8.0f, 6.0f, 5.0f);
glm::vec3 b;

void EnemyBullet::Update(float deltaTime)
{
	if (score >= 5)
	{
		GetGameObject()->GetScene()->FindObjectByName("You Win Text")->Get<GuiPanel>()->IsEnabled = true;
	}
	else
	{
		if (readyFire == false)
		{
			if (fireTime > 0)
			{
				fireTime -= 1 * deltaTime;
			}
		}

		if (fireTime <= 0)
		{
			b = GetGameObject()->GetScene()->FindObjectByName("Player")->GetPosition();
			readyFire = true;
			fireTime = 2;
		}

		if (readyFire)
		{
			_timer += deltaTime;

			float t = _timer / 8.0f;
			GetGameObject()->SetPostion(Lerp(a, b, t));
		}
	}
}

void EnemyBullet::OnTriggerVolumeEntered(const std::shared_ptr<RigidBody>& body)
{
	if (body->GetGameObject()->GetParent())
	{
		_CollideName = body->GetGameObject()->GetParent()->Name;
	}
	_CollideName = body->GetGameObject()->Name;

	if (_CollideName == "Player")
	{
			GetGameObject()->GetScene()->FindObjectByName("Player")->Get<SimpleCameraControl>()->canMove = false;
			GetGameObject()->GetScene()->FindObjectByName("You Lose Text")->Get<GuiPanel>()->IsEnabled = true;
	}
	if (_CollideName == "resetWall")
	{
		readyFire = false;
		_timer = 0;
		GetGameObject()->SetPostion(a);

		score++;
	}
	if (_CollideName == "plane")
	{
		readyFire = false;
		_timer = 0;
		GetGameObject()->SetPostion(a);

		score++;
	}
}



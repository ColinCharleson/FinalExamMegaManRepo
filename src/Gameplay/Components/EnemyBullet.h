#pragma once

#include "Gameplay/Components/IComponent.h"
#include "Gameplay/Physics/RigidBody.h"
#include "Gameplay/GameObject.h"
#include "Gameplay/Physics/TriggerVolume.h"
#include "Utils/TypeHelpers.h"

// Utils
#include "Utils/GUID.hpp"

// GLM
#define GLM_ENABLE_EXPERIMENTAL
#include "GLM/gtx/common.hpp"

// Others
#include "Gameplay/Components/IComponent.h"
#include "Gameplay/Components/ComponentManager.h"

class EnemyBullet : public Gameplay::IComponent
{
public:

	typedef std::shared_ptr<EnemyBullet> Sptr;

	EnemyBullet();
	virtual ~EnemyBullet();
	void Update(float deltaTime);

	std::weak_ptr<Gameplay::IComponent> golem1;
public:
	virtual void RenderImGui() override;
	MAKE_TYPENAME(EnemyBullet);
	virtual nlohmann::json ToJson() const override;
	static EnemyBullet::Sptr FromJson(const nlohmann::json& blob);
	virtual void OnTriggerVolumeEntered(const std::shared_ptr<Gameplay::Physics::RigidBody>& trigger);
	
	bool canMove = true;

private:

protected:
	float _impulse;
	float _timer = 0.0f;

	float _playbackSpeed;
	std::string _CollideName;
};


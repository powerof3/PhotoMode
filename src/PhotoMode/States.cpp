#include "States.h"

namespace PhotoMode
{
	void State::Camera::get()
	{
		fov = RE::PlayerCamera::GetSingleton()->worldFOV;
		translateSpeed = Cache::translateSpeedValue;

		blurMultiplier = Cache::DOF::blurMultiplier;
		nearDist = Cache::DOF::nearDist;
		nearRange = Cache::DOF::nearRange;
		farDist = Cache::DOF::farDist;
		farRange = Cache::DOF::farRange;
	}
	void State::Camera::set()
	{
		RE::PlayerCamera::GetSingleton()->worldFOV = fov;
		Cache::translateSpeedValue = translateSpeed;

		Cache::DOF::blurMultiplier = blurMultiplier;
		Cache::DOF::nearDist = nearDist;
		Cache::DOF::nearRange = nearRange;
		Cache::DOF::farDist = farDist;
		Cache::DOF::farRange = farRange;

		viewRoll = 0.0f;
	}

	void State::Time::get()
	{
		freezeTime = RE::Main::GetSingleton()->freezeTime;
		globalTimeMult = RE::BSTimer::GetCurrentGlobalTimeMult();

		const auto calendar = RE::Calendar::GetSingleton();
		timescale = calendar->GetTimescale();
		gameHour = calendar->gameHour->value;
	}
	void State::Time::set() const
	{
		RE::Main::GetSingleton()->freezeTime = freezeTime;
		RE::BSTimer::GetCurrentGlobalTimeMult() = globalTimeMult;

		const auto calendar = RE::Calendar::GetSingleton();
		calendar->timeScale->value = timescale;
		// shouldn't revert time?
		// calendar->gameHour->value = gameHour;
	}

	void State::Player::get()
	{
		const auto playerRef = RE::PlayerCharacter::GetSingleton();

		visible = playerRef->Get3D() ? !playerRef->Get3D()->GetAppCulled() : false;

		rotZ = playerRef->GetAngleZ();
		pos = playerRef->GetPosition();
	}
	void State::Player::set() const
	{
		const auto playerRef = RE::PlayerCharacter::GetSingleton();

		//culling is handled by manager

		playerRef->SetRotationZ(rotZ);
		playerRef->SetPosition(pos, true);
		playerRef->UpdateActor3DPosition();
	}

	void State::GetState()
	{
		camera.get();
		time.get();
		player.get();
	}
	void State::SetState()
	{
		camera.set();
		time.set();
		player.set();
	}
}

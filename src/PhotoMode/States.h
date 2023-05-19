#pragma once

namespace PhotoMode
{
	class State
	{
	public:
		struct Camera
		{
			void get();
			void set();

			// members
			float fov{};
			float viewRoll{ 0.0f };
			float translateSpeed{};

			bool showGrid{ false };
		};
		struct Time
		{
			void get();
			void set() const;

			// members
		    bool  freezeTime{ false };
			float globalTimeMult{ 1.0f };
			float timescale{};
		};
		struct Player
		{
			void get();
			void set() const;

			// members
		    bool         visible{ true };
			float        rotZ{};
			RE::NiPoint3 pos{};
		};

		Camera camera;
		Time   time;
		Player player;

		void GetState();
		void SetState();
	};
}

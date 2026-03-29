#pragma once

// Pixelsurf calculator ported from lobo v2 src
// Predicts jump height/velocity based on gravity and tick intervals
// Supports both ducked and standing states

namespace pixelsurf_calculator {
	// Gravity constant: sv_gravity(800) / (tickrate(64) * tickrate(64))
	// Equivalent to (sv_gravity / sq_ticks) for 64-tick servers
	static constexpr float GRAVITY_FACTOR = 0.1953125f;

	// Duck height offset in units
	static constexpr float DUCK_HEIGHT_OFFSET = 9.f;

	// Long jump height offset in units
	static constexpr float LONGJUMP_HEIGHT_OFFSET = 8.9999704f;

	// Gravity acceleration over z ticks: sum of 1..z = z*(z+1)/2
	inline float gravity_acceleration(float z) {
		return z * (z + 1.f) / 2.f;
	}

	// Predicted height after z gravity steps, starting from iCalculate
	// Formula from lobo v2 src: p = iCalculate + (0.1953125 * z)
	inline float predict_velocity(float iCalculate, float z) {
		return iCalculate + (GRAVITY_FACTOR * z);
	}

	// Check whether the player's predicted position can land on the target surface
	// playerZ   - current player Z position at apex of trajectory
	// targetZ   - target surface Z position
	// tolerance - acceptable height difference (default 0.03125 = 1/32 unit)
	inline bool can_reach_surface(float playerZ, float targetZ, float tolerance = 0.03125f) {
		return (playerZ >= targetZ - tolerance) && (playerZ <= targetZ + tolerance);
	}

	// Predict height for standing jump
	inline bool check_standing(float playerZ, float targetZ) {
		return can_reach_surface(playerZ, targetZ);
	}

	// Predict height for ducked jump (player is 9 units taller when not ducking)
	inline bool check_ducked(float playerZ, float targetZ) {
		return can_reach_surface(playerZ + DUCK_HEIGHT_OFFSET, targetZ);
	}

	// Run the height prediction loop for a given starting iCalculate value,
	// testing whether any tick within max_ticks steps hits the surface.
	// iCalculate is the per-tick height delta at the start of the jump
	// (negative when ascending, becomes positive once the player starts descending).
	// Returns the tick count at which the surface is reached, or -1 if not reachable.
	// is_ducked - whether the player is ducking (adds DUCK_HEIGHT_OFFSET to playerZ)
	inline int predict_pixelsurf_reach(float iCalculate, float startZ, float targetZ, int max_ticks = 512, bool is_ducked = false) {
		float z = 0.f;
		float current_z = startZ;
		for (int k = 0; k < max_ticks; k++) {
			// p > 0 means player is descending (gravity has overcome the upward momentum)
			float p = predict_velocity(iCalculate, z);
			if (p > 0.f) {
				float check_z = is_ducked ? (current_z + DUCK_HEIGHT_OFFSET) : current_z;
				if (can_reach_surface(check_z, targetZ))
					return k;
			}
			current_z -= p;
			z += 1.f;
		}
		return -1;
	}

	// Determine optimal duck state (true = should duck) for reaching the target surface
	// Returns true if ducking improves the chance of reaching the surface
	inline bool get_optimal_duck_state(float iCalculate, float startZ, float targetZ, int max_ticks = 512) {
		int standing_tick = predict_pixelsurf_reach(iCalculate, startZ, targetZ, max_ticks, false);
		int ducked_tick   = predict_pixelsurf_reach(iCalculate, startZ, targetZ, max_ticks, true);

		if (standing_tick == -1 && ducked_tick == -1)
			return false;
		if (standing_tick == -1)
			return true;
		if (ducked_tick == -1)
			return false;
		// Prefer ducking if it reaches the surface sooner or at the same tick
		return ducked_tick <= standing_tick;
	}
} // namespace pixelsurf_calculator

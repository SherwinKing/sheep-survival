#include "PPU466.hpp"
#include "Mode.hpp"
#include "load_save_png.hpp"

#include <glm/glm.hpp>

#include <vector>
#include <deque>
#include <fstream>
#include <sstream>
#include <unordered_map>

struct PlayMode : Mode {
	PlayMode();
	virtual ~PlayMode();

	virtual void load();
	virtual void init();

	//functions called by main loop:
	virtual bool handle_event(SDL_Event const &, glm::uvec2 const &window_size) override;
	virtual void update(float elapsed) override;
	virtual void draw(glm::uvec2 const &drawable_size) override;

	//----- asset related ----
	std::unordered_map<std::string, int> tile_name_to_tile_id_map;
	std::unordered_map<std::string, int> tile_name_to_palette_id_map;

	//----- sprite related ----
	const u_int8_t out_of_game_bit_mask = 1 << 3;
	const u_int8_t is_grass_bit_mask = 1 << 4;
	const u_int8_t is_wolf_bit_mask = 1 << 5;

	//----- game state -----

	//input tracking:
	struct Button {
		uint8_t downs = 0;
		uint8_t pressed = 0;
	} left, right, down, up;

	//some weird background animation:
	float background_fade = 0.0f;

	//player position:
	glm::vec2 player_at = glm::vec2(0.0f);

	//number of grass in map
	int num_grass = 0;

	//----- drawing handled by PPU466 -----

	PPU466 ppu;
};

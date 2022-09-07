#include "PlayMode.hpp"

//for the GL_ERRORS() macro:
#include "gl_errors.hpp"

//for glm::value_ptr() :
#include <glm/gtc/type_ptr.hpp>

#include <random>

#include "data_path.hpp"

void PlayMode::load() {
	//TODO: do not hard code
	static const std::string asset_metadata_path = data_path("assets/metadata.txt");

	// ref: https://stackoverflow.com/questions/3910326/c-read-file-line-by-line-then-split-each-line-using-the-delimiter
	// open file in text mode
	std::ifstream asset_metadata_file(asset_metadata_path);
	std::string tile_info;

	// skip the first line of the metadata file
	// std::getline(asset_metadata_file, nullptr);
	asset_metadata_file.ignore(LONG_MAX, '\n');

	// load the tiles listed in the metadata file
	while (std::getline(asset_metadata_file, tile_info)) {
		std::stringstream tile_info_stream(tile_info);
		int tile_id;
		std::string tile_name;
		std::string tile_path;

		tile_info_stream >> tile_id >> tile_name >> tile_path;

		// store the tile name to tile id and palette id relationship
		// currently the tile id and the palette id are the same
		int palette_id = tile_id;
		tile_name_to_tile_id_map[tile_name] = tile_id;
		tile_name_to_palette_id_map[tile_name] = palette_id;


		glm::uvec2 size;
		std::vector< glm::u8vec4 > data;
		load_png(data_path(tile_path), &size, &data, UpperLeftOrigin);

		// check if size is 8 x 8
		if (size.x != 8 || size.y != 8) {
			throw std::runtime_error("The tile is not 8x8 pixels!");
		}

		auto obtain_color_index_in_palette = [&palette_table = ppu.palette_table](u_int8_t palette_id, glm::u8vec4 color, int32_t &color_id){
			for (uint32_t i=0; i < palette_table[palette_id].size(); i++) {
				const glm::u8vec4 & palette_color = palette_table[palette_id][i];
				if (palette_color[0] == color[0] &&
					palette_color[1] == color[1] &&
					palette_color[2] == color[2] &&
					palette_color[3] == color[3]) {
						color_id = i;
						return;
					}
			}
			color_id = -1;
		};

		// extract palette and color index in palette
		uint32_t next_color_id_to_set = 0;
		for (uint32_t y = 0; y < 8; y++) {
			uint8_t bit0_row = 0;
			uint8_t bit1_row = 0;
			for (uint32_t x = 0; x < 8; x++) {
				bit0_row <<= 1;
				bit1_row <<= 1;

				int32_t color_id;
				glm::u8vec4 & color = data[8*y+x];
				obtain_color_index_in_palette(palette_id, color, color_id);

				// no color matched
				if (color_id == -1) {
					if (next_color_id_to_set == 4)	// if palette full
						throw std::runtime_error("The tile has more than four colors!");

					// int palette_id = tile_id;
					ppu.palette_table[palette_id][next_color_id_to_set] = color;
					color_id = next_color_id_to_set;
					next_color_id_to_set++;
				}
				
				bit0_row |= color_id & 1;
				bit1_row |= color_id >> 1;
			}
			ppu.tile_table[tile_id].bit0[y] = bit0_row;
			ppu.tile_table[tile_id].bit1[y] = bit1_row;
		}
	}
}
PlayMode::PlayMode() {
	//TODO:
	// you *must* use an asset pipeline of some sort to generate tiles.
	// don't hardcode them like this!
	// or, at least, if you do hardcode them like this,
	//  make yourself a script that spits out the code that you paste in here
	//   and check that script into your repository.

	//Also, *don't* use these tiles in your game:

	{ //use tiles 0-16 as some weird dot pattern thing:
		std::array< uint8_t, 8*8 > distance;
		for (uint32_t y = 0; y < 8; ++y) {
			for (uint32_t x = 0; x < 8; ++x) {
				float d = glm::length(glm::vec2((x + 0.5f) - 4.0f, (y + 0.5f) - 4.0f));
				d /= glm::length(glm::vec2(4.0f, 4.0f));
				distance[x+8*y] = uint8_t(std::max(0,std::min(255,int32_t( 255.0f * d ))));
			}
		}
		for (uint32_t index = 0; index < 16; ++index) {
			PPU466::Tile tile;
			uint8_t t = uint8_t((255 * index) / 16);
			for (uint32_t y = 0; y < 8; ++y) {
				uint8_t bit0 = 0;
				uint8_t bit1 = 0;
				for (uint32_t x = 0; x < 8; ++x) {
					uint8_t d = distance[x+8*y];
					if (d > t) {
						bit0 |= (1 << x);
					} else {
						bit1 |= (1 << x);
					}
				}
				tile.bit0[y] = bit0;
				tile.bit1[y] = bit1;
			}
			ppu.tile_table[index] = tile;
		}
	}

	//use sprite 32 as a "player":
	ppu.tile_table[32].bit0 = {
		0b01111110,
		0b11111111,
		0b11111111,
		0b11111111,
		0b11111111,
		0b11111111,
		0b11111111,
		0b01111110,
	};
	ppu.tile_table[32].bit1 = {
		0b00000000,
		0b00000000,
		0b00011000,
		0b00100100,
		0b00000000,
		0b00100100,
		0b00000000,
		0b00000000,
	};

	//makes the outside of tiles 0-16 solid:
	ppu.palette_table[0] = {
		glm::u8vec4(0x00, 0x00, 0x00, 0x00),
		glm::u8vec4(0x00, 0x00, 0x00, 0xff),
		glm::u8vec4(0x00, 0x00, 0x00, 0x00),
		glm::u8vec4(0x00, 0x00, 0x00, 0xff),
	};

	//makes the center of tiles 0-16 solid:
	ppu.palette_table[1] = {
		glm::u8vec4(0x00, 0x00, 0x00, 0x00),
		glm::u8vec4(0x00, 0x00, 0x00, 0x00),
		glm::u8vec4(0x00, 0x00, 0x00, 0xff),
		glm::u8vec4(0x00, 0x00, 0x00, 0xff),
	};

	//used for the player:
	ppu.palette_table[7] = {
		glm::u8vec4(0x00, 0x00, 0x00, 0x00),
		glm::u8vec4(0xff, 0xff, 0x00, 0xff),
		glm::u8vec4(0x00, 0x00, 0x00, 0xff),
		glm::u8vec4(0x00, 0x00, 0x00, 0xff),
	};

	//used for the misc other sprites:
	ppu.palette_table[6] = {
		glm::u8vec4(0x00, 0x00, 0x00, 0x00),
		glm::u8vec4(0x88, 0x88, 0xff, 0xff),
		glm::u8vec4(0x00, 0x00, 0x00, 0xff),
		glm::u8vec4(0x00, 0x00, 0x00, 0x00),
	};

}

PlayMode::~PlayMode() {
}

bool PlayMode::handle_event(SDL_Event const &evt, glm::uvec2 const &window_size) {

	if (evt.type == SDL_KEYDOWN) {
		if (evt.key.keysym.sym == SDLK_LEFT) {
			left.downs += 1;
			left.pressed = true;
			return true;
		} else if (evt.key.keysym.sym == SDLK_RIGHT) {
			right.downs += 1;
			right.pressed = true;
			return true;
		} else if (evt.key.keysym.sym == SDLK_UP) {
			up.downs += 1;
			up.pressed = true;
			return true;
		} else if (evt.key.keysym.sym == SDLK_DOWN) {
			down.downs += 1;
			down.pressed = true;
			return true;
		}
	} else if (evt.type == SDL_KEYUP) {
		if (evt.key.keysym.sym == SDLK_LEFT) {
			left.pressed = false;
			return true;
		} else if (evt.key.keysym.sym == SDLK_RIGHT) {
			right.pressed = false;
			return true;
		} else if (evt.key.keysym.sym == SDLK_UP) {
			up.pressed = false;
			return true;
		} else if (evt.key.keysym.sym == SDLK_DOWN) {
			down.pressed = false;
			return true;
		}
	}

	return false;
}

void PlayMode::update(float elapsed) {

	//slowly rotates through [0,1):
	// (will be used to set background color)
	background_fade += elapsed / 10.0f;
	background_fade -= std::floor(background_fade);

	constexpr float PlayerSpeed = 30.0f;
	if (left.pressed) player_at.x -= PlayerSpeed * elapsed;
	if (right.pressed) player_at.x += PlayerSpeed * elapsed;
	if (down.pressed) player_at.y -= PlayerSpeed * elapsed;
	if (up.pressed) player_at.y += PlayerSpeed * elapsed;

	//reset button press counters:
	left.downs = 0;
	right.downs = 0;
	up.downs = 0;
	down.downs = 0;
}

void PlayMode::draw(glm::uvec2 const &drawable_size) {
	//--- set ppu state based on game state ---

	//background color will be some hsv-like fade:
	ppu.background_color = glm::u8vec4(
		std::min(255,std::max(0,int32_t(255 * 0.5f * (0.5f + std::sin( 2.0f * M_PI * (background_fade + 0.0f / 3.0f) ) ) ))),
		std::min(255,std::max(0,int32_t(255 * 0.5f * (0.5f + std::sin( 2.0f * M_PI * (background_fade + 1.0f / 3.0f) ) ) ))),
		std::min(255,std::max(0,int32_t(255 * 0.5f * (0.5f + std::sin( 2.0f * M_PI * (background_fade + 2.0f / 3.0f) ) ) ))),
		0xff
	);

	//tilemap gets recomputed every frame as some weird plasma thing:
	//NOTE: don't do this in your game! actually make a map or something :-)
	for (uint32_t y = 0; y < PPU466::BackgroundHeight; ++y) {
		for (uint32_t x = 0; x < PPU466::BackgroundWidth; ++x) {
			//TODO: make weird plasma thing
			ppu.background[x+PPU466::BackgroundWidth*y] = ((x+y)%16);
		}
	}

	//background scroll:
	ppu.background_position.x = int32_t(-0.5f * player_at.x);
	ppu.background_position.y = int32_t(-0.5f * player_at.y);

	//player sprite:
	ppu.sprites[0].x = int8_t(player_at.x);
	ppu.sprites[0].y = int8_t(player_at.y);
	// ppu.sprites[0].index = 32;
	ppu.sprites[0].index = tile_name_to_tile_id_map["wolf"];
	// ppu.sprites[0].attributes = 7;
	ppu.sprites[0].attributes = tile_name_to_palette_id_map["wolf"];

	//some other misc sprites:
	for (uint32_t i = 1; i < 63; ++i) {
		float amt = (i + 2.0f * background_fade) / 62.0f;
		ppu.sprites[i].x = int8_t(0.5f * PPU466::ScreenWidth + std::cos( 2.0f * M_PI * amt * 5.0f + 0.01f * player_at.x) * 0.4f * PPU466::ScreenWidth);
		ppu.sprites[i].y = int8_t(0.5f * PPU466::ScreenHeight + std::sin( 2.0f * M_PI * amt * 3.0f + 0.01f * player_at.y) * 0.4f * PPU466::ScreenWidth);
		// ppu.sprites[i].index = 32;
		ppu.sprites[i].index = tile_name_to_tile_id_map["sheep"];
		// ppu.sprites[i].attributes = 6;
		ppu.sprites[i].attributes = tile_name_to_palette_id_map["sheep"];
		if (i % 2) ppu.sprites[i].attributes |= 0x80; //'behind' bit
	}

	//--- actually draw ---
	ppu.draw(drawable_size);
}

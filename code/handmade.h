#if !defined(HANDMADE_H)

// NOTE Services that the platform layer provides to the game

// NOTE Services that the game provides to the platform layer

// FOUR THIGNS - timing, controller/keyboard input, bitmap buffer to use, sound buffer to use.


// TODO In the future, rendoring specifically will become a three-tiered abstraction.


// A platform indenpendent version of our graphics buffer
struct GameOffscreenBuffer {
	// The actual pointer to memory where our DIB will be written to.	
	// Note Pixels are always 32 bits wide, memory order BB GG RR XX
	void* memory;

	int pitch;
	int height;
	int width;
};

private_function void game_update_and_render(GameOffscreenBuffer* buffer, int blue_offset, int green_offset);


#define HANDMADE_H
#endif
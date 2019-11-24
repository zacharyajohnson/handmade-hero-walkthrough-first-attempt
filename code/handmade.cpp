#include "handmade.h"

// Sets the buffer values based on offsets.
private_function void render_weird_gradient(GameOffscreenBuffer* buffer, int blue_offset, int green_offset) {

	// Cast our void pointer to an unsigned 8 bit integer pointer
	// This in effect moves our pointer 8 bits when we do arthmetic.
	// Which lines up well to when we want to get the next row via the pitch calculation
	// TODO COME BACK TO THIS DEFINITION
	uint8* row = (uint8*)buffer->memory;

	// For loop to set individual pixel colors to see if it works
	for (int y = 0; y < buffer->height; ++y) {

		uint32* pixel = (uint32*)row;

		for (int x = 0; x < buffer->width; ++x) {

			// LITTLE ENDIAN ARCHITECTURE
			// lower bytes that make up large data appears first. So 
			// 00000000 10000000
			// 10000000 will appear first 
			// SO to get a red value, we set the second to last byte since its essentially flipped.

			// Some windows history here. Since windows is on a LITTLE ENDIAN architecture,
			// they decided to flip the bit values in a bitmap to BBGGRRxx so if would naturally flip in the proccesor to RRGGBBxx 

			uint8 blue = (x + blue_offset);
			uint8 green = (y + green_offset);

			// Memory:  BB GG RR xx
			// Register: xx RR GG BB
			*pixel = (green << 8 | blue);

			++pixel;


		}
		row += buffer->pitch;
	}

}

private_function void game_update_and_render(GameOffscreenBuffer* buffer, int blue_offset, int green_offset) {
	render_weird_gradient(buffer, blue_offset, green_offset);
}


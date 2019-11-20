#include <windows.h>
#include <xinput.h>
#include <dsound.h>
#include <stdint.h>


// Casey does something intresting and uses #define to define static to the three different things static can mean to make it clearer to himself.
// and to hunt down instances of things he might want to rethink.
 
// I actually quite like it.

// Static in C means three different things.
// If a function is declared static, it will be only visible in the file it was declared from
// If a variable is part of a function or block, it will store a single instance of that variable in the scope and reuse it

// If a variable is not part of a function or block (Top level), it will make that variable private to the file. 
// Since the variable is declared at the top level of the file, it is essentially a global variable that can
// only be seen in the file it is declared if given the static keyword. A private(In Java or C++ term) global variable for that file. 

// NEVER USE A GLOBAL VARIABLE WITHOUT STATIC. God knows how messy code can get if a source file can depend on another files public global variables.
#define private_function static 
#define local_persist static 
#define global_variable static

// Getting rid of this god awful hungarian notation
typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;

typedef int8_t int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;
typedef int32 bool32;





// DIB - Stands for device independent bitmap. 
// In windows terminology anway.

// Struct that contains our memory / info about a graphics buffer that we can use to
// write data to and display on the window 
// Pixels are always 32 bits wide
struct Win32_OffscreenBuffer {
	// Struct that contains the info used to create our bitmap
	BITMAPINFO info;
  
	// The actual pointer to memory where our DIB will be written to.	
	void *memory;
	
	// See below comments on pitch
	int pitch;
	int height;
	int width;
};

struct win32_WindowDimension {
	int width;
	int height;
};

// global variable that controls if main message/ window loop runs
// Interestnly enough, in C if you use static for a global variable, it initializes it automatically to zero - just a need tibit I found interesting.
// set to true when a window handle is successfully created and to false when you either close or destroy the window 
global_variable bool global_running;

// Our graphics buffer for our window
global_variable Win32_OffscreenBuffer global_back_buffer;

// Our sound buffer for playing sounds
global_variable LPDIRECTSOUNDBUFFER global_secondary_sound_buffer;

// Define macros for use below

// Since xinput is not supported in certain machines, we don't want to load the dll file into our program our it will crash(with no warnings) on certain OS versions
// So we typedef it so we can dynamically get a function point to whatever will work on that system
// This way at least we will know at compile time something is wrong if it can't find this signature
#define X_INPUT_GET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_STATE *pState)
typedef X_INPUT_GET_STATE(x_input_get_state);

X_INPUT_GET_STATE(XInputGetStateStub) {
	return(ERROR_DEVICE_NOT_CONNECTED);
}

// Global function pointers to the typedefs above.
global_variable x_input_get_state* XInputGetState_ = XInputGetStateStub;

#define X_INPUT_SET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_VIBRATION* pVibration)
typedef X_INPUT_SET_STATE(x_input_set_state);

X_INPUT_SET_STATE(XInputSetStateStub) {
	return (ERROR_DEVICE_NOT_CONNECTED);
}

global_variable x_input_set_state* XInputSetState_ = XInputSetStateStub;


#define DIRECT_SOUND_CREATE(name) HRESULT WINAPI name(LPCGUID pcGuidDevice, LPDIRECTSOUND *ppDS, LPUNKNOWN pUnkOuter ); 
typedef DIRECT_SOUND_CREATE(direct_sound_create);


// Extra safety so someone doesn't accidently call the windows function directly instead of using our function pointer
#define XInputGetState XInputGetState_
#define XInputSetState XInputSetState_

// Load our library on our own. Note: We are using version 1.3 because it is supported on a lot more machines then 1.4.
// If we can't find it, it returns null and we know the machine does not have the proper dlls installed.
private_function void win32_load_x_input(void) {
	
	HMODULE x_input_library = LoadLibraryA("xinput1_4.dll");

	// NOTE. Windows 8+ Have xinput 1.4. If we cant find it, default to xinput 1.3, which older machines have.
	if (x_input_library == 0) {
		//TODO Test this on windows 8
		x_input_library = LoadLibraryA("xinput1_3.dll");
	}
	
	if (x_input_library != 0) {
		XInputGetState = (x_input_get_state*)GetProcAddress(x_input_library, "XInputGetState");
		XInputSetState = (x_input_set_state*)GetProcAddress(x_input_library, "XInputSetState");
	}

}


// Sets the buffer values based on offsets.
private_function void render_weird_gradient(Win32_OffscreenBuffer *buffer, int blue_offset, int green_offset) {

	// Cast our void pointer to an unsigned 8 bit integer pointer
	// This in effect moves our pointer 8 bits when we do arthmetic.
	// Which lines up well to when we want to get the next row via the pitch calculation
	// TODO COME BACK TO THIS DEFINITION
	uint8 *row = (uint8*)buffer->memory;

	// For loop to set individual pixel colors to see if it works
	for (int y = 0; y < buffer->height; ++y) {

		uint32 *pixel = (uint32*)row;

		for (int x = 0; x < buffer->width; ++x) {

			// LITTLE ENDIAN ARCHITECTURE
			// lower bytes that make up large data appears first. So 
			// 00000000 10000000
			// 10000000 will appear first 
			// SO to get a red value, we set the second to last byte since its essentially flipped.

			// Some windows history here. Since windows is on a LITTLE ENDIAN architecture,
			// they decided to flip the bit values in a bitmap to BBGGRRxx so if would naturally flip in the proccesor to RRGGBBxx 

			uint8 blue  = (x + blue_offset);
			uint8 green = (y + green_offset);

			// Memory:  BB GG RR xx
			// Register: xx RR GG BB
			*pixel = (green << 8 | blue);

			++pixel;


		}
		row += buffer->pitch;
	}

}

private_function void win32_init_d_sound(HWND window, int32 samples_per_second, int32 buffer_size) {
	// TODO Load the direct sound library itself(dll file)
	HMODULE d_sound_library = LoadLibraryA("dsound.dll");

	if (d_sound_library != 0) {
		direct_sound_create  *DirectSoundCreate = (direct_sound_create *) GetProcAddress(d_sound_library, "DirectSoundCreate");

		// TODO Double check to see if this works on XP - DirectSound 7 or 8?
		LPDIRECTSOUND direct_sound;

		// If method fetch is correct and we get a pointer to our sound handle
		if (DirectSoundCreate != 0 && SUCCEEDED(DirectSoundCreate(0, &direct_sound, 0))) {
			
			// Format struct for our buffers
			WAVEFORMATEX wave_format = {};
			wave_format.wFormatTag = WAVE_FORMAT_PCM;
			// Stereo
			wave_format.nChannels = 2;
			wave_format.nSamplesPerSec = samples_per_second;
			wave_format.wBitsPerSample = 16;
			wave_format.nBlockAlign = (wave_format.nChannels * wave_format.wBitsPerSample) / 8;
			wave_format.nAvgBytesPerSec = wave_format.nSamplesPerSec * wave_format.nBlockAlign;
			wave_format.cbSize = 0;

			// If we can set the priority level(Basically how resources are handled)
			if (SUCCEEDED(direct_sound->SetCooperativeLevel(window, DSSCL_PRIORITY))) {
				
				// Sets up sound buffer and description struct for the primary buffer
				DSBUFFERDESC buffer_description = {};
				buffer_description.dwSize = sizeof(buffer_description);
				buffer_description.dwFlags = DSBCAPS_PRIMARYBUFFER;
		
				// TODO DSBCAPS_GLOBALFOCUS?
				// "Create" a primary sound buffer - this is due to legacy reasons. Prevents upsampling and down sampling by forcing it to be in a specific format
				// Gets a "handle" to the sound card itself and set the format of the sound card to ensure it plays in what format we want it to be.
				// DO NOT THINK OF THIS AS AN ACTUAL BUFFER
				LPDIRECTSOUNDBUFFER primary_sound_buffer;

				// If creating the sound buffer itself is successful
				if (SUCCEEDED(direct_sound->CreateSoundBuffer(&buffer_description, &primary_sound_buffer, 0))) {
					
					// IF Formatting the priamry buffer succeeded
					if (SUCCEEDED(primary_sound_buffer->SetFormat(&wave_format))) {
						OutputDebugStringA("Primary buffer format was set");
					}else {
						// TODO DIAGNOTIC
					}


				}

			} else {
				
			}

			// Sets up sound buffer and description struct for our secondary buffer
			DSBUFFERDESC buffer_description = {};
			buffer_description.dwSize = sizeof(buffer_description);
			buffer_description.dwFlags = 0;
			buffer_description.dwBufferBytes = buffer_size;
			buffer_description.lpwfxFormat = &wave_format;

			
			// If creating the sound buffer itself is successful
			if (SUCCEEDED(direct_sound->CreateSoundBuffer(&buffer_description, &global_secondary_sound_buffer, 0))) {
				OutputDebugStringA("Secondary sound buffer created successfully");
			}

		} else {
			//TODO  Diagnostic
		}
	} else {
		//TODO Diagnostic
	}
	

}

private_function win32_WindowDimension get_window_dimension(HWND window) {
	RECT client_rect;
			
	// First, we call GetClientRect to get the size of our client window
	// The client window is defined as the part of the screen that applications appear in. Not the part where there is the border, x out button, etc.
	// Since this is what we are drawing to we want this.
	GetClientRect(window, &client_rect); 
	
	win32_WindowDimension window_dimension;
	// Get the height and width of our rectangle

	// width - right(x-coordiante of lower-right corner) - left(x coordinate of upper-left corner)
	// height = bottom(y-cooridante of lower-right corner) - top(y coordinate of upper-left corner)
	window_dimension.width = client_rect.right - client_rect.left;
	window_dimension.height = client_rect.bottom - client_rect.top;

	return window_dimension;
}


private_function void win32_resize_dib_section(Win32_OffscreenBuffer *buffer, int width, int height) {

	// If our bitmap_memory has been allocated, we free our memory first so we don't have leaks.

	if(buffer->memory != NULL) {
		// Opposite of VirtualRelease
		VirtualFree(buffer->memory, 0, MEM_RELEASE);
	}
	
	// Save the bitmap width/height values to a global variable for further updates in win32_update_window function
	// Since, when we call that function, we are resizing our bitmap, we need its dimensions so we can properly
	// translate from the source(the window and its dimensions) and the destination(bitmap)
	buffer->width = width;
	buffer->height = height;
	
	int bytes_per_pixel = 4;

	// Set values for the BITMAPINFOHEADER struct - which contains info about the dimensions and color format of a DIB.
	buffer->info.bmiHeader.biSize = sizeof(buffer->info.bmiHeader);
	buffer->info.bmiHeader.biWidth = buffer->width;

	// THIS IS MADNESS
	// So, this may seem insane at glance but there is a reason for it.
	// The sign of the bitmap height determines if the bitmap is a bottom-up DIB(POSITIVE) or a top-down DIB(NEGATIVE)
	// This simply means where the origin of the coordiates for the bitmap starts and what it represents at the start of your bitmap memory address.
	// If bottom-up, the origin will start in the lower left corner(Think typically graphs in math) and the start of our memory address will represent that origin in that corner
	// If top-down, the origin will start in the upper left corner(The way people in CS typically think of windows origins) and the start of our memory address will represent that origin in that corner
	
	// This also impacts how we will get info from our bitmap and how we want to traverse our memory address for info via our stride offset.
	// Since memory is 1D and we are trying to map a 2D image to it, we need a way to traverse and map back to 2D, which is our stride offset.
	// NOTE: For more confusion, stride is also called pitch so don't get confused if you see them somewhere is the code.

	// A stride offset is what it takes for us to get from a chunk of memory that represents a row in a 2D image, to get to next row in memory.
	// And the reason for this has to do with two things: bits per pixel and pixel depth.
	// For example typical RGB images pixels are stored typically in 32 bits per pixel since that is natural for the processor to manage.
	// However, RBG has a pixel depth of 24 bits, the actual amount needed to store pixel data, 8bits each for red, green and blue.
	// THIS MEANS, in memory, if you have your first pixel, we only want the first 24 bits in memory for our pixel data. The last 8 bits is padding because we want performant 
	// operations on the CPU so we used a 32 bit storage unit. And every pixel has that problem.
	// So to get to the next row, we need an offset because the actual data and the container we are storing it in doesn't match up. That is stride.  


	// IF this makes no sense check this out too for more info on stride https://www.collabora.com/news-and-blog/blog/2016/02/16/a-programmers-view-on-digital-images-the-essentials/
	// See https://docs.microsoft.com/en-us/windows/win32/api/wingdi/ns-wingdi-bitmapinfoheader if you need some more info on all this nonsense or stride
 
	// CASEY prefers top-down and so do I since that is how I was taught to think of it so besides that and the paragraph I wrote above on why this is important, we are going with negative.
	// JUST understand the implications of this.
	buffer->info.bmiHeader.biHeight= -buffer->height;
	buffer->info.bmiHeader.biPlanes = 1;
	
	// Sets the number of bits per pixel we get. Usually for RGB you need 24 but Casey says we need 32 for later so that will be fun TODO
	// EPISODE 5 - Casey brought up unaligned byte accessing / alignment, basically when accessing increments of bytes that doesn't align up naturally with what the processor typically likes to handle,
	// YOU HAVE PERFORMANCE PENALITES
	// SEE the linux kernal docs for more info
	// https://www.kernel.org/doc/Documentation/unaligned-memory-access.txt
	// UPDATE - biBitCount is a DWORD(32) bits. SO we make it 32 bits so its natural to get the next value of the pixel instead of having to jump through shit if we had
	// something like 24 bits. The processer would have to jump extra bits to get to the next value in memory
	buffer->info.bmiHeader.biBitCount = 32;

	// uncompressed bitmap format.
	buffer->info.bmiHeader.biCompression = BI_RGB;
	// Used to specify the size of the image so compressed formats know what they are uncompressing to. For us it doesn't matter since BI_RGB is uncompressed. 
	buffer->info.bmiHeader.biSizeImage = 0;

	// TODO Some Intereesting happened with this stream.(Episode 5)
	// Originally we were using CreateDIBSection and having it return a Bitmap handle to us
	// However, Chris Hecker(Smart dude), and ssylvan pointed out we have our memory address to our bitmap. SO WE DON'T NEED A HANDLE TO IT
	// SEE timestamp 0:30 on episode 5 for the explanation
	
	int bitmap_memory_size = buffer->width * buffer->height * bytes_per_pixel;
 	
	// We use virtual alloc to commit memory for our use in the virtual memory address space for our buffer
	// Look up virtual paging system for more info if you don't know why we would do this. 
	// NOTE. We do MEM_RESERVE AND MEM_COMMIT(Which does both), we explicilty tell it to reserve our memory
	buffer->memory = VirtualAlloc(NULL, bitmap_memory_size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE); 

	
	buffer->pitch = buffer->width * bytes_per_pixel;
}

private_function void win32_display_buffer_in_window(HDC device_context, Win32_OffscreenBuffer *buffer, int window_width, int window_height) {

	// This function copies color data for a rectancgle of pixels(DIB, JPEG, PNG only) to a specified destination rectangle(Like a window, hence its use here)
	// Takes the following arguments

	// HDC hdc - A handle to a device context
	// int xDest - The x coordiate of the upper-left corner of the destination rectangle
	// int yDest - The y coordinate of the upper-left corner of the destination rectangle
	// int DestWidth - The width of the destination rectangle
	// int DestHeight - The height of the destination rectangle
	// int xSrc - The x coordiante(pixels) of the source rectangle in the image
	// int ySrc - The y coordinate(pixels) of the source rectangle in the image
	// int SrcWidth - The width(pixels) of the source rectangle in the image
	// int SrcHeight - The width(pixels) of the source rectangle in the image
	// CONST VOID *lpBits - A pointer to the image bits, which are stored as an array of bytes
	// const BITMAPINFO *lpbmi - A pointer to a BITMAPINFO structure that contains information about the DIB (Note, this is the src rectangle specified in the long as comment above lol)
	// UINT iUsage - Specifies how the BITMAPINFO struct stores/looks up color values, DIB_RGB_COLORS is almost always used.
	// DWORD rop - A code that defines how the color date for the source rectangle is combined with the color data for the destination rectangle to achieve final color.
	StretchDIBits(device_context,
			/* 
			x, y, width, height,
			x, y, width, height,
			*/
			0, 0, window_width, window_height,
		        0, 0, buffer->width, buffer->height,	
			buffer->memory, &(buffer->info),
			DIB_RGB_COLORS, SRCCOPY);
	
}

// This is the callback function windows uses when sending messages to your window. 
LRESULT CALLBACK win32_main_window_callback(
	HWND window,
	UINT message,
	WPARAM wParam,
	LPARAM lParam
) {
	LRESULT result = 0;

	switch(message) {
		case WM_SIZE: {

		} break;
		
		case WM_DESTROY: {
			// TODO Handle this with a message to the user?
			global_running = false;
		} break;
		
		case WM_CLOSE: {
			// TODO Handle this as an error - recreate window?
			global_running = false;
		} break;
		case WM_ACTIVATEAPP: {	
		
		} break;
		case WM_PAINT: {

			PAINTSTRUCT paint;

			// BeginPaint Specifies to windows you want to begin painting on the window.
			// HWND hwnd - The handle to the window you paint to
			// LPPAINTSTRUCT - A pointer to a paint struct to store painting info for an application 
			// Returns a device context that allows the window access to any GUI related functions.
			HDC device_context = BeginPaint(window, &paint);
			
	
			win32_WindowDimension window_dimension = get_window_dimension(window);
	
			win32_display_buffer_in_window(device_context, &global_back_buffer, window_dimension.width, window_dimension.height);
			
			// EndPaint tells windows you want to stop painting and to clean up any resources still open
			EndPaint(window, &paint);
			break;
		}
		
		// Keyboard inpu8t
		case WM_SYSKEYDOWN:
		case WM_SYSKEYUP:
		case WM_KEYDOWN:
		case WM_KEYUP: {
			uint32 key = wParam;

			bool was_down = ((lParam & (1 << 30)) != 0);
			bool is_down = ((lParam & (1 << 31)) == 0);

			// THINK ABOUT WHAT THIS MEANS
			if (was_down != is_down) {
				// In Windows, you can check any keys that have ansii equivalents but checking for the capital letter of the key
				// VK codes are used for things that are not in ansii encodings like the up arrow. Hence the vk code flags
				if (key == 'W') {

				}
				else if (key == 'A') {

				}
				else if (key == 'S') {

				}
				else if (key == 'D') {

				}
				else if (key == VK_UP) {

				}
				else if (key == VK_LEFT) {

				}
				else if (key == VK_DOWN) {

				}
				else if (key == VK_RIGHT) {

				}
				else if (key == VK_SPACE) {

				}
				else if (key == VK_ESCAPE) {

					OutputDebugStringA("ESCAPE ");
					if (is_down) {
						OutputDebugStringA("ISDOWN");
					}
					if (was_down) {
						OutputDebugStringA("WASDOWN");
					}

					OutputDebugStringA("\n");
				}
			
				
			} 
			
			// If you ALT F4, close the window
			bool32 alt_key_was_down = (lParam & (1 << 29));

			if (key == VK_F4 && alt_key_was_down) {
				global_running = false;
			}

		} break;

		default: { 
			result = DefWindowProc(window, message, wParam, lParam);
		}
	}
	return result;
};

int CALLBACK WinMain(
  HINSTANCE instance,
  HINSTANCE prevInstance,
  LPSTR     commandLine,
  int       showCode 
) {

	// Load in our library versions we want to use on our own here
	win32_load_x_input();

	// Allocate our size for our global back buffer
	win32_resize_dib_section(&global_back_buffer, 1280, 720);

	WNDCLASS window_class = {};

	// Flags that tell windows to paint the whole screen when any kind of change happens
	// instead of just painting new sections of the screen as they are drawn.
	window_class.style = CS_HREDRAW | CS_VREDRAW;
	window_class.lpfnWndProc = win32_main_window_callback;
	window_class.hInstance = instance;
	//window_class.hIcon;
	window_class.lpszClassName = "HandmadeHeroWindowClass";

	// RegisterClass returns a 0 if it fails to register your defined window class
	// Casey says that this rarely will happen but its good to put notes her for knowledge sake.
	if(RegisterClassA(&window_class)){
		// CreateWindowEx has a lot of function parameters so I will list their meanings here in the order they are passed in
		//
		//
		// DWORD dwExStyle - extended styles for your window
		// LPCTSTR lpClassName - The class name of the window type you want to create
		// LPCTSTR lpWindowName - The Window Name
		// DWORD dwStyle - The various styles and effects your window can have
		// int x - Initial horizontal position of window (When using CS_USEDEFAULT flag, it will randomly choose the value)
		// int y - Initial vertical position
		// int nWidth - Initial width of window
		// int nHeight - Initial height of window
		// HWND hWndParent - a handle to a parent window if you have sub windows or windows in windows
		// HMENU hMenu - a handle to a menu we might want to use in the window
		// HINSTANCE hInstance - handle to an instance we want to associate the window with. In this case, its our program instance from WinMain
		// LPVOID lpParam - Used to pass parameters into the window if needed.
		HWND window = CreateWindowExA(
			0,
			window_class.lpszClassName,
			"Handmade Hero",
			WS_OVERLAPPEDWINDOW | WS_VISIBLE,
			CW_USEDEFAULT,
			CW_USEDEFAULT,
			CW_USEDEFAULT,
			CW_USEDEFAULT,
			0,
			0,
			instance,
			0);

		// If creating the window handle fails, it returns NULL(or 0 if you prefer). 
		// So we can check to see if its created easily
		if(window) {
			
			// Windows doesn't actually start sending messages to a windows callback function until you pull it off a queue. So we loop through the messages and use them continously

			// The loop also functions as our main loop in general that keeps the process going, and as a result, the window from disappering immediatly as soon as its created.
			// Kinda similar to a game loop

			global_running = true;	
			int blue_offset = 0;
			int green_offset = 0;
	
			// Represents our hertz value. cycles/per second. Defines what kind of sound plays
			// The classic 1kHz example(1000 hz) found on the square wave wiki page
			int tone_hz = 256;
			int16 tone_volume = 3000;
			// Counter that keeps track of where we are in our sound buffer to make a square wave.
			uint32 running_sample_index = 0;
			int samples_per_second = 48000;

			int bytes_per_sample = sizeof(int16) * 2;

			// If we want our values in a second to sound like a certain hertz value,
			// We need to split it up into chunks. So we take our samples per second / hz
			int square_wave_period = samples_per_second / tone_hz;
			int half_square_wave_period = square_wave_period / 2;
			int secondary_sound_buffer_size = samples_per_second * bytes_per_sample;

			// Initalize direct sound library
			// NOTE: This direct sound is fairly old and we are using it to be compatible with older machines like XP
			win32_init_d_sound(window, samples_per_second, samples_per_second * sizeof(int16) * 2);

			bool sound_is_playing = false;

			while(global_running) {

				// Where we will store the message			
				MSG message;
				

				while (PeekMessageA(&message, NULL, 0, 0, PM_REMOVE)) {

					// If windows decides to randomly kill our process, quit
					if (message.message == WM_QUIT) {
						global_running = false;
					}

					TranslateMessage(&message);
					DispatchMessageA(&message);
				}
				// call function to render weird gradient to screen!
				
				//TODO Should we pull this more frequently?
				// Get the status of our controller inputs via XInput.
				// Currently Windows supports up to four xbox controllers but this could change in the future
				for (DWORD controller_index = 0; controller_index < XUSER_MAX_COUNT; controller_index++)
				{
					XINPUT_STATE controller_state;
					ZeroMemory(&controller_state, sizeof(XINPUT_STATE));

					// If the get the state is successful(IE, the controller is plugged in or exists)
					if (XInputGetState(controller_index, &controller_state) == ERROR_SUCCESS)
					{	
						// Controller is plugged in 
						
						// Our gamepad
						XINPUT_GAMEPAD *gamepad = &(controller_state.Gamepad);

						// Get which buttons were pressed on the controller
						bool dpad_up = gamepad->wButtons & XINPUT_GAMEPAD_DPAD_UP;
						bool dpad_down = gamepad->wButtons & XINPUT_GAMEPAD_DPAD_DOWN;
						bool dpad_left = gamepad->wButtons & XINPUT_GAMEPAD_DPAD_LEFT;
						bool gamepad_start = gamepad->wButtons & XINPUT_GAMEPAD_START;
						bool gamepad_end = gamepad->wButtons & XINPUT_GAMEPAD_BACK;
						bool gamepad_left_shoulder = gamepad->wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER;
						bool gamepad_right_shoulder = gamepad->wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER;
						bool gamepad_a = gamepad->wButtons & XINPUT_GAMEPAD_A;
						bool gamepad_b = gamepad->wButtons & XINPUT_GAMEPAD_B;
						bool gamepad_x = gamepad->wButtons & XINPUT_GAMEPAD_X;
						bool gamepad_y = gamepad->wButtons & XINPUT_GAMEPAD_Y;

						// Get which way the left stick was tiltted to
						int16 left_stick_x_position = gamepad->sThumbLX;
						int16 left_stick_y_position = gamepad->sThumbLY;

						blue_offset += left_stick_x_position >> 12;
						green_offset += left_stick_y_position >> 12;

					}
					else
					{
						// Controller is not connected 
					}
				}

				//XINPUT_VIBRATION x_input_vibration;
				//x_input_vibration.wLeftMotorSpeed = 60000;
				//x_input_vibration.wRightMotorSpeed = 60000;
				//XInputSetState(0, &x_input_vibration);

				render_weird_gradient(&global_back_buffer, blue_offset, green_offset);

				// NOTE: DirectSound output test

				// Point in our buffer where whatever hardware is playing our sound is currently looking at.
				// Since we can be stalled by the OS, we can't write directly into the play_curser location or it will play garbadgeS
				DWORD play_cursor;

				// Cursor that tells us where we can start playing
				DWORD write_cursor;
				// If we can get the current position in our sound buffer
				if (SUCCEEDED(global_secondary_sound_buffer->GetCurrentPosition(&play_cursor, &write_cursor))) {
					// Where we will write in our sound buffer.
					DWORD bytes_to_lock = running_sample_index * bytes_per_sample % secondary_sound_buffer_size;

				

					// The amount of bytes to write to the sound buffer
					DWORD bytes_to_write;

			
					// THIS SHOULD NEVER HAPPEN SINCE WE WANT TO ALWAYS WRITE AHEAD OF THE PLAY CURSOR
					if (bytes_to_lock == play_cursor) {
						bytes_to_write = secondary_sound_buffer_size;
					}

					// If where we want to lock our buffer is in front of our play cursor, write to end of buffer
					// Then loop around and start at the beginning and write to the play cursor( Non inclusive)
					// Else, we are before the play cursor and we only have one buffer chunk to work with, from our pointer to the play cursor
					else if (bytes_to_lock > play_cursor) {
						bytes_to_write = secondary_sound_buffer_size - bytes_to_lock;
						bytes_to_write += play_cursor;
					}else {
						bytes_to_write = play_cursor - bytes_to_lock;
					}

					// Readies our buffer for data writing and returns pointers to the beginning of where we can write and the size of our region size
					// NOTE: If you try and reserve n bytes at a location of the buffer and it is bigger then what the 
					// rest of the buffer can hold at that point, it will go to the beginning of the buffer and allocate the rest there. This is why there are two region structs
					// we need to handle.
					// We need to handle both cases.
					VOID* region_1;
					DWORD region_1_size;

					VOID* region_2;
					DWORD region_2_size;

					

					// If we can lock the sound buffer for writing to it.
					if (SUCCEEDED(global_secondary_sound_buffer->Lock(
						bytes_to_lock, bytes_to_write,
						&region_1, &region_1_size,
						&region_2, &region_2_size,
						0
					))) {

						//TODO assert that region1size/region2size is valid 
						int16* sample_out_region_1 = (int16*)region_1;
						int16* sample_out_region_2 = (int16*)region_2;

						// Our sizes are byte values. We want the actual number of samples in our buffer to loop through them
						// So we take size(bytes) / bytes per sample(In this case 32)
						DWORD region_1_sample_count = region_1_size / bytes_per_sample;
						DWORD region_2_sample_count = region_2_size / bytes_per_sample;

						for (DWORD sample_index = 0; sample_index < region_1_sample_count; ++sample_index) {
						
							int16 sample_value = ((running_sample_index++ / half_square_wave_period ) % 2) ? tone_volume : -tone_volume;
							*sample_out_region_1++ = sample_value;
							*sample_out_region_1++ = sample_value;

						}

						for (DWORD sample_index = 0; sample_index < region_2_sample_count; ++sample_index) {
						
							int16 sample_value = ((running_sample_index++ / half_square_wave_period) % 2) ? tone_volume : -tone_volume;
							*sample_out_region_2++ = sample_value;
							*sample_out_region_2++ = sample_value;
						}

						// Unlock the buffer to prevent sound glitches / issues
						global_secondary_sound_buffer->Unlock(region_1, region_1_size, region_2, region_2_size);


					}
				}

				if (!sound_is_playing) {
					global_secondary_sound_buffer->Play(0, 0, DSBPLAY_LOOPING);

					sound_is_playing = true;
				}

				HDC device_context = GetDC(window);
			
				win32_WindowDimension window_dimension = get_window_dimension(window);	
			
			 	win32_display_buffer_in_window(device_context, &global_back_buffer, window_dimension.width, window_dimension.height);
				
				ReleaseDC(window,device_context);
			} 
		}else{
		
		}
	}else {
	
	}

	return 0;
}



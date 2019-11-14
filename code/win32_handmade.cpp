#include <windows.h>
#include <stdio.h>
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
#define  private_function static 
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



// DIB - Stands for device independent bitmap. 
// In windows terminology anway.

// global variable that controls if main message/ window loop runs
// Interestnly enough, in C if you use static for a global variable, it initializes it automatically to zero - just a need tibit I found interesting.
// set to true when a window handle is successfully created and to false when you either close or destroy the window 
global_variable bool running;

// Struct that contains the info used to create our bitmap
global_variable BITMAPINFO bitmap_info;
  
// The actual pointer to memory where our DIB will be written to.	
global_variable	void *bitmap_memory;

global_variable int bitmap_height;
global_variable int bitmap_width;
global_variable int bytes_per_pixel = 4;

// Function that finally renders stuff to the screen. WE DID IT.
private_function void render_weird_gradient(int blue_offset, int green_offset) {

	int width = bitmap_width;
	int height = bitmap_height;

	int pitch = bitmap_width * bytes_per_pixel;
	// Cast our void pointer to an unsigned 8 bit integer pointer
	// This in effect moves our pointer 8 bits when we do arthmetic.
	// Which lines up well to when we want to get the next row via the pitch calculation
	// TODO COME BACK TO THIS DEFINITION
	uint8 *row = (uint8*)bitmap_memory;

	// For loop to set individual pixel colors to see if it works
	for (int y = 0; y < bitmap_height; ++y) {

		uint32 *pixel = (uint32*)row;

		for (int x = 0; x < bitmap_width; ++x) {

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
		row += pitch;
	}

}

private_function void win32_resize_dib_section(int width, int height) {

	// If our bitmap_memory has been allocated, we free our memory first so we don't have leaks.

	if(bitmap_memory != NULL) {
		// Opposite of VirtualRelease
		VirtualFree(bitmap_memory, 0, MEM_RELEASE);
	}
	
	// Save the bitmap width/height values to a global variable for further updates in win32_update_window function
	// Since, when we call that function, we are resizing our bitmap, we need its dimensions so we can properly
	// translate from the source(the window and its dimensions) and the destination(bitmap)
	bitmap_width = width;
	bitmap_height = height;

	// Set values for the BITMAPINFOHEADER struct - which contains info about the dimensions and color format of a DIB.
	bitmap_info.bmiHeader.biSize = sizeof(bitmap_info.bmiHeader);
	bitmap_info.bmiHeader.biWidth = bitmap_width;

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
	bitmap_info.bmiHeader.biHeight= -bitmap_height;
	bitmap_info.bmiHeader.biPlanes = 1;
	
	// Sets the number of bits per pixel we get. Usually for RGB you need 24 but Casey says we need 32 for later so that will be fun TODO
	// EPISODE 5 - Casey brought up unaligned byte accessing / alignment, basically when accessing increments of bytes that doesn't align up naturally with what the processor typically likes to handle,
	// YOU HAVE PERFORMANCE PENALITES
	// SEE the linux kernal docs for more info
	// https://www.kernel.org/doc/Documentation/unaligned-memory-access.txt
	// UPDATE - biBitCount is a DWORD(32) bits. SO we make it 32 bits so its natural to get the next value of the pixel instead of having to jump through shit if we had
	// something like 24 bits. The processer would have to jump extra bits to get to the next value in memory
	bitmap_info.bmiHeader.biBitCount = 32;

	// uncompressed bitmap format.
	bitmap_info.bmiHeader.biCompression = BI_RGB;
	// Used to specify the size of the image so compressed formats know what they are uncompressing to. For us it doesn't matter since BI_RGB is uncompressed. 
	bitmap_info.bmiHeader.biSizeImage = 0;

	// TODO Some Intereesting happened with this stream.(Episode 5)
	// Originally we were using CreateDIBSection and having it return a Bitmap handle to us
	// However, Chris Hecker(Smart dude), and ssylvan pointed out we have our memory address to our bitmap. SO WE DON'T NEED A HANDLE TO IT
	// SEE timestamp 0:30 on episode 5 for the explanation
	
	int bitmap_memory_size = bitmap_width * bitmap_height * bytes_per_pixel;
 	
	// We use virtual alloc to commit memory for our use in the virtual memory address space for our buffer
	// Look up virtual paging system for more info if you don't know why we would do this. 
	bitmap_memory = VirtualAlloc(NULL, bitmap_memory_size, MEM_COMMIT, PAGE_READWRITE); 
}

private_function void win32_update_window(HDC device_context, RECT *client_rect, int x, int y, int width, int height) {

	//Get our current window dimensions from the rect struct so we can properly resize the bitmap
	int window_width =  client_rect->right - client_rect->left;
	int window_height = client_rect->bottom - client_rect->top;

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
			0, 0, bitmap_width, bitmap_height,
		        0, 0, window_width, window_height,	
			bitmap_memory, &bitmap_info,
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
		
			RECT client_rect;
			
			// First, we call GetClientRect to get the size of our client window
			// The client window is defined as the part of the screen that applications appear in. Not the part where there is the border, x out button, etc.
			// Since this is what we are drawing to we want this.
			GetClientRect(window, &client_rect); 

			// Get the height and width of our rectangle

			// width - right(x-coordiante of lower-right corner) - left(x coordinate of upper-left corner)
			// height = bottom(y-cooridante of lower-right corner) - top(y coordinate of upper-left corner)
			int width = client_rect.right - client_rect.left;
			int height = client_rect.bottom - client_rect.top;

			// We resize the bitmap to represent correctly our resized window dimensions
			win32_resize_dib_section(width, height);
		} break;
		
		case WM_DESTROY: {
			// TODO Handle this with a message to the user?
			running = false;
		} break;
		
		case WM_CLOSE: {
			// TODO Handle this as an error - recreate window?
			running = false;
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
			
			// TODO duplicate code!
			RECT client_rect;
			GetClientRect(window, &client_rect); 

	
			int x = paint.rcPaint.left;
			int y = paint.rcPaint.top;

			int width = paint.rcPaint.right - paint.rcPaint.left;
			int height = paint.rcPaint.bottom - paint.rcPaint.top;

	
			win32_update_window(device_context, &client_rect, x, y, width, height);
			
			// EndPaint tells windows you want to stop painting and to clean up any resources still open
			EndPaint(window, &paint);
			break;
		}
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
	WNDCLASS window_class = {};

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

			running = true;	
			int blue_offset = 0;
			int green_offset = 0;

			while(running) {

				// Where we will store the message			
				MSG message;
				

				while (PeekMessageA(&message, NULL, 0, 0, PM_REMOVE)) {

					// If windows decides to randomly kill our process, quit
					if (message.message == WM_QUIT) {
						running = false;
					}

					TranslateMessage(&message);
					DispatchMessageA(&message);
				}
				// call function to render weird gradient to screen!
				
				render_weird_gradient(blue_offset, green_offset);

				HDC device_context = GetDC(window);
				
				RECT client_rect;
				GetClientRect(window,&client_rect);

				int window_width = client_rect.right - client_rect.left;
				int window_height = client_rect.bottom - client_rect.top;

				win32_update_window(device_context, &client_rect, 0, 0, window_width, window_height);
				
				ReleaseDC(window,device_context);

				blue_offset++;
				green_offset += 2;
			} 
		}else{
		
		}
	}else {
	
	}

	return 0;
}



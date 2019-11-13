#include <windows.h>
#include <stdio.h>


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

// The actual handle to our bitmap. Essential serves as our back buffer.
global_variable HBITMAP bitmap_handle;

global_variable HDC bitmap_device_context;


private_function void win32_resize_dib_section(int width, int height) {

	// Frees the memory of our bitmap_handle if the function was called before.
	if(bitmap_handle != NULL) {
		DeleteObject(bitmap_handle);
	}
	if(bitmap_device_context == NULL) {
		// Get our device context via weird Windows magic
		bitmap_device_context = CreateCompatibleDC(0);
	}

	// Set values for the BITMAPINFOHEADER struct - which contains info about the dimensions and color format of a DIB.
	bitmap_info.bmiHeader.biSize = sizeof(bitmap_info.bmiHeader);
	bitmap_info.bmiHeader.biWidth = width;
	bitmap_info.bmiHeader.biWidth = height;
	bitmap_info.bmiHeader.biPlanes = 1;
	
	// Sets the number of bits per pixel we get. Usually for RGB you need 24 but Casey says we need 32 for later so that will be fun TODO
	bitmap_info.bmiHeader.biBitCount = 32;

	// uncompressed bitmap format.
	bitmap_info.bmiHeader.biCompression = BI_RGB;
	// Used to specify the size of the image so compressed formats know what they are uncompressing to. For us it doesn't matter since BI_RGB is uncompressed. 
	bitmap_info.bmiHeader.biSizeImage = 0;

	// Not needed since we don't use pallates.
	bitmap_info.bmiHeader.biXPelsPerMeter = 0;
	bitmap_info.bmiHeader.biYPelsPerMeter = 0;
	bitmap_info.bmiHeader.biClrUsed = 0;
	bitmap_info.bmiHeader.biClrImportant = 0;


	// Windows function that actually creates a DIB for our back buffer that we will write to, then push to our window to have it render whatever we specify
	
	// Takes the following arguments
	
	// HDC hdc - A handle to a device context
	// const BITMAPINFO *pbmi - a pointer to a BITMAPINFO strutcure that specifies various attributes of the DIB, including dimensions and color,
	// UINT usage - Specifies what type of data that BITMAPINFO struct stores - DIB_RGB_COLORS is almost always used
	// VOID **ppvBits - A pointer to a variable that receives a pointer to the location of the DIB bit values
	// HANDLE hSection - A handle to a file-mapping object that the function will use to create the DIB - can be NULL - NOT NEEDED
	// offset - NOT NEEDED
	bitmap_handle = CreateDIBSection(bitmap_device_context, 
					&bitmap_info, DIB_RGB_COLORS, 
					&bitmap_memory, NULL, 0);
}

private_function void win32_update_window(HDC device_context, int x, int y, int width, int height) {
	
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
	StretchDIBits(bitmap_device_context, 
			x, y, width, height,
			x, y, width, height,
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
			
			int x = paint.rcPaint.left;
			int y = paint.rcPaint.top;

			int width = paint.rcPaint.right - paint.rcPaint.left;
			int height = paint.rcPaint.bottom - paint.rcPaint.top;

	
			win32_update_window(device_context, x, y, width, height);
			
			// EndPaint tells windows you want to stop painting and to clean up any resources still open
			EndPaint(window, &paint);
			break;
		}
		default: { 
			result = DefWindowProc(window, message, wParam, lParam);
		}
	}	
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
		HWND window_handle = CreateWindowExA(
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
		if(window_handle) {
					
			
			// Windows doesn't actually start sending messages to a windows callback function until you pull it off a queue. So we loop through the messages and use them continously

			// The loop also functions as our main loop in general that keeps the process going, and as a result, the window from disappering immediatly as soon as its created.
			// Kinda similar to a game loop

			// Where we will store the message			
			MSG message;
			
			running = true;	
			while(running) {
				// GetMessage is used to return messages for a window
				// 	
				//
				// LPMSG lpMsg - A pointer to the message struct where we want to store the message we get
				// HWND hWnd - A handle to the window we want to get messages for. If NULL, gets messages for any window that belows to the current thread
				// UINT wMsgFilterMin - value to specify message type
				// UINT wMsgFilterMax - value to specify message type
				// NOTE: If the filters are both set to zero, it will return all available messages.
				BOOL message_result = GetMessageA(&message, NULL, 0, 0);
				
				if(message_result > 0) {
					// Translates the message to get ready for dispatch
					TranslateMessage(&message);
					DispatchMessageA(&message);
				}else {
					break;
				}
				
			} 
		}else{
		
		}
	}else {
	
	}

	return 0;
}



#include <windows.h>
#include <stdio.h>


// This is the callback function windows uses when sending messages to your window. 
LRESULT CALLBACK main_window_callback(
	HWND window,
	UINT message,
	WPARAM wParam,
	LPARAM lParam
) {
	LRESULT result = 0;

	switch(message) {
		case WM_SIZE: {
			printf("WM_SIZE\n");
		} break;
		
		case WM_DESTROY: {
			printf("WM_DESTROY\n");
		} break;
		
		case WM_CLOSE: {
			printf("WM_CLOSE\n");
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

			int height = paint.rcPaint.bottom - paint.rcPaint.top;
			int width = paint.rcPaint.right - paint.rcPaint.left;
	
			PatBlt(device_context, x, y, width, height, WHITENESS);
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

	//TODO Check if HREDRAW/ VREDRAW are needed

	window_class.lpfnWndProc = main_window_callback;
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
			// We use a for loop due to GetMessage return type(BOOL) can return a -1 for a value(Rediculous I know).
			// For instance if a handle we pass is already distroyed, then it returns a -1, which is a value that evaluates to true in C. So a while loop will do
			// the wrong thing and continue operating even though we would want to exit the loop ( return value of 0)
			// So we will check to see if the message result(BOOL) is > 0, then we know its not a false value or an error value like -1, and we can continue pulling
			// messages of the quque indefinitly.
			// The for loop also functions as our main loop in general that keeps the process going, and as a result, the window from disappering immediatly as soon as its created.
			// Kinda similar to a game loop

			// Where we will store the message			
			MSG message;
				
			for(;;) {
				// GetMessage is used to return messages for a window
				// 	
				//
				// LPMSG lpMsg - A pointer to the message struct where we want to store the message we get
				// HWND hWnd - A handle to the window we want to get messages for. If NULL, gets messages for any window that belows to the current thread
				// UINT wMsgFilterMin - value to specify message type
				// UINT wMsgFilterMax - value to specify message type
				// NOTE: If the filters are both set to zero, it will return all available messages.
				BOOL message_result = GetMessage(&message, NULL, 0, 0);
				
				if(message_result > 0) {
					// Translates the message to get ready for dispatch
					TranslateMessage(&message);
					DispatchMessage(&message);
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



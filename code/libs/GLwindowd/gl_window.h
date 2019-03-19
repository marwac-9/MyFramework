//------------------------------------------------------------------------------
/**
	Manages the opening and closing of a window.
	
	(C) 2015 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include <functional>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include "config.h"
namespace Display
{
class Window
{
public:
	/// constructor
	Window();
	/// destructor
	~Window();

	/// set size of window
	void SetSize(int32 width, int32 height);
	/// set title of window
	void SetTitle(const std::string& title);

	/// open window
	bool Open();
	/// close window
	void Close();
	/// returns true if window is open
	const bool IsOpen() const;

	/// make this window current, meaning all draws will direct to this window context
	void MakeCurrent();

	/// update a tick
	void Update();
	/// swap buffers at end of frame
	void SwapBuffers();

	/// set key press function callback
	void SetKeyPressFunction(const std::function<void(int32, int32, int32, int32)>& func);
	/// set mouse press function callback
	void SetMousePressFunction(const std::function<void(int32, int32, int32)>& func);
	/// set mouse move function callback
	void SetMouseMoveFunction(const std::function<void(float64, float64)>& func);
	/// set mouse enter leave function callback
	void SetMouseEnterLeaveFunction(const std::function<void(bool)>& func);
	/// set mouse scroll function callback
	void SetMouseScrollFunction(const std::function<void(float64, float64)>& func);
	/// set close function callback
	void SetCloseFunction(const std::function<void()>& func);
	/// set window resize function callback
	void SetWindowSizeFunction(const std::function<void(int, int)>& func);
	/// set window iconify function callback
	void SetWindowIconifyFunction(const std::function<void(int)>& func);
	/// set drag and drop function callback
	void SetDragAndDropFunction(const std::function<void(int, const char**)>& func);

	/// get mouse button input
	int GetMouseButton(int button);
	/// get keyboard button input
	int GetKey(int key);
	/// set mouse cursor position
	void SetCursorPos(double x, double y);
	/// get mouse cursor position
	void GetCursorPos(double* out_x, double* out_y);
	/// get window size
	void GetWindowSize(int* out_width, int* out_height);
	/// sets cursor mode
	void SetCursorMode(int mode);
	/// get GLFW window
	GLFWwindow* GetGLFWWindow();

private:

	/// static key press callback
	static void StaticKeyPressCallback(GLFWwindow* win, int32 key, int32 scancode, int32 action, int32 mods);
	/// static mouse press callback
	static void StaticMousePressCallback(GLFWwindow* win, int32 button, int32 action, int32 mods);
	/// static mouse move callback
	static void StaticMouseMoveCallback(GLFWwindow* win, float64 x, float64 y);
	/// static mouse enter/leave callback
	static void StaticMouseEnterLeaveCallback(GLFWwindow* win, int32 mode);
	/// static mouse scroll callback
	static void StaticMouseScrollCallback(GLFWwindow* win, float64 x, float64 y);
	/// static close callback
	static void StaticCloseCallback(GLFWwindow* win);
	/// static window resize callback
	static void StaticWindowSizeCallback(GLFWwindow* win, int width, int height);
	/// static window iconify callback
	static void StaticWindowIconifyCallback(GLFWwindow* win, int iconified);
	/// static drag and drop callback
	static void StaticDragAndDropCallback(GLFWwindow* win, int count, const char** paths);

	/// resize update
	void Resize();
	/// title rename update
	void Retitle(); 

	static int32 WindowCount;

	/// function for key press callbacks
	std::function<void(int32, int32, int32, int32)> keyPressCallback;
	/// function for mouse press callbacks
	std::function<void(int32, int32, int32)> mousePressCallback;
	/// function for mouse move callbacks
	std::function<void(float64, float64)> mouseMoveCallback;
	/// function for mouse enter/leave callbacks
	std::function<void(bool)> mouseLeaveEnterCallback;
	/// function for mose scroll callbacks
	std::function<void(float64, float64)> mouseScrollCallback;
	/// function for close callbacks
	std::function<void()> closeCallback;
	/// function for window resize callbacks
	std::function<void(int, int)> windowSizeCallback;
	/// function for window iconify callbacks
	std::function<void(int)> windowIconifyCallback;
	/// function for drag and drop callnbacks
	std::function<void(int, const char**)> dragAndDropCallback;

	int32 width;
	int32 height;
	std::string title;

	GLFWwindow* window;
};

//------------------------------------------------------------------------------
/**
*/
inline void
Window::SetSize(int32 width, int32 height)
{
	this->width = width;
	this->height = height;
	if (nullptr != this->window) this->Resize();
}

//------------------------------------------------------------------------------
/**
*/
inline void
Window::SetTitle(const std::string& title)
{
	this->title = title;
	if (nullptr != this->window) this->Retitle();
}

//------------------------------------------------------------------------------
/**
*/
inline const bool
Window::IsOpen() const
{
	return nullptr != this->window;
}

//------------------------------------------------------------------------------
/**
*/
inline void
Window::SetKeyPressFunction(const std::function<void(int32, int32, int32, int32)>& func)
{
	this->keyPressCallback = func;
}

//------------------------------------------------------------------------------
/**
*/
inline void
Window::SetMousePressFunction(const std::function<void(int32, int32, int32)>& func)
{
	this->mousePressCallback = func;
}

//------------------------------------------------------------------------------
/**
*/
inline void
Window::SetMouseMoveFunction(const std::function<void(float64, float64)>& func)
{
	this->mouseMoveCallback = func;
}

//------------------------------------------------------------------------------
/**
*/
inline void
Window::SetMouseEnterLeaveFunction(const std::function<void(bool)>& func)
{
	this->mouseLeaveEnterCallback = func;
}

//------------------------------------------------------------------------------
/**
*/
inline void
Window::SetMouseScrollFunction(const std::function<void(float64, float64)>& func)
{
	this->mouseScrollCallback = func;
}

//------------------------------------------------------------------------------
/**
*/
inline void
Window::SetCloseFunction(const std::function<void()>& func)
{
	this->closeCallback = func;
}

//------------------------------------------------------------------------------
/**
*/
inline void
Window::SetWindowSizeFunction(const std::function<void(int, int)>& func)
{
	this->windowSizeCallback = func;
}

//------------------------------------------------------------------------------
/**
*/
inline void 
Window::SetWindowIconifyFunction(const std::function<void(int)>& func)
{
	this->windowIconifyCallback = func;
}

//------------------------------------------------------------------------------
/**
*/
inline void
Window::SetDragAndDropFunction(const std::function<void(int, const char**)>& func)
{
	this->dragAndDropCallback = func;
}

//------------------------------------------------------------------------------
/**
*/
inline GLFWwindow*
Window::GetGLFWWindow()
{
	return this->window;
}

} // namespace Display
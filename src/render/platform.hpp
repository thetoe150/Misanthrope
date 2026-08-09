#ifndef PLATFORM_H 
#define PLATFORM_H 
// #include "camera.hpp"
#include <functional>
#include "vulkan/vulkan.hpp"
#include "GLFW/include/glfw3.h"

#include "device.hpp"

class Platform {
public:
	void framebufferResizeCallback(GLFWwindow* window, int width, int height);
	void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
	void mouseCallback(GLFWwindow* window, double xpos, double ypos);
	void scrollCallback(GLFWwindow* window, double xoffset, double yoffset);

	GLFWwindow* initGLFW();
	bool createSurfaceForDevice(Device* i_device);
	void cleanUpGLFW();
	void processInput(GLFWwindow* window);

private:
	GLFWwindow* m_window;
	uint32_t m_windowWidth = 1500;
	uint32_t m_windowHeight = 1000;

	typedef std::vector<std::function<void(int, int)>> Functors;
	Functors m_keyCallbacks;
	Functors m_mouseCallbacks;
	Functors m_scrollCallbacks;


};

#endif /*PLATFORM_H*/

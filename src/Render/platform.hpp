#define GLFW_INCLUDE_VULKAN
#include "GLFW/include/glfw3.h"
#include "camera.hpp"
#include <functional>

constexpr uint32_t WIDTH = 1500;
constexpr uint32_t HEIGHT = 1000;

static GLFWwindow* window;

typedef std::vector<std::function<void(int, int)>> Functors;
static Functors m_keyCallbacks;
static Functors m_mouseCallbacks;
static Functors m_scrollCallbacks;

static void framebufferResizeCallback(GLFWwindow* window, int width, int height) {
	// auto app = reinterpret_cast<Renderer*>(glfwGetWindowUserPointer(window));
	// app->framebufferResized = true;
}

static void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods){
	for (auto& func : m_keyCallbacks) {
		func(key, action);
	}
	// if(key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
	// 	glfwSetWindowShouldClose(window, true);

	// if (glfwGetKey(window, GLFW_KEY_P) == GLFW_PRESS) {
	// 	s_currentTopologyIdx += 1;
	// 	if (s_currentTopologyIdx > 2) {
	// 		s_currentTopologyIdx %= 3;
	// 	}
	// }

	// if (glfwGetKey(window, GLFW_KEY_L) == GLFW_PRESS) {
	// 	if(!useLOD)
	// 		useLOD = true;
	// 	else
	// 		useLOD = false;
	// }

	// if (glfwGetKey(window, GLFW_KEY_K) == GLFW_PRESS) {
	// 	s_isLodUpdated = true;
	// }
}

static void mouseCallback(GLFWwindow* window, double xpos, double ypos){
	for (auto& func : m_mouseCallbacks) {
		func(xpos, ypos);
	}
	// if (firstMouse) {
	// 	lastX = xpos;
	// 	lastY = ypos;
	// 	firstMouse = false;
	// }
	// 
	// float xoffset = xpos - lastX;
	// float yoffset = lastY - ypos;
	// lastX = xpos;
	// lastY = ypos;

	// if (s_moveCam)
	// 	g_camera.processMouseMovement(xoffset, yoffset);
}

static void scrollCallback(GLFWwindow* window, double xoffset, double yoffset){
	for (auto& func : m_mouseCallbacks) {
		func(xoffset, yoffset);
	}
	// g_camera.processMouseScroll(static_cast<float>(yoffset));
}

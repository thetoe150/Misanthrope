#include "platform.hpp"

bool Platform::createSurfaceForDevice(Device* i_device) {
	if (glfwCreateWindowSurface((VkInstance)i_device->getInstance(), m_window, nullptr, reinterpret_cast<VkSurfaceKHR*>(i_device->getSurfacePtr())) != VK_SUCCESS) {
		return true;
	}

	return false;
}

void Platform::framebufferResizeCallback(GLFWwindow* window, int width, int height) {
	// auto app = reinterpret_cast<Renderer*>(glfwGetWindowUserPointer(window));
	// app->framebufferResized = true;
}

void Platform::keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods){
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

void Platform::mouseCallback(GLFWwindow* window, double xpos, double ypos){
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

void Platform::scrollCallback(GLFWwindow* window, double xoffset, double yoffset){
	for (auto& func : m_mouseCallbacks) {
		func(xoffset, yoffset);
	}
	// g_camera.processMouseScroll(_cast<float>(yoffset));
}

GLFWwindow* Platform::initGLFW() {
	glfwInit();
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	m_window = glfwCreateWindow(m_windowWidth, m_windowHeight , "Misanthrope", nullptr, nullptr);

	glfwSetWindowUserPointer(m_window, this);
	{
		auto func = [](GLFWwindow* window, int width, int height) -> void {
			static_cast<Platform*>(glfwGetWindowUserPointer(window))->framebufferResizeCallback(window, width, height);
		};
		glfwSetFramebufferSizeCallback(m_window, func);
	}
	{
		auto func = [](GLFWwindow* window, int key, int scancode, int action, int mods) -> void {
			static_cast<Platform*>(glfwGetWindowUserPointer(window))->keyCallback(window, key, scancode, action, mods);
		};
		glfwSetKeyCallback(m_window, func);
	}
	{
		auto func = [](GLFWwindow* window, double xpos, double ypos) -> void {
			static_cast<Platform*>(glfwGetWindowUserPointer(window))->mouseCallback(window, xpos, ypos);
		};
		glfwSetCursorPosCallback(m_window, func);
	}
	{
		auto func = [](GLFWwindow* window, double xoffset, double yoffset) -> void {
			static_cast<Platform*>(glfwGetWindowUserPointer(window))->scrollCallback(window, xoffset, yoffset);
		};
		glfwSetScrollCallback(m_window, func);
	}

	return m_window;
}

void Platform::cleanUpGLFW(){
	glfwDestroyWindow(m_window);
	glfwTerminate();
}
	
void Platform::processInput(GLFWwindow* window){
	glfwPollEvents();

	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
		glfwSetWindowShouldClose(window, true);

	//// key for camera
	// if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
	// 	g_camera.processKeyboard(MovementDirection::FORWARD, m_currentDeltaTime);
	// if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
	// 	g_camera.processKeyboard(MovementDirection::BACKWARD, m_currentDeltaTime);
	// if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
	// 	g_camera.processKeyboard(MovementDirection::LEFT, m_currentDeltaTime);
	// if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
	// 	g_camera.processKeyboard(MovementDirection::RIGHT, m_currentDeltaTime);
	// if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
	// 	g_camera.processKeyboard(MovementDirection::UP, m_currentDeltaTime);
	// if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
	// 	g_camera.processKeyboard(MovementDirection::DOWN, m_currentDeltaTime);

	// if (glfwGetKey(window, GLFW_KEY_X) == GLFW_PRESS) {
	// 	s_moveCam = true;
	// 	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
	// }
	// if (glfwGetKey(window, GLFW_KEY_C) == GLFW_PRESS) {
	// 	s_moveCam = false;
	// 	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
	// }
	// if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS) {
	// 	recreatePipelines();
	// }
	// if (glfwGetKey(window, GLFW_KEY_H) == GLFW_PRESS) {
	// 	if (m_isHDR) {
	// 		m_renderTargetImageFormat = findHDRColorFormat();
	// 		recreateRenderTargets();
	// 		m_isHDR = false;
	// 	}
	// 	else {
	// 		m_renderTargetImageFormat = VK_FORMAT_R8G8B8A8_SRGB;
	// 		recreateRenderTargets();
	// 		m_isHDR = true;
	// 	}
	// }
}

#include "platform.hpp"

GLFWwindow* initGLFW() {
	glfwInit();

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

	window = glfwCreateWindow(WIDTH, HEIGHT, "Misanthrope", nullptr, nullptr);
	// glfwSetWindowUserPointer(window, this);
	glfwSetFramebufferSizeCallback(window, framebufferResizeCallback);
	glfwSetKeyCallback(window, keyCallback);
	glfwSetCursorPosCallback(window, mouseCallback);
	glfwSetScrollCallback(window, scrollCallback);

	return window;
}

void cleanUpGLFW(){
	glfwDestroyWindow(window);
	glfwTerminate();
}
	
void processInput(GLFWwindow* window){
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

#include "camera.hpp"

Camera::Camera(glm::vec3 position, glm::vec3 front, glm::vec3 up, float yaw, float pitch)
	  : m_position(position), m_front(front), m_worldUp(up),
		m_yaw(yaw), m_pitch(pitch), m_zoom(ZOOM)
{
	updateCameraVectors();
}

void Camera::updateCameraVectors() {
	glm::vec3 front{};
	front.y = glm::sin(glm::radians(m_pitch));
	front.x = glm::cos(glm::radians(m_pitch)) * cos(glm::radians(m_yaw));
	front.z = glm::cos(glm::radians(m_pitch)) * sin(glm::radians(m_yaw));

	m_front = glm::normalize(front);
	m_right = glm::normalize(glm::cross(m_front, m_worldUp));
	m_up = glm::normalize(glm::cross(m_right, m_front));
}

glm::vec3 Camera::getPostion(){
	return m_position;
}

glm::vec3 Camera::getFront(){
	return m_front;
}

glm::mat4 Camera::getViewMatrix(){
	return glm::lookAt(m_position, m_position + m_front, m_up);
}

float Camera::getZoom(){
	return m_zoom;
}

void Camera::processKeyboard(MovementDirection direction, float offset) {
	float speed = offset * MOVE_SPEED;
	switch(direction){
		case MovementDirection::UP: {
			m_position += m_up * speed;
			break;
		}
		case MovementDirection::DOWN: {
			m_position -= m_up * speed;
			break;
		}
		case MovementDirection::FORWARD: {
			m_position += m_front * speed;
			break;
		}
		case MovementDirection::BACKWARD: {
			m_position -= m_front * speed;
			break;
		}
		case MovementDirection::RIGHT: {
			m_position += m_right * speed;
			break;
		}
		case MovementDirection::LEFT: {
			m_position -= m_right * speed;
			break;
		}
	}
}

void Camera::processMouseMovement(float x_offset, float y_offset, bool isPitchBound) {
	m_yaw += x_offset * MOUSE_MOVE_SPEED;
	m_pitch += y_offset * MOUSE_MOVE_SPEED;

	if (isPitchBound && m_pitch > 89.f)
		m_pitch = 89.f;
	if (isPitchBound && m_pitch < -89.f)
		m_pitch = -89.f;

	updateCameraVectors();
}

void Camera::processMouseScroll(float scrollOffset) {
	m_zoom -= scrollOffset * MOUSE_SCROLL_SPEED;

	if(m_zoom < 1.f)
		m_zoom = 1.f;
	if(m_zoom > 45.f)
		m_zoom = 45.f;
}

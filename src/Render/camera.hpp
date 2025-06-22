#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/hash.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/string_cast.hpp>

enum MovementDirection 
{
	FORWARD,
	BACKWARD,
	LEFT,
	RIGHT,
	UP,
	DOWN
};

constexpr glm::vec3 POSITION{4.0f, 2.5f, 8.5f};
constexpr glm::vec3 FRONT{0.2f, -0.2f, -0.95f};
constexpr glm::vec3 WORLD_UP{0.f, 1.f, 0.f};

constexpr float YAW{-90.f};
constexpr float PITCH{-10.f};

constexpr float MOVE_SPEED{5.f};
constexpr float MOUSE_MOVE_SPEED{0.1f};
constexpr float MOUSE_SCROLL_SPEED{1.f};
constexpr float ZOOM{45.f};

class Camera{
public:
	Camera(glm::vec3 position = POSITION, glm::vec3 front = FRONT, glm::vec3 up = WORLD_UP, float yaw = YAW, float pitch = PITCH);

	glm::vec3 getPostion();
	glm::vec3 getFront();
	glm::mat4 getViewMatrix();
	float getZoom();
	void processKeyboard(MovementDirection direction, float offset);
	void processMouseMovement(float x_offset, float y_offset, bool isPitchBound = true);
	void processMouseScroll(float scrollOffset);

private:
	void updateCameraVectors();

private:
	glm::vec3 m_position;

	glm::vec3 m_front;
	glm::vec3 m_up{0.f};
	glm::vec3 m_worldUp{0.f};
	glm::vec3 m_right{0.f};

	float m_yaw;
	float m_pitch;
	float m_zoom;
};

inline bool s_moveCam = true;
inline bool firstMouse = true;
inline float lastX = 0.f;
inline float lastY = 0.f;
inline unsigned int speedCount = 0;

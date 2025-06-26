#include "Render/util.hpp"
#include <chrono>
#include <array>

constexpr int MAX_FRAMES_IN_FLIGHT = 2;

constexpr float s_candlesScale[3] = {10.0f, 10.0f, 10.0f};
constexpr float s_candlesRotate[3] = {0.f, 0.f, 0.f};
constexpr float s_candlesTranslate[3] = {0.f, 0.f, 0.f};

static float s_snowScale[3] = {0.008f, 0.008f, 0.005f};
static float s_snowRotate[3] = {0.f, 0.f, 0.f};
static float s_snowTranslate[3] = {0.f, 5.f, 0.f};

static glm::vec3 s_lightDir {3.f, 4.f, 5.f};
static float s_nearPlane = 0.1f;
static float s_farPlane = 100.f;
static float s_shadowFarPlane = 20.f;
static float s_shadowLeftPlane = -10.f;
static float s_shadowRightPlane = 10.f;
static float s_shadowBotPlane = -10.f;
static float s_shadowTopPlane = 10.f;

static const char* CANDLE_MODEL_PATH = "../../data/models/candles_set/scene.gltf";
static const char* SNOWFLAKE_MODEL_PATH = "../../data/models/snowflake/scene.gltf";
static const char* TOWER_TEXTURE_PATH = "../../data/textudata/Wood_Tower_Col.jpg";
// const std::string SNOWFLAKE_TEXTURE_PATH = "data/textudata/Wood_Tower_Col.jpg";
// const std::string TOWER_MODEL_PATH = "../../data/models/wooden_watch_tower2.obj";

constexpr unsigned int SNOWFLAKE_COUNT = 4096;
constexpr float CANDLE_ANIMATION_SPEED = 0.5f;

constexpr unsigned int CANDLES_INSTANCE_MAX = 10;
constexpr unsigned int CANDLES_BASE_MESH_COUNT = 10;

constexpr int MAX_VORTEX_COUNT = 10;
constexpr float VORTEX_COVER_RANGE = 3.f;
constexpr float MAX_FORCE = 5.f;
constexpr float MIN_FORCE = 3.f;
constexpr float MAX_RADIUS = 15.f;
constexpr float MIN_RADIUS = 5.f;
constexpr float PHASE_RANGE = 2;

static auto startTime = std::chrono::high_resolution_clock::now();
static std::array<float, MAX_VORTEX_COUNT> s_baseRadius;
static std::array<float, MAX_VORTEX_COUNT> s_basePhase;
static std::array<float, MAX_VORTEX_COUNT> s_baseForce;


inline float quadListVertices[] = {
	// positions        // texture Coords
	// first triangle
     1.0f,  1.0f, 0.0f,  1.0f,  1.0f, // top right
     1.0f, -1.0f, 0.0f,  1.0f,  0.0f,  // bottom right
    -1.0f,  1.0f, 0.0f,  0.0f,  1.0f,  // top left 
    // second triangle
     1.0f, -1.0f, 0.0f,  1.0f,  0.0f,  // bottom right
    -1.0f, -1.0f, 0.0f,  0.0f,  0.0f,  // bottom left
    -1.0f,  1.0f, 0.0f,  0.0f,  1.0f,  // top left
};

inline float quadStripVertices[] = {
	// positions        // texture Coords
	-1.0f,  1.0f, 0.0f, 0.0f, 1.0f,
	-1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
	 1.0f,  1.0f, 0.0f, 1.0f, 1.0f,
	 1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
};

static float cubeVertices[] = {
	// positions          // normals
	-0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
	 0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
	 0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
	 0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
	-0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
	-0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,

	-0.5f, -0.5f,  0.5f,  0.0f,  0.0f, 1.0f,
	 0.5f, -0.5f,  0.5f,  0.0f,  0.0f, 1.0f,
	 0.5f,  0.5f,  0.5f,  0.0f,  0.0f, 1.0f,
	 0.5f,  0.5f,  0.5f,  0.0f,  0.0f, 1.0f,
	-0.5f,  0.5f,  0.5f,  0.0f,  0.0f, 1.0f,
	-0.5f, -0.5f,  0.5f,  0.0f,  0.0f, 1.0f,

	-0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,
	-0.5f,  0.5f, -0.5f, -1.0f,  0.0f,  0.0f,
	-0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,
	-0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,
	-0.5f, -0.5f,  0.5f, -1.0f,  0.0f,  0.0f,
	-0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,

	 0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,
	 0.5f,  0.5f, -0.5f,  1.0f,  0.0f,  0.0f,
	 0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,
	 0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,
	 0.5f, -0.5f,  0.5f,  1.0f,  0.0f,  0.0f,
	 0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,

	-0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,
	 0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,
	 0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,
	 0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,
	-0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,
	-0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,

	-0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,
	 0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,
	 0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,
	 0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,
	-0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,
	-0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f
};

static float skyboxVertices[] = {
	// positions          
	-1.0f,  1.0f, -1.0f,
	-1.0f, -1.0f, -1.0f,
	 1.0f, -1.0f, -1.0f,
	 1.0f, -1.0f, -1.0f,
	 1.0f,  1.0f, -1.0f,
	-1.0f,  1.0f, -1.0f,

	-1.0f, -1.0f,  1.0f,
	-1.0f, -1.0f, -1.0f,
	-1.0f,  1.0f, -1.0f,
	-1.0f,  1.0f, -1.0f,
	-1.0f,  1.0f,  1.0f,
	-1.0f, -1.0f,  1.0f,

	 1.0f, -1.0f, -1.0f,
	 1.0f, -1.0f,  1.0f,
	 1.0f,  1.0f,  1.0f,
	 1.0f,  1.0f,  1.0f,
	 1.0f,  1.0f, -1.0f,
	 1.0f, -1.0f, -1.0f,

	-1.0f, -1.0f,  1.0f,
	-1.0f,  1.0f,  1.0f,
	 1.0f,  1.0f,  1.0f,
	 1.0f,  1.0f,  1.0f,
	 1.0f, -1.0f,  1.0f,
	-1.0f, -1.0f,  1.0f,

	-1.0f,  1.0f, -1.0f,
	 1.0f,  1.0f, -1.0f,
	 1.0f,  1.0f,  1.0f,
	 1.0f,  1.0f,  1.0f,
	-1.0f,  1.0f,  1.0f,
	-1.0f,  1.0f, -1.0f,

	-1.0f, -1.0f, -1.0f,
	-1.0f, -1.0f,  1.0f,
	 1.0f, -1.0f, -1.0f,
	 1.0f, -1.0f, -1.0f,
	-1.0f, -1.0f,  1.0f,
	 1.0f, -1.0f,  1.0f
};


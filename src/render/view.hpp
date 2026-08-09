#include "renderer.hpp"

struct Frustum {
};

class ViewSink {
public:
	void Submit();
	void Add();
private:
	std::vector<Drawcall> m_drawcall;
};

class View {
public:
	void Cull();
	void Sort();
private:
	Frustum m_frustum;
	// Camera m_camera;
	ViewSink m_sink;
};

class ShadowView {
public:
	void Cull();
	void Sort();
private:
	Frustum m_frustum;
	ViewSink m_sink;
};

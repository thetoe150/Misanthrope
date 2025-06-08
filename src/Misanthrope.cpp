#include "Misanthrope.hpp"
#include "spirv_reflect.h"
#include "vulkan/vulkan_core.h"

int main() {
	srand(static_cast<unsigned>(time(0)));
	// testAlignment();
    Renderer app;

    try {
		std::cout << "Start Rendering" << std::endl;
        app.run();
    } catch (const std::exception& e) {
	}
}

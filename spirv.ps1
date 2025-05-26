glslc -fshader-stage=vertex data/shaders/snowflake.vert -o data/shaders/snowflake.vert.spv
glslc -fshader-stage=vertex data/shaders/candles.vert -o data/shaders/candles.vert.spv
glslc -fshader-stage=vertex data/shaders/shadow_batch.vert -o data/shaders/shadow_batch.vert.spv
glslc -fshader-stage=vertex data/shaders/quad.vert -o data/shaders/quad.vert.spv
glslc -fshader-stage=vertex data/shaders/floor.vert -o data/shaders/floor.vert.spv
glslc -fshader-stage=vertex data/shaders/skybox.vert -o data/shaders/skybox.vert.spv
glslc -fshader-stage=fragment data/shaders/snowflake.frag -o data/shaders/snowflake.frag.spv
glslc -fshader-stage=fragment data/shaders/candles.frag -o data/shaders/candles.frag.spv
glslc -fshader-stage=fragment data/shaders/shadow_viewport.frag -o data/shaders/shadow_viewport.frag.spv
glslc -fshader-stage=fragment data/shaders/bloom.frag -o data/shaders/bloom.frag.spv
glslc -fshader-stage=fragment data/shaders/combine.frag -o data/shaders/combine.frag.spv
glslc -fshader-stage=fragment data/shaders/floor.frag -o data/shaders/floor.frag.spv
glslc -fshader-stage=fragment data/shaders/skybox.frag -o data/shaders/skybox.frag.spv
glslc -fshader-stage=compute data/shaders/snowflake.comp -o data/shaders/snowflake.comp.spv

spirv-dis data/shaders/snowflake.vert.spv > data/shaders/snowflake.vert.spvasm
spirv-dis data/shaders/floor.vert.spv > data/shaders/floor.vert.spvasm
spirv-dis data/shaders/skybox.vert.spv > data/shaders/skybox.vert.spvasm
spirv-dis data/shaders/candles.vert.spv > data/shaders/candles.vert.spvasm
spirv-dis data/shaders/quad.vert.spv > data/shaders/quad.vert.spvasm
spirv-dis data/shaders/candles.frag.spv > data/shaders/candles.frag.spvasm
spirv-dis data/shaders/snowflake.frag.spv > data/shaders/snowflake.frag.spvasm
spirv-dis data/shaders/shadow_batch.vert.spv > data/shaders/shadow_batch.vert.spvasm
spirv-dis data/shaders/shadow_viewport.frag.spv > data/shaders/shadow_viewport.frag.spvasm
spirv-dis data/shaders/bloom.frag.spv > data/shaders/bloom.frag.spvasm
spirv-dis data/shaders/floor.frag.spv > data/shaders/floor.frag.spvasm
spirv-dis data/shaders/skybox.frag.spv > data/shaders/skybox.frag.spvasm
spirv-dis data/shaders/combine.frag.spv > data/shaders/combine.frag.spvasm
spirv-dis data/shaders/snowflake.comp.spv > data/shaders/snowflake.comp.spvasm

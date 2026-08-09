#ifndef MODEL_H
#define MODEL_H

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "cgltf/cgltf.h"
#include "global.hpp"
#include "material.hpp"
#include "mesh.hpp"
#include "robin_map/robin_map.h"

class Model {
   public:
    struct ModelDesc {
        std::string name;
        uint32_t meshCount;
        uint32_t instanceCount;
        uint32_t isAnimated;
        glm::vec3 initPosition;
        bool isBlend;
        std::vector<Mesh::MeshDesc> m_meshDesc;
    };

    Model(ModelDesc, const cgltf_data*);
    virtual void build();
    virtual std::vector<std::string> gatherMeshesToLoad();
    virtual std::vector<std::string> gatherTexturesToLoad();

    ModelDesc m_modelDesc;
    glm::vec3 m_position;

    const cgltf_data* m_model;
    std::vector<glm::mat4> m_modelMeshTransforms;
    std::vector<MeshImages> m_modelImages;

   protected:
    std::vector<Mesh> m_meshes;
};

class AnimatedModel : Model {
   public:
    AnimatedModel(ModelDesc, const cgltf_data*);

    void build() override;
    std::vector<std::string> gatherMeshesToLoad() override;
    std::vector<std::string> gatherTexturesToLoad() override;

    void computeAnimation(cgltf_data i_data);
    std::vector<float> computeWeights(unsigned int meshIdx, float deltaTime);
    void computeMorphTargets(unsigned int meshIdx, std::vector<float> weights);
    void traverseModelNodesForTransform();

   private:
    float m_currentAnimTime;
    std::vector<AnimatedMesh> m_animatedMeshes;
};

class BatchedModel : Model {
   public:
    BatchedModel(const rapidjson::Value&);

   private:
    BatchedMesh mesh;
};

class ModelManager {
   public:
    ModelManager();
    ~ModelManager();
    bool loadModels(std::vector<std::string> i_paths);
    const cgltf_data* getModel(const char* i_name);

   private:
    tsl::robin_map<std::string, cgltf_data*> m_models;
};

#endif  // MODEL_H

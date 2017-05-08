/**
* Credit of original goes to Stamate Cosmin
*
* Description: World generator.
* The algorithm used in generation is the diamond square fractal generator:
* http://www.gameprogrammer.com/fractal.html
* http://www.bluh.org/code-the-diamond-square-algorithm/
*/

#pragma once

#include "glm\glm.hpp"
//#include "mesh_loader.h"
#include "raw_model.h"
#include <math.h>
#include <random>

// World modes.
#define WORLD_MODE_BASE 0
#define WORLD_MODE_FRACTAL 1

#define WORLD_MODE_COUNT 2

// Number of squares on the side of a terrain block, for the base mode.
#define WORLD_SQUARE_COUNT 256

// The range in which the height is displaced for the fractal generation.
#define WORLD_FRACTAL_DISPLACEMENT_RANGE 1200.0f
static const float WORLD_FRACTAL_Y_OFFSET = -WORLD_FRACTAL_DISPLACEMENT_RANGE * 0.5f;

// Infinity number to mark un-initialized vertices.
#define WORLD_INFINITY -2000.0f

// How big a block is compared to the visible area around the camera.
#define WORLD_RADIUS_MULTIPLY 3.0f

// Colors for the mountains.
static const glm::vec4 WORLD_TOP_COLOR = glm::vec4(0.95f, 0.95f, 0.95f, 1);
static const glm::vec4 WORLD_BOTTOM_COLOR = glm::vec4(0.1f, 0.1f, 0.1f, 1);

// Boundary of the mountain colors, relative to the maximum / minimum height.
#define WORLD_BOUNDARY_TOP 0.53f
#define WORLD_BOUNDARY_TOP_HIGH 1.5f
#define WORLD_BOUNDARY_BOTTOM 0.9f
#define WORLD_BOUNDARY_BOTTOM_LOW 1.8f

// Materials used for the various modes.
static const RawModelMaterial material_neutral = RawModelMaterial(50,
    glm::vec4(0.18f, 0.18f, 0.18f, 1),
    glm::vec4(0.08f, 0.08f, 0.08f, 1),
    glm::vec4(0.1f, 0.1f, 0.1f, 1),
    glm::vec4(0.2f, 0.2f, 0.2f, 1));

static const RawModelMaterial material_mountains = RawModelMaterial(10,
    glm::vec4(0.39f, 0.36f, 0.29f, 1),
    glm::vec4(0.1f, 0.1f, 0.1f, 1),
    glm::vec4(0.19f, 0.19f, 0.09f, 1),
    glm::vec4(0.06f, 0.06f, 0.06f, 1));

static const RawModelMaterial* materials[] = {
    &material_neutral,
    &material_mountains, &material_mountains,
    &material_mountains, &material_mountains
};

// A vertex structure, containing position and normal.
struct WorldVertex {
    glm::vec3 position;
    glm::vec3 normal;

    WorldVertex();
    WorldVertex(glm::vec3 position);
    WorldVertex(glm::vec3 position, glm::vec3 normal);
};

// Block structure, containing the actual VBO information.
struct WorldBlock {
    unsigned int vao;
    unsigned int vbo;
    unsigned int ibo;

    WorldVertex* vertices;
    unsigned int* indexes;

    unsigned int square_count;
    unsigned int vertex_count;

    unsigned int total_square_count;
    unsigned int total_triangle_count;
    unsigned int total_index_count;
    unsigned int total_vertex_count;
    float square_size;
};

class World {
public:
    World(glm::vec3 position, float radius, unsigned int mode);
    ~World();

    void setMode(unsigned int mode);
    void render(unsigned int shader, glm::mat4 model_matrix,
        glm::vec3 position, glm::mat4* objectToWorldMatrix, glm::mat4* projectionMatrix, glm::mat4* cameraToWorldMatrix, glm::mat4* modelViewProjectionMatrix, glm::mat3* objectToWorldNormalMatrix, GLuint uniformBindingPoint, GLuint uniformBlock, GLint uniformOffset[]);

    void generateBase(unsigned int mode, unsigned int square_count);
    void generateTerrain(unsigned int mode, unsigned int square_count);
    void tessellateTerrain(unsigned int source_mode, unsigned int mode);

    void generateFractal(unsigned int mode, unsigned int iteration);
    WorldBlock* initializeBlock(unsigned int mode, unsigned int square_count);
    void bufferData();
    void computeNormals(unsigned int mode);

	glm::vec3 getBlockPos(glm::vec3 pos);

private:
    int mode;
    glm::vec3 position;
    float length;
    float radius;
    glm::vec2 boundary_top, boundary_bottom;
    WorldBlock* blocks[WORLD_MODE_COUNT];
};
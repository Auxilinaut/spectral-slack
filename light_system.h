/**
* Credit of original goes to Stamate Cosmin
*
* Description: Light ensemble system, controlling all light sources
*/

#include <stdlib.h>
#include <time.h>
#include "glm\glm.hpp"
#include "glm\gtx\vector_angle.hpp"
#include "raw_model.h"
#include "entity.h"
#include "camera.h"
#include "texture_loader.h"

// Fog constants
#define FOG_START_RADIUS 1200.0f
#define FOG_END_RADIUS 3000.0f
#define MOUNTAIN_JAG 1000.0f
#define FOG_COLOR glm::vec4(1, 0.996f, 0.549f, 1)

// Types of lights
#define LIGHT_OMNI 0
#define LIGHT_SPOT 1

// Light constants.
#define LIGHT_SPEED 10.0f // Movement speed

// Maximum radius from the relative point, in which lights are instantiated
#define LIGHT_MAXIMUM_RADIUS 200.0f
static const float LIGHT_MAXIMUM_RADIUS_2 = LIGHT_MAXIMUM_RADIUS * 2.0f;

// Height range in which lights are created from the relative point
#define LIGHT_MAXIMUM_HEIGHT 15.0f
static const float LIGHT_MAXIMUM_HEIGHT_2 = LIGHT_MAXIMUM_HEIGHT * 2.0f;

// Light color constants
static const glm::vec4 LIGHT_MINIMUM_COLOR = glm::vec4(0.5f, 0.5f, 0.5f, 1);
static const glm::vec4 LIGHT_MAXIMUM_COLOR = glm::vec4(0.95f, 0.95f, 0.95f, 1);
static const glm::vec4 LIGHT_RANGE_COLOR = LIGHT_MAXIMUM_COLOR - LIGHT_MINIMUM_COLOR;

// Spotlight constants
static const float LIGHT_MAXIMUM_SPOT_ANGLE = glm::radians(50.0f);
static const float LIGHT_MINIMUM_SPOT_ANGLE = glm::radians(15.0f);
static const float LIGHT_FADE_SPOT_ANGLE = glm::radians(10.0f);
static const float LIGHT_RANGE_SPOT_ANGLE = (LIGHT_MAXIMUM_SPOT_ANGLE -
    LIGHT_MINIMUM_SPOT_ANGLE);

static const glm::vec3 LIGHT_SPOT_DIRECTION = glm::vec3(0, -1, 0);

// Light sizes, influence the light's influence area
#define LIGHT_MINIMUM_SIZE 5.0f
#define LIGHT_MAXIMUM_SIZE 15.0f
static const float LIGHT_RANGE_SIZE = LIGHT_MAXIMUM_SIZE - LIGHT_MINIMUM_SIZE;

// Shininess of the light objects
#define LIGHT_SHININESS 100

// Global ambiental color value
static const glm::vec4 LIGHT_AMBIENTAL = glm::vec4(0, 0, 0, 1);

// Maximum number of lights
#define LIGHT_MAXIMUM_COUNT 100

class Light : public Entity {
public:
	//of type at position with material for light calculations and a size
    Light(unsigned int type, glm::vec3 position, RawModelMaterial* material,
        float size);
    ~Light();

	//omni vs spotlight
    void setType(unsigned int type);

	//directional and dependant move functions
    void move(glm::vec3 movement);
	void moveToward(float time, glm::vec3 pos, glm::vec3 toward, float speed);

	//hands off matrices and other required values to renderer
    void render(unsigned int shader, glm::vec3 offset, glm::mat4 model_matrix, glm::mat4* objectToWorldMatrix, glm::mat4* projectionMatrix, glm::mat4* cameraToWorldMatrix, glm::mat4* modelViewProjectionMatrix, glm::mat3* objectToWorldNormalMatrix, GLuint uniformBindingPoint, GLuint uniformBlock, GLint uniformOffset[]);

    glm::vec3 getPosition();

private:
    glm::vec3 position;
    glm::vec3 size;
    unsigned int type;
    RawModelMaterial* material;
};

class LightSystem : public Entity {
public:
	//controls all lights
    LightSystem(unsigned int type, Camera* camera);
    ~LightSystem();

	void addLight(glm::vec3 camPos);
    void switchType();

	void switchCanMove();

	//can be set relative to the player or another object
    void setRelativePosition(glm::vec3 position);
    void switchFog();

	//directional move function
    void move(float time, glm::vec3 camPos, float speed);

	//hands off matrices and other required values to renderer
    void render(unsigned int shader, glm::mat4 model_matrix, glm::mat4* objectToWorldMatrix, glm::mat4* projectionMatrix, glm::mat4* cameraToWorldMatrix, glm::mat4* modelViewProjectionMatrix, glm::mat3* objectToWorldNormalMatrix, GLuint uniformBindingPoint, GLuint uniformBlock, GLint uniformOffset[]);

private:
    glm::vec3 relative_position;
    Light* lights[LIGHT_MAXIMUM_COUNT];
    glm::vec3 light_positions[LIGHT_MAXIMUM_COUNT];
    glm::vec4 light_colors[LIGHT_MAXIMUM_COUNT];
    float light_sizes[LIGHT_MAXIMUM_COUNT];
    float light_inner_angles[LIGHT_MAXIMUM_COUNT];
    float light_outer_angles[LIGHT_MAXIMUM_COUNT];
    int light_count;
    unsigned int type;
    bool fog;
	bool canMove;
};
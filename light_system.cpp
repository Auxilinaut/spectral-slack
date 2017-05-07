/**
* Credit of original goes to Stamate Cosmin
*
* Description: Light ensemble system, controlling all the light sources and
* the main light source, the sun.
*/

#include "light_system.h"

// Instantiate a simple light, with its variables.
Light::Light(unsigned int type, glm::vec3 position, RawModelMaterial* material,
    float size) {
    this->position = position;
    this->type = type;
    this->material = material;
    this->size = glm::vec3(size, size, size);
}

Light::~Light() {}

// Set a light's type.
void Light::setType(unsigned int type) { this->type = type; }

// Set a light's position.
void Light::move(glm::vec3 movement) {
    this->position = this->position + movement;
}

void Light::moveToward(float time, glm::vec3 pos, glm::vec3 toward, float speed) {
	this->position = Movable::moveToward(time, pos, toward, speed);
}

// Get a light's position.
glm::vec3 Light::getPosition() { return this->position; }

// Render a simple model to give a hint as what the light is.
void Light::render(unsigned int shader, glm::vec3 offset,
    glm::mat4 model_matrix, glm::mat4* objectToWorldMatrix, glm::mat4* projectionMatrix, glm::mat4* cameraToWorldMatrix, glm::mat4* modelViewProjectionMatrix, glm::mat3* objectToWorldNormalMatrix, GLuint uniformBindingPoint, GLuint uniformBlock, GLint uniformOffset[]) {
    // If it is a point light, draw a sphere.
    if (this->type == LIGHT_OMNI) {
        RawModelFactory::renderModel(RAW_MODEL_SPHERE, material,
            this->position + offset, this->size, model_matrix, glm::mat4(),
            shader, objectToWorldMatrix, projectionMatrix, cameraToWorldMatrix, modelViewProjectionMatrix, objectToWorldNormalMatrix, uniformBindingPoint, uniformBlock, uniformOffset);
    } // If it is a spotlight, draw a cone.
    else {
        RawModelFactory::renderModel(RAW_MODEL_CONE, material,
            this->position + offset, this->size, model_matrix, glm::mat4(),
            shader, objectToWorldMatrix, projectionMatrix, cameraToWorldMatrix, modelViewProjectionMatrix, objectToWorldNormalMatrix, uniformBindingPoint, uniformBlock, uniformOffset);
    }
}

// Assign variables, initialize the simple random number generator from C++.
LightSystem::LightSystem(unsigned int type, Camera* camera)
: Movable(glm::vec3(0, 0, 0), camera->forward, camera->right, camera->up) {
    this->type = type;
    this->setRelativePosition(camera->position + glm::vec3(0,5.0f,0));
    this->light_count = 0;
    this->fog = true;

    // Initialize random seed.
    srand((unsigned int)time(NULL));
}

// Deconstructor.
LightSystem::~LightSystem() {}

// Adds a new light to the system.
void LightSystem::addLight(glm::vec3 cameraPosition) {
    // Add a light only if there's still enough space.
    if (this->light_count < LIGHT_MAXIMUM_COUNT) {
		glm::vec3 position = cameraPosition;

        // Compute the basic color.
        glm::vec4 color = glm::vec4(
            LIGHT_MINIMUM_COLOR.r + (rand() % 100) / 100.0f * LIGHT_RANGE_COLOR.r,
            LIGHT_MINIMUM_COLOR.g + (rand() % 100) / 100.0f * LIGHT_RANGE_COLOR.g,
            LIGHT_MINIMUM_COLOR.b + (rand() % 100) / 100.0f * LIGHT_RANGE_COLOR.b,
            LIGHT_MINIMUM_COLOR.a + (rand() % 100) / 100.0f * LIGHT_RANGE_COLOR.a);

        // Compute the size.
        float size = LIGHT_MINIMUM_SIZE +
            (rand() % 100) / 100.0f * LIGHT_MAXIMUM_SIZE;

        // Compute the spotlight angles.
        float inner_angle = LIGHT_MINIMUM_SPOT_ANGLE +
            (rand() % 100) / 100.0f * LIGHT_RANGE_SPOT_ANGLE;
        float outer_angle = inner_angle + LIGHT_FADE_SPOT_ANGLE;

        // Assign the material, based on the color.
        RawModelMaterial* material = new RawModelMaterial(LIGHT_SHININESS,
            color * 1.2f, color, color, color * 1.4f);

        Light* new_light = new Light(this->type, position, material, size);

        // Assign the light variables, for later use.
        this->lights[this->light_count] = new_light;
        this->light_colors[this->light_count] = color;
        this->light_sizes[this->light_count] = size;
        this->light_inner_angles[this->light_count] = glm::cos(inner_angle);
        this->light_outer_angles[this->light_count] = glm::cos(outer_angle);

        this->light_count++;
    }
}

// Switch the lighting system from point to spotlight and vice-versa.
void LightSystem::switchType() {
    if (this->type == LIGHT_OMNI) this->type = LIGHT_SPOT;
    else this->type = LIGHT_OMNI;

    for (int i = 0; i < this->light_count; i++) {
        this->lights[i]->setType(type);
    }
}

// Update the system's relative position.
void LightSystem::setRelativePosition(glm::vec3 position) {
    this->relative_position = position;
}


void LightSystem::move(float time, glm::vec3 camPos, float speed) {
    glm::vec3 movement = Movable::move(time, glm::vec2(0, 0)); // Move the light system on the intended path. The system is actually rendered based on the relative position.

    // Move all the lights when not close to player
    for (int i = 0; i < this->light_count; i++) {
		if (glm::distance(camPos, this->lights[i]->getPosition()) >= 2.0f) {
			this->lights[i]->moveToward(time, this->lights[i]->getPosition(), camPos, speed);
		}
        this->lights[i]->move(movement);
    }
}

// Switch the fog on and off.
void LightSystem::switchFog() { this->fog = !this->fog; }

// Render the light system.
void LightSystem::render(unsigned int shader, glm::mat4 model_matrix, glm::mat4* objectToWorldMatrix, glm::mat4* projectionMatrix, glm::mat4* cameraToWorldMatrix, glm::mat4* modelViewProjectionMatrix, glm::mat3* objectToWorldNormalMatrix, GLuint uniformBindingPoint, GLuint uniformBlock, GLint uniformOffset[]) {
    glm::vec3 offset = this->relative_position;

    // Turn the fog on or off and send fog variables.
    glUniform1i(glGetUniformLocation(shader, "fog_switch"), this->fog);
	glUniform1i(glGetUniformLocation(shader, "light_type"), this->type);
    

    // Render all the individual light models.
    for (int i = 0; i < this->light_count; i++) {
        this->lights[i]->render(shader, offset, model_matrix, objectToWorldMatrix, projectionMatrix, cameraToWorldMatrix, modelViewProjectionMatrix, objectToWorldNormalMatrix, uniformBindingPoint, uniformBlock, uniformOffset);
        this->light_positions[i] = offset + this->lights[i]->getPosition();
    }

    // Send light sources information to the shader.
    glUniform1i(glGetUniformLocation(shader, "light_count"), this->light_count);
    //glUniform1i(glGetUniformLocation(shader, "light_type"), this->type);
    glUniform3fv(glGetUniformLocation(shader, "light_positions"),
        this->light_count, (GLfloat*)this->light_positions);
    glUniform4fv(glGetUniformLocation(shader, "light_colors"),
        this->light_count, (GLfloat*)this->light_colors);
    glUniform1fv(glGetUniformLocation(shader, "light_inner_angles"),
        this->light_count, (GLfloat*)this->light_inner_angles);
    glUniform1fv(glGetUniformLocation(shader, "light_outer_angles"),
        this->light_count, (GLfloat*)this->light_outer_angles);
    glUniform1fv(glGetUniformLocation(shader, "light_sizes"),
        this->light_count, (GLfloat*)this->light_sizes);
}
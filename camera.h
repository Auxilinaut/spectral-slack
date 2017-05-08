/**
* Credit of original goes to Stamate Cosmin
*
* Description: Camera entity, based on the entity class.
*/

#pragma once

#include "entity.h"
#include "glm\glm.hpp"
#include "glm\gtc\type_ptr.hpp"

class Camera : public Entity {
public:
    Camera();
    Camera(glm::vec3 position, glm::vec3 forward, glm::vec3 right, glm::vec3 up);
    ~Camera();
};
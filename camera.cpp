/**
* Credit of original goes to Stamate Cosmin
*
* Description: Camera entity, based on the entity class. Used mainly for
* the principal viewer camera.
*/

#pragma once
#include "camera.h"

// Default constructor.
Camera::Camera() : Entity() {}

// Constructor with specified vectors.
Camera::Camera(glm::vec3 position, glm::vec3 forward, glm::vec3 right,
    glm::vec3 up)
    : Entity(position, forward, right, up) {}

// Deconstructor.
Camera::~Camera() {}
/**
* Credit of original goes to Stamate Cosmin
*
* Description: Entity on X, Y, Z axes.
*/

#pragma once
#include "entity.h"

// Initialize vectors and control signals.
void Entity::initialize(glm::vec3 position, glm::vec3 forward, glm::vec3 right,
    glm::vec3 up) {
    this->position = position;
    this->forward = forward;
    this->right = right;
    this->up = up;
    this->z_angle = 0;

    for (int i = 0; i < ENTITY_AXIS_COUNT; i++)
        this->controls[i] = ENTITY_NO_CONTROL;
}
Entity::~Entity() {}

// Constructors.
Entity::Entity(){
    this->initialize(glm::vec3(0, 200, 0), glm::vec3(-1, 0, 0),
        glm::vec3(0, 0, -1), glm::vec3(0, 1, 0));
}
Entity::Entity(glm::vec3 position, glm::vec3 forward, glm::vec3 right,
    glm::vec3 up) {
    this->initialize(position, forward, right, up);
}

// Move the entity taking the signals into account.
glm::vec3 Entity::move(float time, glm::vec2 angles) {
    float distance = time * ENTITY_MOVE_SPEED;
    float direction_distance;
    glm::vec3 previous_position = this->position;

    // Test each signal and translate in corresponding direction.
    for (int i = 0; i < ENTITY_AXIS_COUNT; i++) {
        if (this->controls[i] != ENTITY_NO_CONTROL) {
            direction_distance = distance *
                ENTITY_CONTROL_AXIS_DIRECTION[this->controls[i]];

            switch (i){
            case ENTITY_X_AXIS: this->translateX(direction_distance); break;
            case ENTITY_Y_AXIS: this->translateY(direction_distance); break;
            case ENTITY_Z_AXIS: this->translateZ(direction_distance); break;
            }
        }
    }

    // We only rotate on Y and Z axes.
    this->rotateY(angles.x);
    this->rotateZ(angles.y);

    // Return the distance travelled.
    return this->position - previous_position;
}

glm::vec3 Entity::moveToward(float time, glm::vec3 pos, glm::vec3 toward, float speed) {
	glm::vec3 direction = toward - pos;
	direction = glm::normalize(direction);
	return pos + (direction * speed);
}

// Translate entity forward, up or right.
void Entity::translateX(float distance){
    this->position += this->forward * distance;
}
void Entity::translateY(float distance){
    this->position += this->up * distance;
}
void Entity::translateZ(float distance){
    this->position += this->right * distance;
}

// Rotate the entity on yaw and pitch.
void Entity::rotateY(float angle){
    // In order to avoid rotating on X axis, we first have to revert the Z
    // rotation, rotate on Y and then rotate back to the original Z offset.
    this->rotateRawZ(-this->z_angle);
    this->forward = glm::rotate(this->forward, angle, this->up);
    this->right = glm::rotate(this->right, angle, this->up);
    this->rotateRawZ(this->z_angle);
}
void Entity::rotateZ(float angle){
    this->z_angle += angle;
    this->rotateRawZ(angle);
}
void Entity::rotateRawZ(float angle){
    this->forward = glm::rotate(this->forward, angle, this->right);
    this->up = glm::rotate(this->up, angle, this->right);
}

// Activate / de-activate controls.
void Entity::setControl(int control) {
    this->controls[ENTITY_CONTROL_AXIS[control]] = control;
}
void Entity::unsetControl(int control) {
    this->controls[ENTITY_CONTROL_AXIS[control]] = ENTITY_NO_CONTROL;
}

// Retrieve various vectors.
glm::vec3 Entity::getPosition() { return this->position; }
glm::vec3 Entity::getForward() { return this->forward; }
glm::vec3 Entity::getRight() { return this->right; }
glm::vec3 Entity::getUp() { return this->up; }
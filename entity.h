/**
* Credit of original goes to Stamate Cosmin
*
* Description: Entity on X, Y, Z axes.
*/

#pragma once

#include "glm\glm.hpp"
#include "glm\gtx\rotate_vector.hpp"

#define ENTITY_MOVE_SPEED 100

// Axes identifiers
#define ENTITY_X_AXIS 0
#define ENTITY_Y_AXIS 1
#define ENTITY_Z_AXIS 2

#define ENTITY_AXIS_COUNT 3

// Control identifiers.
#define ENTITY_NO_CONTROL -1
#define ENTITY_CONTROL_FORWARD 0
#define ENTITY_CONTROL_BACKWARD 1
#define ENTITY_CONTROL_UP 2
#define ENTITY_CONTROL_DOWN 3
#define ENTITY_CONTROL_RIGHT 4
#define ENTITY_CONTROL_LEFT 5

// Link control identifiers to axis identifiers.
const int ENTITY_CONTROL_AXIS[] = {
    ENTITY_X_AXIS, ENTITY_X_AXIS,
    ENTITY_Y_AXIS, ENTITY_Y_AXIS,
    ENTITY_Z_AXIS, ENTITY_Z_AXIS
};

// Axis movement direction.
const int ENTITY_CONTROL_AXIS_DIRECTION[] = { 1, -1, 1, -1, 1, -1 };

class Entity {
public:
    Entity();
    Entity(glm::vec3 position, glm::vec3 forward, glm::vec3 right,
        glm::vec3 up);
    ~Entity();

    void initialize(glm::vec3 position, glm::vec3 forward, glm::vec3 right,
        glm::vec3 up);

    glm::vec3 move(float time, glm::vec2 angles);

	glm::vec3 moveToward(float time, glm::vec3 pos, glm::vec3 toward, float speed);

    void translateX(float distance);
    void translateY(float distance);
    void translateZ(float distance);

    void rotateY(float angle);
    void rotateZ(float angle);

    void setControl(int control);
    void unsetControl(int control);

    glm::vec3 getPosition();
    glm::vec3 getForward();
    glm::vec3 getRight();
    glm::vec3 getUp();

private:
    void rotateRawZ(float angle);

public:
    glm::vec3 forward;
    glm::vec3 up;
    glm::vec3 right;
    glm::vec3 position;
    float z_angle;
    int controls[ENTITY_AXIS_COUNT];
};
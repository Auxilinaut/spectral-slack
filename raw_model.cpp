/**
* Credit to Stamate Cosmin
*
* Description: Raw model factory. The factory is a singleton to ensure that
* models are loaded a single time throughout the program.
*/

#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "raw_model.h"

// Load the object at the respective path.
_RawModel::_RawModel(const RawModelInfo* info) {
    this->info = info;
    mesh::loadObj(info->path, this->vao, this->vbo, this->ibo, this->index_count);
}

// Free the memory used by the buffers
_RawModel::~_RawModel() {
    glDeleteBuffers(1, &(this->vao));
    glDeleteBuffers(1, &(this->vbo));
    glDeleteBuffers(1, &(this->ibo));
}

// Render a model based on attributes.
void _RawModel::render(RawModelMaterial* material,
    glm::vec3 position, glm::vec3 size,
    glm::mat4 model_matrix, glm::mat4 transform_matrix,
    unsigned int shader, glm::mat4* objectToWorldMatrix,  glm::mat4 projectionMatrix, glm::mat4 cameraToWorldMatrix, glm::mat4* modelViewProjectionMatrix) {
    // Delegate to the generic render function.
    RawModelFactory::render(this->vao, this->index_count,
        material, position,
        glm::vec3(size.x / this->info->size.x, size.y / this->info->size.y,
        size.z / this->info->size.z),
        model_matrix, transform_matrix, shader, objectToWorldMatrix, projectionMatrix, cameraToWorldMatrix, modelViewProjectionMatrix);
}

// Instantiate all models.
RawModelFactory::RawModelFactory() {
    for (int i = 0; i < RAW_MODEL_COUNT; i++) {
        RawModelFactory::models[i] = new _RawModel(&(RAW_MODELS[i]));
    }
}

// Destruct all models.
RawModelFactory::~RawModelFactory() {
    for (int i = 0; i < RAW_MODEL_COUNT; i++) {
        RawModelFactory::models[i]->~_RawModel();
    }
}

// Initialize static variables.
RawModelFactory* RawModelFactory::instance = 0;
_RawModel* RawModelFactory::models[RAW_MODEL_COUNT];

// Singleton instantiator.
void RawModelFactory::instantiateModelFactory() {
    if (RawModelFactory::instance == 0) {
        RawModelFactory::instance = new RawModelFactory();
    }
}

void RawModelFactory::destructModelFactory() {
    if (RawModelFactory::instance == 0) {
        RawModelFactory::instance->~RawModelFactory();
    }
}

// Render a model based on the requested ID.
void RawModelFactory::renderModel(int model_id, RawModelMaterial* material,
    glm::vec3 position, glm::vec3 size,
    glm::mat4 model_matrix, glm::mat4 transform_matrix,
    unsigned int shader, glm::mat4* objectToWorldMatrix, glm::mat4 projectionMatrix, glm::mat4 cameraToWorldMatrix, glm::mat4* modelViewProjectionMatrix) {
    // Make sure the models are loaded first.
    RawModelFactory::instantiateModelFactory();

    // Render the model.
    RawModelFactory::models[model_id]->render(material, position, size,
        model_matrix, transform_matrix, shader, objectToWorldMatrix, projectionMatrix, cameraToWorldMatrix, modelViewProjectionMatrix);
}

// Render a generic model based on its Vertex Array Object.
void RawModelFactory::render(unsigned int vao, unsigned int index_count,
    RawModelMaterial* material,
    glm::vec3 position, glm::vec3 size,
    glm::mat4 model_matrix, glm::mat4 transform_matrix,
    unsigned int shader, glm::mat4* objectToWorldMatrix, glm::mat4 projectionMatrix, glm::mat4 cameraToWorldMatrix, glm::mat4* modelViewProjectionMatrix) {
    glm::mat4 scale_matrix, translation_matrix;

    // Send material data to the shader.
    glUniform1i(glGetUniformLocation(shader, "material_shininess"),
        material->shininess);
    glUniform4f(glGetUniformLocation(shader, "material_ke"),
        material->ke.r, material->ke.g, material->ke.b, material->ke.a);
    glUniform4f(glGetUniformLocation(shader, "material_ka"),
        material->ka.r, material->ka.g, material->ka.b, material->ka.a);
    glUniform4f(glGetUniformLocation(shader, "material_kd"),
        material->kd.r, material->kd.g, material->kd.b, material->kd.a);
    glUniform4f(glGetUniformLocation(shader, "material_ks"),
        material->ks.r, material->ks.g, material->ks.b, material->ks.a);

    // Compute matrices for scaling and translation.
    //scale_matrix = glm::scale(glm::mat4(), glm::vec3(size.x, size.y, size.z));
    //translation_matrix = glm::translate(model_matrix, position);

    // Send model rendering info via uniform block to shader.
    /*glUniformMatrix4fv(
		1, 1, false,
		glm::value_ptr(model_matrix * translation_matrix * transform_matrix *
		scale_matrix));*/

	/*memcpy(ptr + uniformOffset[1], glm::value_ptr(model_matrix * translation_matrix * transform_matrix *
	scale_matrix), sizeof(glm::mat4));*/

	*objectToWorldMatrix = model_matrix * translation_matrix * transform_matrix *
		scale_matrix;
	*modelViewProjectionMatrix = projectionMatrix * glm::inverse(cameraToWorldMatrix) * *objectToWorldMatrix;


    // Bind VAO buffer and call draw the object.
    glBindVertexArray(vao);
    glDrawElements(GL_TRIANGLES, index_count, GL_UNSIGNED_INT,0);
}
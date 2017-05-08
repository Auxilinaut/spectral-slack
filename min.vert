#version 410 // -*- c++ -*-

// Attributes
layout(location=6) in vec3 position;
layout(location=7) in vec3 normal;
layout(location=8) in vec2 texCoord;

// Interpolated outputs
out Varying {
    vec3        normal;
    vec2        texCoord;
    vec3        position;
} vertexOutput;

// Uniforms
uniform Uniform {
    mat3x3      objectToWorldNormalMatrix;
    mat4x4      objectToWorldMatrix;
    mat4x4      modelViewProjectionMatrix;
    vec3        cameraPosition;
} object;

void main () {
    vertexOutput.texCoord   = texCoord;
    vertexOutput.normal     = normalize(mat3(object.objectToWorldMatrix) * normal);
	vertexOutput.position   = position;

    gl_Position = object.modelViewProjectionMatrix * vec4(position, 1.0);
}

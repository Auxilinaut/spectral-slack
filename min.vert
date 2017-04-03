#version 410 // -*- c++ -*-

// Attributes
layout(location=0) in vec3 position;
layout(location=1) in vec3 normal;
layout(location=2) in vec2 texCoord;

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
    // TODO objectToWorldNormalMatrix is not being passed correctly
    vertexOutput.normal     = normalize(mat3(object.objectToWorldMatrix) * normal);
   vertexOutput.position   = (object.objectToWorldMatrix * vec4(position, 1.0)).xyz;

    gl_Position = object.modelViewProjectionMatrix * vec4(position, 1.0);
}

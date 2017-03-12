#version 410 // -*- c++ -*-

// Attributes
in vec3         position;
in vec3         normal;
in vec2         texCoord;

// Interpolated outputs
out Varying {
    vec3        normal;
    vec2        texCoord;
    vec3        position;
} vertexOutput;

// Uniforms
layout(shared) uniform Uniform {
    mat3x3      objectToWorldNormalMatrix;
    mat4x4      objectToWorldMatrix;
    mat4x4      modelViewProjectionMatrix;
    vec3        cameraPosition;
} object;

void main () {
    vertexOutput.texCoord   = texCoord;
    // TODO objectToWorldNormalMatrix is not being passed correctly
    vertexOutput.normal     = normalize(object.objectToWorldNormalMatrix * normal);
   vertexOutput.position   = (object.objectToWorldMatrix * vec4(position, 1.0)).xyz;

    gl_Position = object.modelViewProjectionMatrix * vec4(position, 1.0);
}

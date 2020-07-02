#version 150

in vec3 position;
in vec3 normal;
out vec3 viewPosition;
out vec3 viewNormal;
out vec3 viewLightDirection;

uniform vec3 lightDirection;
uniform mat4 viewMatrix;
uniform mat4 modelViewMatrix;
uniform mat4 projectionMatrix;
uniform mat4 normalMatrix;

void main()
{
// view-space position of the vertex
  vec4 viewPosition4 = modelViewMatrix * vec4(position, 1.0f);
  viewPosition = viewPosition4.xyz;
// final position in the normalized device coordinates space
gl_Position = projectionMatrix * viewPosition4;
viewLightDirection = (viewMatrix*vec4(lightDirection, 0.0f)).xyz;
viewNormal = normalize((normalMatrix*vec4(normal, 0.0f)).xyz);
}


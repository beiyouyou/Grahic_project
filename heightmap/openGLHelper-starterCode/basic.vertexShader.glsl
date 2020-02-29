#version 150

in vec3 position;
in vec3 left;
in vec3 right;
in vec3 up;
in vec3 down;

in vec4 color;
out vec4 col;

uniform mat4 modelViewMatrix;
uniform mat4 projectionMatrix;
uniform bool mode = false; 

void main()
{
  // compute the transformed and projected vertex position (into gl_Position) 
  // compute the vertex color (into col)
  //execute mode 4
  if(mode){
    vec3 newpos = (left + right + up + down)/4.0f;
    gl_Position = projectionMatrix * modelViewMatrix * vec4(newpos, 1.0f);
    col = newpos.z * max(color, vec4(0.00000004f)) / max(position.z, 0.00001f); 
  }
  else{
    gl_Position = projectionMatrix * modelViewMatrix * vec4(position, 1.0f);
    col = color;
  }

}


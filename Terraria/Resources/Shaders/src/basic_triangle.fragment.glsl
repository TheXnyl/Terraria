#version 460

layout(location = 0) in vec3 oVertexColor;
layout(location = 0) out vec4 oColor;

void main()
{
  oColor = vec4(oVertexColor, 1.0);
}

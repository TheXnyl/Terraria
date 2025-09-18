#version 460

const vec2 positions[3] = { { 0.0, -0.5 }, { 0.5, 0.5 }, { -0.5, 0.5 } };
vec3 colors[3] = { { 1.0, 0.0, 0.0 }, { 0.0, 1.0, 0.0 }, { 0.0, 0.0, 1.0 } };

layout(location = 0) out vec3 oVertexColor;

void main()
{
  gl_Position = vec4(positions[gl_VertexIndex], 0.0, 1.0);
  oVertexColor = colors[gl_VertexIndex];
}

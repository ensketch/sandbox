#include <ensketch/sandbox/viewer.hpp>
//
#include <ensketch/sandbox/application.hpp>
#include <ensketch/sandbox/hyper_surface_smoothing.hpp>
#include <ensketch/sandbox/ray_tracer.hpp>
//
#include <ensketch/opengl/shader_object.hpp>
#include <ensketch/opengl/shader_program.hpp>
#include <ensketch/opengl/vertex_array.hpp>
//
#include <geometrycentral/surface/flip_geodesics.h>
#include <geometrycentral/surface/halfedge_element_types.h>
//
#include <igl/avg_edge_length.h>
//
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

namespace ensketch::sandbox {

namespace {
void /*APIENTRY*/ glDebugOutput(GLenum source,
                                GLenum type,
                                unsigned int id,
                                GLenum severity,
                                GLsizei length,
                                const char* message,
                                const void* userParam) {
  ensketch::sandbox::app().info(message);
}
}  // namespace

// OpenGL will be set to 4.6.
//
sf::ContextSettings viewer::opengl_context_settings{
    /*.depthBits = */ 24,
    /*.stencilBits = */ 8,
    /*.antialiasingLevel = */ 4,
    /*.majorVersion = */ 4,
    /*.minorVersion = */ 6,
    /*.attributeFlags = */
    sf::ContextSettings::Core | sf::ContextSettings::Debug,
    /*.sRgbCapable = */ false};

void viewer::open(int width, int height) {
  if (is_open()) close();

  window.create(sf::VideoMode(width, height),  //
                "ensketch-sandbox",            //
                sf::Style::Default,            //
                opengl_context_settings);

  window.setVerticalSyncEnabled(true);
  window.setKeyRepeatEnabled(false);
  window.setActive(true);

  _running = true;
  init();

  // const auto s = window.getSize();
  resize(width, height);
}

void viewer::close() {
  free();
  window.close();
  _running = false;
}

void viewer::init() {
  glbinding::initialize(sf::Context::getFunction);

  // int flags;
  // glGetIntegerv(GL_CONTEXT_FLAGS, &flags);
  // if (flags & static_cast<int>(GL_CONTEXT_FLAG_DEBUG_BIT)) {
  //   glEnable(GL_DEBUG_OUTPUT);
  //   glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
  //   glDebugMessageCallback(glDebugOutput, nullptr);
  //   glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr,
  //                         GL_TRUE);
  // }

  glEnable(GL_DEPTH_TEST);
  glEnable(GL_MULTISAMPLE);
  glEnable(GL_POINT_SMOOTH);
  glEnable(GL_POINT_SPRITE);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glPointSize(10.0f);
  glClearColor(1.0f, 1.0f, 1.0f, 1.0f);

  device = device_storage{};

  const auto vs = opengl::vertex_shader{"#version 460 core\n",  //
                                        R"##(
uniform mat4 projection;
uniform mat4 view;

layout (location = 0) in vec3 p;
layout (location = 1) in vec3 n;
layout (location = 2) in float f;

out vec3 position;
out vec3 normal;
out float field;

void main() {
  gl_Position = projection * view * vec4(p, 1.0);
  position = vec3(view * vec4(p, 1.0));
  normal = vec3(view * vec4(n, 0.0));
  field = f;
}
)##"};

  const auto gs = opengl::geometry_shader{R"##(
#version 460 core

uniform mat4 view;
uniform mat4 viewport;

layout (triangles) in;
layout (triangle_strip, max_vertices = 3) out;

in vec3 position[];
in vec3 normal[];
in float field[];

out vec3 pos;
out vec3 nor;
out vec3 vnor;
noperspective out vec3 edge_distance;
out float phi;

void main(){
  vec3 p0 = vec3(viewport * (gl_in[0].gl_Position /
                             gl_in[0].gl_Position.w));
  vec3 p1 = vec3(viewport * (gl_in[1].gl_Position /
                             gl_in[1].gl_Position.w));
  vec3 p2 = vec3(viewport * (gl_in[2].gl_Position /
                             gl_in[2].gl_Position.w));

  float a = length(p1 - p2);
  float b = length(p2 - p0);
  float c = length(p1 - p0);

  vec3 n = normalize(cross(gl_in[1].gl_Position.xyz - gl_in[0].gl_Position.xyz, gl_in[2].gl_Position.xyz - gl_in[0].gl_Position.xyz));

  float alpha = acos((b * b + c * c - a * a) / (2.0 * b * c));
  float beta  = acos((a * a + c * c - b * b) / (2.0 * a * c));

  float ha = abs(c * sin(beta));
  float hb = abs(c * sin(alpha));
  float hc = abs(b * sin(alpha));

  gl_PrimitiveID = gl_PrimitiveIDIn;

  edge_distance = vec3(ha, 0, 0);
  nor = n;
  vnor = normal[0];
  pos = position[0];
  phi = field[0];
  gl_Position = gl_in[0].gl_Position;
  EmitVertex();

  edge_distance = vec3(0, hb, 0);
  nor = n;
  vnor = normal[1];
  pos = position[1];
  phi = field[1];
  gl_Position = gl_in[1].gl_Position;
  EmitVertex();

  edge_distance = vec3(0, 0, hc);
  nor = n;
  vnor = normal[2];
  pos = position[2];
  phi = field[2];
  gl_Position = gl_in[2].gl_Position;
  EmitVertex();

  EndPrimitive();
}
)##"};

  const auto fs = opengl::fragment_shader{R"##(
#version 460 core

// uniform bool lighting = true;

in vec3 pos;
in vec3 nor;
in vec3 vnor;
noperspective in vec3 edge_distance;
in float phi;

layout (location = 0) out vec4 frag_color;

layout (std430, binding = 0) readonly buffer ssbo {
  float label[];
};

float colormap_h(float x) {
  if (x < 0.1151580585723306) {
    return (2.25507158009032E+00 * x - 1.17973110308697E+00) * x + 7.72551618145170E-01; // H1
  } else if (x < (9.89643667779019E-01 - 6.61604251019618E-01) / (2.80520737708568E+00 - 1.40111938331467E+00)) {
    return -2.80520737708568E+00 * x + 9.89643667779019E-01; // H2
  } else if (x < (6.61604251019618E-01 - 4.13849520734156E-01) / (1.40111938331467E+00 - 7.00489176507247E-01)) {
    return -1.40111938331467E+00 * x + 6.61604251019618E-01; // H3
  } else if (x < (4.13849520734156E-01 - 2.48319927251200E-01) / (7.00489176507247E-01 - 3.49965224045823E-01)) {
    return -7.00489176507247E-01 * x + 4.13849520734156E-01; // H4
  } else {
    return -3.49965224045823E-01 * x + 2.48319927251200E-01; // H5
  }
}

float colormap_v(float x) {
  float v = 1.0;
  if (x < 0.5) {
    v = clamp(2.10566088679245E+00 * x + 7.56360684411500E-01, 0.0, 1.0);
  } else {
    v = clamp(-1.70132918347782E+00 * x + 2.20637371757606E+00, 0.0, 1.0);
  }
  float period = 4.0 / 105.0;
  float len = 3.0 / 252.0;
  float t = mod(x + 7.0 / 252.0, period);
  if (0.0 <= t && t < len) {
    if (x < 0.12) {
      v = (1.87862631683169E+00 * x + 6.81498517051705E-01);
    } else if (x < 0.73) {
      v -= 26.0 / 252.0;
    } else {
      v = -1.53215278202992E+00 * x + 1.98649818445446E+00;
    }
  }
  return v;
}

// H1 - H2 = 0
// => [x=-0.8359672286003642,x=0.1151580585723306]

vec4 colormap_hsv2rgb(float h, float s, float v) {
  float r = v;
  float g = v;
  float b = v;
  if (s > 0.0) {
    h *= 6.0;
    int i = int(h);
    float f = h - float(i);
    if (i == 1) {
      r *= 1.0 - s * f;
      b *= 1.0 - s;
    } else if (i == 2) {
      r *= 1.0 - s;
      b *= 1.0 - s * (1.0 - f);
    } else if (i == 3) {
      r *= 1.0 - s;
      g *= 1.0 - s * f;
    } else if (i == 4) {
      r *= 1.0 - s * (1.0 - f);
      g *= 1.0 - s;
    } else if (i == 5) {
      g *= 1.0 - s;
      b *= 1.0 - s * f;
    } else {
      g *= 1.0 - s * (1.0 - f);
      b *= 1.0 - s;
    }
  }
  return vec4(r, g, b, 1.0);
}

vec4 colormap(float x) {
  float h = colormap_h(clamp(x, 0.0, 1.0));
  float s = 1.0;
  float v = colormap_v(clamp(x, 0.0, 1.0));
  return colormap_hsv2rgb(h, s, v);
}

void main() {
  // vec3 n = normalize(normal);
  // vec3 view_dir = vec3(0.0, 0.0, 1.0);
  // vec3 light_dir = view_dir;
  // float d = max(dot(light_dir, n), 0.0);
  // vec3 color = vec3(vec2(0.2 + 1.0 * pow(d,1000) + 0.75 * pow(d,0.2)),1.0);
  // frag_color = vec4(color, 1.0);

  // Compute distance from edges.
  float d = min(edge_distance.x, edge_distance.y);
  d = min(d, edge_distance.z);
  float line_width = 0.01;
  float line_delta = 1.0;
  float alpha = 1.0;
  vec4 line_color = vec4(vec3(0.5), alpha);
  float mix_value =
      smoothstep(line_width - line_delta, line_width + line_delta, d);
  // float mix_value = 1.0;
  // Compute viewer shading.
  float s = abs(normalize(nor).z);
  // float s = abs(normalize(vnor).z);
  float light = 0.2 + 1.0 * pow(s, 1000) + 0.75 * pow(s, 0.2);
  // float light = 0.2 + 0.75 * pow(s, 0.2);
  vec4 light_color = vec4(vec3(light), alpha);
  // Mix both color values.
  float transition = 0.5;
  // light_color = mix(vec4(0.0, 0.0, 0.0, 1.0), light_color, smoothstep(phi, -10.0, 10.0));
  // if (!lighting) light_color = color;
  frag_color = mix(line_color, light_color, mix_value);
  // if (mix_value > 0.9) discard;
  //   frag_color = (1 - mix_value) * line_color;

  // if (label[gl_PrimitiveID] > 0)
  //   frag_color = mix(frag_color, vec4(1.0, 0.0, 0.0, 1.0), 0.5);
  // else if (label[gl_PrimitiveID] < 0)
  //   frag_color = mix(frag_color, vec4(0.0, 0.0, 1.0, 1.0), 0.5);
}
)##"};

  if (!vs) {
    app().error(vs.info_log());
    app().close_viewer();
    return;
  }

  if (!gs) {
    app().error(gs.info_log());
    app().close_viewer();
    return;
  }

  if (!fs) {
    app().error(fs.info_log());
    app().close_viewer();
    return;
  }

  device->shader.attach(vs);
  device->shader.attach(gs);
  device->shader.attach(fs);
  device->shader.link();

  if (!device->shader.linked()) {
    app().error(device->shader.info_log());
    app().close_viewer();
    return;
  }

  const auto level_set_vs = opengl::vertex_shader{R"##(
#version 460 core

uniform mat4 projection;
uniform mat4 view;

layout (location = 0) in vec3 p;
layout (location = 2) in float f;

out float field;

void main() {
  gl_Position = projection * view * vec4(p, 1.0);
  field = f;
}
)##"};

  const auto level_set_gs = opengl::geometry_shader{R"##(
#version 460 core

uniform float line_width;

uniform float screen_width;
uniform float screen_height;

layout (triangles) in;
layout (triangle_strip, max_vertices = 12) out;

in float field[];

noperspective out vec2 uv;

void main(){
  vec4 a = gl_in[0].gl_Position;
  vec4 b = gl_in[1].gl_Position;
  vec4 c = gl_in[2].gl_Position;

  float sa = field[0];
  float sb = field[1];
  float sc = field[2];

  vec4 position[2];
  int index = 0;

  if (sa * sb < 0) {
    position[index++] = ((abs(sb) * a + abs(sa) * b) / (abs(sa) + abs(sb)));
    // EmitVertex();
  }
  if (sa * sc < 0) {
    position[index++] = ((abs(sc) * a + abs(sa) * c) / (abs(sa) + abs(sc)));
    // EmitVertex();
  }
  if (sb * sc < 0) {
    position[index++] = ((abs(sc) * b + abs(sb) * c) / (abs(sb) + abs(sc)));
    // EmitVertex();
  }
  // EndPrimitive();

  if (index < 2) return;

  float width = line_width;

  vec4 pos1 = position[0] / position[0].w;
  vec4 pos2 = position[1] / position[1].w;

  vec2 p = vec2(0.5 * screen_width * pos1.x, 0.5 * screen_height * pos1.y);
  vec2 q = vec2(0.5 * screen_width * pos2.x, 0.5 * screen_height * pos2.y);

  vec2 d = normalize(q - p);
  vec2 n = vec2(-d.y, d.x);
  float delta = 0.5 * width;

  vec2 t = vec2(0);

  t = p - delta * n;
  uv = vec2(0.0, -1.0);
  gl_Position = vec4(2.0 * t.x / screen_width,
                     2.0 * t.y / screen_height,
                     pos1.z, 1.0);
  EmitVertex();
  t = p + delta * n;
  uv = vec2(0.0, 1.0);
  gl_Position = vec4(2.0 * t.x / screen_width,
                     2.0 * t.y / screen_height,
                     pos1.z, 1.0);
  EmitVertex();
  t = p - delta * n - delta * d;
  uv = vec2(-1.0, -1.0);
  gl_Position = vec4(2.0 * t.x / screen_width,
                     2.0 * t.y / screen_height,
                     pos1.z, 1.0);
  EmitVertex();
  t = p + delta * n - delta * d;
  uv = vec2(-1.0, 1.0);
  gl_Position = vec4(2.0 * t.x / screen_width,
                     2.0 * t.y / screen_height,
                     pos1.z, 1.0);
  EmitVertex();
  EndPrimitive();

  t = q - delta * n;
  uv = vec2(0.0, -1.0);
  gl_Position = vec4(2.0 * t.x / screen_width,
                     2.0 * t.y / screen_height,
                     pos2.z, 1.0);
  EmitVertex();
  t = q + delta * n;
  uv = vec2(0.0, 1.0);
  gl_Position = vec4(2.0 * t.x / screen_width,
                     2.0 * t.y / screen_height,
                     pos2.z, 1.0);
  EmitVertex();
  t = q - delta * n + delta * d;
  uv = vec2(1.0, -1.0);
  gl_Position = vec4(2.0 * t.x / screen_width,
                     2.0 * t.y / screen_height,
                     pos2.z, 1.0);
  EmitVertex();
  t = q + delta * n + delta * d;
  uv = vec2(1.0, 1.0);
  gl_Position = vec4(2.0 * t.x / screen_width,
                     2.0 * t.y / screen_height,
                     pos2.z, 1.0);
  EmitVertex();
  EndPrimitive();


  t = p - delta * n;
  uv = vec2(0.0, -1.0);
  gl_Position = vec4(2.0 * t.x / screen_width,
                     2.0 * t.y / screen_height,
                     pos1.z, 1.0);
  EmitVertex();
  t = q - delta * n;
  uv = vec2(0.0, -1.0);
  gl_Position = vec4(2.0 * t.x / screen_width,
                     2.0 * t.y / screen_height,
                     pos2.z, 1.0);
  EmitVertex();
  t = p + delta * n;
  uv = vec2(0.0, 1.0);
  gl_Position = vec4(2.0 * t.x / screen_width,
                     2.0 * t.y / screen_height,
                     pos1.z, 1.0);
  EmitVertex();
  t = q + delta * n;
  uv = vec2(0.0, 1.0);
  gl_Position = vec4(2.0 * t.x / screen_width,
                     2.0 * t.y / screen_height,
                     pos2.z, 1.0);
  EmitVertex();
  EndPrimitive();
}
)##"};

  const auto level_set_fs = opengl::fragment_shader{R"##(
#version 460 core

uniform vec3 line_color;

noperspective in vec2 uv;

layout (location = 0) out vec4 frag_color;

void main() {
  // frag_color = vec4(0.1, 0.5, 0.9, 1.0);
  if (length(uv) >= 1.0) discard;
    frag_color = vec4(line_color, 1.0);
}
)##"};

  if (!level_set_vs) {
    app().error(level_set_vs.info_log());
    app().close_viewer();
    return;
  }

  if (!level_set_gs) {
    app().error(level_set_gs.info_log());
    app().close_viewer();
    return;
  }

  if (!level_set_fs) {
    app().error(level_set_fs.info_log());
    app().close_viewer();
    return;
  }

  device->level_set_shader.attach(level_set_vs);
  device->level_set_shader.attach(level_set_gs);
  device->level_set_shader.attach(level_set_fs);
  device->level_set_shader.link();

  if (!device->level_set_shader.linked()) {
    app().error(device->level_set_shader.info_log());
    app().close_viewer();
    return;
  }

  const auto point_vs = opengl::vertex_shader{R"##(
#version 460 core

uniform mat4 projection;
uniform mat4 view;

layout (location = 0) in vec3 p;

void main(){
  gl_Position = projection * view * vec4(p, 1.0);
}
)##"};

  const auto point_fs = opengl::fragment_shader{R"##(
#version 460 core

layout (location = 0) out vec4 frag_color;

void main() {
  float alpha = 1.0;
  vec2 tmp = 2.0 * gl_PointCoord - 1.0;
  float r = dot(tmp, tmp);
  float delta = fwidth(0.5 * r);
  alpha = 1.0 - smoothstep(1.0 - delta, 1.0 + delta, r);
  frag_color = vec4(0.1, 0.5, 0.9, alpha);
}
)##"};

  if (!point_vs) {
    app().error(point_vs.info_log());
    app().close_viewer();
    return;
  }

  if (!point_fs) {
    app().error(point_fs.info_log());
    app().close_viewer();
    return;
  }

  device->point_shader.attach(point_vs);
  device->point_shader.attach(point_fs);
  device->point_shader.link();

  if (!device->point_shader.linked()) {
    app().error(device->point_shader.info_log());
    app().close_viewer();
    return;
  }

  device->va.bind();
  device->vertices.bind();
  device->faces.bind();

  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE,
                        sizeof(polyhedral_surface::vertex),
                        (void*)offsetof(polyhedral_surface::vertex, position));
  glEnableVertexAttribArray(1);
  glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE,
                        sizeof(polyhedral_surface::vertex),
                        (void*)offsetof(polyhedral_surface::vertex, normal));

  device->scalar_field.bind();
  glEnableVertexAttribArray(2);
  glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, sizeof(float), (void*)0);

  glBindBuffer(GL_SHADER_STORAGE_BUFFER, device->ssbo.id());
  glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, device->ssbo.id());

  // surface_should_update = true;

  const auto surface_vertex_curve_vs = opengl::vertex_shader{R"##(
#version 460 core

uniform mat4 projection;
uniform mat4 view;

layout (location = 0) in vec3 p;

void main(){
  gl_Position = projection * view * vec4(p, 1.0);
}
)##"};

  const auto surface_vertex_curve_gs = opengl::geometry_shader{R"##(
#version 420 core

uniform float line_width;

uniform float screen_width;
uniform float screen_height;

layout (lines) in;
layout (triangle_strip, max_vertices = 12) out;

noperspective out vec2 uv;

void main(){
  // float width = 10.0;
  float width = line_width;

  vec4 pos1 = gl_in[0].gl_Position / gl_in[0].gl_Position.w;
  vec4 pos2 = gl_in[1].gl_Position / gl_in[1].gl_Position.w;

  vec2 p = vec2(0.5 * screen_width * pos1.x, 0.5 * screen_height * pos1.y);
  vec2 q = vec2(0.5 * screen_width * pos2.x, 0.5 * screen_height * pos2.y);

  vec2 d = normalize(q - p);
  vec2 n = vec2(-d.y, d.x);
  float delta = 0.5 * width;

  vec2 t = vec2(0);

  t = p - delta * n;
  uv = vec2(0.0, -1.0);
  gl_Position = vec4(2.0 * t.x / screen_width,
                     2.0 * t.y / screen_height,
                     pos1.z, 1.0);
  EmitVertex();
  t = p + delta * n;
  uv = vec2(0.0, 1.0);
  gl_Position = vec4(2.0 * t.x / screen_width,
                     2.0 * t.y / screen_height,
                     pos1.z, 1.0);
  EmitVertex();
  t = p - delta * n - delta * d;
  uv = vec2(-1.0, -1.0);
  gl_Position = vec4(2.0 * t.x / screen_width,
                     2.0 * t.y / screen_height,
                     pos1.z, 1.0);
  EmitVertex();
  t = p + delta * n - delta * d;
  uv = vec2(-1.0, 1.0);
  gl_Position = vec4(2.0 * t.x / screen_width,
                     2.0 * t.y / screen_height,
                     pos1.z, 1.0);
  EmitVertex();
  EndPrimitive();

  t = q - delta * n;
  uv = vec2(0.0, -1.0);
  gl_Position = vec4(2.0 * t.x / screen_width,
                     2.0 * t.y / screen_height,
                     pos2.z, 1.0);
  EmitVertex();
  t = q + delta * n;
  uv = vec2(0.0, 1.0);
  gl_Position = vec4(2.0 * t.x / screen_width,
                     2.0 * t.y / screen_height,
                     pos2.z, 1.0);
  EmitVertex();
  t = q - delta * n + delta * d;
  uv = vec2(1.0, -1.0);
  gl_Position = vec4(2.0 * t.x / screen_width,
                     2.0 * t.y / screen_height,
                     pos2.z, 1.0);
  EmitVertex();
  t = q + delta * n + delta * d;
  uv = vec2(1.0, 1.0);
  gl_Position = vec4(2.0 * t.x / screen_width,
                     2.0 * t.y / screen_height,
                     pos2.z, 1.0);
  EmitVertex();
  EndPrimitive();


  t = p - delta * n;
  uv = vec2(0.0, -1.0);
  gl_Position = vec4(2.0 * t.x / screen_width,
                     2.0 * t.y / screen_height,
                     pos1.z, 1.0);
  EmitVertex();
  t = q - delta * n;
  uv = vec2(0.0, -1.0);
  gl_Position = vec4(2.0 * t.x / screen_width,
                     2.0 * t.y / screen_height,
                     pos2.z, 1.0);
  EmitVertex();
  t = p + delta * n;
  uv = vec2(0.0, 1.0);
  gl_Position = vec4(2.0 * t.x / screen_width,
                     2.0 * t.y / screen_height,
                     pos1.z, 1.0);
  EmitVertex();
  t = q + delta * n;
  uv = vec2(0.0, 1.0);
  gl_Position = vec4(2.0 * t.x / screen_width,
                     2.0 * t.y / screen_height,
                     pos2.z, 1.0);
  EmitVertex();
  EndPrimitive();
}
)##"};

  const auto surface_vertex_curve_fs = opengl::fragment_shader{R"##(
#version 460 core

uniform vec3 line_color;

noperspective in vec2 uv;

layout (location = 0) out vec4 frag_color;

void main() {
  // frag_color = vec4(0.1, 0.5, 0.9, 1.0);
  if (length(uv) >= 1.0) discard;
  frag_color = vec4(line_color, 1.0);
}
)##"};

  if (!surface_vertex_curve_vs) {
    app().error(surface_vertex_curve_vs.info_log());
    app().close_viewer();
    return;
  }

  if (!surface_vertex_curve_gs) {
    app().error(surface_vertex_curve_gs.info_log());
    app().close_viewer();
    return;
  }

  if (!surface_vertex_curve_fs) {
    app().error(surface_vertex_curve_fs.info_log());
    app().close_viewer();
    return;
  }

  device->surface_vertex_curve_shader.attach(surface_vertex_curve_vs);
  device->surface_vertex_curve_shader.attach(surface_vertex_curve_gs);
  device->surface_vertex_curve_shader.attach(surface_vertex_curve_fs);
  device->surface_vertex_curve_shader.link();

  if (!device->surface_vertex_curve_shader.linked()) {
    app().error(device->surface_vertex_curve_shader.info_log());
    app().close_viewer();
    return;
  }

  device->surface_mesh_curve_va.bind();
  device->surface_mesh_curve_data.bind();
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(vec3), (void*)0);

  const auto mouse_curve_vs = opengl::vertex_shader{R"##(
#version 460 core

uniform float screen_width;
uniform float screen_height;

layout (location = 0) in vec2 p;

void main(){
  const float x = 2.0 * p.x / screen_width - 1.0;
  const float y = 1.0 - 2.0 * p.y / screen_height;
  gl_Position = vec4(x, y, 0.0, 1.0);
}
)##"};

  const auto mouse_curve_fs = opengl::fragment_shader{R"##(
#version 460 core

layout (location = 0) out vec4 frag_color;

void main() {
  float alpha = 1.0;
  vec2 tmp = 2.0 * gl_PointCoord - 1.0;
  float r = dot(tmp, tmp);
  float delta = fwidth(0.5 * r);
  alpha = 1.0 - smoothstep(1.0 - delta, 1.0 + delta, r);
  frag_color = vec4(0.1, 0.5, 0.9, alpha);
}
)##"};

  if (!mouse_curve_vs) {
    app().error(mouse_curve_vs.info_log());
    app().close_viewer();
    return;
  }

  if (!mouse_curve_fs) {
    app().error(mouse_curve_fs.info_log());
    app().close_viewer();
    return;
  }

  device->mouse_curve_shader.attach(mouse_curve_vs);
  device->mouse_curve_shader.attach(mouse_curve_fs);
  device->mouse_curve_shader.link();

  if (!device->mouse_curve_shader.linked()) {
    app().error(device->mouse_curve_shader.info_log());
    app().close_viewer();
    return;
  }

  device->mouse_curve_va.bind();
  device->mouse_curve_data.bind();
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(vec2), (void*)0);
}

void viewer::free() {
  device.reset();
}

void viewer::resize(int width, int height) {
  glViewport(0, 0, width, height);
  camera.set_screen_resolution(width, height);
  view_should_update = true;
}

void viewer::process_events() {
  // Get new mouse position and compute movement in space.
  const auto new_mouse_pos = sf::Mouse::getPosition(window);
  const auto mouse_move = new_mouse_pos - mouse_pos;
  mouse_pos = new_mouse_pos;

  sf::Event event;
  while (window.pollEvent(event)) {
    if (event.type == sf::Event::Closed)
      app().quit();
    else if (event.type == sf::Event::Resized)
      resize(event.size.width, event.size.height);
    else if (event.type == sf::Event::MouseWheelScrolled)
      zoom(0.1 * event.mouseWheelScroll.delta);
    else if (event.type == sf::Event::MouseButtonPressed) {
      switch (event.mouseButton.button) {
        case sf::Mouse::Middle:
          look_at(event.mouseButton.x, event.mouseButton.y);
          break;
          // case sf::Mouse::Right:
          //   select_surface_vertex_from_mouse(mouse_pos.x, mouse_pos.y);
          //   break;
      }
    } else if (event.type == sf::Event::KeyPressed) {
      switch (event.key.code) {
        case sf::Keyboard::Escape:
          app().quit();
          break;
        case sf::Keyboard::Num1:
          set_y_as_up();
          break;
        case sf::Keyboard::Num2:
          set_z_as_up();
          break;
        case sf::Keyboard::Space:
          mouse_curve_recording = !mouse_curve_recording;
          break;
        case sf::Keyboard::M:
          reset_mouse_curve();
          mouse_curve_recording = false;
          break;
        case sf::Keyboard::P:
          project_mouse_curve_to_surface_vertex_curve();
          break;
        case sf::Keyboard::C:
          close_regular_surface_vertex_curve();
          break;
        case sf::Keyboard::R:
          reset_surface_vertex_curve();
          reset_surface_mesh_curve();
          reset_surface_bipartition();
          reset_surface_scalar_field();
          break;
        case sf::Keyboard::G:
          compute_surface_geodesic();
          break;
        case sf::Keyboard::S:
          compute_smooth_surface_mesh_curve();
          break;
        case sf::Keyboard::Up:
          tolerance *= 1.1f;
          compute_smooth_surface_mesh_curve();
          break;
        case sf::Keyboard::Down:
          tolerance *= 0.9f;
          compute_smooth_surface_mesh_curve();
          break;
        case sf::Keyboard::Right:
          set_heat_time_scale(heat_time_scale * 1.1f);
          compute_smooth_surface_mesh_curve();
          break;
        case sf::Keyboard::Left:
          set_heat_time_scale(heat_time_scale * 0.9f);
          compute_smooth_surface_mesh_curve();
          break;
        case sf::Keyboard::B:
          compute_surface_bipartition_from_surface_vertex_curve();
          break;
        case sf::Keyboard::H:
          compute_hyper_surface_smoothing();
          break;
      }
    }
  }

  if (window.hasFocus()) {
    if (sf::Mouse::isButtonPressed(sf::Mouse::Left)) {
      if (sf::Keyboard::isKeyPressed(sf::Keyboard::LShift))
        shift({mouse_move.x, mouse_move.y});
      else
        turn({-0.01 * mouse_move.x, 0.01 * mouse_move.y});
    }

    if (sf::Mouse::isButtonPressed(sf::Mouse::Right)) {
      mouse_append_surface_vertex_curve(mouse_pos.x, mouse_pos.y);
    }
  }
}

void viewer::update() {
  handle_surface_load_task();

  if (view_should_update) {
    update_view();
    view_should_update = false;
  }

  if (surface_should_update) {
    // surface.update();

    compute_heat_data();

    device->vertices.allocate_and_initialize(surface.vertices);
    device->faces.allocate_and_initialize(surface.faces);

    vector<float> tmp{};
    tmp.assign(surface.faces.size(), 0.0f);
    device->ssbo.allocate_and_initialize(tmp);
    tmp.assign(surface.vertices.size(), 0.0f);
    device->scalar_field.allocate_and_initialize(tmp);

    surface_should_update = false;
  }

  if (mouse_curve_recording) record_mouse_curve();
}

void viewer::update_view() {
  // Compute camera position by using spherical coordinates.
  // This transformation is a variation of the standard
  // called horizontal coordinates often used in astronomy.
  //
  auto p = cos(altitude) * sin(azimuth) * right +  //
           cos(altitude) * cos(azimuth) * front +  //
           sin(altitude) * up;
  p *= radius;
  p += origin;
  camera.move(p).look_at(origin, up);

  camera.set_near_and_far(std::max(1e-3f * radius, radius - bounding_radius),
                          radius + bounding_radius);
  // camera.set_near_and_far(0.1f, 100.0f);

  // shaders.apply([this](opengl::shader_program_handle shader) {
  //   shader.bind()
  //       .try_set("projection", camera.projection_matrix())
  //       .try_set("view", camera.view_matrix())
  //       .try_set("viewport", camera.viewport_matrix())
  //       .try_set("viewport_width", camera.screen_width())
  //       .try_set("viewport_height", camera.screen_height());
  // });

  if (device) {
    device->shader.try_set("projection", camera.projection_matrix());
    device->shader.try_set("view", camera.view_matrix());
    device->shader.try_set("viewport", camera.viewport_matrix());

    device->level_set_shader.try_set("projection", camera.projection_matrix());
    device->level_set_shader.try_set("view", camera.view_matrix());
    device->level_set_shader.try_set("viewport", camera.viewport_matrix());
    device->level_set_shader.set("screen_width", float(camera.screen_width()));
    device->level_set_shader.set("screen_height",
                                 float(camera.screen_height()));

    device->point_shader.try_set("projection", camera.projection_matrix());
    device->point_shader.try_set("view", camera.view_matrix());
    device->point_shader.try_set("viewport", camera.viewport_matrix());

    device->surface_vertex_curve_shader.try_set("projection",
                                                camera.projection_matrix());
    device->surface_vertex_curve_shader.try_set("view", camera.view_matrix());
    device->surface_vertex_curve_shader.try_set("viewport",
                                                camera.viewport_matrix());

    device->surface_vertex_curve_shader.set("screen_width",
                                            float(camera.screen_width()));
    device->surface_vertex_curve_shader.set("screen_height",
                                            float(camera.screen_height()));

    device->mouse_curve_shader.set("screen_width",
                                   float(camera.screen_width()));
    device->mouse_curve_shader.set("screen_height",
                                   float(camera.screen_height()));
  }
}

void viewer::render() {
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  glDepthFunc(GL_LEQUAL);

  device->va.bind();
  device->faces.bind();
  device->shader.use();
  glDrawElements(GL_TRIANGLES, 3 * surface.faces.size(), GL_UNSIGNED_INT, 0);
  // glDrawArrays(GL_TRIANGLES, 0, 3);

  glDepthFunc(GL_ALWAYS);

  device->level_set_shader.set("line_width", 3.5f);
  device->level_set_shader.set("line_color", vec3{0.9, 0.5, 0.1});
  device->level_set_shader.use();
  glDrawElements(GL_TRIANGLES, 3 * surface.faces.size(), GL_UNSIGNED_INT, 0);

  if (!surface_vertex_curve.empty()) {
    device->va.bind();
    device->surface_vertex_curve_data.bind();
    device->surface_vertex_curve_shader.use();
    device->surface_vertex_curve_shader.set("line_width", 3.5f);
    device->surface_vertex_curve_shader.set("line_color", vec3{0.5f});
    glDrawElements(
        GL_LINE_STRIP,
        surface_vertex_curve.size() + ((surface_vertex_curve_closed) ? 1 : 0),
        GL_UNSIGNED_INT, 0);
  }

  if (!surface_mesh_curve.empty()) {
    device->surface_mesh_curve_va.bind();
    device->surface_mesh_curve_data.bind();
    device->surface_vertex_curve_shader.use();
    device->surface_vertex_curve_shader.set("line_width", 3.5f);
    device->surface_vertex_curve_shader.set("line_color", vec3{0.1, 0.5, 0.9});
    glDrawArrays(GL_LINE_STRIP, 0, surface_mesh_curve.size());
  }

  if (selected_vertex != polyhedral_surface::invalid) {
    device->va.bind();
    device->selected_vertices.bind();
    device->point_shader.use();
    glDrawElements(GL_POINTS, 1, GL_UNSIGNED_INT, 0);
  }

  if (!mouse_curve.empty()) {
    device->mouse_curve_va.bind();
    device->mouse_curve_data.bind();
    device->mouse_curve_shader.use();
    glDrawArrays(GL_POINTS, 0, mouse_curve.size());
  }

  window.display();

  if (!store_image_path.empty()) {
    view_should_update = true;
    --store_image_frames;
    if (store_image_frames <= 0) store_image();
  }
}

void viewer::store_image(const filesystem::path& path) {
  store_image_path = path;
  store_image_frames = 3;
}

void viewer::store_image() {
  int width = camera.screen_width();
  int height = camera.screen_height();
  GLsizei nrChannels = 3;
  GLsizei stride = nrChannels * width;
  // stride += (stride % 4) ? (4 - stride % 4) : 0;
  GLsizei bufferSize = stride * height;
  std::vector<char> buffer(bufferSize);
  // glPixelStorei(GL_PACK_ALIGNMENT, 4);
  glPixelStorei(GL_PACK_ALIGNMENT, 1);
  glReadBuffer(GL_FRONT);
  glReadPixels(0, 0, width, height, GL_RGB, GL_UNSIGNED_BYTE, buffer.data());

  stbi_flip_vertically_on_write(true);
  stbi_write_png(store_image_path.c_str(), width, height, nrChannels,
                 buffer.data(), stride);

  app().info(format("Successfully stored view as image.\nfile = '{}'.",
                    store_image_path.string()));

  store_image_path.clear();
  store_image_frames = 0;
}

void viewer::store_image_from_view(const filesystem::path& path) {
  const auto p = app().path_from_lookup(path);

  int width = camera.screen_width();
  int height = camera.screen_height();
  GLsizei nrChannels = 3;
  GLsizei stride = nrChannels * width;
  // stride += (stride % 4) ? (4 - stride % 4) : 0;
  GLsizei bufferSize = stride * height;
  std::vector<char> buffer(bufferSize);
  // glPixelStorei(GL_PACK_ALIGNMENT, 4);
  glPixelStorei(GL_PACK_ALIGNMENT, 1);
  glReadBuffer(GL_FRONT);
  glReadPixels(0, 0, width, height, GL_RGB, GL_UNSIGNED_BYTE, buffer.data());

  stbi_flip_vertically_on_write(true);
  stbi_write_png(p.c_str(), width, height, nrChannels, buffer.data(), stride);

  app().info(format("Successfully stored framebuffer as image.\nfile = '{}'.",
                    p.string()));
}

void viewer::save_perspective(const filesystem::path& path) {
  const auto p = app().path_from_lookup(path);
  ofstream file{p};
  if (!file) {
    app().error(
        format("Failed to save view to file.\nfile = '{}'", p.string()));
    return;
  }
  file << format("{} {} {}\n", origin.x, origin.y, origin.z)
       << format("{} {} {}\n", up.x, up.y, up.z)
       << format("{} {} {}\n", right.x, right.y, right.z)
       << format("{} {} {}\n", front.x, front.y, front.z)
       << format("{} {} {}\n", radius, altitude, azimuth);

  app().info(
      format("Successfully saved view to file.\nfile = '{}'", p.string()));
}

void viewer::load_perspective(const filesystem::path& path) {
  const auto p = app().path_from_lookup(path);
  ifstream file{p};
  if (!file) {
    app().error(
        format("Failed to load view from file.\nfile = '{}'", p.string()));
    return;
  }
  file                                     //
      >> origin.x >> origin.y >> origin.z  //
      >> up.x >> up.y >> up.z              //
      >> right.x >> right.y >> right.z     //
      >> front.x >> front.y >> front.z     //
      >> radius >> altitude >> azimuth;

  app().info(
      format("Successfully loaded view from file.\nfile = '{}'", p.string()));

  view_should_update = true;
}

void viewer::turn(const vec2& angle) {
  altitude += angle.y;
  azimuth += angle.x;
  constexpr float bound = pi / 2 - 1e-5f;
  altitude = std::clamp(altitude, -bound, bound);
  view_should_update = true;
}

void viewer::shift(const vec2& pixels) {
  const auto shift = -pixels.x * camera.right() + pixels.y * camera.up();
  const auto scale = camera.pixel_size() * radius;
  origin += scale * shift;
  view_should_update = true;
}

void viewer::zoom(float scale) {
  radius *= exp(-scale);
  view_should_update = true;
}

void viewer::look_at(float x, float y) {
  const auto r = primary_ray(camera, x, y);
  if (const auto p = intersection(r, surface)) {
    origin = r(p.t);
    radius = p.t;
    view_should_update = true;
  }
}

void viewer::set_z_as_up() {
  right = {1, 0, 0};
  front = {0, -1, 0};
  up = {0, 0, 1};
  view_should_update = true;
}

void viewer::set_y_as_up() {
  right = {1, 0, 0};
  front = {0, 0, 1};
  up = {0, 1, 0};
  view_should_update = true;
}

void viewer::load_surface(const filesystem::path& path) {
  const auto p = app().path_from_lookup(path);
  try {
    const auto load_start = clock::now();

    surface = polyhedral_surface_from(p);
    surface.generate_edges();

    const auto load_end = clock::now();

    // Evaluate loading and processing time.
    surface_load_time = duration(load_end - load_start).count();

    // surface.update();
    fit_view_to_surface();
    print_surface_info();
    compute_surface_topology_and_geometry();

    surface_should_update = true;

    app().info(format("Sucessfully loaded surface mesh from file.\nfile = '{}'",
                      p.string()));

  } catch (exception& e) {
    app().error(
        format("Failed to load surface mesh from file.\n{}\nfile = '{}'",
               e.what(), p.string()));
    return;
  }
}

void viewer::async_load_surface(const filesystem::path& path) {
  if (surface_load_task.valid()) {
    app().error(format(
        "Failed to start asynchronous loading of surface mesh from file.\nIt "
        "seems another surface mesh is already loading.\n file = '{}'",
        path.string()));
    return;
  }
  surface_load_task =
      async(launch::async, [this, &path] { load_surface(path); });
  app().info(format(
      "Started asynchronous loading of surface mesh from file.\nfile = '{}'",
      path.string()));
}

void viewer::handle_surface_load_task() {
  if (!surface_load_task.valid()) return;
  if (future_status::ready != surface_load_task.wait_for(0s)) {
    // cout << "." << flush;
    return;
  }

  app().info("Sucessfully finished asynchronous loading of surface mesh.");

  surface_load_task = {};
}

void viewer::fit_view_to_surface() {
  const auto box = aabb_from(surface);
  origin = box.origin();
  bounding_radius = box.radius();

  // cout << "bounding radius = " << bounding_radius << endl;

  radius = bounding_radius / tan(0.5f * camera.vfov());
  camera.set_near_and_far(1e-4f * radius, 2 * radius);
  view_should_update = true;
}

void viewer::print_surface_info() {
  app().info(format(  //
      "load time = {:6.3f}s\n"
      "process time = {:6.3f}s\n"
      "vertices = {}\n"
      "faces = {}\n",
      surface_load_time, surface_process_time, surface.vertices.size(),
      surface.faces.size()));
}

auto viewer::surface_vertex_from(const mouse_position& m) noexcept
    -> polyhedral_surface::vertex_id {
  const auto r = primary_ray(camera, m.x, m.y);
  const auto p = intersection(r, surface);
  if (!p) return polyhedral_surface::invalid;

  const auto& f = surface.faces[p.f];
  const auto w = real(1) - p.u - p.v;

  const auto position = surface.vertices[f[0]].position * w +
                        surface.vertices[f[1]].position * p.u +
                        surface.vertices[f[2]].position * p.v;

  const auto l0 = length(position - surface.vertices[f[0]].position);
  const auto l1 = length(position - surface.vertices[f[1]].position);
  const auto l2 = length(position - surface.vertices[f[2]].position);

  if (l0 <= l1) {
    if (l0 <= l2)
      return f[0];
    else
      return f[2];
  } else {
    if (l1 <= l2)
      return f[1];
    else
      return f[2];
  }

  return polyhedral_surface::invalid;
}

void viewer::select_surface_vertex_from_mouse(float x, float y) noexcept {
  selected_vertex = surface_vertex_from(mouse_position{x, y});
  if (selected_vertex == polyhedral_surface::invalid) return;
  app().info(format("Vertex ID = {}", selected_vertex));
  if (device)
    device->selected_vertices.allocate_and_initialize(selected_vertex);
}

void viewer::reset_mouse_curve() noexcept {
  mouse_curve.clear();
  // if (device) device->mouse_curve_data.clear();
}

void viewer::record_mouse_curve() noexcept {
  mouse_curve.push_back(vec2{mouse_pos.x, mouse_pos.y});
  if (device) device->mouse_curve_data.allocate_and_initialize(mouse_curve);
}

void viewer::project_mouse_curve_to_surface_vertex_curve() {
  surface_vertex_curve.clear();
  surface_vertex_curve_closed = false;

  for (auto& m : mouse_curve) {
    const auto x = surface_vertex_from(mouse_position{m.x, m.y});

    if (x == polyhedral_surface::invalid) continue;

    if (surface_vertex_curve.empty()) {
      surface_vertex_curve.push_back(x);
      continue;
    }

    const auto p = surface_vertex_curve.back();
    if (x == p) continue;

    {
      using namespace geometrycentral;
      using namespace surface;

      // Get the shortest edge path by using
      // Dijkstra's Algorithm by Geometry Central.
      //
      const auto network = FlipEdgeNetwork::constructFromDijkstraPath(
          *mesh, *geometry, Vertex{mesh.get(), p}, Vertex{mesh.get(), x});
      network->posGeom = geometry.get();
      const auto paths = network->getPathPolyline();

      // Check the path for consistency.
      //
      assert(paths.size() == 1);
      const auto& path = paths[0];
      assert(path.front().vertex.getIndex() == p);
      assert(path.back().vertex.getIndex() == x);

      // Store this path at the end of the current line.
      //
      for (size_t i = 1; i < path.size(); ++i)
        surface_vertex_curve.push_back(path[i].vertex.getIndex());
    }
  }

  regularize_open_surface_vertex_curve();

  if (device)
    device->surface_vertex_curve_data.allocate_and_initialize(
        surface_vertex_curve);
}

void viewer::compute_surface_topology_and_geometry() {
  using namespace geometrycentral;
  using namespace surface;

  // Generate polygon data for constructors.
  //
  vector<vector<size_t>> polygons(surface.faces.size());
  for (size_t i = 0; const auto& f : surface.faces) {
    polygons[i].resize(3);
    for (size_t j = 0; j < 3; ++j) polygons[i][j] = f[j];
    ++i;
  }
  //
  mesh = make_unique<ManifoldSurfaceMesh>(polygons);

  // Generate vertex data for constructors.
  //
  VertexData<Vector3> vertices(*mesh);
  for (size_t i = 0; i < surface.vertices.size(); ++i) {
    vertices[i].x = surface.vertices[i].position.x;
    vertices[i].y = surface.vertices[i].position.y;
    vertices[i].z = surface.vertices[i].position.z;
  }
  //
  geometry = make_unique<VertexPositionGeometry>(*mesh, vertices);
}

void viewer::regularize_open_surface_vertex_curve() {
  // at this point, we assume that the surface vertex curve is valid,
  // i.e. adjacent vertices are connected by an edge (they might be equal).
  // This function may only reduce the amount of vertices.
  // Because of that, it can be run in-place.

  auto& curve = surface_vertex_curve;
  // valid curves are not empty
  // if (curve.empty()) return;

  // first element will be kept the same
  // no self-assignment necessary
  size_t count = 1;

  for (auto x : surface_vertex_curve) {
    // count can never be zero
    // if (count == 0) {
    //   curve[count++] = x;  // push back
    //   continue;
    // }

    const auto p = curve[count - 1];
    if (x == p) continue;

    if (count < 2) {
      curve[count++] = x;  // push back
      continue;
    }

    const auto q = curve[count - 2];
    if (x == q) {
      --count;  // pop back
      continue;
    }

    if (surface.edges.contains({q, x}) || surface.edges.contains({x, q})) {
      curve[count - 1] = x;  // remove previous and push back current
      continue;
    }

    curve[count++] = x;  // push back
  }

  curve.resize(count);
}

void viewer::regularize_closed_surface_vertex_curve() {
  // we assume the curve to be a valid closed vertex curve

  auto& curve = surface_vertex_curve;
  // if (curve.empty()) return;

  regularize_open_surface_vertex_curve();
  size_t count;

  do {
    count = curve.size();
    if (count == 1) return;

    const auto p = curve[count - 1];
    const auto q = curve[count - 2];

    for (auto i = count - 1; i >= 2; --i)  //
      curve[i] = curve[i - 2];
    curve[1] = p;
    curve[0] = q;

    regularize_open_surface_vertex_curve();

  } while (curve.size() != count);
}

void viewer::close_regular_surface_vertex_curve() {
  const auto p = surface_vertex_curve.back();
  const auto q = surface_vertex_curve.front();

  if (p != q) {
    using namespace geometrycentral;
    using namespace surface;

    // Get the shortest edge path by using
    // Dijkstra's Algorithm by Geometry Central.
    //
    const auto network = FlipEdgeNetwork::constructFromDijkstraPath(
        *mesh, *geometry, Vertex{mesh.get(), p}, Vertex{mesh.get(), q});
    network->posGeom = geometry.get();
    const auto paths = network->getPathPolyline();

    // Check the path for consistency.
    //
    assert(paths.size() == 1);
    const auto& path = paths[0];
    assert(path.front().vertex.getIndex() == p);
    assert(path.back().vertex.getIndex() == q);

    // Store this path at the end of the current line.
    //
    for (size_t i = 1; i < path.size(); ++i)
      surface_vertex_curve.push_back(path[i].vertex.getIndex());
  }

  regularize_closed_surface_vertex_curve();
  surface_vertex_curve.push_back(surface_vertex_curve.front());
  surface_vertex_curve_closed = true;

  if (device)
    device->surface_vertex_curve_data.allocate_and_initialize(
        surface_vertex_curve);

  surface_vertex_curve.pop_back();
}

void viewer::reset_surface_vertex_curve() {
  surface_vertex_curve.clear();
  if (device)
    device->surface_vertex_curve_data.allocate_and_initialize(
        surface_vertex_curve);
}

void viewer::mouse_append_surface_vertex_curve(float x, float y) {
  const auto vid = surface_vertex_from(mouse_position{x, y});
  if (vid == polyhedral_surface::invalid) return;

  if (surface_vertex_curve.empty()) {
    surface_vertex_curve.push_back(vid);
    return;
  }

  const auto p = surface_vertex_curve.back();
  if (vid == p) return;

  {
    using namespace geometrycentral;
    using namespace surface;

    // Get the shortest edge path by using
    // Dijkstra's Algorithm by Geometry Central.
    //
    const auto network = FlipEdgeNetwork::constructFromDijkstraPath(
        *mesh, *geometry, Vertex{mesh.get(), p}, Vertex{mesh.get(), vid});
    network->posGeom = geometry.get();
    const auto paths = network->getPathPolyline();

    // Check the path for consistency.
    //
    assert(paths.size() == 1);
    const auto& path = paths[0];
    assert(path.front().vertex.getIndex() == p);
    assert(path.back().vertex.getIndex() == vid);

    // Store this path at the end of the current line.
    //
    for (size_t i = 1; i < path.size(); ++i)
      // surface_vertex_curve.push_back(path[i].vertex.getIndex());
      regular_append_to_surface_vertex_curve(path[i].vertex.getIndex());
  }
}

void viewer::regular_append_to_surface_vertex_curve(
    polyhedral_surface::vertex_id vid) {
  auto& curve = surface_vertex_curve;
  auto count = curve.size();
  surface_vertex_curve_closed = false;

  // count can never be zero
  if (count == 0) {
    curve.push_back(vid);
    return;
  }

  const auto p = curve[count - 1];
  if (vid == p) return;

  if (count < 2) {
    curve.push_back(vid);
    return;
  }

  const auto q = curve[count - 2];
  if (vid == q) {
    curve.pop_back();
    return;
  }

  if (surface.edges.contains({q, vid}) || surface.edges.contains({vid, q})) {
    curve[count - 1] = vid;  // remove previous and push back current
    return;
  }

  curve.push_back(vid);  // push back

  if (device)
    device->surface_vertex_curve_data.allocate_and_initialize(
        surface_vertex_curve);
}

void viewer::reset_surface_mesh_curve() {
  surface_mesh_curve.clear();
  if (device)
    device->surface_mesh_curve_data.allocate_and_initialize(surface_mesh_curve);
}

void viewer::compute_surface_geodesic() {
  auto& curve = surface_vertex_curve;

  if (curve.size() <= 2) return;

  using namespace geometrycentral;
  using namespace surface;

  // Construct path of halfedges from vertex indices.
  // We have to do this anyway as the surface point data
  // structure does not provide correctly oriented halfedges.
  //
  vector<Halfedge> edges{};
  for (size_t i = 1; i < curve.size(); ++i) {
    Vertex p(mesh.get(), curve[i - 1]);
    Vertex q(mesh.get(), curve[i]);

    auto he = q.halfedge();
    while (he.tipVertex() != p) he = he.nextOutgoingNeighbor();
    // The halfedge must point to the previous point.
    // Otherwise, edges do not count as path for FlipEdgeNetwork construction.
    edges.push_back(he.twin());

    // cout << line_vids[i - 1] << " -> " << line_vids[i] << '\t'
    //      << he.tipVertex().getIndex() << "," << he.tailVertex().getIndex()
    //      << endl;
  }

  if (surface_vertex_curve_closed) {
    Vertex p(mesh.get(), curve.back());
    Vertex q(mesh.get(), curve.front());
    auto he = q.halfedge();
    while (he.tipVertex() != p) he = he.nextOutgoingNeighbor();
    edges.push_back(he.twin());
  }

  auto g = geometry.get();

  FlipEdgeNetwork network(*mesh, *g, {edges});
  network.iterativeShorten(INVALID_IND, 0.2);
  network.posGeom = g;
  vector<Vector3> path = network.getPathPolyline3D().front();

  surface_mesh_curve.clear();
  for (const auto& v : path)
    surface_mesh_curve.push_back(vec3{real(v.x), real(v.y), real(v.z)});

  if (device)
    device->surface_mesh_curve_data.allocate_and_initialize(surface_mesh_curve);
}

void viewer::compute_heat_data() {
  // Construct vertex matrix.
  //
  surface_vertex_matrix.resize(surface.vertices.size(), 3);
  for (size_t i = 0; i < surface.vertices.size(); ++i)
    for (size_t j = 0; j < 3; ++j)
      surface_vertex_matrix(i, j) = surface.vertices[i].position[j];

  // Construct face matrix.
  //
  surface_face_matrix.resize(surface.faces.size(), 3);
  for (size_t i = 0; i < surface.faces.size(); ++i)
    for (size_t j = 0; j < 3; ++j)
      surface_face_matrix(i, j) = surface.faces[i][j];

  // Construct heat data.
  //
  const auto e =
      igl::avg_edge_length(surface_vertex_matrix, surface_face_matrix);

  avg_edge_length = e;

  // tolerance = 1 / (2 * e);
  // bound = e;
  // cout << "tolerance = " << tolerance << '\n'
  //      << "bound = " << bound << '\n'
  //      << endl;

  // cout << "avg edge length = " << e << endl;

  auto t = heat_time_scale * pow(e, 2);
  if (!igl::heat_geodesics_precompute(surface_vertex_matrix,
                                      surface_face_matrix, t, heat_data)) {
    cerr << "ERROR: Precomputation of heat data failed." << endl;
    exit(1);
  }

  potential.assign(surface.vertices.size(), 0);
  // device_heat.allocate_and_initialize(potential);
}

void viewer::update_heat() {
  const auto& line_vids = surface_vertex_curve;

  heat = Eigen::VectorXd::Zero(surface_vertex_matrix.rows());
  Eigen::VectorXi gamma(line_vids.size());
  for (size_t i = 0; i < line_vids.size(); ++i) gamma[i] = line_vids[i];

  igl::heat_geodesics_solve(heat_data, gamma, heat);

  // device_heat.allocate_and_initialize(potential);

  potential.assign(heat.size(), 0);
  for (size_t i = 0; i < potential.size(); ++i) potential[i] = heat[i];

  double max_distance = 0;
  for (size_t i = 0; i < heat.size(); ++i)
    max_distance = std::max(max_distance, heat[i]);

  // cout << "max distance = " << max_distance << endl;
  app().info(
      format("max distance from surface vertex curve = {}", max_distance));

  for (auto i : line_vids) potential[i] = 0;

  const auto modifier = [this](auto x) {
    const auto f = [](auto x) { return (x <= 1e-4) ? 0 : exp(-1 / x); };
    // const auto bump = [f](auto x) {
    //   return f(x) / (f(x) + f(1 - x));
    // };
    const auto square = [](auto x) { return x * x; };
    // const auto t = tolerance * x - bound;
    return x * x;
    // return x * x * f(x);
    // return exp(x) * f(x);
    // return x * x * x * x * f(x);
    // return tolerance * (x + sin(tolerance * x));
    // return (x <= 1e-4)
    //            ? 0
    //            : tolerance * tolerance * x * x * exp(-1.0f / tolerance / x);
    // return tolerance * sqrt(x);
    // return (tolerance * x) * (tolerance * x);
  };

  for (size_t i = 0; i < potential.size(); ++i)
    potential[i] =
        max_distance * modifier(tolerance * potential[i] / max_distance);
  // potential[i] = modifier(tolerance * (potential[i] / avg_edge_length));
  // modifier(tolerance * (potential[i] / surface.mean_edge_length[i]));

  // device_heat.allocate_and_initialize(potential);
  // device->scalar_field.allocate_and_initialize(potential);

  // Generate vertex data for constructors.
  //
  using namespace geometrycentral;
  using namespace surface;
  EdgeData<double> edge_lengths(*mesh);
  for (auto e : mesh->edges()) {
    const auto vid1 = e.halfedge().tipVertex().getIndex();
    const auto vid2 = e.halfedge().tailVertex().getIndex();
    const auto squared = [](auto x) { return x * x; };
    edge_lengths[e] = sqrt(length2(surface.vertices[vid1].position -
                                   surface.vertices[vid2].position) +
                           squared(potential[vid1] - potential[vid2]));
    // edge_lengths[e] = sqrt(length2(surface.vertices[vid1].position -
    //                                surface.vertices[vid2].position) /
    //                            pow(avg_edge_length, 2) +
    //                        squared(potential[vid1] - potential[vid2]));
  }
  //
  lifted_geometry = make_unique<EdgeLengthGeometry>(*mesh, edge_lengths);
}

void viewer::set_heat_time_scale(float scale) {
  heat_time_scale = scale;
  compute_heat_data();
}

void viewer::compute_smooth_surface_mesh_curve() {
  const auto& line_vids = surface_vertex_curve;
  // cout << "initial line vertices = " << line_vids.size() << endl;

  if (line_vids.size() <= 1) return;

  app().info(
      format("heat time scale = {}\n"
             "avg edge length = {}\n"
             "tolerance = {}\n"
             "minimal_length_scale = {}\n"
             "laplace_iterations = {}\n"
             "laplace_relaxation = {}\n",
             heat_time_scale, avg_edge_length, tolerance, minimal_length_scale,
             laplace_iterations, laplace_relaxation));

  const auto start = clock::now();

  update_heat();

  const auto heat_end = clock::now();

  using namespace geometrycentral;
  using namespace surface;

  // Construct path of halfedges from vertex indices.
  // We have to do this anyway as the surface point data
  // structure does not provide correctly oriented halfedges.
  //
  vector<Halfedge> edges{};
  for (size_t i = 1; i < line_vids.size(); ++i) {
    Vertex p(mesh.get(), line_vids[i - 1]);
    Vertex q(mesh.get(), line_vids[i]);

    auto he = q.halfedge();
    while (he.tipVertex() != p) he = he.nextOutgoingNeighbor();
    // The halfedge must point to the previous point.
    // Otherwise, edges do not count as path for FlipEdgeNetwork construction.
    edges.push_back(he.twin());

    // cout << line_vids[i - 1] << " -> " << line_vids[i] << '\t'
    //      << he.tipVertex().getIndex() << "," << he.tailVertex().getIndex()
    //      << endl;
  }
  if (surface_vertex_curve_closed) {
    Vertex p(mesh.get(), line_vids.back());
    Vertex q(mesh.get(), line_vids.front());
    auto he = q.halfedge();
    while (he.tipVertex() != p) he = he.nextOutgoingNeighbor();
    edges.push_back(he.twin());
  }

  FlipEdgeNetwork network(*mesh, *lifted_geometry, {edges});
  network.iterativeShorten(INVALID_IND, minimal_length_scale);
  // network.iterativeShorten();
  network.posGeom = geometry.get();

  // vector<Vector3> path = network.getPathPolyline3D().front();
  auto paths = network.getPathPolyline();
  auto& path = paths.front();
  auto positions = network.pathTo3D(paths).front();

  // Laplacian Relaxation
  //
  {
    const auto relax = [&](const auto& l, const auto& r, const auto& v1,
                           const auto& v2, float t0) {
      const auto p = r - v1;
      const auto q = l - v1;
      const auto v = v2 - v1;
      const auto vl = norm(v);
      const auto ivl = 1 / vl;
      const auto vn = ivl * v;

      const auto py = dot(p, vn);
      const auto qy = dot(q, vn);
      const auto px = -norm(p - py * vn);
      const auto qx = norm(q - qy * vn);

      const auto t = (py * qx - qy * px) / (qx - px) * ivl;
      const auto relaxation = laplace_relaxation;
      return std::clamp((1 - relaxation) * t0 + relaxation * t, 0.0, 1.0);
    };

    const auto apply_relax = [&](size_t i, size_t j, size_t k) {
      if (path[j].type != SurfacePointType::Edge) return;
      const auto p = positions[i];
      const auto q = positions[k];

      auto& edge = path[j].edge;
      const auto v1 = network.posGeom->vertexPositions[edge.firstVertex()];
      const auto v2 = network.posGeom->vertexPositions[edge.secondVertex()];
      const auto t0 = path[j].tEdge;

      path[j].tEdge = relax(p, q, v1, v2, t0);
    };

    for (size_t it = 0; it < laplace_iterations; ++it) {
      for (size_t i = 1; i < path.size() - 1; ++i) apply_relax(i - 1, i, i + 1);
      if (surface_vertex_curve_closed) {
        apply_relax(path.size() - 2, path.size() - 1, 0);
        apply_relax(path.size() - 1, 0, 1);
      }
      positions = network.pathTo3D(paths).front();
    }
  }

  const auto end = clock::now();

  const auto heat_time = duration(heat_end - start).count();
  const auto geoesic_time = duration(end - heat_end).count();
  const auto time = duration(end - start).count();

  // cout << "time = " << time << " s\n"
  //      << "heat time = " << heat_time << " s\n"
  //      << "geodesic time = " << geoesic_time << " s\n"
  //      << "tolerance = " << tolerance << '\n'
  //      << endl;

  surface_mesh_curve.clear();
  for (const auto& v : positions)
    surface_mesh_curve.push_back(vec3{real(v.x), real(v.y), real(v.z)});
  if (device)
    device->surface_mesh_curve_data.allocate_and_initialize(surface_mesh_curve);

  // cout << "smoothed line vertices = " << device_line.vertices.size() << endl;
}

void viewer::compute_surface_bipartition_from_surface_vertex_curve() {
  try {
    const auto face_mask = bipartition_from(surface, surface_vertex_curve,
                                            surface_vertex_curve_closed);
    device->ssbo.allocate_and_initialize(face_mask);
    app().info("Surface bi-partition computed.");
  } catch (runtime_error& e) {
    app().error(e.what());
    reset_surface_bipartition();
  }
}

void viewer::reset_surface_bipartition() {
  vector<float> tmp{};
  tmp.assign(surface.faces.size(), 0.0f);
  device->ssbo.allocate_and_initialize(tmp);
}

void viewer::reset_surface_scalar_field() {
  vector<float> tmp{};
  tmp.assign(surface.vertices.size(), 0.0f);
  device->scalar_field.allocate_and_initialize(tmp);
}

void viewer::compute_hyper_surface_smoothing() try {
  vector<double> coords(3 * surface.vertices.size());
  for (size_t i = 0; i < surface.vertices.size(); ++i) {
    const auto& v = surface.vertices[i].position;
    coords[3 * i + 0] = v.x;
    coords[3 * i + 1] = v.y;
    coords[3 * i + 2] = v.z;
  }

  vector<uint> elements(3 * surface.faces.size());
  for (size_t i = 0; i < surface.faces.size(); ++i) {
    const auto& f = surface.faces[i];
    elements[3 * i + 0] = f[0];
    elements[3 * i + 1] = f[1];
    elements[3 * i + 2] = f[2];
  }

  Trimesh<> m(coords, elements);

  // ScalarField sf();
  // assert(sf.size() == m.num_polys());
  const auto face_mask = bipartition_from(surface, surface_vertex_curve,
                                          surface_vertex_curve_closed);

  for (uint pid = 0; pid < m.num_polys(); ++pid)
    m.poly_data(pid).label = (face_mask[pid] < 0.0f) ? 0 : 1;

  ScalarField res =
      smooth_discrete_hyper_surface(m, hyper_lambda, hyper_smoothing_passes);
  vector<float> field(res.size());
  for (size_t i = 0; i < res.size(); ++i) field[i] = res[i];

  device->scalar_field.allocate_and_initialize(field);

} catch (runtime_error& e) {
  app().error(e.what());
}

void viewer::save_surface_vertex_curve(const filesystem::path& path) {
  const auto p = app().path_from_lookup(path);
  ofstream file{p};
  if (!file) {
    app().error(
        format("Failed to save surface vertex curve to file.\nfile = '{}'",
               p.string()));
    return;
  }

  for (auto vid : surface_vertex_curve) file << vid << '\n';
  if (surface_vertex_curve_closed) file << surface_vertex_curve.front() << '\n';

  app().info(
      format("Successfully saved surface vertex curve to file.\nfile = '{}'",
             p.string()));
}

void viewer::load_surface_vertex_curve(const filesystem::path& path) {
  const auto p = app().path_from_lookup(path);
  ifstream file{p};
  if (!file) {
    app().error(
        format("Failed to load surface vertex curve from file.\nfile = '{}'",
               p.string()));
    return;
  }

  surface_vertex_curve.clear();
  surface_vertex_curve_closed = false;

  polyhedral_surface::vertex_id vid{};

  while (file >> vid) surface_vertex_curve.push_back(vid);

  device->surface_vertex_curve_data.allocate_and_initialize(
      surface_vertex_curve);

  if (surface_vertex_curve.front() == surface_vertex_curve.back()) {
    surface_vertex_curve.pop_back();
    surface_vertex_curve_closed = true;
  }

  app().info(
      format("Successfully loaded surface vertex curve from file.\nfile = '{}'",
             p.string()));

  view_should_update = true;
}

}  // namespace ensketch::sandbox

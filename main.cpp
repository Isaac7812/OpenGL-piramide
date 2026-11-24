// Aston Martin: glTF, texturas, Phong y Bezier sobre una carretera.
#define _CRT_SECURE_NO_WARNINGS
// Requiere tiny_gltf_v3.h, tiny_gltf_v3.c y tinygltf_json_c.h
// TinyGLTF3 implementado
#define GLM_ENABLE_EXPERIMENTAL
#define TINYGLTF3_ENABLE_FS
#define TINYGLTF3_IMPLEMENTATION
#include "tiny_gltf_v3.h"
#define STB_IMAGE_STATIC
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>
#include <iostream>
#include <vector>
#include <string>
#include <cmath>
#include <cstddef>
#include <algorithm>
#include <cstring>
#include <cstdint>

const int WIDTH = 1000, HEIGHT = 700;
const char* MODEL_PATH = "modelo/2011_aston_martin_one-77/scene.gltf";

const char* sceneVS = R"GLSL(#version 330 core
layout(location=0) in vec3 aPos;
layout(location=1) in vec3 aNormal;
layout(location=2) in vec2 aUV;
uniform mat4 model, view, proj;
out vec3 FragPos;
out vec3 Normal;
out vec2 TexCoord;
void main(){
 vec4 world = model * vec4(aPos,1.0);
 FragPos = world.xyz;
 Normal = mat3(transpose(inverse(model))) * aNormal;
 TexCoord = aUV;
 gl_Position = proj * view * world;
}
)GLSL";
const char* sceneFS = R"GLSL(#version 330 core
in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoord;
out vec4 FragColor;
uniform sampler2D baseTexture;
uniform bool useTexture;
uniform int alphaMode;
uniform float alphaCutoff, brillo;
uniform vec4 baseColorFactor;
uniform vec3 lightPos, lightColor, viewPos;
void main(){
 vec4 base = baseColorFactor;
 if(useTexture){
  vec4 texel=texture(baseTexture,TexCoord);
  // Las texturas baseColor estan en sRGB; la luz se calcula en lineal.
  texel.rgb=mix(texel.rgb/12.92,pow((texel.rgb+0.055)/1.055,vec3(2.4)),step(vec3(0.04045),texel.rgb));
  base *= texel;
 }
 if(alphaMode == 1 && base.a < alphaCutoff) discard;
 if(alphaMode == 2 && base.a < 0.01) discard;
 if(alphaMode != 2) base.a = 1.0;
 vec3 n = normalize(Normal);
 if(!gl_FrontFacing) n = -n;
 vec3 l = normalize(lightPos-FragPos);
 vec3 v = normalize(viewPos-FragPos);
 vec3 r = reflect(-l,n);
 float ambient = 0.06; // Luz minima del ambiente nocturno.
 float diffuse = max(dot(n,l),0.0);
 float specular = pow(max(dot(v,r),0.0),32.0) * brillo * step(0.0,diffuse-0.0001);
 // Atenuacion: la luz pierde intensidad con la distancia.
 float distancia = length(lightPos - FragPos);
 float atenuacion = 1.0 / (1.0 + 0.035 * distancia + 0.008 * distancia * distancia);
 // Iluminacion Phong: ambiente, difusa y especular.
 vec3 luzAmbiente = ambient * base.rgb;
 float intensidad = 3.5; // Potencia de la fuente puntual.
 vec3 luzDifusa = diffuse * base.rgb * lightColor * intensidad;
 vec3 luzEspecular = specular * lightColor * intensidad;
 vec3 color = luzAmbiente + (luzDifusa + luzEspecular) * atenuacion;
 // Los factores glTF estan en espacio lineal. Convertir a sRGB al mostrar.
 color = mix(12.92*color, 1.055*pow(max(color,vec3(0.0)),vec3(1.0/2.4))-0.055, step(vec3(0.0031308),color));
 FragColor = vec4(clamp(color,0.0,1.0),base.a);
}
)GLSL";
const char* lightVS = R"GLSL(#version 330 core
layout(location=0) in vec3 aPos;
uniform mat4 model,view,proj;
void main(){gl_Position=proj*view*model*vec4(aPos,1.0);}
)GLSL";
const char* lightFS = R"GLSL(#version 330 core
out vec4 FragColor;
uniform vec3 lightColor;
void main(){FragColor=vec4(lightColor,1.0);}
)GLSL";

GLuint compileShader(GLenum type,const char* src){
 GLuint s=glCreateShader(type); glShaderSource(s,1,&src,nullptr); glCompileShader(s);
 GLint ok=0; glGetShaderiv(s,GL_COMPILE_STATUS,&ok);
 if(!ok){char log[2048];glGetShaderInfoLog(s,sizeof(log),nullptr,log);std::cerr<<"Error shader: "<<log<<'\n';}
 return s;
}
GLuint makeProgram(const char* vs,const char* fs){
 GLuint v=compileShader(GL_VERTEX_SHADER,vs),f=compileShader(GL_FRAGMENT_SHADER,fs);
 GLuint p=glCreateProgram();glAttachShader(p,v);glAttachShader(p,f);glLinkProgram(p);
 GLint ok=0;glGetProgramiv(p,GL_LINK_STATUS,&ok);
 if(!ok){char log[2048];glGetProgramInfoLog(p,sizeof(log),nullptr,log);std::cerr<<"Error enlace: "<<log<<'\n';}
 glDeleteShader(v);glDeleteShader(f);return p;
}

struct Vertex {glm::vec3 pos,normal;glm::vec2 uv;};
struct MeshGPU {glm::vec3 minPos{0},maxPos{0};GLuint vao=0,vbo=0,ebo=0;GLsizei count=0;GLenum mode=GL_TRIANGLES;int material=-1;};
struct SphereGPU {GLuint vao=0,vbo=0,ebo=0;GLsizei count=0;};

tg3_model scene = {}, nieve = {};
bool esNieve=false;
std::vector<std::vector<MeshGPU>> mallasNieve;
std::vector<GLuint> texturasNieve;
std::vector<std::vector<MeshGPU>> gpuMeshes;
std::vector<GLuint> gpuTextures;
GLuint sceneProgram=0,lightProgram=0;
SphereGPU sphere;

// Lee un atributo float desde los buffers de TinyGLTF v3.
int attributeIndex(const tg3_primitive& p, const char* name){
 for(uint32_t i=0;i<p.attributes_count;++i)
  if(tg3_str_equals_cstr(p.attributes[i].key,name))return p.attributes[i].value;
 return -1;
}
const uint8_t* accessorData(const tg3_accessor& a, size_t index, size_t bytes){
 if(a.buffer_view<0 || uint32_t(a.buffer_view)>=scene.buffer_views_count || index>=a.count)return nullptr;
 const auto& bv=scene.buffer_views[a.buffer_view];
 if(bv.buffer<0 || uint32_t(bv.buffer)>=scene.buffers_count)return nullptr;
 const auto& buf=scene.buffers[bv.buffer];
 uint64_t stride=bv.byte_stride?bv.byte_stride:bytes;
 uint64_t offset=bv.byte_offset+a.byte_offset+uint64_t(index)*stride;
 if(stride<bytes || offset>buf.data.count || bytes>buf.data.count-offset)return nullptr;
 return buf.data.data+offset;
}
bool readFloats(const tg3_primitive& p,const char* name,size_t index,float* dst,int count){
 int ai=attributeIndex(p,name);
 if(ai<0 || uint32_t(ai)>=scene.accessors_count)return false;
 const auto& a=scene.accessors[ai];
 if(a.component_type!=TG3_COMPONENT_TYPE_FLOAT)return false;
 const uint8_t* ptr=accessorData(a,index,size_t(count)*sizeof(float));
 if(!ptr)return false;
 std::memcpy(dst,ptr,size_t(count)*sizeof(float));return true;
}
bool readIndex(const tg3_accessor& a,size_t i,unsigned int& value){
 size_t bytes=a.component_type==TG3_COMPONENT_TYPE_UNSIGNED_BYTE?1:
  a.component_type==TG3_COMPONENT_TYPE_UNSIGNED_SHORT?2:
  a.component_type==TG3_COMPONENT_TYPE_UNSIGNED_INT?4:0;
 if(!bytes)return false;
 const uint8_t* ptr=accessorData(a,i,bytes);
 if(!ptr)return false;
 value=0;
 if(bytes==1){uint8_t n;std::memcpy(&n,ptr,1);value=n;}
 if(bytes==2){uint16_t n;std::memcpy(&n,ptr,2);value=n;}
 if(bytes==4){uint32_t n;std::memcpy(&n,ptr,4);value=n;}
 return true;
}

void uploadScene(){
 gpuMeshes.resize(scene.meshes_count);
 for(uint32_t m=0;m<scene.meshes_count;++m){
  const auto& mesh=scene.meshes[m];
  for(uint32_t pi=0;pi<mesh.primitives_count;++pi){
   const auto& p=mesh.primitives[pi];
   int pos=attributeIndex(p,"POSITION");
   if(pos<0 || uint32_t(pos)>=scene.accessors_count || (p.mode!=-1 && p.mode!=TG3_MODE_TRIANGLES))continue;
   const auto& a=scene.accessors[pos];
   std::vector<Vertex> verts(size_t(a.count));
   for(size_t i=0;i<verts.size();++i){
    float xyz[3]={0,0,0},n[3]={0,1,0},uv[2]={0,0};
    if(!readFloats(p,"POSITION",i,xyz,3))continue;
    readFloats(p,"NORMAL",i,n,3);
    int uvSet=0;
    if(p.material>=0 && uint32_t(p.material)<scene.materials_count)
     uvSet=scene.materials[p.material].pbr_metallic_roughness.base_color_texture.tex_coord;
    std::string uvName="TEXCOORD_"+std::to_string(uvSet);
    readFloats(p,uvName.c_str(),i,uv,2);
    verts[i]={{xyz[0],xyz[1],xyz[2]},{n[0],n[1],n[2]},{uv[0],uv[1]}};
   }
   std::vector<unsigned int> idx;
   if(p.indices>=0 && uint32_t(p.indices)<scene.accessors_count){
    const auto& ia=scene.accessors[p.indices];
    for(size_t i=0;i<size_t(ia.count);++i){unsigned int v=0;if(readIndex(ia,i,v))idx.push_back(v);}
   }else{for(size_t i=0;i<verts.size();++i)idx.push_back(static_cast<unsigned int>(i));}
   if(idx.empty() || verts.empty())continue;
   MeshGPU g;
   g.minPos=g.maxPos=verts.front().pos;
   for(const auto& v:verts){g.minPos=glm::min(g.minPos,v.pos);g.maxPos=glm::max(g.maxPos,v.pos);}
   g.material=p.material;g.count=static_cast<GLsizei>(idx.size());
   glGenVertexArrays(1,&g.vao);glGenBuffers(1,&g.vbo);glGenBuffers(1,&g.ebo);
   glBindVertexArray(g.vao);
   glBindBuffer(GL_ARRAY_BUFFER,g.vbo);glBufferData(GL_ARRAY_BUFFER,verts.size()*sizeof(Vertex),verts.data(),GL_STATIC_DRAW);
   glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,g.ebo);glBufferData(GL_ELEMENT_ARRAY_BUFFER,idx.size()*sizeof(unsigned int),idx.data(),GL_STATIC_DRAW);
   glEnableVertexAttribArray(0);glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,sizeof(Vertex),(void*)offsetof(Vertex,pos));
   glEnableVertexAttribArray(1);glVertexAttribPointer(1,3,GL_FLOAT,GL_FALSE,sizeof(Vertex),(void*)offsetof(Vertex,normal));
   glEnableVertexAttribArray(2);glVertexAttribPointer(2,2,GL_FLOAT,GL_FALSE,sizeof(Vertex),(void*)offsetof(Vertex,uv));
   glBindVertexArray(0);gpuMeshes[m].push_back(g);
  }
 }
 // Cargar directamente las imagenes PNG con stb_image.
 const char* archivos[] = {
  "textures/wwAstonMartin_One77RewardRecycled_2010_Wheel2A_3D_3DWheel_a9f1ae1_baseColor.png",
  "textures/wwAstonMartin_One77RewardRecycled_2010_Wheel2A_3D_3DWheel_a9f1ae1_normal.png",
  "textures/wwAstonMartin_One77Reward_2010CalliperBadgeA_Material1_baseColor.png",
  "textures/wwAstonMartin_One77Reward_2010CalliperGloss_Material1_normal.png",
  "textures/AstonAstonMartin_One77Reward_2010ManufacturerPlateA_Material1_baseColor.png",
  "textures/AstonAstonMartin_One77Reward_2010LightA_Material1_baseColor.png",
  "textures/AstonAstonMartin_One77Reward_2010Coloured_Material1_baseColor.png",
  "textures/AstonAstonMartin_One77Reward_2010Carbon1_Material1_baseColor.png",
  "textures/AstonAstonMartin_One77Reward_2010Carbon1_Material1_normal.png",
  "textures/AstonAstonMartin_One77Reward_2010BadgeA_Material1_baseColor.png",
  "textures/AstonAstonMartin_One77Reward_2010BadgeA_Material1_normal.png",
  "textures/AstonAstonMartin_One77Reward_2010EngineA_Material1_baseColor.png",
  "textures/AstonAstonMartin_One77Reward_2010EngineA_Material1_normal.png",
  "textures/AstonAstonMartin_One77Reward_2010Grille1A_Material1_baseColor.png",
  "textures/AstonAstonMartin_One77Reward_2010Grille1A_Material1_normal.png",
  "textures/AstonAstonMartin_One77Reward_2010Grille2A_Material1_baseColor.png",
  "textures/AstonAstonMartin_One77Reward_2010Grille2A_Material1_normal.png",
  "textures/AstonAstonMartin_One77Reward_2010Grille3A_Material1_baseColor.png",
  "textures/AstonAstonMartin_One77Reward_2010Grille3A_Material1_normal.png",
  "textures/AstonAstonMartin_One77Reward_2010InteriorA_Material1_baseColor.png",
  "textures/AstonAstonMartin_One77Reward_2010InteriorA_Material1_normal.png",
 };
 const char* archivosNieve[] = {
  "textures/MESSY_SNOW_DECAL_baseColor.png",
  "textures/MESSY_SNOW_DECAL_normal.png",
  "textures/SNOW_EDGE_baseColor.png",
  "textures/SNOW_EDGE_metallicRoughness.png",
  "textures/SNOW_EDGE_normal.png",
  "textures/ROAD_MARKING_baseColor.png",
  "textures/ROAD_MARKING_metallicRoughness.png",
  "textures/ROAD_MARKING_normal.png",
  "textures/SNOWY_ROAD_baseColor.png",
  "textures/SNOWY_ROAD_metallicRoughness.png",
  "textures/SNOWY_ROAD_normal.png",
  "textures/SNOW_FLOOR_baseColor.png",
  "textures/SNOW_FLOOR_metallicRoughness.png",
  "textures/SNOW_FLOOR_normal.png",
  "textures/ROAD_SNOW_OVERLAY_TILING_baseColor.png",
  "textures/ROAD_SNOW_OVERLAY_TILING_metallicRoughness.png",
  "textures/DIRTY_SNOW_DECAL_baseColor.png",
  "textures/DIRTY_SNOW_DECAL_metallicRoughness.png",
  "textures/DIRTY_SNOW_DECAL_normal.png",
  "textures/BACKGROUND_TREES_baseColor.png",
  "textures/CLIFF_baseColor.png",
  "textures/CLIFF_metallicRoughness.png",
  "textures/CLIFF_normal.png",
  "textures/ROAD_POLE_baseColor.png",
  "textures/MOUNTAIN_baseColor.png",
  "textures/MOUNTAIN_metallicRoughness.png",
  "textures/MOUNTAIN_normal.png",
  "textures/MIST_baseColor.png",
  "textures/MIST_emissive.png",
 };
 gpuTextures.resize(scene.textures_count,0);
 stbi_set_flip_vertically_on_load(false);
 int cargadas=0;
 for(uint32_t t=0;t<scene.textures_count && t<(esNieve?29:21);++t){
  std::string ruta=std::string(esNieve?"modelo/escenario/":"modelo/2011_aston_martin_one-77/")+(esNieve?archivosNieve[t]:archivos[t]);
  int ancho,alto,canales;
  unsigned char* datos=stbi_load(ruta.c_str(),&ancho,&alto,&canales,4);
  if(!datos){std::cerr<<"Error cargando: "<<ruta<<"\n"<<stbi_failure_reason()<<"\n";continue;}
  GLuint tex;glGenTextures(1,&tex);glBindTexture(GL_TEXTURE_2D,tex);
  glPixelStorei(GL_UNPACK_ALIGNMENT,1);
  glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA8,ancho,alto,0,GL_RGBA,GL_UNSIGNED_BYTE,datos);
  glGenerateMipmap(GL_TEXTURE_2D);
  glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR_MIPMAP_LINEAR);
  glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
  stbi_image_free(datos);
  gpuTextures[t]=tex;++cargadas;
  std::cout<<"Textura "<<t<<" cargada correctamente: "<<ancho<<"x"<<alto<<"\n";
 }
 glBindTexture(GL_TEXTURE_2D,0);
 std::cout<<"Total de texturas cargadas: "<<cargadas<<"/"<<scene.textures_count<<"\n";
}

glm::mat4 nodeTransform(const tg3_node& n){
 if(n.has_matrix){glm::mat4 m(1.0f);for(int col=0;col<4;++col)for(int row=0;row<4;++row)m[col][row]=static_cast<float>(n.matrix[col*4+row]);return m;}
 glm::mat4 m(1.0f);
 m=glm::translate(m,glm::vec3(n.translation[0],n.translation[1],n.translation[2]));
 glm::quat q(static_cast<float>(n.rotation[3]),static_cast<float>(n.rotation[0]),static_cast<float>(n.rotation[1]),static_cast<float>(n.rotation[2]));
 m*=glm::toMat4(q);
 m=glm::scale(m,glm::vec3(n.scale[0],n.scale[1],n.scale[2]));
 return m;
}
// Los materiales transparentes se dibujan despues de los opacos.
struct TransparentMesh { const MeshGPU* mesh; glm::mat4 world; bool nieve; };
std::vector<TransparentMesh> transparentes;
void drawMesh(const MeshGPU& g,const glm::mat4& world,bool transparentPass){

  glUniformMatrix4fv(glGetUniformLocation(sceneProgram,"model"),1,GL_FALSE,glm::value_ptr(world));
   glm::vec4 factor(1.0f);int texIndex=-1, alpha=0; float cutoff=0.5f, brillo=0.10f;
   if(g.material>=0 && uint32_t(g.material)<scene.materials_count){
    const auto& mat=scene.materials[g.material];
    const auto& pbr=mat.pbr_metallic_roughness;
    factor=glm::vec4(float(pbr.base_color_factor[0]),float(pbr.base_color_factor[1]),float(pbr.base_color_factor[2]),float(pbr.base_color_factor[3]));
    texIndex=pbr.base_color_texture.index;
    if(tg3_str_equals_cstr(mat.alpha_mode,"MASK"))alpha=1;
    if(tg3_str_equals_cstr(mat.alpha_mode,"BLEND"))alpha=2;
    cutoff=float(mat.alpha_cutoff);
    if(esNieve && g.material==7){alpha=1;cutoff=0.4f;}
    if(esNieve && g.material==11)return; // Omitir plano de niebla original.
    brillo=0.04f+0.12f*(1.0f-glm::clamp(float(pbr.roughness_factor),0.0f,1.0f));
    // Aproximacion Phong: conservar el negro de rejillas y piezas oscuras.
    brillo *= 1.0f-0.65f*glm::clamp(float(pbr.metallic_factor),0.0f,1.0f);
    // Los dos conjuntos de vidrio se superponen; usar un tinte oscuro visible.
    if(!esNieve && g.material==3){factor=glm::vec4(0.015f,0.02f,0.025f,0.65f);brillo=0.035f;}
    if(mat.double_sided)glDisable(GL_CULL_FACE);else glEnable(GL_CULL_FACE);
   }else glDisable(GL_CULL_FACE);
   if(alpha==2 && !transparentPass){transparentes.push_back({&g,world,esNieve});return;}
   glFrontFace(glm::determinant(glm::mat3(world))<0?GL_CW:GL_CCW);
   glUniform1i(glGetUniformLocation(sceneProgram,"alphaMode"),alpha);
   glUniform1f(glGetUniformLocation(sceneProgram,"alphaCutoff"),cutoff);
   glUniform1f(glGetUniformLocation(sceneProgram,"brillo"),brillo);
   bool textured=texIndex>=0 && uint32_t(texIndex)<gpuTextures.size() && gpuTextures[texIndex]!=0;
   glUniform4fv(glGetUniformLocation(sceneProgram,"baseColorFactor"),1,glm::value_ptr(factor));
   glUniform1i(glGetUniformLocation(sceneProgram,"useTexture"),textured?1:0);
   glActiveTexture(GL_TEXTURE0);glBindTexture(GL_TEXTURE_2D,textured?gpuTextures[texIndex]:0);
   glBindVertexArray(g.vao);glDrawElements(g.mode,g.count,GL_UNSIGNED_INT,0);
}
void drawNode(int nodeIndex,const glm::mat4& parent){
 if(nodeIndex<0 || uint32_t(nodeIndex)>=scene.nodes_count)return;
 const auto& n=scene.nodes[nodeIndex];glm::mat4 world=parent*nodeTransform(n);
 if(n.mesh>=0 && uint32_t(n.mesh)<gpuMeshes.size())
  for(const auto& g:gpuMeshes[n.mesh])drawMesh(g,world,false);
 for(uint32_t i=0;i<n.children_count;++i)drawNode(n.children[i],world); 
}

// Alternar el contexto de datos sin copiar buffers GPU.
void cambiarModelo(){
 std::swap(scene,nieve);gpuMeshes.swap(mallasNieve);gpuTextures.swap(texturasNieve);esNieve=!esNieve;
}
void createSphere(){
 const int stacks=16,slices=24;
 std::vector<glm::vec3> verts;std::vector<unsigned int> idx;
 for(int y=0;y<=stacks;++y){float v=float(y)/stacks,phi=v*3.14159265f;
  for(int x=0;x<=slices;++x){float u=float(x)/slices,theta=u*6.2831853f;
   verts.emplace_back(std::sin(phi)*std::cos(theta),std::cos(phi),std::sin(phi)*std::sin(theta));
  }
 }
 for(int y=0;y<stacks;++y)for(int x=0;x<slices;++x){
  unsigned int a=y*(slices+1)+x,b=a+slices+1;
  idx.insert(idx.end(),{a,b,a+1,b,b+1,a+1});
 }
 sphere.count=static_cast<GLsizei>(idx.size());
 glGenVertexArrays(1,&sphere.vao);glGenBuffers(1,&sphere.vbo);glGenBuffers(1,&sphere.ebo);
 glBindVertexArray(sphere.vao);
 glBindBuffer(GL_ARRAY_BUFFER,sphere.vbo);glBufferData(GL_ARRAY_BUFFER,verts.size()*sizeof(glm::vec3),verts.data(),GL_STATIC_DRAW);
 glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,sphere.ebo);glBufferData(GL_ELEMENT_ARRAY_BUFFER,idx.size()*sizeof(unsigned int),idx.data(),GL_STATIC_DRAW);
 glEnableVertexAttribArray(0);glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,sizeof(glm::vec3),(void*)0);
 glBindVertexArray(0);
}

// Ruta tridimensional: 12 puntos centrales de la carretera (X, Y, Z).
// La coordenada Y permite seguir los desniveles.
// Restamos el origen para centrar el escenario y la ruta por igual.
const glm::vec3 origen(320,-15,-280);
const glm::vec3 ruta[] = {
 {289,-8.9f,-284.5f},{300,-10.2f,-286.5f},{316,-12.5f,-290},
 {328,-14.5f,-293.7f},{336,-15.6f,-292.8f},{340.3f,-16,-288.7f},
 {342.4f,-16.1f,-283.4f},{340,-16,-277.2f},{332,-15.9f,-271.9f},
 {320,-16.3f,-269},{305,-17.2f,-265},{294,-18,-261.9f}
};
const int N=sizeof(ruta)/sizeof(ruta[0]);
const float CAR_YAW_OFFSET=0.0f;
// Bezier compuesta: 12 puntos de ruta forman 11 tramos cubicos.
glm::vec3 bezier(float t){
 // Convertir el tiempo global [0,1] en un indice de tramo.
 float x = glm::clamp(t, 0.f, 1.f) * (N - 1);
 int i = std::min(int(x), N - 2);
 // Parametro local del tramo, entre cero y uno.
 float v = x - i;
 float u = 1.f - v;
 // a y d son los extremos del tramo (P0 y P3).
 glm::vec3 a = ruta[i], d = ruta[i+1];
 // b y c son los controles internos (P1 y P2).
 // Se calculan con los vecinos para unir los tramos suavemente.
 // max y min evitan acceder fuera del arreglo en los extremos.
 glm::vec3 b = a + (d - ruta[std::max(0,i-1)]) / 6.f;
 glm::vec3 c = d - (ruta[std::min(N-1,i+2)] - a) / 6.f;
 // Formula cubica de Bezier; trasladar al origen del escenario.
 return u*u*u*a + 3*u*u*v*b + 3*u*v*v*c + v*v*v*d - origen;
}

// Aproximar la tangente restando dos posiciones cercanas.
// Su direccion permite orientar el auto en curvas y pendientes.
glm::vec3 tangente(float t){
 return bezier(std::min(1.f,t+.001f)) - bezier(std::max(0.f,t-.001f));
}

// Calcular limites del modelo considerando la jerarquia glTF.
void limitesNodo(int index,glm::mat4 parent,glm::vec3& minimo,glm::vec3& maximo){
 if(index<0 || uint32_t(index)>=scene.nodes_count)return;
 const auto& n=scene.nodes[index];glm::mat4 m=parent*nodeTransform(n);
 if(n.mesh>=0 && uint32_t(n.mesh)<gpuMeshes.size()){
  for(const auto& g:gpuMeshes[n.mesh])for(int i=0;i<8;++i){
   glm::vec3 p(i&1?g.maxPos.x:g.minPos.x,i&2?g.maxPos.y:g.minPos.y,i&4?g.maxPos.z:g.minPos.z);
   p=glm::vec3(m*glm::vec4(p,1));minimo=glm::min(minimo,p);maximo=glm::max(maximo,p);
  }
 }
 for(uint32_t i=0;i<n.children_count;++i)limitesNodo(n.children[i],m,minimo,maximo);
}

// Camara FPS
 glm::vec3 cameraPos(5.0f,24.0f,35.0f),cameraFront(0.0f,-0.574f,-0.819f),cameraUp(0,1,0);
 float yaw=-90.0f,pitch=-35.0f;
 bool mouseCaptured=true,firstMouse=true;
 double lastX=WIDTH/2.0,lastY=HEIGHT/2.0;
void mouseCallback(GLFWwindow*,double xpos,double ypos){
 if(!mouseCaptured)return;
 if(firstMouse){lastX=xpos;lastY=ypos;firstMouse=false;return;}
 float dx=static_cast<float>(xpos-lastX),dy=static_cast<float>(lastY-ypos);lastX=xpos;lastY=ypos;
 yaw+=dx*0.1f;pitch+=dy*0.1f;pitch=glm::clamp(pitch,-89.0f,89.0f);
 glm::vec3 f(std::cos(glm::radians(yaw))*std::cos(glm::radians(pitch)),std::sin(glm::radians(pitch)),std::sin(glm::radians(yaw))*std::cos(glm::radians(pitch)));
 cameraFront=glm::normalize(f);
}
void mouseButtonCallback(GLFWwindow* w,int button,int action,int){
 if(button==GLFW_MOUSE_BUTTON_LEFT&&action==GLFW_PRESS&&!mouseCaptured){
  mouseCaptured=true;firstMouse=true;glfwSetInputMode(w,GLFW_CURSOR,GLFW_CURSOR_DISABLED);
 }
}
void framebufferCallback(GLFWwindow*,int w,int h){glViewport(0,0,w,h);}

int main(){
 if(!glfwInit()){std::cerr<<"No se pudo iniciar GLFW\n";return 1;}
 glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR,3);glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR,3);
 glfwWindowHint(GLFW_OPENGL_PROFILE,GLFW_OPENGL_CORE_PROFILE);
 GLFWwindow* window=glfwCreateWindow(WIDTH,HEIGHT,"Aston Martin - Phong y Bezier",nullptr,nullptr);
 if(!window){std::cerr<<"No se pudo crear ventana\n";glfwTerminate();return 1;}
 glfwMakeContextCurrent(window);glfwSwapInterval(1);
 if(!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)){std::cerr<<"Fallo GLAD\n";glfwTerminate();return 1;}
 glfwSetFramebufferSizeCallback(window,framebufferCallback);
 glfwSetCursorPosCallback(window,mouseCallback);
 glfwSetMouseButtonCallback(window,mouseButtonCallback);
 glfwSetInputMode(window,GLFW_CURSOR,GLFW_CURSOR_DISABLED);
 glEnable(GL_DEPTH_TEST);glDisable(GL_CULL_FACE);
 tg3_error_stack errors;
 tg3_error_stack_init(&errors);
 tg3_parse_options opts;
 tg3_parse_options_init(&opts);
 opts.images_as_is=0; // decodificar imagenes usando stb_image
 tg3_error_code result=tg3_parse_file(&scene,&errors,MODEL_PATH,static_cast<uint32_t>(std::strlen(MODEL_PATH)),&opts);
 if(result!=TG3_OK){
  std::cerr<<"No se pudo cargar "<<MODEL_PATH<<". Codigo: "<<int(result)<<"\n";
  for(uint32_t i=0;i<errors.count;++i)if(errors.entries[i].message)std::cerr<<errors.entries[i].message<<"\n";
  tg3_error_stack_free(&errors);tg3_model_free(&scene);
  glfwDestroyWindow(window);glfwTerminate();return 1;
 }
 tg3_error_stack_free(&errors);
 std::cout<<"Modelo cargado: "<<scene.meshes_count<<" meshes, "<<scene.nodes_count<<" nodos\n";
 sceneProgram=makeProgram(sceneVS,sceneFS);lightProgram=makeProgram(lightVS,lightFS);
 uploadScene();createSphere();
 const char* rutaNieve="modelo/escenario/scene.gltf";
 tg3_error_stack_init(&errors);
 result=tg3_parse_file(&nieve,&errors,rutaNieve,static_cast<uint32_t>(std::strlen(rutaNieve)),&opts);
 if(result!=TG3_OK){std::cerr<<"No se pudo cargar "<<rutaNieve<<"\n";tg3_error_stack_free(&errors);return 1;}
 tg3_error_stack_free(&errors);
 cambiarModelo();uploadScene();cambiarModelo();
 int escenaNieve=nieve.default_scene>=0?nieve.default_scene:0;
 if(uint32_t(escenaNieve)>=nieve.scenes_count)return 1;
 glm::mat4 matrizNieve=glm::translate(glm::mat4(1),-origen);
 int escena=scene.default_scene>=0?scene.default_scene:0;
 if(escena<0 || uint32_t(escena)>=scene.scenes_count){std::cerr<<"Escena vacia\n";glfwTerminate();return 1;}
 glm::vec3 minimo(1e30f),maximo(-1e30f);
 for(uint32_t i=0;i<scene.scenes[escena].nodes_count;++i)limitesNodo(scene.scenes[escena].nodes[i],glm::mat4(1),minimo,maximo);
 glm::vec3 dimensiones=maximo-minimo;
 float largo=std::max(dimensiones.x,dimensiones.z);
 if(largo<=0 || minimo.x>maximo.x){std::cerr<<"Modelo sin geometria\n";glfwTerminate();return 1;}
 float escala=4.5f/largo;
 glm::vec3 centro((minimo.x+maximo.x)/2,minimo.y,(minimo.z+maximo.z)/2);
 glm::mat4 ajuste=glm::scale(glm::mat4(1),glm::vec3(escala))*glm::translate(glm::mat4(1),-centro);
 std::cout<<"Dimensiones originales: "<<dimensiones.x<<" "<<dimensiones.y<<" "<<dimensiones.z<<"\n";
 float tiempoAnimacion=0;
 glm::vec3 lightPos(15.0f,5.0f,-8.0f),lightColor(1.0f,0.94f,0.84f);
 float lastFrame=static_cast<float>(glfwGetTime());
 std::cout<<"WASD: camara | Shift: correr | Flechas: mover luz X/Z | Q/E: subir/bajar | Esc: liberar mouse | Click: capturar\n";
 while(!glfwWindowShouldClose(window)){
  float now=static_cast<float>(glfwGetTime()),dt=std::min(now-lastFrame,0.1f);lastFrame=now;
  glfwPollEvents();
  if(glfwGetKey(window,GLFW_KEY_ESCAPE)==GLFW_PRESS&&mouseCaptured){mouseCaptured=false;glfwSetInputMode(window,GLFW_CURSOR,GLFW_CURSOR_NORMAL);}
  float speed=6.0f*dt;
  if(glfwGetKey(window,GLFW_KEY_LEFT_SHIFT)==GLFW_PRESS)speed*=2.0f;
  glm::vec3 right=glm::normalize(glm::cross(cameraFront,cameraUp));
  if(glfwGetKey(window,GLFW_KEY_W)==GLFW_PRESS)cameraPos+=cameraFront*speed;
  if(glfwGetKey(window,GLFW_KEY_S)==GLFW_PRESS)cameraPos-=cameraFront*speed;
  if(glfwGetKey(window,GLFW_KEY_A)==GLFW_PRESS)cameraPos-=right*speed;
  if(glfwGetKey(window,GLFW_KEY_D)==GLFW_PRESS)cameraPos+=right*speed;
  float ls=8.0f*dt;
  if(glfwGetKey(window,GLFW_KEY_LEFT)==GLFW_PRESS)lightPos.x-=ls;
  if(glfwGetKey(window,GLFW_KEY_RIGHT)==GLFW_PRESS)lightPos.x+=ls;
  if(glfwGetKey(window,GLFW_KEY_UP)==GLFW_PRESS)lightPos.z-=ls;
  if(glfwGetKey(window,GLFW_KEY_DOWN)==GLFW_PRESS)lightPos.z+=ls;
  if(glfwGetKey(window,GLFW_KEY_Q)==GLFW_PRESS)lightPos.y+=ls;
  if(glfwGetKey(window,GLFW_KEY_E)==GLFW_PRESS)lightPos.y-=ls;
  // Fondo oscuro. Cambia estos 3 primeros valores si deseas otro color.
  glClearColor(0.025f,0.035f,0.08f,1.0f);
  glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
  int fbw=WIDTH,fbh=HEIGHT;glfwGetFramebufferSize(window,&fbw,&fbh);
  glm::mat4 view=glm::lookAt(cameraPos,cameraPos+cameraFront,cameraUp);
  glm::mat4 proj=glm::perspective(glm::radians(60.0f),float(fbw)/float(std::max(fbh,1)),0.3f,1800.0f);
  glUseProgram(sceneProgram);
  glUniformMatrix4fv(glGetUniformLocation(sceneProgram,"view"),1,GL_FALSE,glm::value_ptr(view));
  glUniformMatrix4fv(glGetUniformLocation(sceneProgram,"proj"),1,GL_FALSE,glm::value_ptr(proj));
  glUniform3fv(glGetUniformLocation(sceneProgram,"lightPos"),1,glm::value_ptr(lightPos));
  glUniform3fv(glGetUniformLocation(sceneProgram,"lightColor"),1,glm::value_ptr(lightColor));
  glUniform3fv(glGetUniformLocation(sceneProgram,"viewPos"),1,glm::value_ptr(cameraPos));
  glUniform1i(glGetUniformLocation(sceneProgram,"baseTexture"),0);
  transparentes.clear();
  cambiarModelo();
  for(uint32_t i=0;i<scene.scenes[escenaNieve].nodes_count;++i)drawNode(scene.scenes[escenaNieve].nodes[i],matrizNieve);
  cambiarModelo();
  // Recorrer la curva en 24 segundos; regresar al inicio al terminar.
  tiempoAnimacion+=dt;
  float t=std::fmod(tiempoAnimacion/24.0f,1.0f);
  // Posicion en XYZ; pequeno margen sobre el pavimento.
  glm::vec3 posicion=bezier(t)+glm::vec3(0,0.10f,0);
  glm::vec3 direccion=tangente(t);
  // Giro horizontal alrededor de Y.
  float angulo=std::atan2(direccion.x,direccion.z)+glm::radians(CAR_YAW_OFFSET);
  glm::mat4 modelo=glm::translate(glm::mat4(1),posicion);
  modelo=glm::rotate(modelo,angulo,glm::vec3(0,1,0));
  // Inclinacion alrededor de X para subir o bajar pendientes.
  float inclinacion=std::atan2(direccion.y,glm::length(glm::vec2(direccion.x,direccion.z)));
  modelo=glm::rotate(modelo,-inclinacion,glm::vec3(1,0,0))*ajuste;
  for(uint32_t i=0;i<scene.scenes[escena].nodes_count;++i)drawNode(scene.scenes[escena].nodes[i],modelo);
  // Ordenar vidrio y otros materiales transparentes de lejos a cerca.
  auto distancia=[&](const TransparentMesh& item){
   glm::vec3 p=glm::vec3(item.world*glm::vec4((item.mesh->minPos+item.mesh->maxPos)*.5f,1));
   glm::vec3 d=p-cameraPos;return glm::dot(d,d);
  };
  std::sort(transparentes.begin(),transparentes.end(),[&](const TransparentMesh& a,const TransparentMesh& b){return distancia(a)>distancia(b);});
  glEnable(GL_BLEND);glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);glDepthMask(GL_FALSE);
  for(const auto& item:transparentes){if(item.nieve!=esNieve)cambiarModelo();drawMesh(*item.mesh,item.world,true);}
  if(esNieve)cambiarModelo();
  glDepthMask(GL_TRUE);glDisable(GL_BLEND);glFrontFace(GL_CCW);
  glDisable(GL_CULL_FACE);
  glUseProgram(lightProgram);
  glm::mat4 sphereModel=glm::scale(glm::translate(glm::mat4(1.0f),lightPos),glm::vec3(0.35f));
  glUniformMatrix4fv(glGetUniformLocation(lightProgram,"model"),1,GL_FALSE,glm::value_ptr(sphereModel));
  glUniformMatrix4fv(glGetUniformLocation(lightProgram,"view"),1,GL_FALSE,glm::value_ptr(view));
  glUniformMatrix4fv(glGetUniformLocation(lightProgram,"proj"),1,GL_FALSE,glm::value_ptr(proj));
  glUniform3fv(glGetUniformLocation(lightProgram,"lightColor"),1,glm::value_ptr(lightColor));
  glBindVertexArray(sphere.vao);glDrawElements(GL_TRIANGLES,sphere.count,GL_UNSIGNED_INT,0);
  glBindVertexArray(0);glfwSwapBuffers(window);
 }
 for(auto& mesh:gpuMeshes)for(auto& g:mesh){glDeleteVertexArrays(1,&g.vao);glDeleteBuffers(1,&g.vbo);glDeleteBuffers(1,&g.ebo);}
 for(GLuint t:gpuTextures)if(t)glDeleteTextures(1,&t);
 glDeleteVertexArrays(1,&sphere.vao);glDeleteBuffers(1,&sphere.vbo);glDeleteBuffers(1,&sphere.ebo);
 glDeleteProgram(sceneProgram);glDeleteProgram(lightProgram);
 for(auto& mesh:mallasNieve)for(auto& g:mesh){glDeleteVertexArrays(1,&g.vao);glDeleteBuffers(1,&g.vbo);glDeleteBuffers(1,&g.ebo);}
 for(GLuint t:texturasNieve)if(t)glDeleteTextures(1,&t);
 tg3_model_free(&nieve);tg3_model_free(&scene);
 glfwDestroyWindow(window);glfwTerminate();return 0;
}

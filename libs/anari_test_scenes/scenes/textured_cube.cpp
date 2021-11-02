// Copyright 2021 The Khronos Group
// SPDX-License-Identifier: Apache-2.0

#include "textured_cube.h"

#include "anari/anari_cpp/ext/glm.h"
#include "glm/ext/matrix_transform.hpp"

static void anari_free(void *ptr, void *)
{
  std::free(ptr);
}

namespace anari {
namespace scenes {

static std::vector<glm::vec3> vertices = {
    //
    {-.5f, .5f, 0.f},
    {.5f, .5f, 0.f},
    {-.5f, -.5f, 0.f},
    {.5f, -.5f, 0.f},
    //
};

static std::vector<glm::uvec3> indices = {
    //
    {0, 2, 3},
    {3, 1, 0},
    //
};

static std::vector<glm::vec2> texcoords = {
    //
    {0.f, 1.f},
    {1.f, 1.f},
    {0.f, 0.f},
    {1.f, 0.f},
    //
};

static anari::Array2D makeTextureData(ANARIDevice d, int dim)
{
  auto *data = new glm::vec3[dim * dim];

  for (int h = 0; h < dim; h++) {
    for (int w = 0; w < dim; w++) {
      bool even = h & 1;
      if (even)
        data[h * dim + w] = w & 1 ? glm::vec3(.8f) : glm::vec3(.2f);
      else
        data[h * dim + w] = w & 1 ? glm::vec3(.2f) : glm::vec3(.8f);
    }
  }

  return anariNewArray2D(
      d, data, &anari_free, nullptr, ANARI_FLOAT32_VEC3, dim, dim);
}

// CornelBox definitions //////////////////////////////////////////////////////

TexturedCube::TexturedCube(ANARIDevice d) : TestScene(d)
{
  m_world = anariNewWorld(m_device);
}

TexturedCube::~TexturedCube()
{
  anari::release(m_device, m_world);
}

ANARIWorld TexturedCube::world()
{
  return m_world;
}

void TexturedCube::commit()
{
  ANARIDevice d = m_device;

  ANARIArray1D vertexData = anariNewArray1D(d,
      vertices.data(),
      nullptr,
      nullptr,
      ANARI_FLOAT32_VEC3,
      vertices.size());

  ANARIArray1D texcoordData = anariNewArray1D(d,
      texcoords.data(),
      nullptr,
      nullptr,
      ANARI_FLOAT32_VEC2,
      texcoords.size());

  ANARIArray1D indexData = anariNewArray1D(
      d, indices.data(), nullptr, nullptr, ANARI_UINT32_VEC3, indices.size());

  ANARIGeometry geom = anariNewGeometry(d, "mesh");
  anari::setAndReleaseParameter(d, geom, "vertex.position", vertexData);
  anari::setAndReleaseParameter(d, geom, "vertex.texcoord", texcoordData);
  anari::setAndReleaseParameter(d, geom, "index", indexData);
  anariCommit(d, geom);

  ANARISurface surface = anariNewSurface(d);
  anari::setAndReleaseParameter(d, surface, "geometry", geom);

  ANARISampler tex = anariNewSampler(d, "texture2d");
  anari::setAndReleaseParameter(d, tex, "data", makeTextureData(d, 8));
  anari::setParameter(d, tex, "filter", "nearest");
  anariCommit(d, tex);

  ANARIMaterial mat = anariNewMaterial(d, "matte");
  anari::setAndReleaseParameter(d, mat, "map_kd", tex);
  anariCommit(d, mat);
  anari::setAndReleaseParameter(d, surface, "material", mat);
  anariCommit(d, surface);

  ANARIArray1D surfaceArray =
      anariNewArray1D(d, &surface, nullptr, nullptr, ANARI_SURFACE, 1);

  ANARIGroup group = anariNewGroup(d);
  anari::setParameter(d, group, "surface", surfaceArray);
  anariCommit(d, group);

  anariRelease(d, surfaceArray);
  anariRelease(d, surface);

  std::vector<ANARIInstance> instances;

  auto createInstance = [&](float rotation, glm::vec3 axis) {
    ANARIInstance inst = anariNewInstance(d);

    auto tl = glm::translate(glm::mat4(1.f), glm::vec3(0, 0, .5f));
    auto rot = glm::rotate(glm::mat4(1.f), rotation, axis);
    glm::mat4x3 xfm = rot * tl;
    anari::setParameter(d, inst, "transform", xfm);
    anari::setParameter(d, inst, "group", group);
    anariCommit(d, inst);
    return inst;
  };

  instances.push_back(createInstance(glm::radians(0.f), glm::vec3(0, 1, 0)));
  instances.push_back(createInstance(glm::radians(180.f), glm::vec3(0, 1, 0)));
  instances.push_back(createInstance(glm::radians(90.f), glm::vec3(0, 1, 0)));
  instances.push_back(createInstance(glm::radians(270.f), glm::vec3(0, 1, 0)));
  instances.push_back(createInstance(glm::radians(90.f), glm::vec3(1, 0, 0)));
  instances.push_back(createInstance(glm::radians(270.f), glm::vec3(1, 0, 0)));

  ANARIArray1D instanceArray = anariNewArray1D(
      d, instances.data(), nullptr, nullptr, ANARI_INSTANCE, instances.size());

  anari::setAndReleaseParameter(d, m_world, "instance", instanceArray);

  anariRelease(d, group);
  for (auto i : instances)
    anariRelease(d, i);

  setDefaultAmbientLight(m_world);

  anariCommit(d, m_world);
}

std::vector<Camera> TexturedCube::cameras()
{
  Camera cam;
  cam.position = glm::vec3(1.25f);
  cam.at = glm::vec3(0.f);
  cam.direction = glm::normalize(cam.at - cam.position);
  cam.up = glm::vec3(0, 1, 0);
  return {cam};
}

TestScene *sceneTexturedCube(ANARIDevice d)
{
  return new TexturedCube(d);
}

} // namespace scenes
} // namespace anari
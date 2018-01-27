#include <fstream>
#include <sstream>
#include <stdexcept>
#include <algorithm>
#include <iostream>
#include "Display.hpp"
#include "Bind.hpp"

#include <cstring>
#include <cerrno>

inline RenderContext contextFromFiles(std::string name)
{
  std::stringstream vert;
  std::stringstream frag;
  std::ifstream vertInput("shaders/" + name + ".vert");
  std::ifstream fragInput("shaders/" + name + ".frag");

  if (!fragInput || !vertInput)
  {
	  std::cout << "shaders/" + name + ".vert" << std::endl;
	  std::cout << "shaders/" + name + ".frag" << std::endl;
	  throw std::runtime_error(strerror(errno));
  }
  vert << vertInput.rdbuf();
  frag << fragInput.rdbuf();
  return {Vao(), my_opengl::createProgram<2>({static_cast<unsigned int>(GL_VERTEX_SHADER), static_cast<unsigned int>(GL_FRAGMENT_SHADER)},
                                             {vert.str(), frag.str()})};
}

Display::GlfwContext::GlfwContext()
{
  glfwSetErrorCallback([](int, char const *str)
                       {
                         throw std::runtime_error(str);
                       });
  if (!glfwInit())
    throw std::runtime_error("opengl: failed to initialize glfw");

}

Display::GlfwContext::~GlfwContext()
{
  glfwTerminate();
}

Display::Display()
  : camera{}
  , window([this]{
      glfwWindowHint(GLFW_DEPTH_BITS, 1);
      std::unique_ptr<GLFWwindow, decltype(&glfwDestroyWindow)> window(glfwCreateWindow(1920, 1080, "Nano Swarm II", nullptr, nullptr), &glfwDestroyWindow);

      if (!window)
        throw std::runtime_error("opengl: failed to open window");
      glfwMakeContextCurrent(window.get());
      glfwSwapInterval(1);
      if (gl3wInit())
        throw std::runtime_error("opengl: failed to initialize 3.0 bindings");
      if (!gl3wIsSupported(3, 0))
        throw std::runtime_error("opengl: Opengl 3.0 not supported");
      return window;
    }())
  , fontHandler("./resources/ObelixPro-Broken-cyr.ttf")
  , textureContext(contextFromFiles("texture"))
  , textContext(contextFromFiles("text"))
  , planetRenderTexture({1024u, 1024u})
  , size{0.0f, 0.0f}
  , dim{0.0f, 0.0f}
{
  static auto setFrameBuffer =
    [this] (int width, int height)
    {
      glViewport(0, 0, width, height);
      size = {static_cast<float>(width), static_cast<float>(height)};
      dim = {static_cast<float>(height) / static_cast<float>(width), 1.0f};
    };

  glfwSetFramebufferSizeCallback(window.get(), [] (GLFWwindow *, int width, int height) {
      setFrameBuffer(width, height);
    });
  setFrameBuffer(1920, 1080);

  {
    Bind<RenderContext> bind(textureContext);

    glBindBuffer(GL_ARRAY_BUFFER, textureBuffer);
    glBufferData(GL_ARRAY_BUFFER, 0, nullptr, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(glGetAttribLocation(textureContext.program, "coord"), 2, GL_FLOAT, false, 4 * sizeof(float), nullptr);
    glVertexAttribPointer(glGetAttribLocation(textureContext.program, "pos"), 2, GL_FLOAT, false, 4 * sizeof(float), reinterpret_cast<void *>(2u * sizeof(float)));
  }
  {
    Bind<RenderContext> bind(rectContext);

    glBindBuffer(GL_ARRAY_BUFFER, rectBuffer);
    glBufferData(GL_ARRAY_BUFFER, 0, nullptr, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(glGetAttribLocation(textContext.program, "pos"), 2, GL_FLOAT, false, 2 * sizeof(float), nullptr);
  }
  {
    Bind<RenderContext> bind(textContext);

    glBindBuffer(GL_ARRAY_BUFFER, textBuffer);
    glBufferData(GL_ARRAY_BUFFER, 0, nullptr, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(glGetAttribLocation(textContext.program, "coord"), 2, GL_FLOAT, false, 4 * sizeof(float), nullptr);
    glVertexAttribPointer(glGetAttribLocation(textContext.program, "pos"), 2, GL_FLOAT, false, 4 * sizeof(float), reinterpret_cast<void *>(2u * sizeof(float)));
  }
}

Display::~Display()
{
}

GLFWwindow *Display::getWindow() const {
  return window.get();
}

void Display::displayText(std::string const &text, unsigned int fontSize, claws::Vect<2u, float> step, claws::Vect<2u, float> textPos, claws::Vect<2u, float> rotation, claws::Vect<3u, float> color)
{
  fontHandler.renderText(text, [this, textPos, rotation, color](claws::Vect<2u, float> pen, claws::Vect<2u, float> size, unsigned char *buffer, claws::Vect<2u, int> fontDim)
                         {
                           Texture texture;
                           Bind<RenderContext> bind(textContext);

                           glActiveTexture(GL_TEXTURE0);
                           glBindTexture(GL_TEXTURE_2D, texture);
                           glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
                           glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
                           glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
                           glTexImage2D(GL_TEXTURE_2D,
                                        0,
                                        GL_RED,
                                        fontDim[0],
                                        fontDim[1],
                                        0,
                                        GL_RED,
                                        GL_UNSIGNED_BYTE,
                                        static_cast<void *>(buffer));
                           float data[16];

                           for (unsigned int i(0); !(i & 4u); ++i)
                             {
                               claws::Vect<2u, float> corner{static_cast<float>(i & 1u), static_cast<float>(i >> 1u)};
                               claws::Vect<2u, float> destCorner(rotate(pen + textPos + corner * size, rotation));

                               data[i * 4 + 0] = corner[0];
                               data[i * 4 + 1] = 1.0f - corner[1];
                               data[i * 4 + 2] = destCorner[0];
                               data[i * 4 + 3] = destCorner[1];
                             }
                           glBindBuffer(GL_ARRAY_BUFFER, textBuffer);
                           glBufferData(GL_ARRAY_BUFFER, sizeof(data), data, GL_STATIC_DRAW);
                           my_opengl::setUniform(dim, "dim", textContext.program);
                           my_opengl::setUniform(color, "textColor", textContext.program);
                           my_opengl::setUniform(0u, "tex", textContext.program);
                           glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
                         }, fontSize, step);
}

void Display::displayRect(Rect const &rect)
{
  Bind<RenderContext> bind(rectContext);
  float buffer[4u * 4u];

  for (unsigned int j(0u); j != 4u; ++j)
    {
      claws::Vect<2u, float> const corner(static_cast<float>(j & 1u), static_cast<float>(j >> 1u));
      claws::Vect<2u, float> const destCorner(corner * rect.size + rect.pos);

      std::copy(&corner[0u], &corner[2u], &buffer[j * 4u]);
      std::copy(&destCorner[0u], &destCorner[2u], &buffer[j * 4u + 2u]);
    }
  glActiveTexture(GL_TEXTURE0);
  glBindBuffer(GL_ARRAY_BUFFER, rectBuffer);
  my_opengl::setUniform(dim, "dim", rectContext.program);
  my_opengl::setUniform(rect.color, "rect_color", rectContext.program);
  glBufferData(GL_ARRAY_BUFFER, sizeof(buffer), buffer, GL_STATIC_DRAW);
  glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

void Display::displayRenderableAsHUD(Renderable const& renderable, GLuint texture)
{
  Bind<RenderContext> bind(textureContext);
  float buffer[4u * 4u];
  claws::Vect<2u, float> const up(renderable.destPos.normalized());

  for (unsigned int j(0u); j != 4u; ++j)
    {
      claws::Vect<2u, float> const corner(static_cast<float>(j & 1u), static_cast<float>(j >> 1u));
      claws::Vect<2u, float> const sourceCorner(renderable.sourcePos + corner * renderable.sourceSize);
      claws::Vect<2u, float> const destCorner(renderable.destPos + (corner * renderable.destSize));

      std::copy(&sourceCorner[0u], &sourceCorner[2u], &buffer[j * 4u]);
      std::copy(&destCorner[0u], &destCorner[2u], &buffer[j * 4u + 2u]);
    }
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, texture);
  glBindBuffer(GL_ARRAY_BUFFER, textureBuffer);
  my_opengl::setUniform(dim, "dim", textureContext.program);
  my_opengl::setUniform(0u, "tex", textureContext.program);
  glBufferData(GL_ARRAY_BUFFER, sizeof(buffer), buffer, GL_STATIC_DRAW);
  glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

void Display::render()
{
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  float scale(0.0f);
  glClearColor(scale, scale, scale, scale);
  glClear(GL_COLOR_BUFFER_BIT);

  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glEnable(GL_BLEND);
  for (auto const &renderables : displayInfo.entityRenderables) {
    displayRenderables(renderables.second.begin(), static_cast<GLuint>(renderables.second.size()), renderables.first);
  }
  for (auto const &renderables : displayInfo.renderables) {
    displayRenderables(renderables.second.begin(), static_cast<GLuint>(renderables.second.size()), renderables.first);
  }
  displayInterface();
  glDisable(GL_BLEND);
  glfwSwapBuffers(window.get());
  glfwPollEvents();
}

void Display::displayInterface()
{
	//displayRect(Rect(claws::Vect( 0.8, 0.8 ), { 0.1, 0.1 }, { 0.0, 0.0, 0.9, 0.5 }));
	displayRenderableAsHUD({ claws::Vect<2u, float>(0.0f, 0.0f),
		claws::Vect<2u, float>(1.0f, 1.0f),
		claws::Vect<2u, float>(0.0f, 0.0f),
		claws::Vect<2u, float>(1.0f, 1.0f) },
		textureHandler[TextureHandler::TextureList::TEST]);


	displayText(displayInfo.time, 256, { 0.075f, 0.075f }, { 0.8f, 0.8f }, { 0.0f, 0.0f }, { 1.0f, 1.0f, 1.0f });
}

void Display::copyRenderData(Logic const &logic)
{
  auto renderEntity = [this](auto entity)
    {
      displayInfo.entityRenderables[this.textureHandler.getTexture(entity.getTexture())]
      .push_back({{0.0f, 0.0f}, {1.0f, 1.0f}, fixture.pos, {1.0f, 0.0f}});
    };

  displayInfo.entityRenderables.clear();
  displayInfo.entityRenderables[textureHandler.getTexture(TextureHandler::TextureList::BOMB_SPRITE)]
    .push_back({{0.0f, 0.0f}, {1.0f / 6.0f, 1.0f}, {0.0f, 0.0f}, {1.0f, 0.0f}});
  logic.getEntityManager().allies.iterOnTeam(renderEntity, textureHandler);
  logic.getEntityManager().ennemies.iterOnTeam(renderEntity, textureHandler);
}

bool Display::isRunning() const
{
  return (!glfwWindowShouldClose(window.get()));
}

bool Display::isKeyPressed(int key) const
{
  return glfwGetKey(window.get(), key);
}

Camera const &Display::getCamera() const
{
  return camera;
}

claws::Vect<2u, float> Display::getDim() const
{
  return dim;
}

claws::Vect<2u, float> Display::getSize() const
{
  return size;
}

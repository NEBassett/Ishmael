#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <stdlib.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include <tuple>
#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <array>
#include <vector>
#include <complex>
#include <random>
#include <math.h>
#include <utility>
#include <algorithm>
#include <boost/hana/tuple.hpp>
#include <boost/hana/zip.hpp>
#include <boost/hana/for_each.hpp>
#include <boost/hana/ext/std/tuple.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include "program.hpp"

const std::string g_infoPath   = "../src/config.xml";
const std::string g_vertexPath = "../src/vertex.vs";
const std::string g_fragmentPath = "../src/fragment.fs";

void GLAPIENTRY msgCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam )
{
  std::cout << message << '\n';
}



struct simInfo
{
  size_t gridlen, simlen, diffuseIterations, pressureIterations;
  float density, viscosity;

  simInfo(const std::string &filename)
  {
    boost::property_tree::ptree settings;
    boost::property_tree::xml_parser::read_xml(filename, settings);
    gridlen = settings.get<unsigned int>("simInfo.gridlen");
    simlen = settings.get<unsigned int>("simInfo.gridlen"); // for now
    density = settings.get<float>("simInfo.density");
    viscosity = settings.get<float>("simInfo.viscosity");
    diffuseIterations = settings.get<unsigned int>("simInfo.diffuseIterations");
    pressureIterations = settings.get<unsigned int>("simInfo.pressureIterations");
  }
};



struct simulationSpace
{
  GLuint vao;
  GLuint vbo;

  simulationSpace(const simInfo& info)
  {
    std::vector<glm::vec4> grid;

    for(auto i = size_t{0}; i < info.gridlen; i++)
    {
      for(auto j = size_t{0}; j < info.gridlen; j++)
      {
        for(auto k = size_t{0}; k < info.gridlen; k++)
        {
          grid.emplace_back(
            info.simlen*float(k)/(info.gridlen),
            info.simlen*float(j)/(info.gridlen),
            info.simlen*float(i)/(info.gridlen),
            1.0f);
        }
      }
    }

    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec4)*grid.size(), grid.data(), GL_STATIC_DRAW);

    glGenVertexArrays(1, &vao);

    glBindVertexArray(vao);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, sizeof(glm::vec4), (void* )0);
    glEnableVertexAttribArray(0);
  }
};



struct screenQuad
{
  GLuint vao, vbo;

  screenQuad()
  {
    static auto verts = std::array<glm::vec4, 6>{
            glm::vec4(1.0f,  1.0f, 0.0f, 0.0f),
            glm::vec4(1.0f, -1.0f, 0.0f, 0.0f),
            glm::vec4(-1.0f,  1.0f, 0.0f, 1.0f),
            glm::vec4(-1.0f, -1.0f, 0.0f, 1.0f),
            glm::vec4(1.0f, -1.0f, 0.0f, 1.0f),
            glm::vec4(-1.0f, 1.0f, 0.0f, 1.0f)
    };

    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec4)*6, verts.data(), GL_STATIC_DRAW);

    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, sizeof(glm::vec4), (void*)0);
    glEnableVertexAttribArray(0);
  }
};

class fluidSim
{
  using jacobiType = decltype(
    GLDSEL::make_program_from_paths(
      boost::hana::make_tuple("", ""),
      glDselUniform("dim", int),
      glDselUniform("beta", float),
      glDselUniform("alpha", float),
      glDselUniform("old", int)
    )
  );

  using advectType = decltype(
    GLDSEL::make_program_from_paths(
      boost::hana::make_tuple("", ""),
      glDselUniform("dim", int),
      glDselUniform("tdelta", float),
      glDselUniform("scale", float),
      glDselUniform("old", int),
      glDselUniform("velocity", int)
    )
  );

  using forceType = decltype(
    GLDSEL::make_program_from_paths(
      boost::hana::make_tuple("", ""),
      glDselUniform("dim", int),
      glDselUniform("tdelta", float),
      glDselUniform("old", int),
      glDselUniform("fluid", int)
    )
  );

  using divergenceType = decltype(
    GLDSEL::make_program_from_paths(
      boost::hana::make_tuple("", ""),
      glDselUniform("dim", int),
      glDselUniform("delta", float)
    )
  );

  using projectType = decltype(
    GLDSEL::make_program_from_paths(
      boost::hana::make_tuple("", ""),
      glDselUniform("dim", int),
      glDselUniform("delta", float)
    )
  );

  using boundaryType = decltype(
    GLDSEL::make_program_from_paths(
      boost::hana::make_tuple("", ""),
      glDselUniform("dim", int),
      glDselUniform("scale", float)
    )
  );

  using texclearType = decltype(
    GLDSEL::make_program_from_paths(
      boost::hana::make_tuple("", ""),
      glDselUniform("dim", int)
    )
  );

  simInfo info_;
  jacobiType jacobiProgram_;
  advectType advectProgram_;
  forceType forceProgram_;
  divergenceType divergenceProgram_;
  projectType projectionProgram_;
  boundaryType boundaryProgram_;
  texclearType texclearProgram_;
  GLuint intermediate_;
  GLuint intermediateDiv_;
  GLuint intermediateFluid_;
  GLuint velocity_;
  GLuint pressure_;
  GLuint fluid_;
  GLuint fbuf_;
  GLuint fbuftex_;
  screenQuad quad_;

  auto cleanup()
  {
    glDeleteTextures(1, &intermediate_);
    glDeleteTextures(1, &velocity_);
    glDeleteTextures(1, &pressure_);
    glDeleteTextures(1, &fluid_);
    glDeleteFramebuffers(1, &fbuf_);
    glDeleteTextures(1, &fbuftex_);
    glDeleteTextures(1, &intermediateFluid_);
  }

  auto advect(float timeDelta, GLuint &quantity, GLuint &intermed)
  {
    glBindImageTexture(0, intermed, 0, GL_TRUE, 0, GL_WRITE_ONLY, GL_RGBA32F);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_3D, quantity);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_3D, velocity_);

    advectProgram_.setUniforms(
      glDselArgument("dim", info_.gridlen),
      glDselArgument("scale", 1.0f/float{info_.simlen}),
      glDselArgument("tdelta", timeDelta),
      glDselArgument("old", 0),
      glDselArgument("velocity", 1)
    );

    glDispatchCompute(info_.gridlen, info_.gridlen, info_.gridlen);

    std::swap(quantity, intermed);
  }

  auto force(float timeDelta)
  {
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_3D, velocity_);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_3D, fluid_);
    glBindImageTexture(0, intermediate_, 0, GL_TRUE, 0, GL_WRITE_ONLY, GL_RGBA32F);

    forceProgram_.setUniforms(
      glDselArgument("dim", info_.gridlen),
      glDselArgument("tdelta", timeDelta),
      glDselArgument("old", 0),
      glDselArgument("fluid", 1)
    );

    glDispatchCompute(info_.gridlen, info_.gridlen, info_.gridlen);

    std::swap(velocity_, intermediate_); // swap handles
  }

  auto fluidBoundary()
  {
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_3D, fluid_);
    glBindImageTexture(0, intermediateFluid_, 0, GL_TRUE, 0, GL_WRITE_ONLY, GL_RGBA32F);

    boundaryProgram_.setUniforms(
      glDselArgument("dim", info_.gridlen),
      glDselArgument("scale", 0.0f)
    );

    glDispatchCompute(info_.gridlen, info_.gridlen, info_.gridlen);

    std::swap(fluid_, intermediateFluid_);
  }

  auto velocityBoundary()
  {
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_3D, velocity_);
    glBindImageTexture(0, intermediate_, 0, GL_TRUE, 0, GL_WRITE_ONLY, GL_RGBA32F);

    boundaryProgram_.setUniforms(
      glDselArgument("dim", info_.gridlen),
      glDselArgument("scale", -1.0f)
    );

    glDispatchCompute(info_.gridlen, info_.gridlen, info_.gridlen);

    std::swap(velocity_, intermediate_);
  }

  auto pressureBoundary()
  {
    boundaryProgram_.setUniforms(
      glDselArgument("dim", info_.gridlen),
      glDselArgument("scale", 1.0f)
    );

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_3D, pressure_);
    glBindImageTexture(0, intermediate_, 0, GL_TRUE, 0, GL_WRITE_ONLY, GL_RGBA32F);

    glDispatchCompute(info_.gridlen, info_.gridlen, info_.gridlen);

    std::swap(pressure_, intermediate_);
  }

  auto projection()
  {
    static auto diffprog = GLDSEL::make_program_from_paths(
        boost::hana::make_tuple(boost::none,boost::none,boost::none,boost::none,boost::none, "../src/difference/main.cs"),
        glDselUniform("dim", int),
        glDselUniform("delta", float)
    );

    velocityBoundary();

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_3D, velocity_);
    glBindImageTexture(0, intermediate_, 0, GL_TRUE, 0, GL_WRITE_ONLY, GL_RGBA32F);

    auto halfdelta = float{0.5f*float{info_.simlen}/info_.gridlen};

    divergenceProgram_.setUniforms(
      glDselArgument("dim", info_.gridlen),
      glDselArgument("delta", halfdelta)
    );

    glDispatchCompute(info_.gridlen, info_.gridlen, info_.gridlen);

    glMemoryBarrier(GL_ALL_BARRIER_BITS);
    std::swap(intermediateDiv_, intermediate_);

    // solver converges slower if this is uncommented
    // glBindImageTexture(0, intermediate_, 0, GL_TRUE, 0, GL_WRITE_ONLY, GL_RGBA32F);
    //
    // texclearProgram_.setUniforms(
    //   glDselArgument("dim", info_.gridlen)
    // );
    //
    // glDispatchCompute(info_.gridlen, info_.gridlen, info_.gridlen);
    //
    // glMemoryBarrier(GL_ALL_BARRIER_BITS);
    // std::swap(pressure_, intermediate_); // set pressure to 0

    auto delta = float{info_.simlen}/info_.gridlen;  // gridspacing
    float alpha = -delta*delta*delta; // one unit volume
    float beta = 6.0f; // divide by 6 to average

    jacobiProgram_.setUniforms(
      glDselArgument("dim", info_.gridlen),
      glDselArgument("alpha", alpha),
      glDselArgument("beta", beta),
      glDselArgument("old", 0)
    );

    for(auto i = size_t{0}; i < info_.pressureIterations; i++) // perform jacobi iterations
    {
      pressureBoundary();

      jacobiProgram_.activate();
      glActiveTexture(GL_TEXTURE0);
      glBindTexture(GL_TEXTURE_3D, pressure_);
      glBindImageTexture(1, intermediate_, 0, GL_TRUE, 0, GL_WRITE_ONLY, GL_RGBA32F);
      glBindImageTexture(2, intermediateDiv_, 0, GL_TRUE, 0, GL_READ_ONLY, GL_RGBA32F);

      glDispatchCompute(info_.gridlen, info_.gridlen, info_.gridlen);

      glMemoryBarrier(GL_ALL_BARRIER_BITS);
      std::swap(pressure_, intermediate_); // swap handles
    }

    pressureBoundary();

    // a diffprog here is nice for tinkering around / checking the results of the poisson solver
    // glBindImageTexture(0, intermediate_, 0, GL_TRUE, 0, GL_WRITE_ONLY, GL_RGBA32F);
    // glBindImageTexture(1, intermediateDiv_, 0, GL_TRUE, 0, GL_READ_ONLY, GL_RGBA32F);
    // glActiveTexture(GL_TEXTURE0);
    // glBindTexture(GL_TEXTURE_3D, pressure_);
    //
    // diffprog.setUniforms(
    //     glDselArgument("dim", info_.gridlen),
    //     glDselArgument("delta", delta*delta)
    // );
    //
    // glDispatchCompute(info_.gridlen, info_.gridlen, info_.gridlen);
    // std::swap(intermediateDiv_, intermediate_);

    projectionProgram_.setUniforms(
        glDselArgument("dim", info_.gridlen),
        glDselArgument("delta", halfdelta)
    );

    glBindTexture(GL_TEXTURE_3D, pressure_);
    glBindImageTexture(0, intermediate_, 0, GL_TRUE, 0, GL_WRITE_ONLY, GL_RGBA32F);
    glBindImageTexture(1, velocity_, 0, GL_TRUE, 0, GL_READ_ONLY, GL_RGBA32F);

    glDispatchCompute(info_.gridlen, info_.gridlen, info_.gridlen);

    glMemoryBarrier(GL_ALL_BARRIER_BITS);
    std::swap(velocity_, intermediate_);
  }

  auto diffuse(float timeDelta)
  {
    auto delta = float{info_.simlen}/info_.gridlen;  // gridspacing
    auto alpha = float{delta*delta*delta/(info_.viscosity*timeDelta)};
    auto beta = alpha + 6.0f;

    jacobiProgram_.setUniforms(
      glDselArgument("dim", info_.gridlen),
      glDselArgument("alpha", alpha),
      glDselArgument("beta", beta),
      glDselArgument("old", 0)
    );

    for(auto i = size_t{0}; i < info_.diffuseIterations; i++)
    {
      glBindTexture(GL_TEXTURE_3D, velocity_);
      glBindImageTexture(1, intermediate_, 0, GL_TRUE, 0, GL_WRITE_ONLY, GL_RGBA32F);
      glBindImageTexture(2, velocity_, 0, GL_TRUE, 0, GL_READ_ONLY, GL_RGBA32F);

      glDispatchCompute(info_.gridlen, info_.gridlen, info_.gridlen);

      std::swap(velocity_, intermediate_); // swap handles
    }
  }

public:
  fluidSim(const simInfo &info) :
   info_(info),
   jacobiProgram_(
     GLDSEL::make_program_from_paths(
       boost::hana::make_tuple(boost::none,boost::none,boost::none,boost::none,boost::none, "../src/jacobiIteration/main.cs"),
       glDselUniform("dim", int),
       glDselUniform("beta", float),
       glDselUniform("alpha", float),
       glDselUniform("old", int)
     )
   ),
   advectProgram_(
     GLDSEL::make_program_from_paths(
       boost::hana::make_tuple(boost::none,boost::none,boost::none,boost::none,boost::none, "../src/advection/main.cs"),
       glDselUniform("dim", int),
       glDselUniform("tdelta", float),
       glDselUniform("scale", float),
       glDselUniform("old", int),
       glDselUniform("velocity", int)
     )
   ),
   forceProgram_(
     GLDSEL::make_program_from_paths(
       boost::hana::make_tuple(boost::none,boost::none,boost::none,boost::none,boost::none, "../src/force/main.cs"),
       glDselUniform("dim", int),
       glDselUniform("tdelta", float),
       glDselUniform("old", int),
       glDselUniform("fluid", int)
     )
   ),
   divergenceProgram_(
     GLDSEL::make_program_from_paths(
       boost::hana::make_tuple(boost::none,boost::none,boost::none,boost::none,boost::none, "../src/divergence/main.cs"),
       glDselUniform("dim", int),
       glDselUniform("delta", float)
     )
   ),
   projectionProgram_(
     GLDSEL::make_program_from_paths(
       boost::hana::make_tuple(boost::none,boost::none,boost::none,boost::none,boost::none, "../src/projection/main.cs"),
       glDselUniform("dim", int),
       glDselUniform("delta", float)
     )
   ),
   boundaryProgram_(
     GLDSEL::make_program_from_paths(
       boost::hana::make_tuple(boost::none,boost::none,boost::none,boost::none,boost::none, "../src/boundary/main.cs"),
       glDselUniform("dim", int),
       glDselUniform("scale", float)
     )
   ),
   texclearProgram_(
     GLDSEL::make_program_from_paths(
       boost::hana::make_tuple(boost::none,boost::none,boost::none,boost::none,boost::none, "../src/texclear/main.cs"),
         glDselUniform("dim", int)
     )
   ),
   intermediate_(0),
   intermediateDiv_(0),
   intermediateFluid_(0),
   velocity_(0),
   pressure_(0),
   fluid_(0),
   fbuf_(0),
   fbuftex_(0),
   quad_()
  {
    const auto newVolume = [this](auto&& seed, GLenum border = GL_CLAMP_TO_EDGE)
    {
      std::vector<glm::vec4> data;
      for(auto i = size_t{0}; i < info_.gridlen; i++)
      {
        for(auto j = size_t{0}; j < info_.gridlen; j++)
        {
          for(auto k = size_t{0}; k < info_.gridlen; k++)
          {
            data.emplace_back(seed(k,j,i));
          }
        }
      }

      GLuint newV;
      glGenTextures(1, &newV);
      glBindTexture(GL_TEXTURE_3D, newV);
      glTexImage3D(GL_TEXTURE_3D, 0, GL_RGBA32F, info_.gridlen, info_.gridlen, info_.gridlen, 0, GL_RGBA, GL_FLOAT, data.data());
      glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
      glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
      glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, border);
      glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, border);
      glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, border);

      float color[] = { 0.0f, 0.0f, 0.0f, 0.0f };
      glTexParameterfv(GL_TEXTURE_3D, GL_TEXTURE_BORDER_COLOR, color);

      return newV;
    };

    // i = info.gridlen*info.gridlen*x + info.gridlen*y + z

    intermediate_ = newVolume([](const auto &...){return glm::vec4();});
    intermediateDiv_ = newVolume([](const auto &...){return glm::vec4();});
    pressure_ = newVolume([](const auto &...){return glm::vec4(0,0,0,0);});
    velocity_ = newVolume([](const auto &...){return glm::vec4(0,0,0,0);});
    fluid_ = newVolume([this](const auto x, const auto y, const auto z)
    {
      auto retval = glm::vec4(0.0);
      if(y > info_.gridlen/2 && ((x != 0) &&( x != info_.gridlen-1) && (y != 0) && (y != info_.gridlen-1) && (y != info_.gridlen-2) && (z != 0) && (z != info_.gridlen-1)))
      {
        retval = retval + glm::vec4(1.0); // some at the ceiling, none beyond boundary of course
      }
      return retval;
    });
    intermediateFluid_ = newVolume([](const auto &...){return glm::vec4();});
  }

  auto bindResources()
  {
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_3D, fluid_);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_3D, velocity_);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_3D, intermediateDiv_);
  }

  auto step(float timeDelta)
  {
    velocityBoundary();
    fluidBoundary();
    glMemoryBarrier(GL_ALL_BARRIER_BITS);
    advect(timeDelta, fluid_, intermediateFluid_);
    glMemoryBarrier(GL_ALL_BARRIER_BITS);
    advect(timeDelta, velocity_, intermediate_);
    glMemoryBarrier(GL_ALL_BARRIER_BITS);
    force(timeDelta);
    glMemoryBarrier(GL_ALL_BARRIER_BITS);
    //diffuse(timeDelta);
    projection();
    glMemoryBarrier(GL_ALL_BARRIER_BITS);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glMemoryBarrier(GL_ALL_BARRIER_BITS);
  }

  ~fluidSim()
  {
    cleanup();
  }
};



void update()
{

}



int main()
{
  glfwSetErrorCallback([](auto err, const auto* desc){ std::cout << "Error: " << desc << '\n'; });

  // glfw init
  if(!glfwInit())
  {
    std::cout << "glfw failed to initialize\n";
    std::exit(1);
  }

  // context init
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
  auto window = glfwCreateWindow(640, 480, "Navier Stokes", NULL, NULL);
  if (!window)
  {
    std::cout << "window/glcontext failed to initialize\n";
    std::exit(1);
  }

  glfwMakeContextCurrent(window);

  // glew init
  auto err = glewInit();
  if(GLEW_OK != err)
  {
    std::cout << "glew failed to init: " << glewGetErrorString(err) << '\n';
    std::exit(1);
  }

  // gl init
  glEnable(GL_DEBUG_OUTPUT);
  glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
  glPointSize(15.5f);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glDebugMessageCallback(msgCallback, 0);
  glEnable(GL_DEPTH_TEST);

  glfwSetKeyCallback(window, [](auto window, auto key, auto scancode, auto action, auto mods){
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
      glfwSetWindowShouldClose(window, GLFW_TRUE);
    if(key == GLFW_KEY_F4)
    {
      update();
    }
  });

  // program initialization
  auto siminfo = simInfo{g_infoPath};

  fluidSim fsim{siminfo};
  simulationSpace space{siminfo};

  auto gridDraw = GLDSEL::make_program_from_paths(
      boost::hana::make_tuple("../src/main/vertex.vs", "../src/main/fragment.fs"),
      glDselUniform("time", float),
      glDselUniform("model", glm::mat4),
      glDselUniform("view", glm::mat4),
      glDselUniform("proj", glm::mat4),
      glDselUniform("positions", int),
      glDselUniform("pressure", int),
      glDselUniform("div", int)
  );

  int width, height;
  double time = glfwGetTime();
  glm::mat4 proj;
  double tdelta;
  double temp;



  glfwSwapInterval(1);
  while(!glfwWindowShouldClose(window))
  {
    auto oldT = time;
    time = glfwGetTime();
    fsim.step(1.0f/120);

    glfwGetFramebufferSize(window, &width, &height);
    glViewport(0, 0, width, height);
    proj = glm::perspective(1.57f, float(width)/float(height), 0.1f, 7000.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    fsim.bindResources();

    //draw(proj, view, glm::mat4(), glfwGetTime(), plane);
    gridDraw.setUniforms( // set uniforms
      glDselArgument("model", glm::mat4()),
      glDselArgument("view", glm::mat4()),
      glDselArgument("proj", proj),
      glDselArgument("time", float(glfwGetTime())),
      glDselArgument("positions", 0),
      glDselArgument("pressure", 1),
      glDselArgument("div", 2)
    );

    glBindVertexArray(space.vao);

    glDrawArrays(GL_POINTS, 0, siminfo.gridlen*siminfo.gridlen*siminfo.gridlen); // draw our points

    glfwSwapBuffers(window);
    glfwPollEvents();
  }

  glfwTerminate();
  return 0;
}

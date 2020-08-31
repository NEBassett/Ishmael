#ifndef PROGRAM_HPP_
#define PROGRAM_HPP_

#include "common.hpp"

#define glDselUniform(str, type) (boost::hana::basic_type<GLDSEL::uniform<BOOST_METAPARSE_STRING(str), type>>{})

namespace GLDSEL
{
  struct noMetadata { };
  template<typename...> class glProgram;
  template<typename, typename, typename, typename, typename, typename...> struct dselProgram;
  template<typename T> struct uniMetadata;
  template<typename,typename = void> struct getMetadata;
  template<typename name, typename T>
  using uniform = parameter<name, uniMetadata<T>>;

  GLDSEL_FUNC_DECL static auto setUniform(GLuint loc, const int x)
  {
    glUniform1i(loc, x);
  }

  GLDSEL_FUNC_DECL static auto setUniform(GLuint loc, const unsigned int x)
  {
    glUniform1ui(loc, x);
  }

  GLDSEL_FUNC_DECL static auto setUniform(GLuint loc, const float x)
  {
    glUniform1f(loc, x);
  }

  GLDSEL_FUNC_DECL static auto setUniform(GLuint loc, const glm::mat4& x)
  {
    glUniformMatrix4fv(loc, 1, GL_FALSE, &x[0][0]);
  }

  GLDSEL_FUNC_DECL static auto setUniform(GLuint loc, const glm::mat3& x)
  {
    glUniformMatrix3fv(loc, 1 , GL_FALSE, &x[0][0]);
  }

  GLDSEL_FUNC_DECL static auto setUniform(GLuint loc, const glm::vec4& x)
  {
    glUniform4fv(loc, 1, &x[0]);
  }

  GLDSEL_FUNC_DECL static auto setUniform(GLuint loc, const glm::vec3& x)
  {
    glUniform3fv(loc, 1, &x[0]);
  }

  template<typename T, typename U, typename T::isTexture = (typename T::isTexture){}>
  GLDSEL_FUNC_DECL static auto setUniform(GLuint loc, const T& tex, const U& metadata)
  {
    glActiveTexture(GL_TEXTURE0+metadata.textureUnit);
    glBindTexture(tex.target, tex.texture);
    glUniform1i(loc, tex.texture);
  }

  template<typename T, typename U, bool = T::metadata::textureBuffer>
  GLDSEL_FUNC_DECL static auto setUniform(GLuint loc, T&& tex, U&& metadata)
  {
    glActiveTexture(GL_TEXTURE0+metadata.textureUnit);
    tex.load();
    glUniform1i(loc, tex.tex);
  }



  template<typename T>
  struct getMetadata<T, decltype((void)std::declval<typename T::metadata>())>
  {
      using type = typename T::metadata;
  };

  template<typename T, typename U>
  struct getMetadata
  {
      using type = noMetadata;
  };

  template<typename T>
  struct uniMetadata
  {
    using type = T;
    using md = typename getMetadata<T>::type;
    GLuint location;
    md metadata;

    template<typename U, typename = decltype(T(std::declval<U>()))>
    GLDSEL_FUNC_DECL auto set(const U& x)
    {
      boost::hana::if_(boost::hana::type_c<md> == boost::hana::type_c<noMetadata>,
        [this, x](auto&& y) { setUniform(location, x); },
        [this, x](auto&& y) { setUniform(location, x, metadata); }
      )(x);
      // return boost::hana::if_(std::is_same<md, noMetadata>::value,
      //   [](auto&& y, auto&& mdata){ setUniform(location, std::forward<decltype(y)>(y), metadata); return 0; },
      //   [](auto&& y, auto&& mdata){ setUniform(location, std::forward<decltype(y)>(y)); return 0; }
      // )(x, metadata);
    }
  };

  template<typename... parameters>
  class glProgram
  {
    boost::hana::tuple<parameters...> args_;
    GLuint program_;
    GLuint texUnit_;
  public:
    template<typename... Ts>
    GLDSEL_FUNC_DECL glProgram(
      const boost::optional<std::string>& vertexSource,
      const boost::optional<std::string>& fragSource,
      const boost::optional<std::string>& tessContSource = boost::optional<std::string>(boost::none),
      const boost::optional<std::string>& tessEvalSource = boost::optional<std::string>(boost::none),
      const boost::optional<std::string>& geomSource = boost::optional<std::string>(boost::none),
      const boost::optional<std::string>& compSource = boost::optional<std::string>(boost::none),
      Ts&&...)
    {
      texUnit_ = 0;
      program_ = glCreateProgram();

      const auto compileShader = [](auto&& src, const auto type){
        GLuint shader = glCreateShader(type);
        GLint len = src.length();
        const char *shaderSrc = src.c_str();
        glShaderSource(shader, 1, (const GLchar**)&shaderSrc, &len);
        glCompileShader(shader);

        GLint success;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
        if(success == GL_FALSE)
        {
          GLint len;
          glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &len);
          std::string buf(size_t(len), '\0');
          glGetShaderInfoLog(shader, len, &len, &buf[0]);
          std::cerr << "Shader error:\t" << buf << '\n';
          return boost::optional<GLuint>(boost::none);
        }

        return boost::optional<GLuint>(shader);
      };

      std::vector<GLuint> shaders;

      boost::hana::for_each(
        boost::hana::zip(
          std::forward_as_tuple(vertexSource, fragSource, tessContSource, tessEvalSource, geomSource, compSource),
          boost::hana::make_tuple(GL_VERTEX_SHADER,GL_FRAGMENT_SHADER,GL_TESS_CONTROL_SHADER,GL_TESS_EVALUATION_SHADER,GL_GEOMETRY_SHADER,GL_COMPUTE_SHADER)),
        [&compileShader, &shaders, this](auto&& src){
          if (src[boost::hana::int_c<0>])
          {
            boost::optional<GLuint> shaderResult = compileShader(*(src[boost::hana::int_c<0>]), src[boost::hana::int_c<1>]);
            if(shaderResult)
            {
              glAttachShader(program_, *shaderResult);
              shaders.emplace_back(*shaderResult);
            }
          }
        }
      );

      GLint success;
      glLinkProgram(program_);
      glGetProgramiv(program_, GL_LINK_STATUS, &success);

      for(auto& shader : shaders)
      {
        glDeleteShader(shader);
      }

      if (success == GL_FALSE)
      {
        GLint len;
        glGetProgramiv(program_, GL_INFO_LOG_LENGTH, &len);
        std::string buf(size_t(len), '\0');
        glGetProgramInfoLog(program_, len, &len, &buf[0]);
        std::cerr << "Linkage error:\t" << buf << '\n';
        glDeleteProgram(program_);
      }


      boost::hana::for_each(args_,
          [this](auto&& x){
            x.data.location = glGetUniformLocation(program_, x.getParamName().c_str());
            boost::hana::if_(boost::hana::is_valid([](auto&& y) -> typename decltype(y)::type::isTexture {})(x),
              [this](auto&& y){y.data.metadata.textureUnit = texUnit_++;},
              [this](auto&&){ });
          }
      );
    }

    GLDSEL_FUNC_DECL glProgram(const glProgram &) = delete;

    GLDSEL_FUNC_DECL glProgram(glProgram &&other) : args_(std::move(other.args_)), program_(other.program_), texUnit_(other.texUnit_)
    {
      //other.args_ = 0;
      other.program_ = 0;
      other.texUnit_ = 0;
    }

    GLDSEL_FUNC_DECL auto operator=(glProgram &&other)
    {
      if(this != &other)
      {
        glDeleteProgram(program_);

        program_ = 0;
        std::swap(other.args_, args_);
        std::swap(other.program_, program_);
        std::swap(other.texUnit_, texUnit_);
      }
    }

    GLDSEL_FUNC_DECL ~glProgram()
    {
      glDeleteProgram(program_);
    }

    GLDSEL_FUNC_DECL auto activate()
    {
      glUseProgram(program_);
    }

    template<typename... arguments>
    GLDSEL_FUNC_DECL auto setUniforms(arguments&&... args)
    {
      activate();

      auto setArg = [this](auto&& x){
        constexpr auto index = getIndex(boost::hana::type_c<typename std::remove_reference<decltype(x)>::type::name>, boost::hana::type_c<typename parameters::name>...);
        if (!(index == boost::hana::nothing))
        {
          args_[index].data.set(static_cast<typename std::remove_reference<decltype(args_[index])>::type::type::type>(x.data));
        }
      };

      boost::hana::for_each(std::forward_as_tuple(args...), [&setArg](auto&& x){
        setArg(x);
      });
    }
  };



  template<typename str, typename T>
  GLDSEL_FUNC_DECL auto make_uniform(str name, T&& value)
  {
    return uniform<typename str::type, T>{std::forward<T>(value), 0};
  }

  template<typename ctorArgs, typename... parameters>
  GLDSEL_FUNC_DECL auto make_program(ctorArgs&& tuple, parameters&&... arguments)
  {
    using type = glProgram<typename parameters::type...>;
    return boost::hana::unpack(tuple, [](auto&&... xs){ return type(std::forward<decltype(xs)>(xs)...); });
  }

  template<typename paths, typename... parameters>
  GLDSEL_FUNC_DECL auto make_program_from_paths(const paths& tuple, parameters&&... arguments)
  {
    std::ifstream input;
    input.exceptions(std::ifstream::failbit|std::ifstream::badbit);

    const auto toFileContents = boost::hana::overload(
    [](const auto& lambda, const boost::optional<std::string>& opt){ if(opt){ return lambda(lambda, *opt); } return boost::optional<std::string>(boost::none); },
    [&input](const auto& lambda, const std::string& path)
    {
      std::cout << "Shader being read:\t" << path << '\n';
      std::string src;

      input.open(path);
      std::stringstream stream;
      stream << input.rdbuf();
      src = stream.str();
      input.close();

      return boost::optional<std::string>(src);
    });

    const auto tfc = [&toFileContents](const auto& arg){ return toFileContents(toFileContents, arg); };

    return make_program(boost::hana::transform(tuple, tfc), std::forward<parameters>(arguments)...);
  }

  template<typename vertexSource, typename fragSource, typename tessContSource, typename tessEvalSource, typename geomSource, typename... uniforms>
  struct staticGlProgram : public glProgram<uniforms...>
  {
    GLDSEL_FUNC_DECL staticGlProgram() : glProgram<uniforms...>(getRuntimeStr<vertexSource>(), getRuntimeStr<fragSource>(), getRuntimeStr<tessContSource>(), getRuntimeStr<tessEvalSource>(), getRuntimeStr<geomSource>())
    { }
  };
}

#endif

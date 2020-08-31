#ifndef GLDSL_COMMON_HPP_
#define GLDSL_COMMON_HPP_

#include "config.hpp"

#define glDselName(str) (boost::hana::type_c<BOOST_METAPARSE_STRING(str)>)
#define glDselArgument(str, value) (GLDSEL::make_argument(boost::hana::basic_type<BOOST_METAPARSE_STRING(str)>{}, value))

namespace GLDSEL
{
  template<typename inputBuffer, typename outputBuffer, typename input, typename drawingFunction, typename globalState, typename program>
  class processingPass
  {
    outputBuffer buf_;
    input load_;
    drawingFunction draw_;
    globalState state_;
    program prog_;

  public:
    GLDSEL_FUNC_DECL processingPass() : buf_(), load_(), draw_(), state_(), prog_()
    {
      state_();
    }

    template<typename... Ts>
    GLDSEL_FUNC_DECL auto createBuffer(Ts&&... args)
    {
      return inputBuffer(std::forward<Ts>(args)...);
    }

    template<typename... Ts>
    GLDSEL_FUNC_DECL void setState(Ts&&... inputState)
    {
      state_(std::forward<Ts>(inputState)...);
      prog_.setArgs(std::forward<Ts>(inputState)...);
      buf_.setState(std::forward<Ts>(inputState)...);
    }

    template<typename... Ts>
    GLDSEL_FUNC_DECL decltype(auto) operator()(Ts&&...  passArgs)
    {
      decltype(auto) vao = getNamedArg(boost::hana::type_c<vaoName>, passArgs...);
      buf_.prepare(vao.size());
      // buf will be your output buffer, like a fbo for normal purposes or a vbo for vertex transform feedback

      prog_.setArgs(std::forward<Ts>(passArgs)...);
      load_(draw_, std::forward<Ts>(passArgs)...);
      // load will determine the usage pattern of the draw function; consider a collection of vaos to loop through given as an argument. the usage pattern must be mutated depending upon the input.

      return buf_.extract();
      //if buf_ is a framebuffer or something of the like, the idea is to return the texture for which it has drawn to; this will allow composition of processing passes in an elegant way
    }
  };

  template<typename> struct getRuntimeStr;

  template<template<char...> class str, char... strContents>
  struct getRuntimeStr<str<strContents...>>
  {
      GLDSEL_FUNC_DECL auto operator()()
      {
        return (std::string("") + ... + strContents);
      }
  };

  template<typename T, typename U>
  struct unify : public T, public U
  { };

  template<typename, typename> struct parameter;

  template<template<char...> class str, typename T, char... strContents>
  struct parameter<str<strContents...>, T>
  {
    using name = str<strContents...>;
    using type = T;
    T data;

    GLDSEL_FUNC_DECL auto getParamName()
    {
      return (std::string("") + ... + strContents);
    }
  };

  template<typename toFind, typename... parameters>
  GLDSEL_FUNC_DECL constexpr auto getIndex(const toFind& findThis, const parameters&...) // boost hana makes this so much cleaner
  {
    constexpr auto sequence = boost::hana::unpack(std::index_sequence_for<parameters...>{}, boost::hana::make_tuple);
    constexpr auto map = boost::hana::unpack(boost::hana::zip_with(boost::hana::make_pair, boost::hana::make_tuple(parameters{}...), sequence), boost::hana::make_map);
    constexpr auto index = boost::hana::find(map, findThis);
    return boost::hana::if_(!(index == boost::hana::nothing), boost::hana::uint_c<boost::hana::value(index.value())>, boost::hana::nothing);

    // if (!(index == boost::hana::nothing))
    // {
    //   return boost::hana::just(boost::hana::uint_c<boost::hana::value(index.value())>);
    // }
    // return boost::hana::nothing;
    //return boost::hana::transform(index, [](auto x){ return boost::hana::uint_c<decltype(x)::value>; });
  }

  template<typename T, typename... names>
  GLDSEL_FUNC_DECL auto getNamedArg(T nameType, names&&... args)
  {
    constexpr auto index = getIndex(nameType, boost::hana::type_c<decltype(args)>...);
    return std::get<boost::hana::value(index.value())>(std::tie(args...));
  }

  template<typename T, typename U, typename... names>
  GLDSEL_FUNC_DECL auto getNamedArgWithDefault(T nameType, const U& def, names&&... args)
  {
    constexpr auto index = getIndex(nameType, boost::hana::type_c<decltype(args)>...);
    if(index == boost::hana::nothing)
    {
      return def;
    }
    return std::get<boost::hana::value(index.value())>(std::tie(args...));
  }


  template<typename str, typename T>
  GLDSEL_FUNC_DECL auto make_argument(str name, const T& data)
  {
    return parameter<typename str::type, T>{data};
  }
}

#endif

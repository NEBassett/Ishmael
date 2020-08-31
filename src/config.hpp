#ifndef CONFIG_HPP_
#define CONFIG_HPP_

#define GLDSEL_FUNC_DECL inline

#include <GL/gl.h>
#include <GL/glew.h>
#include <string>
#include <utility>
#include <type_traits>
#include <algorithm>
#include <boost/hana/tuple.hpp>
#include <boost/hana/map.hpp>
#include <boost/hana/pair.hpp>
#include <boost/hana/type.hpp>
#include <boost/hana/zip_with.hpp>
#include <boost/hana/ext/std/tuple.hpp>
#include <boost/hana/find.hpp>
#include <boost/hana/unpack.hpp>
#include <boost/hana/ext/std/integral_constant.hpp>
#include <boost/hana/ext/std/integer_sequence.hpp>
#include <boost/hana/optional.hpp>
#include <boost/hana/functional/overload.hpp>
#include <boost/hana/transform.hpp>
#include <boost/hana/core/make.hpp>
#include <boost/hana/for_each.hpp>
#include <boost/hana/integral_constant.hpp>
#include <boost/hana/type.hpp>
#include <boost/hana/tuple.hpp>
#include <boost/hana/zip.hpp>
#include <boost/optional.hpp>
#include <boost/optional/optional_io.hpp>
#include <type_traits>
#include <boost/metaparse/string.hpp>
#include <boost/hana/ext/std/tuple.hpp>
#include <boost/hana/integral_constant.hpp>
#include <iostream>
#include <exception>
#include <glm/glm.hpp>
#include <boost/hana/functional/overload.hpp>
#include <boost/hana/if.hpp>
#include <boost/hana/for_each.hpp>
#include <boost/hana/ext/std/vector.hpp>
#include <boost/hana/ext/std/array.hpp>
#include <boost/hana/functional/id.hpp>
#include <utility>
#include <boost/metaparse/alphanum.hpp>
#include <boost/metaparse/transform.hpp>
#include <boost/metaparse/keyword.hpp>
#include <boost/metaparse/token.hpp>
#include <boost/metaparse/sequence.hpp>
#include <boost/metaparse/token.hpp>
#include <boost/metaparse/one_of.hpp>
#include <boost/metaparse/repeated1.hpp>
#include <boost/metaparse/lit_c.hpp>
#include <boost/metaparse/except.hpp>
#include <boost/metaparse/optional.hpp>
#include <boost/metaparse/repeated.hpp>
#include <boost/metaparse/define_error.hpp>
#include <boost/metaparse/int_.hpp>
#include <boost/mpl/transform.hpp>
#include <boost/mpl/lambda.hpp>
#include <boost/mpl/at.hpp>



namespace GLDSEL
{
  using indexType = size_t;
  using vaoName = BOOST_METAPARSE_STRING("vaoBuffer");
  using drawTypeName = BOOST_METAPARSE_STRING("drawType");
}

#endif

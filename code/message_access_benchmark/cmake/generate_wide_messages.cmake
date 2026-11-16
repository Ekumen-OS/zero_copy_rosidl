# Generate real ROS interfaces and direct named-field kernels, not array loops.
set(wide_interfaces "")
set(wide_sources "")
file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/msg")
foreach(count 16 256 1024 4096)
  set(name "Wide${count}")
  set(stem "wide${count}")
  set(fields "")
  set(reads "")
  set(writes "")
  set(updates "")
  math(EXPR last "${count} - 1")
  foreach(index RANGE ${last})
    string(APPEND fields "float64 field_${index}\n")
    string(APPEND reads "    sum += static_cast<double>(msg.field_${index});\n")
    string(APPEND writes "    msg.field_${index} = seed + ${index}.0;\n")
    string(APPEND updates
      "    msg.field_${index} = static_cast<double>(msg.field_${index}) + seed;\n")
  endforeach()
  file(CONFIGURE OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/msg/${name}.msg" CONTENT "${fields}" @ONLY)
  list(APPEND wide_interfaces "${CMAKE_CURRENT_BINARY_DIR}:msg/${name}.msg")
  set(experimental_includes "")
  set(experimental_alias "")
  if(MESSAGE_ACCESS_EXPERIMENTAL)
    set(experimental_includes
      "#include \"message_access_benchmark/msg/experimental/${stem}.hpp\"\n#include \"message_access_benchmark/msg/experimental/detail/${stem}__rosidl_typesupport_xcdr_cpp.hpp\"")
    set(experimental_alias "using Experimental = message_access_benchmark::msg::experimental::${name};")
  endif()
  set(opaque_declarations "")
  set(opaque_definitions "")
  set(variants Standard)
  if(MESSAGE_ACCESS_EXPERIMENTAL)
    list(APPEND variants Experimental)
  endif()
  foreach(variant ${variants})
    string(APPEND opaque_declarations
      "  static double opaque_read(const ${variant} & msg);\n"
      "  static void opaque_write(${variant} & msg, double seed);\n"
      "  static void opaque_update(${variant} & msg, double seed);\n")
    string(APPEND opaque_definitions
      "double ${name}Ops::opaque_read(const ${variant} & msg) {return read(msg);}\n"
      "void ${name}Ops::opaque_write(${variant} & msg, double seed) {write(msg, seed);}\n"
      "void ${name}Ops::opaque_update(${variant} & msg, double seed) {update(msg, seed);}\n")
  endforeach()
  string(CONCAT kernels
    "#pragma once\n#include <cstddef>\n"
    "#include \"message_access_benchmark/msg/${stem}.hpp\"\n"
    "${experimental_includes}\n"
    "struct ${name}Ops {\n"
    "  using Standard = message_access_benchmark::msg::${name};\n"
    "  ${experimental_alias}\n"
    "  static constexpr std::size_t fields = ${count};\n"
    "  static constexpr const char * name = \"${name}\";\n"
    "  template<typename M> static double read(const M & msg) {\n"
    "    double sum = 0;\n${reads}    return sum;\n  }\n"
    "  template<typename M> static void write(M & msg, double seed) {\n${writes}  }\n"
    "  template<typename M> static void update(M & msg, double seed) {\n${updates}  }\n"
    "${opaque_declarations}};\n")
  file(CONFIGURE OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/${stem}_kernels.hpp"
    CONTENT "${kernels}" @ONLY)
  file(CONFIGURE OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/${stem}_opaque.cpp"
    CONTENT "#include \"${stem}_kernels.hpp\"\n${opaque_definitions}" @ONLY)
  string(CONCAT registration
    "#include \"${stem}_kernels.hpp\"\n"
    "#include \"message_access_benchmark/scalar_benchmark.hpp\"\n"
    "namespace {\nconst bool registered = "
    "(message_access_benchmark::register_scalar<${name}Ops>(), true);\n}\n")
  file(CONFIGURE OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/${stem}_benchmark.cpp"
    CONTENT "${registration}" @ONLY)
  list(APPEND wide_sources
    "${CMAKE_CURRENT_BINARY_DIR}/${stem}_opaque.cpp"
    "${CMAKE_CURRENT_BINARY_DIR}/${stem}_benchmark.cpp")
endforeach()

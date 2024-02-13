add_library(lwip_port INTERFACE)

target_include_directories(lwip_port INTERFACE
   ${PROJECT_ROOT}/include/configs/lwip
)
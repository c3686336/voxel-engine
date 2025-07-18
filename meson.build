project('voxel-engine', 'cpp', version: '0.1.0', license: 'AGPL 3.0', default_options: ['cpp_std=c++23'], meson_version: '>=1.4.0') 

fs = import('fs')

warning_level = 'everything' 

if get_option('buildtype') == 'release'
  add_global_arguments('-O2', language: 'cpp')
else
  add_global_arguments('-g3', '-ggdb', language: 'cpp')
endif
add_global_arguments('-Wshadow', '-Wnon-virtual-dtor', language: 'cpp')

glbinding_dep = dependency('glbinding')
glfw_dep = dependency('glfw3')
spdlog_dep = dependency('spdlog')
glm_dep = dependency('glm')
catch2_dep = dependency('catch2-with-main')

stb_subproj = subproject('stb')
stb_dep = stb_subproj.get_variable('stb_dep')

imgui_subproj = subproject('imgui')
imgui_dep = imgui_subproj.get_variable('imgui_dep')

deps = [glbinding_dep, glfw_dep, spdlog_dep, glm_dep, catch2_dep, stb_dep, imgui_dep]

inc = include_directories('include')
subdir('include')
subdir('src')
subdir('test')

shaders_str = []
foreach shader : shaders
  shaders_str += fs.name(shader)
endforeach

validate_shaders = custom_target(
  'Validate Shaders',
  input: shaders,
  output: ['asdf'],
  command: ['glslang', '@INPUT@']
)

copy_shaders = custom_target(
  'Copy Shaders',
  input: shaders,
  output: shaders_str,
  command: ['cp', '@INPUT@', '@BUILD_ROOT@']
)

executable('voxel-engine', main_src + voxel_engine_srcs + validate_shaders + copy_shaders, include_directories: inc, dependencies: deps)

executable('raymarcher', raymarcher_src + voxel_engine_srcs, include_directories: inc, dependencies: deps)

test = executable('voxel-engine-test', test_srcs + voxel_engine_srcs, include_directories: inc, dependencies: deps)
test('Test', test)

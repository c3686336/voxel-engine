project('voxel-engine', 'cpp', version: '0.1.0', license: 'AGPL 3.0', default_options: ['cpp_std=c++23']) 

if get_option('buildtype') == 'release'
  add_global_arguments('-O2', language: 'cpp')
else
  add_global_arguments('-g3', '-ggdb', language: 'cpp')
endif

glbinding_dep = dependency('glbinding')
glfw_dep = dependency('glfw')

inc = include_directories('include')
subdir('include')
subdir('src')
subdir('test')

executable('voxel-engine', voxel_engine_srcs, include_directories: inc, dependencies: [glbinding_dep, glfw_dep])

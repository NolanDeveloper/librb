project('librb', 'c',
  version : '0.1',
  default_options : ['warning_level=3', 'werror=true', 'c_std=c11'])

include_directories = include_directories('include')

install_headers('include/librb.h')

src = [
  'src/librb.c',
]

librb = library(
  'rb', 
  src, 
  install : true,
  include_directories: include_directories
)

pkg = import('pkgconfig')
pkg.generate(librb)

librb_test = executable(
  'librb_test', 
  'test/main.c', 
  link_with: [librb],
  include_directories: include_directories
)

test('unit test', librb_test)

include(hunter_add_version)
include(hunter_cacheable)
include(hunter_cmake_args)
include(hunter_download)
include(hunter_pick_scheme)

hunter_add_version(
    PACKAGE_NAME mylog
    VERSION "0.2.0"
    URL "https://github.com/maxopetya/mylog/archive/v0.2.0.tar.gz"
    SHA1 46009ec73c2881734ea6c49ba54e998d8c15e9dc
)

hunter_cmake_args(mylog CMAKE_ARGS CMAKE_BUILD_TYPE=Release)
hunter_pick_scheme(DEFAULT url_sha1_cmake)
hunter_cacheable(mylog)
hunter_download(PACKAGE_NAME mylog)

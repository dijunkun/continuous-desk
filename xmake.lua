set_project("remote_desk")
set_version("0.0.1")
set_license("GPL-3.0")

add_rules("mode.release", "mode.debug")
set_languages("c++17")

add_rules("mode.release", "mode.debug")

add_requires("spdlog 1.11.0", "ffmpeg", {system = false})

if is_os("windows") then
    add_ldflags("/SUBSYSTEM:CONSOLE")
    add_links("Shell32")
    add_requires("vcpkg::sdl2 2.26.4")
elseif is_os("linux") then 
    add_links("pthread")
    set_config("cxxflags", "-fPIC")
end

add_packages("spdlog")

includes("../../thirdparty")

target("log")
    set_kind("headeronly")
    add_packages("spdlog")
    add_headerfiles("../../src/log/log.h")
    add_includedirs("../../src/log", {public = true})

target("remote_desk")
    set_kind("binary")
    add_deps("projectx")
    add_packages("log")
    add_packages("vcpkg::sdl2", "ffmpeg")
    add_links("avfilter", "avdevice", "avformat", "avcodec", "swscale", "swresample", "avutil")
    add_files("*.cpp")
    add_includedirs("../../src/interface")
    add_links("SDL2main", "SDL2-static")

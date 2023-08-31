set_project("remote_desk")
set_version("0.0.1")
set_license("GPL-3.0")

add_rules("mode.release", "mode.debug")
set_languages("c++17")

add_rules("mode.release", "mode.debug")

add_requires("spdlog 1.11.0", "ffmpeg 5.1.2", {system = false})
add_defines("UNICODE")

if is_os("windows") then
    add_ldflags("/SUBSYSTEM:CONSOLE")
    add_links("Shell32", "windowsapp", "dwmapi", "User32", "kernel32")
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

target("screen_capture")
    set_kind("static")
    add_packages("log")
    add_files("screen_capture/*.cpp")
    add_includedirs("screen_capture", {public = true})

target("remote_desk_server")
    set_kind("binary")
    add_packages("log", "ffmpeg")
    add_deps("projectx", "screen_capture")
    add_files("remote_desk_server/*.cpp")
    add_includedirs("../../src/interface")
    -- add_links("avformat", "swscale")

target("remote_desk_client")
    set_kind("binary")
    add_deps("projectx")
    add_packages("log")
    add_packages("ffmpeg")
    add_packages("vcpkg::sdl2")
    add_files("remote_desk_client/*.cpp")
    add_includedirs("../../src/interface")
    add_links("SDL2-static", "SDL2main", "Shell32", "gdi32", "winmm", 
        "setupapi", "version", "WindowsApp", "Imm32", "avutil")

-- target("remote_desk")
--     set_kind("binary")
--     add_deps("projectx")
--     add_packages("log")
--     add_packages("ffmpeg")
--     add_packages("vcpkg::sdl2")
--     add_links("avfilter", "avdevice", "avformat", "avcodec", "swscale", "swresample", "avutil")
--     add_files("**.cpp")
--     add_includedirs("../../src/interface")
--     add_links("SDL2-static", "SDL2main", "Shell32", "gdi32", "winmm", 
--         "setupapi", "version", "WindowsApp", "Imm32", "avutil")



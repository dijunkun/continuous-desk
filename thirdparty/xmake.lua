includes("sdl2", "projectx")
if is_plat("windows") then
elseif is_plat("linux") then
    includes("ffmpeg")
elseif is_plat("macosx") then
    includes("ffmpeg")
end
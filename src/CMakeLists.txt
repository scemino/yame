# select the 3D backend for sokol_gfx.h and sokol_app.h
add_definitions(-D${sokol_backend})

fips_begin_app(mo5 windowed)
    fips_files(main.c m6809.c m6809.h mo5rom.h mo5.h mo5.c kbd.h data.h fs.c fs.h keybuf.h keybuf.c clock.h clock.c clk.h)
    sokol_shader(sokol_shaders.glsl ${slang})
    fips_deps(sokol)
fips_end_app()

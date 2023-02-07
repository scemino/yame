# select the 3D backend for sokol_gfx.h and sokol_app.h
add_definitions(-D${sokol_backend})

# optional UI library (using Dear ImGui)
fips_begin_lib(ui)
    fips_files(ui.cc ui.h)
    fips_deps(imgui)
fips_end_lib()

fips_begin_app(mo5 windowed)
    fips_files(main.c m6809.c m6809.h mo5rom.h mo5.h mo5.c kbd.h data.h fs.c fs.h keybuf.h keybuf.c clock.h clock.c clk.h ui.h ui.cc mo5-ui-impl.cc ui/ui_mo5.h ui/ui_mc6809.h ui/ui_kbd.h)
    sokol_shader(sokol_shaders.glsl ${slang})
    fips_deps(sokol ui)
fips_end_app()

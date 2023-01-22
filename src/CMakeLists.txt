# select the 3D backend for sokol_gfx.h and sokol_app.h
add_definitions(-D${sokol_backend})

fips_begin_app(mo5 windowed)
    fips_files(main.c)
    fips_deps(sokol )
fips_end_app()

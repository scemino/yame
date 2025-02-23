@vs offscreen_vs
layout(location=0) in vec2 in_pos;
out vec2 uv;

void main() {
    gl_Position = vec4(in_pos * 2.0 - 1.0, 0.5, 1.0);
    uv = vec2(in_pos.x, 1.0 - in_pos.y);
}
@end

// pixel shader to perform color palette lookup
@fs offscreen_fs

layout(binding=0) uniform texture2D pix_img;
layout(binding=1) uniform texture2D pal_img;
layout(binding=0) uniform sampler smp;
in vec2 uv;
out vec4 frag_color;

void main() {
    float pix = texture(sampler2D(pix_img, smp), uv).x;
    frag_color = texture(sampler2D(pal_img, smp), vec2(pix,0)).rgba;
}
@end

@vs display_vs
@glsl_options flip_vert_y
layout(location=0) in vec2 in_pos;
out vec2 uv;

void main() {
    gl_Position = vec4(in_pos * 2.0 - 1.0, 0.5, 1.0);
    uv = vec2(in_pos.x, 1.0 - in_pos.y);
}
@end

// pixel shader to perform upscale-rendering to display framebuffer
@fs display_fs

layout(binding=0) uniform texture2D rgba_img;
layout(binding=0) uniform sampler smp;
in vec2 uv;
out vec4 frag_color;

// TODO: why do I need to do this ?
// RGBA => ABGR

void main() {
    frag_color = texture(sampler2D(rgba_img, smp), uv).abgr;
}
@end

@program offscreen offscreen_vs offscreen_fs
@program display display_vs display_fs

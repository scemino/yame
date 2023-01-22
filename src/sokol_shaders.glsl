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

uniform sampler2D pix_img;
uniform sampler2D pal_img;
in vec2 uv;
out vec4 frag_color;

void main() {
    float pix = texture(pix_img, uv).x;
    frag_color = vec4(texture(pal_img, vec2(pix,0)).xyz, 1.0);
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

uniform sampler2D rgba_img;
in vec2 uv;
out vec4 frag_color;

void main() {
    frag_color = texture(rgba_img, uv);
}
@end

@program offscreen offscreen_vs offscreen_fs
@program display display_vs display_fs

attribute vec4 seed;
varying float vAlpha;
varying vec2 vCorner;
uniform mat4 u_matrix;
uniform vec4 u_view;
uniform vec4 u_top;
uniform vec4 u_bottom;
void main(){
    float size = seed.z * 1.5 + 2.6;
    float time = u_top.w * 0.18;
    float x = fract(time * (seed.x - 0.5) / 15.0 + seed.y * 50.0) * 2.0 - 1.0;
    float y = sin(sign(seed.y) * time * (seed.y + 1.5) / 4.0 + seed.x * 100.0)
            / ((6.0 - seed.x * 4.0 * seed.y) / u_bottom.w);
    float opVar = mix(
        sin(time * (seed.x + 0.5) * 12.0 + seed.y * 10.0),
        sin(time * (seed.y + 1.5) * 6.0 + seed.x * 4.0),
        y * 0.5 + 0.5) * seed.x + seed.y;
    vAlpha = opVar * opVar * (1.0 - fract(seed.x + time * 0.00285));
    vec2 corner = vec2(mod(seed.w, 2.0), step(1.5, seed.w)) * 2.0 - 1.0;
    vCorner = corner;
    vec2 center = vec2((x * 0.5 + 0.5) * u_view.x, (0.5 - y * 0.5) * u_view.y);
    gl_Position = u_matrix * vec4(center + corner * (size * 0.5 / u_view.w), 0.0, 1.0);
}

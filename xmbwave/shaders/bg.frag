precision mediump float;
varying float vT;
uniform float u_opacity;
uniform vec4 u_top;
uniform vec4 u_bottom;
void main(){
    float u = clamp(vT, 0.0, 1.0);
    float g = u * u * (3.0 - 2.0 * u);
    vec2 q = mod(gl_FragCoord.xy, 64.0);
    float n = fract(sin(dot(q, vec2(12.9898, 78.233))) * 437.585453) - 0.5;
    vec3 c = mix(u_top.rgb, u_bottom.rgb, g);
    c += n / 255.0 * step(1.0 / 255.0, max(c.r, max(c.g, c.b)));
    gl_FragColor = vec4(c, 1.0) * u_opacity;
}

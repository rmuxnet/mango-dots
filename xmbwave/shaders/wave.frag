precision mediump float;
varying vec3 vPos;
varying vec3 vNormal;
varying float vEdge;
uniform float u_opacity;
uniform float u_gain;
uniform vec3 u_ink;
void main(){
    float l = length(vNormal);
    if (l < 1e-20) discard;
    float f = 0.5 * pow(1.0 - abs(vNormal.z) / l, 4.0);
    float fade = smoothstep(1.0, 0.9, abs(vPos.z)) * smoothstep(1.0, 0.92, abs(vEdge));
    float a = clamp(f * 0.7 * 0.98 * u_gain, 0.0, 1.0) * fade * u_opacity;
    gl_FragColor = vec4(u_ink * a, a);
}

precision mediump float;
varying float vAlpha;
varying vec2 vCorner;
uniform float u_opacity;
uniform float u_gain;
uniform vec3 u_ink;
void main(){
    float d = dot(vCorner, vCorner);
    if (d > 1.0) discard;
    float s = (1.0 - d) * (1.0 - d);
    float a = clamp(vAlpha * 0.75 * s * u_gain, 0.0, 1.0) * u_opacity;
    gl_FragColor = vec4(u_ink * a, a * (1.0 - dot(u_ink, vec3(0.299, 0.587, 0.114))));
}

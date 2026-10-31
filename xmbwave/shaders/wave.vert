attribute vec4 data;
attribute vec4 next;
varying vec3 vPos;
varying vec3 vNormal;
varying float vEdge;
uniform mat4 u_matrix;
uniform vec4 u_view;

const float flowSpeed = 0.18;
const float tension = 0.12;
const float damping = 0.0001;
const float len = 0.306001;
const float spacing = 407.658;
const float perturbation = 0.0998587;
const float perturbationScale = 0.07;
const float timeStep = 1.0;
const float waveCosAmp = 0.09;
const float waveBias = -0.1;
const float waveHeightScale = 0.5;
const float waveSoftClip = 0.22;
const float ffdYAmp = 0.05;
const float ffdZAmp = 0.06;
const float zDetailScale = 0.08;
const vec3 ffdScale1 = vec3(5.67726, 1.00077, 1.0);
const vec3 ffdScale2 = vec3(2.82755, 1.27579, 2.88782);
const vec3 ffdOffset = vec3(0.0, -0.469999, 0.0);

float softClip(float x){
    return 1.0 - 2.0 / (exp(2.0 * clamp(x, -15.0, 15.0)) + 1.0);
}
vec3 wavePos(vec2 a, float d1, float d2, float t){
    vec3 p = vec3(a.x, d1, a.y);
    vec3 ffd1 = p * ffdScale1 + ffdOffset;
    vec3 ffd2 = p * ffdScale2 + ffdOffset;
    p.y += sin(ffd1.x + t * flowSpeed) * ffdYAmp;
    p.z += cos(ffd2.z + t * flowSpeed) * ffdZAmp;
    float baseWave = cos(p.x * 2.0 - t * 0.5 * timeStep) * waveCosAmp + waveBias;
    baseWave *= (1.0 - damping);
    baseWave += tension * sin(p.x * len + t * flowSpeed * timeStep * 0.25);
    float structured = perturbation * perturbationScale * (
        sin((p.x * len * 6.0 + p.z * 0.5) * spacing * 0.01 + t * flowSpeed * timeStep * 0.7) * 0.5 +
        sin((p.x * len * 10.0 - p.z * 0.8) * spacing * 0.005 - t * flowSpeed * timeStep * 0.35) * 0.25);
    float total = (baseWave + structured) * waveHeightScale;
    total = waveSoftClip * softClip(total / waveSoftClip);
    p.y -= total;
    p.z -= d2 * zDetailScale;
    return p;
}
void main(){
    float t = u_view.z;
    float cell = 2.0 / 99.0;
    float sx = data.x > 0.999 ? -cell : cell;
    float sy = data.y > 0.999 ? -cell : cell;
    vec3 p = wavePos(data.xy, data.z, data.w, t);
    vec3 px = wavePos(data.xy + vec2(sx, 0.0), next.x, next.y, t);
    vec3 py = wavePos(data.xy + vec2(0.0, sy), next.z, next.w, t);
    vNormal = cross(px - p, py - p) * sign(sx) * sign(sy);
    vEdge = data.y;
    vPos = p;
    vec2 pos = vec2((p.x * 0.5 + 0.5) * u_view.x, (0.5 - p.y * 0.5) * u_view.y);
    gl_Position = u_matrix * vec4(pos, 0.0, 1.0);
}

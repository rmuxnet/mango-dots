attribute vec2 pos;
varying float vT;
uniform mat4 u_matrix;
uniform vec4 u_view;
void main(){
    vT = pos.y / u_view.y;
    gl_Position = u_matrix * vec4(pos, 0.0, 1.0);
}

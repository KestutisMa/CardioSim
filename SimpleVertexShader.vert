#version 430 core
layout(location = 0) in vec2 attributePos;
layout(location = 1) in uvec2 attributeCellNr;
layout(binding = 3, std430) buffer b1 { float u[]; };
layout(binding = 9, std430) buffer buff_u_reg { float u_reg[]; };
layout(binding = 10, std430) buffer b6 { float gj[]; }; // W,NW,NE
out vec4 attributeColor;

layout(location = 20) uniform ivec2 size;
layout(location = 26) uniform uint whatToPlot;

void main() {
  //    gl_Position = vec4((attributePos-.5), 0.0, 1.0);
  //			gl_Position = vec4((attributePos.x-.5f)*1.2f,
  //(attributePos.y-.8f)*1.2f, 0.0, 1.0); 			attributeColor =
  // vec4(0.,ssb_u[attributeCellNr.y*size.x+attributeCellNr.x],0.,1.);
  switch (whatToPlot) {
  case 1: // u
    gl_Position = vec4((attributePos.x) * .7f - 1.f,
                       (attributePos.y) * .7f - .21f, 0.0, 1.0);
    attributeColor =
        vec4(0., u[attributeCellNr.y * size.x + attributeCellNr.x], 0., 1.);
        // vec4(0., u_reg[0], 0., 1.);
    break;
  case 2: // vj
    gl_Position = vec4((attributePos.x) * .7f - 1.f,
                       (attributePos.y) * .7f - .95f, 0.0, 1.0);
    // float gjS = gj[attributeCellNr.y*size.x*3+attributeCellNr.x*3+0];
    // float gjS = gj[attributeCellNr.y*size.x*3+attributeCellNr.x*3+0] +
    // 			gj[attributeCellNr.y*size.x*3+attributeCellNr.x*3+1] +
    // 			gj[attributeCellNr.y*size.x*3+attributeCellNr.x*3+2];
    // attributeColor = vec4(0.,gjS/(6.f*90e-9f),0.,1.);
    //			float u1 =
    // u[attributeCellNr.y*size.x+attributeCellNr.x]; 			float u2
    // = u[(attributeCellNr.y+1)*size.x+attributeCellNr.x];
    float vj = abs(u[(attributeCellNr.y) * size.x + attributeCellNr.x+1] -
                   u[attributeCellNr.y * size.x + attributeCellNr.x]);
    attributeColor =
        vec4(0., abs(vj * (15e-3 - -85e-3) * 1e3) > 0 ? vj : 0, 0., 1.);
    // attributeColor = vec4(1,1,1,1);
    break;
  case 3: // gj
    gl_Position = vec4((attributePos.x) * .7f - 0.f,
                       (attributePos.y) * .7f - .95f, 0.0, 1.0);
    // float gjS = gj[attributeCellNr.y*size.x*3+attributeCellNr.x*3+0];
    float gjS = gj[attributeCellNr.y * size.x * 3 + attributeCellNr.x * 3 + 0] +
                gj[attributeCellNr.y * size.x * 3 + attributeCellNr.x * 3 + 1] +
                gj[attributeCellNr.y * size.x * 3 + attributeCellNr.x * 3 + 2];
    float gjW = gj[attributeCellNr.y * size.x * 3 + attributeCellNr.x * 3 + 0];
    float maxG = 3e-8f;
    // float drop = 0.9; // realitive drop of gj for which to scale color 0..1
    float gj_norm = gjW / maxG; // normalizuotas gj
    // float gj_drop_norm = 10. * gj_norm - 9.;
    attributeColor = vec4(0., gj_norm, 0., 1.);
    // attributeColor = vec4(0., gj_drop_norm, 0., 1.); //0..1 sutalpinam 10%
    // nuo virsutines gj ribos attributeColor = vec4(0.,1.,0.,1.);
    break;
  }
}
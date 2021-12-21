#version 430 core
in float coord1d; // x
layout(binding = 3, std430) buffer b1 { float u_[]; };
layout(binding = 10, std430) buffer b4 { float gj[]; }; // W,NW,NE
layout(binding = 15, std430) buffer b_dbg { float dbg[]; };

layout(location = 20) uniform ivec2 size;   // XY
layout(location = 25) uniform uint i_frame_in_current_chunk; // iteration
layout(location = 26) uniform uint whatToPlot;
// layout(location = 27) uniform uint i_frame_max;

float rand(vec2 co) { // bandymams, trink
  return fract(sin(dot(co.xy, vec2(12.9898, 78.233))) * 43758.5453);
}

const float Vfi = 15e-3;  // nernst potential of fast gate
const float V0 = -85e-3;

float normToVolt(float u_norm) {
  return (u_norm * (Vfi-V0) + V0) * 1.0e3;
}

void main(void) {
  //  float y = sin(coord1d*10.f)/4+.5f;

  uvec2 cellReg = uvec2(31, 0); // REG.

  // uint cell_adr = cellReg.y * size.x + cellReg.x;
  uint i_adr = uint((coord1d + 1.f) / 2.f * i_frame_in_current_chunk); // konvertuojam opengl x koord. (-1:1) i u_ masyvo koord
  // uint adr = cell_adr 11dr;

  float y = 0;
  switch (whatToPlot) {
  case 1:                // u
    if (coord1d == -1.0) // kai apdoroja pirma pixel
    {
      // !!!!!!!!! Registracija vyksta kai kvieciam Draw, t.y. sampling rate 10x
      // leciau nei compute !!!!!
      // u_reg[i_frame] = u_[cell_adr]; //
      
      // u_reg[i_frame] = u_[cellReg.y * size.x + size.x - 1 - 5]; //
      
      // u_reg[i_frame] = u_[cellReg.y*size.x*3+(cellReg.x + 1)*3] -
      // u_[cell_adr*3];// vj

  // DBG!!!!
      // laikinai
      // const int dbg_size = 10;
      // dbg[i_frame * dbg_size + 0] = normToVolt(u_[cellReg.y * size.x + (20)]); // v_1 - for CV measure
      // dbg[i_frame * dbg_size + 1] = normToVolt(u_[cellReg.y * size.x + (30)]); // v_2 - for vj/CV measure
      // dbg[i_frame * dbg_size + 2] = normToVolt(u_[cellReg.y * size.x + (31)]); // v_3 - for vj/CV measure
      // dbg[i_frame * dbg_size + 3] = normToVolt(u_[cellReg.y * size.x + (32)]); // v_4 - for CV measure
      // dbg[i_frame * dbg_size + 4] = normToVolt(u_[cellReg.y * size.x + (33)]); // v_5 - for CV measure
      // dbg[i_frame * dbg_size + 5] = normToVolt(u_[cellReg.y * size.x + (54)]); // v_6 - for CV measure
      // dbg[i_frame * dbg_size + 6] = normToVolt(u_[cellReg.y * size.x + (55)]); // v_7 - for CV measure
      // dbg[i_frame * 3 + 0] = u_[cellReg.y * size.x + (cellReg.x)]; // v1 last
      // if (cellReg.y % 2 == 0) // choose second cell according to gj_reg (W NW
      //                         // NE)
      //   dbg[i_frame * 3 + 1] = normToVolt(u_[(cellReg.y) * size.x + (cellReg.x - 1)]); // v2 last. (vj matavimui)
      // else
      //   dbg[i_frame * 3 + 1] = normToVolt(u_[(cellReg.y) * size.x + (cellReg.x - 1)]); // v2 last. (vj matavimui)
      
      // dbg[i_frame * 3 + 2] = normToVolt(u_[(cellReg.y) * size.x + size.x-4]); // galine last (CV matavimui) -4 (kad nebutu per arti sieneles)

      // vj dbg[i_frame*3+2] = u_[(cellReg.y+1)*size.x+(cellReg.x)] - u_[cellReg.y*size.x+(cellReg.x)]; //vj
    }
    // y = u_reg[i_adr] / 8.f + .5f;
    break;
  case 2: // gj
    if (coord1d == -1.0) {
      float gjW = gj[cellReg.y * size.x * 3 + cellReg.x * 3 + 0]; // todo W=0?
      float gjNW = gj[cellReg.y * size.x * 3 + cellReg.x * 3 + 1];
      float maxG = 2.e-8f;
      // float drop = 0.9; // realitive drop of gj for which to scale color 0..1
      float gj_norm = gjNW / maxG; // normalizuotas gj
      float gj_drop_norm = 10. * gj_norm - 9.;
      // gj_reg[0 * i_frame_max + i_frame] = gjW;
      // gj_reg[1 * i_frame_max + i_frame] = gj[cellReg.y * size.x * 3 + (cellReg.x+1) * 3 + 0]; //cellReg.x+1, W
      // cellReg = uvec2(51, 0); // REG.2
      // gj_reg[2 * i_frame_max + i_frame] = gj[cellReg.y * size.x * 3 + (cellReg.x) * 3 + 0]; //cellReg.x+1, W
      // gj_reg[0 * i_frame_max + i_frame] =
      //     rand(vec2(i_frame, i_frame)); // testams;
    }
    // y = (gj_reg[i_adr]) / 24.f + .6f; // gjNWga
    break;
  }
  gl_Position = vec4(coord1d, y, 0.0, 1.0);
}
#version 430 core
in float coord1d; // x

layout(binding = 3, std430) buffer b1 { float u_[]; };
layout(binding = 9, std430) buffer b3 { float u_reg[]; };
layout(binding = 10, std430) buffer b4 { float gj[]; }; // W,NW,NE
layout(binding = 11, std430) buffer b5 { float gj_reg[]; };
layout(binding = 15, std430) buffer b_dbg { float dbg[]; };

layout(location = 20) uniform uvec2 size;   // XY
layout(location = 25) uniform uint i_frame; // iteration
layout(location = 26) uniform uint whatToPlot;

float rand(vec2 co) { // bandymams, trink
  return fract(sin(dot(co.xy, vec2(12.9898, 78.233))) * 43758.5453);
}

void main(void) {
  //  float y = sin(coord1d*10.f)/4+.5f;
  // uvec2 cellReg = uvec2(6,6);
  uvec2 cellReg = uvec2(32 + 0, 32); // Orig
  uint cell_adr = cellReg.y * size.x + cellReg.x;
  uint i_adr =
      uint((coord1d + 1.f) / 2.f *
           i_frame); // konvertuojam opengl x koord. (-1:1) i u_ masyvo koord
  // uint adr = cell_adr + i_adr;

  float y = 0;
  switch (whatToPlot) {
  case 1:                // u
    if (coord1d == -1.0) // kai apdoroja pirma pixel
    {
      // !!!!!!!!! Registracija vyksta kai kvieciam Draw, t.y. sampling rate 10x
      // leciau nei compute !!!!!
      u_reg[i_frame] = u_[cell_adr]; //
      // u_reg[i_frame] = u_[cellReg.y*size.x*3+(cellReg.x + 1)*3] -
      // u_[cell_adr*3];// vj

      // laikinai
      dbg[i_frame * 3 + 0] = u_[cellReg.y * size.x + (cellReg.x)]; // v1 last
      if (cellReg.y % 2 == 0) // choose second cell according to gj_reg (W NW
                              // NE)
        dbg[i_frame * 3 + 1] = u_[(cellReg.y + 1) * size.x +
                                  (cellReg.x - 1)]; // v2 last. (vj matavimui)
      else
        dbg[i_frame * 3 + 1] = u_[(cellReg.y + 1) * size.x +
                                  (cellReg.x)]; // v2 last. (vj matavimui)
      dbg[i_frame * 3 + 2] =
          u_[(0) * size.x + cellReg.x]; // galine last (CV matavimui)

      // vj dbg[i_frame*3+2] = u_[(cellReg.y+1)*size.x+(cellReg.x)] -
      // u_[cellReg.y*size.x+(cellReg.x)]; //vj
    }
    y = u_reg[i_adr] / 8.f + .5f;
    break;
  case 2: // gj
    if (coord1d == -1.0) {
      float gjW = gj[cellReg.y * size.x * 3 + cellReg.x * 3 + 1]; // todo W=0?
      float gjNW = gj[cellReg.y * size.x * 3 + cellReg.x * 3 + 1];
      float maxG = .8e-8f;
      // float drop = 0.9; // realitive drop of gj for which to scale color 0..1
      float gj_norm = gjNW / maxG; // normalizuotas gj
      float gj_drop_norm = 10. * gj_norm - 9.;
      const int i_frame_max = 100000;
      gj_reg[0 * i_frame_max + i_frame] = gjNW;
      gj_reg[1 * i_frame_max + i_frame] = gjW;
      // gj_reg[0 * i_frame_max + i_frame] =
      //     rand(vec2(i_frame, i_frame)); // testams;
    }
    y = (gj_reg[i_adr]) / 24.f + .6f; // gjNW
    break;
  }
  gl_Position = vec4(coord1d, y, 0.0, 1.0);
}
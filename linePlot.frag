#version 430
out vec4 FragColor;
layout (location = 26) uniform uint whatToPlot;
void main()
{
		switch (whatToPlot) {
			case 1 :
				FragColor = vec4(0.,1.f,0.,1.);
				break;
			case 2 :
				FragColor = vec4(1.f,0,0.,1.);
				break;
			default :
				FragColor = vec4(0.,0.,1.f,1.);
				break;
		}
}
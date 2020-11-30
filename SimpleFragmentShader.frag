#version 430
in vec4 attributeColor;
out vec4 FragColor;
//layout (location = 20) uniform uvec2 size;
void main()
{
    FragColor = attributeColor;
	//FragColor = vec4(size.x*10.f,0.,0.,1.);
	//FragColor = vec4((gl_FragCoord.x-200)/400,0.0,0.0,1.0);
}
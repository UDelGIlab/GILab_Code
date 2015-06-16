#version 150
 
in vec3 Color;
out float outputF ;
 
void main()
{
	
	outputF = (gl_FragCoord.z/ gl_FragCoord.w/10.0);
	
}
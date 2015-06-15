#version 150
 
in vec3 Color;
out float outputF ;
 
void main()
{
	//float depth;
	//glReadPixels(0, 0, 1, 1, GL_DEPTH_COMPONENT, GL_FLOAT, &depth);

	//out float gl_FragDepth ;
    //outputF = gl_FragDepth;
	outputF = (gl_FragCoord.z/ gl_FragCoord.w/10.0);
}
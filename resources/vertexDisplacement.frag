varying vec4 color;
varying vec4 pos;

void main(void)
{
	//gl_FragColor = color;
    gl_FragColor = vec4(0.3, 2.0*abs(color.xyz)) + vec4(pos.xyz, 0.0);
   //gl_FragColor = vec4(0.0,0.6,0.4,0.3);
}
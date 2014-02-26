uniform sampler2D displacementMap;
uniform sampler2D velocityMap;
uniform sampler2D imgTex;

varying vec4 color;
varying vec4 pos;
void main()
{
    //using the displacement map to move vertices
	pos = texture2D( displacementMap, gl_MultiTexCoord0.xy );
    color = texture2D( imgTex, gl_MultiTexCoord0.xy);
	//color = texture2D( velocityMap, gl_MultiTexCoord0.xy );
	//color = vec4(1.0,1.0,0.0,1.0);
	gl_Position = gl_ModelViewProjectionMatrix * vec4( pos.xyz, 1.0) ;
}
//#extension GL_ARB_draw_buffers : enable
//#extension GL_ARB_texture_rectangle : enable
//#extension GL_ARB_texture_non_power_of_two : enable

#define EPS 0.001

uniform sampler2D positions;
uniform sampler2D velocities;

uniform vec3 newVert;
uniform int newVertIdx;

uniform vec3 attractorPos;
varying vec4 texCoord;

float a = 28;
float b = 46.92;
float c = 4;

void main(void)
{
vec3 p0 = texture2D( positions, texCoord.st).rgb;
vec3 v0 = texture2D( velocities, texCoord.st).rgb;
float invmass = texture2D( positions, texCoord.st).a;
int idx = int(texture2D( velocities, texCoord.st ).a);

    float h = 1.0; //time step
    vec3 f = attractorPos-p0; //force
    float fMag = length(f); //force magnitude
    vec3 v1 = v0 + h * 0.05 * invmass * f/(fMag*fMag + EPS); //velocity update
    v1 = v1 - 0.08 * v1; //friction

 //lorenz attractor
   //float vx1 = a*(p0.y - p0.x);
   //float vy1 = p0.x*(b - p0.z) - p0.y;
   //float vz1 = p0.x*p0.y - c*p0.z;
   //vec3 v1 = vec3(vx1, vy1, vz1);

    vec3 p1	= p0 + h * v1; //(symplectic euler) position update
   
	if(newVertIdx == idx)
	{
		gl_FragData[0] = vec4(newVert, invmass);
		gl_FragData[1] = vec4(0,0,0,idx);
	}
	else{

		//Render to positions texture
		gl_FragData[0] = vec4(p1, invmass);
		//Render to velocities texture
		gl_FragData[1] = vec4(v1, idx);
		//gl_FragData[1] = vec4(v1, v0); //alpha component used for coloring
	}
} 


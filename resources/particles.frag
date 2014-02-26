//#extension GL_ARB_draw_buffers : enable
//#extension GL_ARB_texture_rectangle : enable
//#extension GL_ARB_texture_non_power_of_two : enable

#define EPS 0.001

uniform sampler2D positions;
uniform sampler2D velocities;

uniform vec3 newVert;
uniform int newVertIdx;

uniform vec3 attractors[20];
uniform int numAttractors;

//uniform vec3 attractor3;

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

	 vec3 v1 = v0;
		for(int i = 0; i < numAttractors; i++)
		{
		   vec3 f = attractors[i] - p0; //force
		   float fMag = length(f);  //force magnitude
		   vec3 v =  h * 0.05 * invmass * f/(fMag*fMag + EPS); //velocity component for each attractor

		   v1 += v;
		}



    //vec3 f1 = attractor-p0; //force
	//vec3 f2 = attractor2-p0;
	//vec3 f3 = attractor3-p0;

    

	//float fMag1 = length(f1); //force magnitude
	//float fMag2 = length(f2);
	

   // v1 = v0 + h * 0.05 * invmass * f1/(fMag1*fMag1 + EPS); //velocity update
	//vec3 v2 = v1 + h * 0.05 * invmass * f2/(fMag2*fMag2 + EPS); //velocity update
    v1 = v1 - 0.04 * v1; //friction


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


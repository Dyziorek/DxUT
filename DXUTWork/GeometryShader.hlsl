
#include "ShaderCommon.fxh"

[maxvertexcount(3)]
void GSScene( triangleadj GSSceneIn input[6], inout TriangleStream<PSSceneIn> OutputStream )
{	
    PSSceneIn output = (PSSceneIn)0;

    for( uint i=0; i<6; i+=2 )
    {
        output.psPos = input[i].Pos;
        output.wsNorm = input[i].Norm;
        output.Tex = input[i].Tex;
		
        OutputStream.Append( output );
    }
	
    OutputStream.RestartStrip();
}

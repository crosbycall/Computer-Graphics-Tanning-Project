#version 120

varying float depth;

float mapRanges(float oldStart, float oldEnd, float newStart, float newEnd, float value);

void main()
{
	float depthNew = mapRanges(0, 100, 0, 1, depth);
	gl_FragColor=vec4(depthNew, depthNew, depthNew, 1.0); 
	//gl_FragColor = vec4(1.0);
}

/*
Helper method for mapping a value from one range to another.
For example, mapRanges(0, 100, 0, 1, 50) should return 0.5
*/
float mapRanges(float oldStart, float oldEnd, float newStart, float newEnd, float value) {
	float input_range = oldEnd - oldStart;
	float output_range = newEnd - newStart;

	return (value - oldStart) * output_range / input_range + newStart;
}

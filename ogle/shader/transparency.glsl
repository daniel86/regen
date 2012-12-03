
------------------------
---- Order Independent Transparency using a weighted average.
---- The first pass renders transparent meshes using add blending
---- to a floating point render target. The output is transparency weighted
---- color of the mesh plus a counter of the fragment shader invocations.
---- The accumulation pass can use these outputs to compute a average
---- color for each fragment.
-----------------------

-- weightedAverage.update.fs
out vec4 color;
out float fragmentCounter;

void main(void)
{
	vec4 color = ShadeFragment();
	color = vec4(color.rgb * color.a, color.a);
	fragmentCounter = 1.0;
}

-- weightedAverage.accumulate.fs
uniform sampler2D colorTexture;
uniform sampler2D counterTexture;
uniform vec3 in_backgroundColor;

out vec3 accumulated;

void main(void)
{
    vec4 backgroundColor = in_backgroundColor;
	float count = texture(counterTexture, in_texco).r;
	if (count == 0.0) {
        // no transparent meshes at this fragment
		accumulated = backgroundColor;
	}
    else {
	    vec4 colorSum = texture(colorTexture, in_texco);
	    vec3 colorAvg = colorSum.rgb / colorSum.a;
	    float alphaAvg = colorSum.a / count;
	    float T = pow(1.0-alphaAvg, count);
	    accumulated = colorAvg*(1 - T) + backgroundColor*T;
    }
}


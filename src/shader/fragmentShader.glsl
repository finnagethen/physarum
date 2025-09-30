#version 430

// >! For comments see "computeShader.glsl"
struct SimulationSettings{
	float agents, 
		s1inp, s2inp, s3inp,
		fps, fpsoff,
		avoid, 
		blurRadius,
		trailWeight, 
		diffuseWeight, 
		decayRate;
};

struct SpeciesSettings{
	int spawnMode;
	float sensorSize, 
		sensorOffsetDistance, 
		sensorAngle, 
		turnSpeed, 
		moveSpeed,
		r, g, b;
};

layout(binding = 1, rgba32f) uniform image2D trailMap;
layout(binding = 3, std430) buffer speciesSettings{
	SpeciesSettings settings[];
};
layout(binding = 4, std430) buffer simulationSettings{
	SimulationSettings simSettings;
};

uniform ivec2 windowSize;
out vec4 fragColor;

ivec2 imgSize = imageSize(trailMap);

// !<

// Function to diffuse and decay the colors in the trail map 
void diffuse(ivec2 coord){
	// Return if the coordinate is outside of the image bounds
	if (coord.x > imgSize.x || coord.y > imgSize.y){
		return;
	}
	
	vec3 sum = vec3(0.0);
	// Load the original color at the coordinate
	vec3 originalCol = imageLoad(trailMap, coord).rgb;
	
	int radius = int(simSettings.blurRadius);
	// Sum up the pixel values in the area around the coordinate
	for (int offsetX = -radius; offsetX <= radius; offsetX++) {
		for (int offsetY = -radius; offsetY <= radius; offsetY++) {
			int sampleX = int(min(imgSize.x - 1.0, max(0.0, coord.x + offsetX)));
			int sampleY = int(min(imgSize.y - 1.0, max(0.0, coord.y + offsetY)));
			sum += imageLoad(trailMap, ivec2(sampleX, sampleY)).rgb;
		}
	}
	
	// Calculate the average value of the sum
	vec3 blurredCol = sum / float((radius * 2 + 1) * (radius * 2 + 1));
	// Blend the original color with the blurred color using the diffuse weight from the simulation settings
	blurredCol = originalCol * (1.0 - simSettings.diffuseWeight) + blurredCol * simSettings.diffuseWeight;
	// Decrease the color's intensity by the decay rate from the simulation settings
	blurredCol = max(vec3(0.0), blurredCol - simSettings.decayRate);
	
	// Store the resulting color in the trail map
	imageStore(trailMap, coord.xy, vec4(blurredCol, 1.0));
}
	
void main(){
	// Calculate the size of each grid cell in pixels
	ivec2 gridSize = windowSize.xy / imgSize.xy;
	
	// Call the diffuse function on the current fragment location
	diffuse(ivec2(gl_FragCoord.xy));
	
	// Load the resulting color from the trail map
	vec3 result = imageLoad(trailMap, ivec2(gl_FragCoord.xy) / gridSize).rgb;
	
	/*Blending between two colors*/
	/*Use blendWithBackground instead of one, two and / or three to blend colors*/
	/*
	float blendWeight = result.r; // pow(result.r, 2); 
	vec3 background = vec3(0.0f, 0.0f, 0.0f);
	vec3 colorA = vec3(1.0f, 0.0f, 1.0f);
	vec3 colorB = vec3(0.0f, 1.0f, 0.0f);
	vec3 blendColor = vec3(colorA + (colorB - colorA) * blendWeight);
	vec3 blendWithBackground = vec3(vec3(0.0f, 0.0f, 0.0f) + (blendColor - background) * blendWeight);
	*/
	
	// Calculate the final color by multiplying the result color by the color of each species
	vec3 one = vec3(result.r * settings[0].r, result.r * settings[0].g, result.r * settings[0].b);
	vec3 two = vec3(result.g * settings[1].r, result.g * settings[1].g, result.g * settings[1].b);
	vec3 three = vec3(result.b * settings[2].r, result.b * settings[2].g, result.b * settings[2].b);
	
	// Set the fragment color to the final color
	fragColor = vec4(one + two + three, 1.);
}

#version 430 core

// Define a constant for pi
#define PI 3.141592

// Define a structure to represent an agent
struct Agent{
	// The x and y position of the agent
	float x, y;
	// The angle the agent is facing, in radians
	float angle;
	// The index of the species the agent belongs to (3 species are supported)
	int speciesIdx;
};

// Define a structure to represent the settings for a species
struct SpeciesSettings{
  // The mode in which agents of this species should spawn (e.g. randomly, in a specific pattern)
  int spawnMode;
  // The size of the sensor for this species (in pixels)
  float sensorSize;
  // The distance between the agent's position and the center of the sensor (in pixels)
  float sensorOffsetDistance;
  // The angle at which the sensor is positioned relative to the agent (in radians)
  float sensorAngle;
  // The speed at which this species turns (in radians per second)
  float turnSpeed;
  // The speed at which this species moves (in pixels per second)
  float moveSpeed;
  // The red, green, and blue values of this species (used for visualization)
  float r, g, b;
};

// Define a structure to represent the settings for the simulation
struct SimulationSettings{
  // The number of agents in the simulation
  float agents;
  // Species in percentage to the total number of agents
  float s1inp, s2inp, s3inp;
  // The number of frames per second the simulation should run at
  float fps;
  // A flag indicating whether the fps limit is on (0) or off (1)
  float fpsoff;
  // A flag indicating whether the agents should avoid (1) or follow the trails left by other species (0)
  float avoid;
  // The radius of the blur applied to the trail map
  float blurRadius;
  // The weight applied to the current trail value when updating the trail map
  float trailWeight;
  // The weight applied to the diffuse value when updating the trail map
  float diffuseWeight;
  // The rate at which the trail map decays over time
  float decayRate;
};

// Declare the layout of the compute shader
layout(local_size_x = 16, local_size_y = 1, local_size_z = 1) in;
// Declare the image2D uniform for the trail map, and bind it to binding point 1
layout(binding = 1, rgba32f) uniform image2D trailMap;
// Declare the buffer for the agents, and bind it to binding point 2
layout(binding = 2, std430) buffer agents{
	Agent dataMap[];
};
// Declare the buffer for the species settings, and bind it to binding point 3
layout(binding = 3, std430) buffer speciesSettings{
	SpeciesSettings settings[];
};
// Declare the buffer for the simulation settings, and bind it to binding point 4
layout(binding = 4, std430) buffer simulationSettings{
	SimulationSettings simSettings;
};

// Declare a uniform integer for the current time
uniform int time;

// Declare a variable for the size of the trail map image
ivec2 imgSize = imageSize(trailMap);

// Hash function www.cs.ubc.ca/~rbridson/docs/schechter-sca08-turbulence.pdf
// Define a hash function to generate a random number from a given input
uint hash(uint state){
    state ^= 2747636419;
    state *= 2654435769;
    state ^= state >> 16;
    state *= 2654435769;
    state ^= state >> 16;
    state *= 2654435769;
    return state;
}

// Define a function to scale a given number to the range [0, 1]
float scaleToRange01(uint num){
	// Divide the input number by the maximum possible value for a uint (4294967295)
	// and return the result as a float
    return float(num) / 4294967295.0;
}

// Declare a function to sense the environment based on the agent's species, position, and orientation
float sense(Agent agent, vec3 speciesMask, float sensorAngleOffset){
	// Calculate the angle of the sensor by adding the angle offset to the agent's current angle
	float sensorAngle = agent.angle + sensorAngleOffset;
	// Calculate the direction of the sensor based on the sensor an
	
	vec2 sensorDir = vec2(cos(sensorAngle), sin(sensorAngle));
	// Calculate the center position of the sensor based on the agent's position and the sensor offset distance
	ivec2 sensorCenter = ivec2(vec2(agent.x, agent.y) + sensorDir * settings[agent.speciesIdx].sensorOffsetDistance);

	float sum = 0.0;
	int sensorSize = int(settings[agent.speciesIdx].sensorSize);
	
	// Iterate over the pixels in the sensor
	for (int offsetX = -sensorSize; offsetX <= sensorSize; offsetX ++) {
		for (int offsetY = -sensorSize; offsetY <= sensorSize; offsetY ++) {
			// Calculate the x and y positions of the current pixel relative to the center of the sensor
			int sampleX = int(min(imgSize.x - 1.0, max(0.0, sensorCenter.x + offsetX)));
			int sampleY = int(min(imgSize.y - 1.0, max(0.0, sensorCenter.y + offsetY)));
			
			// If the avoid flag is set, use the dot product to avoid other species
			if (simSettings.avoid == 1){
				sum += dot(speciesMask * 2 - 1, imageLoad(trailMap, ivec2(sampleX, sampleY)).rgb);
			}
			// Otherwise, sum the values of the red, green, and blue channels of the current pixel
			else{
				sum += imageLoad(trailMap, ivec2(sampleX, sampleY)).r;
				sum += imageLoad(trailMap, ivec2(sampleX, sampleY)).g;
				sum += imageLoad(trailMap, ivec2(sampleX, sampleY)).b;
			}
		}
	}
	return sum;
}

void main(){
	// Get the id of the current agent
	ivec2 id = ivec2(gl_GlobalInvocationID.xy);
	
	// Update Agents
	Agent agent = dataMap[id.x];
	SpeciesSettings config = settings[agent.speciesIdx];
	
	// Create a mask vector representing the agent's species
	vec3 speciesMask = vec3(
		(agent.speciesIdx == 0) ? 1.0 : 0.0, 
		(agent.speciesIdx == 1) ? 1.0 : 0.0, 
		(agent.speciesIdx == 2) ? 1.0 : 0.0
	);
		
	// Convert the sensor angle from degrees to radians
	float sensorAngleRad = config.sensorAngle * (PI / 180.0);
	// Get sensor readings for the current agent
	float weightForward = sense(agent, speciesMask, 0.0);
	float weightLeft = sense(agent, speciesMask, sensorAngleRad);
	float weightRight = sense(agent, speciesMask, -sensorAngleRad);
	
	// Generate a random value based on the agent's position and time
	uint random = hash(int(agent.y) * imgSize.x + int(agent.x) + hash(id.x + time * 100000));
	// Scale the random value to a range of 0 to 1
	float randomSteerStrength = scaleToRange01(random);
	
	float turnSpeed = config.turnSpeed * 2 * PI;
	
	// Stear based on the sensor readings
	
	// Do nothing 
	if (weightForward > weightLeft && weightForward > weightRight) {
		dataMap[id.x].angle += 0;
	}
	// Left or right randomly
	else if (weightForward < weightLeft && weightForward < weightRight) {
		dataMap[id.x].angle += (randomSteerStrength - 0.5) * 2.0 * turnSpeed;
	}
	// Right
	else if (weightRight > weightLeft) {
		dataMap[id.x].angle -= randomSteerStrength * turnSpeed;
	}
	// Left
	else if (weightLeft > weightRight) {
		dataMap[id.x].angle += randomSteerStrength * turnSpeed;
	}
	
	// Calculate the new position of the agent based on its current angle and move speed
	vec2 direction = vec2(cos(agent.angle), sin(agent.angle));
	vec2 newPos = vec2(agent.x + direction.x * config.moveSpeed, agent.y + direction.y * config.moveSpeed);

	// If the new position is outside of the screen bounds, bounce the agent off the wall
	if (newPos.x < 0.0 || newPos.x >= imgSize.x || newPos.y < 0.0 || newPos.y >= imgSize.y) {
		// Generate a new random value based on the old random value
		random = hash(random);
		// Scale the new random value to a range of 0 to 2 * PI (a full circle)
		float randomAngle = scaleToRange01(random) * 2 * PI;
		
		// Clamp the new position to the screen bounds
		newPos.x = min(imgSize.x - 1.0, max(0.0, newPos.x));
		newPos.y = min(imgSize.y - 1.0, max(0.0, newPos.y));
		
		// Set the agent's angle to the new random angle
		dataMap[id.x].angle = randomAngle;
	}
	// If the new position is within the screen bounds, leave a trail
	else {
		// Leave a trail
		vec3 oldTrail = imageLoad(trailMap, ivec2(newPos)).rgb;
		imageStore(trailMap, ivec2(newPos), vec4(min(vec3(1.0), oldTrail + speciesMask * simSettings.trailWeight), 1.0));
	}
	
	// Update the agents position
	dataMap[id.x].x = newPos.x;
	dataMap[id.x].y = newPos.y;
}
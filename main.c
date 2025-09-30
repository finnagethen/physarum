#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>
#include <math.h>

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#ifdef _WIN32
	#include <pdcurses/curses.h>
#else
	#include <ncurses.h>
#endif

#include "sfd.c" // library to open file explorer (link comdlg32 when compiling on windows)
#include "shader.c"

#define WIDTH 1080
#define HEIGHT 720
#define COLUMNS 1080	// WIDTH / COLUMNS has to be an Integer
#define ROWS 720	// HEIGHT / ROWS has to be an Integer


typedef enum Mode{
	CENTER, CIRCLE, RING, RANDOM, ICIRCLE
}Mode;

typedef struct Agent{
	float x, y, angle;
	int speciesIdx; // 3 species supported --> 0, 1, 2
}Agent;

typedef struct SpeciesSettings{
	Mode spawnMode;
	float sensorSize, 
		sensorOffsetDistance, 
		sensorAngle, 
		turnSpeed, 
		moveSpeed,
		r, g, b;
}Species;

typedef struct SimulationSettings{
	float agents, 
		s1inp, s2inp, s3inp,	// s1inp = species 1 in percent
		fps, fpsoff,
		avoid, 
		blurRadius,
		trailWeight, 
		diffuseWeight, 
		decayRate;
}Simulation;

typedef struct Setting{
	char* name;
	float min, max, step;
	float* valuePtr;
}Setting;

sfd_Options opt = {
    .title = "Save / Load Settings",
    .filter_name = "Text File",
    .filter = "*.txt|*"
};

// Set default settings for the species
Species speciesSettings[] = {
	(Species){.spawnMode = CENTER, .sensorSize = 1., .sensorOffsetDistance = 13., .sensorAngle = 22, .turnSpeed = 0.125, .moveSpeed = 1.0, .r = 1., .g = 0., .b = 0.}, 
	(Species){.spawnMode = ICIRCLE, .sensorSize = 1., .sensorOffsetDistance = 42., .sensorAngle = 22, .turnSpeed = 0.125, .moveSpeed = .8, .r = 0., .g = 1., .b = 0.},
	(Species){.spawnMode = ICIRCLE, .sensorSize = 1., .sensorOffsetDistance = 4., .sensorAngle = 22, .turnSpeed = 0.125, .moveSpeed = 1., .r = 0., .g = 0., .b = 1.}
};

// Set default settings for simulation
Simulation simulationSettings = {
	.agents = 25000,
	.s1inp = 100,
	.s2inp = 0,
	.s3inp = 0,
	.fps = 120,
	.fpsoff = 0,
	.avoid = 1,
	.blurRadius = 1,
	.trailWeight = 0.2,
	.diffuseWeight = 1.,
	.decayRate = 0.01
};

// Give each setting thats being displayed a name, min, max and step value
Setting speciesSettingsTable[] = {
	(Setting){
		.name = "Species",
		.min = 0, 
		.max = 2,
		.step = 1
	},
	(Setting){
		.name = "Spawn Mode",
		.min = 0, 
		.max = 4,
		.step = 1
	},
	(Setting){
		.name = "Sensor Size",
		.min = 0, 
		.max = 5,
		.step = 1
	},
	(Setting){
		.name = "Sensor Offset",
		.min = 0, 
		.max = 100,
		.step = 1
	},
	(Setting){
		.name = "Sensor Angle",
		.min = -200, 
		.max = 200,
		.step = 1
	},
	(Setting){
		.name = "Turn Speed",
		.min = -1, 
		.max = 1,
		.step = 0.005
	},
	(Setting){
		.name = "Move Speed",
		.min = -2, 
		.max = 2,
		.step = 0.01
	},
	(Setting){
		.name = "Red Value",
		.min = 0, 
		.max = 1,
		.step = 0.01
	},
	(Setting){
		.name = "Green Value",
		.min = 0, 
		.max = 1,
		.step = 0.01
	},
	(Setting){
		.name = "Blue Value",
		.min = 0, 
		.max = 1,
		.step = 0.01
	}
};

Setting simulationSettingsTable[] = {
	(Setting){
		.name = "Agents",
		.min = 5000, 
		.max = 1000000,
		.step = 5000,
		.valuePtr = &simulationSettings.agents
	},
	(Setting){
		.name = "Species 1 %",
		.min = 0, 
		.max = 100,
		.step = 1,
		.valuePtr = &simulationSettings.s1inp
	},
	(Setting){
		.name = "Species 2 %",
		.min = 0, 
		.max = 100,
		.step = 1,
		.valuePtr = &simulationSettings.s2inp
	},
	(Setting){
		.name = "Species 3 %",
		.min = 0, 
		.max = 100,
		.step = 1,
		.valuePtr = &simulationSettings.s3inp
	},
	(Setting){
		.name = "FPS",
		.min = 0, 
		.max = 500,
		.step = 10,
		.valuePtr = &simulationSettings.fps
	},
	(Setting){
		.name = "FPS OFF",
		.min = 0, 
		.max = 1,
		.step = 1,
		.valuePtr = &simulationSettings.fpsoff
	},
	(Setting){
		.name = "AVOID",
		.min = 0, 
		.max = 1,
		.step = 1,
		.valuePtr = &simulationSettings.avoid
	},
	(Setting){
		.name = "Blur Radius",
		.min = 0, 
		.max = 10,
		.step = 1,
		.valuePtr = &simulationSettings.blurRadius
	},
	(Setting){
		.name = "Trail Weight",
		.min = 0, 
		.max = 1,
		.step = 0.01,
		.valuePtr = &simulationSettings.trailWeight
	},
	(Setting){
		.name = "Diffuse Weight",
		.min = 0, 
		.max = 1,
		.step = 0.1,
		.valuePtr = &simulationSettings.diffuseWeight
	},
	(Setting){
		.name = "Decay Rate",
		.min = 0, 
		.max = 1,
		.step = 0.001,
		.valuePtr = &simulationSettings.decayRate
	}
};

// Keep track of current Species thats being edited
float speciesIdx = 0;
// Keep track of current Settings Table
int table = 0; // 0 = Species Settings, 1 = Simulation Settings

// return pointer to specific setting from current species
float* getSpeciesSetting(int idx){
	switch(idx){
		case 0: return &speciesIdx;
		break;
		case 1: return (float*)&(speciesSettings[(int)speciesIdx].spawnMode);
		break;
		case 2:	return &(speciesSettings[(int)speciesIdx].sensorSize);
		break;
		case 3:	return &(speciesSettings[(int)speciesIdx].sensorOffsetDistance);
		break;
		case 4:	return &(speciesSettings[(int)speciesIdx].sensorAngle);
		break;
		case 5:	return &(speciesSettings[(int)speciesIdx].turnSpeed);
		break;
		case 6:	return &(speciesSettings[(int)speciesIdx].moveSpeed);
		break;
		case 7:	return &(speciesSettings[(int)speciesIdx].r);
		break;
		case 8:	return &(speciesSettings[(int)speciesIdx].g);
		break;
		case 9:	return &(speciesSettings[(int)speciesIdx].b);
		break;
	}
}

// display tui (text user interface)
void display(int oldOption, int newOption, int startX, int startY){
	startY += 4;

    if (oldOption == -1){
        attron(A_BOLD);
        mvprintw(startY - 2, startX + 12, "Physarum Settings");
		attroff(A_BOLD);
		
		attron(A_NORMAL);
		if (table == 0){
			int i;
			for (i = 0; i < sizeof(speciesSettingsTable) / sizeof(Setting); i++){
				mvprintw(startY + i, startX + 12, "%s: < %.3f >\t", speciesSettingsTable[i].name, *getSpeciesSetting(i));
			}
		}else if (table = 1){
			int i;
			for (i = 0; i < sizeof(simulationSettingsTable) / sizeof(Setting); i++){
				mvprintw(startY + i, startX + 12, "%s: < %.3f >\t", simulationSettingsTable[i].name, *simulationSettingsTable[i].valuePtr);
			}
		}
		attroff(A_NORMAL);
		attron(A_BOLD);
		mvprintw(startY + 12, startX + 12, "ENTER to reset the simulation");
		mvprintw(startY + 13, startX + 12, "SPACE to change the table");
		mvprintw(startY + 14, startX + 12, "S to save / L to load settings");
		attroff(A_BOLD);
    }
    else{
		attron(A_NORMAL);
		if (table == 0){
			mvprintw(startY + oldOption, startX + 12, "%s: < %.3f >\t", speciesSettingsTable[oldOption].name, *getSpeciesSetting(oldOption));
		}else if (table == 1){
			mvprintw(startY + oldOption, startX + 12, "%s: < %.3f >\t", simulationSettingsTable[oldOption].name, *simulationSettingsTable[oldOption].valuePtr);
		}
		attroff(A_NORMAL);
	}
	
	attron(A_STANDOUT);
	if (table == 0){
			mvprintw(startY + newOption, startX + 12, "%s: < %.3f >\t", speciesSettingsTable[newOption].name, *getSpeciesSetting(newOption));
		}else if (table == 1){
			mvprintw(startY + newOption, startX + 12, "%s: < %.3f >\t", simulationSettingsTable[newOption].name, *simulationSettingsTable[newOption].valuePtr);
		}
	attroff(A_STANDOUT);
	
	attron(A_BOLD);
    mvprintw(startY + 16, startX + 12, "Use arrows to navigate - ESC to quit");
	attroff(A_BOLD);
	
    refresh();
}

// save all settings in a text file
// use sfd library to open file explorer
void saveSettings(){
    const char *filename = sfd_save_dialog(&opt);
    
    if (filename){
        FILE* fptr = fopen(filename, "w");
        
        float tempSpeciesIdx = speciesIdx;
        int i, j;
        for (j = 0; j < 3; j++){
            speciesIdx = (float)j;
            for (i = 1; i < sizeof(speciesSettingsTable) / sizeof(Setting); i++){ 
                fprintf(fptr, "%f\n", *getSpeciesSetting(i));
            }
        }
        for (i = 0; i < sizeof(simulationSettingsTable) / sizeof(Setting); i++){
            fprintf(fptr, "%f\n", *simulationSettingsTable[i].valuePtr);
        }
        speciesIdx = tempSpeciesIdx;
        fclose(fptr);
    }
}

// load all settings from textfile
// use sfd to open file explorer
void loadSettings(){
    const char* filename = sfd_open_dialog(&opt);
    
    if (filename){
        FILE* fptr = fopen(filename, "r");
        char str[16];
        
        float tempSpeciesIdx = speciesIdx;
        int i, j;
        for (j = 0; j < 3; j++){
            speciesIdx = (float)j;
            for (i = 1; i < sizeof(speciesSettingsTable) / sizeof(Setting); i++){
                fgets(str, 16, fptr);
                *getSpeciesSetting(i) = atof(str);
            }
        }
        for (i = 0; i < sizeof(simulationSettingsTable) / sizeof(Setting); i++){
            fgets(str, 16, fptr);
            *simulationSettingsTable[i].valuePtr = atof(str);
        }
        speciesIdx = tempSpeciesIdx;
        fclose(fptr);
    }
}

// give agents a x, y and angle value based on spawnMode
unsigned int initAgents(){
	Agent* agents = (Agent*)malloc(simulationSettings.agents * sizeof(Agent));
	
	srand(time(NULL));
	
	// Spawn variables
	int radius = ROWS / 2;
	int center[] = {COLUMNS / 2, ROWS / 2};
	float x, y, alpha;
	Mode spawnMode;

	int a;
	for (a = 0; a < simulationSettings.agents; a++){
		if (a < simulationSettings.agents * (simulationSettings.s1inp / 100.)){
			spawnMode = (int)*(float*)&(speciesSettings[0].spawnMode);
			agents[a].speciesIdx = 0;
		}else if (a < simulationSettings.agents * ((simulationSettings.s2inp + simulationSettings.s1inp) / 100.)){
			spawnMode = (int)*(float*)&(speciesSettings[1].spawnMode);
			agents[a].speciesIdx = 1;
		}else if (a < simulationSettings.agents * ((simulationSettings.s3inp + simulationSettings.s1inp + simulationSettings.s2inp) / 100.)){
			spawnMode = (int)*(float*)&(speciesSettings[2].spawnMode);
			agents[a].speciesIdx = 2;
		}
		
		switch (spawnMode){
			case CENTER: 
				agents[a].x = COLUMNS * 0.5;
				agents[a].y = ROWS * 0.5;
				agents[a].angle = ((float)rand() / (float)RAND_MAX) * 2 * M_PI;
			break;
			case RING:
				alpha = ((float)rand() / (float)RAND_MAX) * 2 * M_PI;
				x = center[0] + cos(alpha) * radius;
				y = center[1] + sin(alpha) * radius;
				
				agents[a].x = x;
				agents[a].y = y;
				agents[a].angle = atan2((double)(center[1] - y), (double)(center[0] - x));	// Angle pointing to the center
			break;
			case ICIRCLE:
				while (1){	// Repeat until the point is in the circle
					x = (rand() % (2 * radius)) + (center[0] - radius);
					y = (rand() % (2 * radius)) + (center[1] - radius);
					
					if ((x - center[0]) * (x - center[0]) + (y - center[1]) * (y - center[1]) <= radius * radius){
						agents[a].x = x;
						agents[a].y = y;
						agents[a].angle = atan2((double)(center[1] - y), (double)(center[0] - x));	// Angle pointing to the center
						break;
					}
				}
			break;
			case CIRCLE:
				while (1){	// Repeat until the point is in the circle
					x = (rand() % (2 * radius)) + (center[0] - radius);
					y = (rand() % (2 * radius)) + (center[1] - radius);
					
					if ((x - center[0]) * (x - center[0]) + (y - center[1]) * (y - center[1]) <= radius * radius){
						agents[a].x = x;
						agents[a].y = y;
						agents[a].angle = ((float)rand() / (float)RAND_MAX) * 2 * M_PI;	// Random angle
						break;
					}
				}
			break;
			case RANDOM:
				agents[a].x = rand() % COLUMNS;
				agents[a].y = rand() % ROWS;
				agents[a].angle = ((float)rand() / (float)RAND_MAX) * 2 * M_PI;
			break;
		}
	}

	// Create SSBO (Shader Storage Buffer Object) for agents
	unsigned int agentsSSBO;
	glGenBuffers(1, &agentsSSBO);
	
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, agentsSSBO);
	glBufferData(GL_SHADER_STORAGE_BUFFER, simulationSettings.agents * sizeof(Agent), agents, GL_STATIC_DRAW);
	// "layout(binding = 2)"
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, agentsSSBO);
	
	// Free Memory
	free(agents);
	
	return agentsSSBO;
}

// load species settings into shader
void updateSpeciesSettings(unsigned int speciesSettingsSSBO){
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, speciesSettingsSSBO);
	glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(speciesSettings), speciesSettings, GL_STATIC_DRAW);
}

// loas simulation settings into shader
void updateSimulationSettings(unsigned int simulationSettingsSSBO){
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, simulationSettingsSSBO);
	glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(simulationSettings), &simulationSettings, GL_STATIC_DRAW);
}

// reset simulation with current settings
void reset(unsigned int agentsSSBO, unsigned int trailMapTexture){
    // Reset agents
    glDeleteBuffers(1, &agentsSSBO);
	agentsSSBO = initAgents();

	// Reset trailMap
	float* trailMap = calloc(COLUMNS * ROWS * 3, sizeof(float));
	glBindTexture(GL_TEXTURE_2D, trailMapTexture); 
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, COLUMNS, ROWS, 0, GL_RGB, GL_FLOAT, trailMap);
	free(trailMap);
}


int main(int argc, char* argv[]) {
	// Check user input
	if (simulationSettings.s1inp + simulationSettings.s2inp + simulationSettings.s3inp > 100.0){
		printf("Species percentages are bigger then 100.\n");
		return -1;
	}
	
	/*----------------------------------*/
	
	/*
	http://www.opengl-tutorial.org/beginners-tutorials/tutorial-1-opening-a-window/	(Create window using glfw, glew)
	*/
    // Initialize GLFW
	if(!glfwInit())
	{
		printf("Failed to initialize GLFW\n");
		return -1;
	}
	
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4); // We want OpenGL 4.3 (necessary for image load / store)
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);	
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE); // We don't want the old OpenGL 
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);	// Make window not resizable

	// Open a window and create its OpenGL context
	GLFWwindow* window; 
	window = glfwCreateWindow(WIDTH, HEIGHT, "Physarum", NULL, NULL);
	if(window == NULL){
		printf("Failed to open GLFW window.\n");
		glfwTerminate();
		return -1;
	}
	glfwMakeContextCurrent(window); 
	
	// Initialize GLEW
	if (glewInit() != GLEW_OK) {
		printf("Failed to initialize GLEW\n");
		return -1;
	}
	
	/*----------------------------------*/
	
	// Create Normal shader with function from shader.c
	unsigned int shaderProgram = createShader("vertexShader.glsl", "fragmentShader.glsl");
	
	// Create Compute shader with function from shader.c
	unsigned int computeProgram = createComputeShader("computeShader.glsl");
	
	// Create shader variable
	int uniformWindowSize = glGetUniformLocation(shaderProgram, "windowSize");
	int uniformTime = glGetUniformLocation(computeProgram, "time");
	
	/*----------------------------------*/
	
	float vertices[] = {
		// positions
		1.0f,  1.0f, 0.0f,	// top right
		1.0f, -1.0f, 0.0f,  // bottom right
	   -1.0f, -1.0f, 0.0f,  // bottom left
	   -1.0f,  1.0f, 0.0f, 	// top left 
	};  
	
	unsigned int indices[] = {  // start from 0
		0, 1, 3,   // first triangle
		1, 2, 3    // second triangle
	}; 
	
	unsigned int VBO, VAO, EBO;
	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &VBO);
	glGenBuffers(1, &EBO);
	
	// Bind the Vertex Array Object first, then bind and set vertex buffer(s), and then configure vertex attributes(s).
	glBindVertexArray(VAO);
	
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
	
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
	
	// Position attribute
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
	
	/*----------------------------------*/
	
	// Create textures
	unsigned int trailMapTexture;
	// 3: RGB(A)
	float* trailMap = calloc(COLUMNS * ROWS * 3, sizeof(float));
	
	// trailMapTexture
	glGenTextures(1, &trailMapTexture);
	glBindTexture(GL_TEXTURE_2D, trailMapTexture); 
	
	// s = x, t = y when using textures
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	
	// Set texture filtering parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		
    // GL_RGB: our image has R, G and B values; GL_RGBA32F: Our R, G and B values are interpreted in this format
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, COLUMNS, ROWS, 0, GL_RGB, GL_FLOAT, trailMap);
	
	// Free Memory
	free(trailMap);
	
	// Unbind
    glBindTexture(GL_TEXTURE_2D, 0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
	
	/*----------------------------------*/
	
	// Spawn agents and create SSBO (returns SSBO so we can delete it later)
	unsigned int agentsSSBO = initAgents();
	
	// Create SSBO for species settings
	unsigned int speciesSettingsSSBO;
	glGenBuffers(1, &speciesSettingsSSBO);
	
	// Set settings
	updateSpeciesSettings(speciesSettingsSSBO);
	
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, speciesSettingsSSBO);
	
	// Create SSBO for simulation settings
	unsigned int simulationSettingsSSBO;
	glGenBuffers(1, &simulationSettingsSSBO);
	
	// Set settings
	updateSimulationSettings(simulationSettingsSSBO);
	
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, simulationSettingsSSBO);
	
	// Unbind
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
	
	/*----------------------------------*/
	
	// init ncurses / pdcurses
	initscr();		
	noecho();
    keypad(stdscr, 1);
    raw();
	timeout(0);
	refresh();
    
	// create window
	WINDOW *win;
	int height = 20, width = 50;
	// start on upper left corner
	int startY = 0; // (LINES - height) / 2;	(start in the middle)
	int startX = 0; // (COLS - width) / 2;	(start in the middle)
    int key, oldOption = -1, newOption = 0, quit = 0, maxSettings;
	
    win = newwin(height, width, startY, startX);
	wrefresh(win);
	
	// display tui menu 
	display(oldOption, newOption, startX, startY);
	
	/*----------------------------------*/
		
	// DeltaTime variables
    float deltaTime = 0.0f;
    float lastFrame = 0.0f;
	
	// Ensure we can capture the escape key being pressed below
	glfwSetInputMode(window, GLFW_STICKY_KEYS, GL_TRUE);
	
	// Check if the ESC key was pressed or the window was closed
	while(glfwGetKey(window, GLFW_KEY_ESCAPE) != GLFW_PRESS && glfwWindowShouldClose(window) == 0 && !quit){
		
		/*----------------------------------*/
		
		// listen for keypress
        key = getch();
		
        switch(key){
			case KEY_UP:	// move up in tui
				oldOption = newOption;
				newOption = (newOption == 0) ? newOption : newOption - 1;
				oldOption = (newOption == 0 && table == 0)? -1 : oldOption;	// Refrsh settings if species changed
				display(oldOption, newOption, startX, startY);
			break;
			case KEY_DOWN:	// move down in tui
				maxSettings = table == 0 ? sizeof(speciesSettingsTable) / sizeof(Setting) : sizeof(simulationSettingsTable) / sizeof(Setting);
				oldOption = newOption;
				newOption = (newOption == maxSettings - 1) ? newOption : newOption + 1;
				display(oldOption, newOption, startX, startY);
			break;
			case KEY_LEFT:	// decrease setting value
				if (table == 0){
					if (*getSpeciesSetting(newOption) > speciesSettingsTable[newOption].min){
						*getSpeciesSetting(newOption) -= speciesSettingsTable[newOption].step;
					}else {
						*getSpeciesSetting(newOption) = speciesSettingsTable[newOption].min;
					}
				}else if (table == 1){
					if (*simulationSettingsTable[newOption].valuePtr > simulationSettingsTable[newOption].min){
						*simulationSettingsTable[newOption].valuePtr -= simulationSettingsTable[newOption].step;
					}else{
						*simulationSettingsTable[newOption].valuePtr = simulationSettingsTable[newOption].min;
					}
				}
				display(oldOption, newOption, startX, startY);
			break;
			case KEY_RIGHT:	// increase setting value
				if (table == 0){
					if (*getSpeciesSetting(newOption) < speciesSettingsTable[newOption].max){
						*getSpeciesSetting(newOption) += speciesSettingsTable[newOption].step;
					}else {
						*getSpeciesSetting(newOption) = speciesSettingsTable[newOption].max;
					}
				}else if (table == 1){
					if (*simulationSettingsTable[newOption].valuePtr < simulationSettingsTable[newOption].max){
						*simulationSettingsTable[newOption].valuePtr += simulationSettingsTable[newOption].step;
					}else{
						*simulationSettingsTable[newOption].valuePtr = simulationSettingsTable[newOption].max;
					}
				}
				display(oldOption, newOption, startX, startY);
			break;
			case KEY_RESIZE:	// Triggerd if settings window is rezised 
				resize_term(0, 0);
				oldOption = -1;
				erase();
				display(oldOption, newOption, startX, startY);
            break;
			case 32:	// SPACE: swtich settings table
				table = table == 0 ? 1 : 0;
				oldOption = -1;
				newOption = 0;
				erase();
				display(oldOption, newOption, startX, startY);
			break;
            case 10:	// ENTER 
			case 13:	// ENTER: Reset simulation
                reset(agentsSSBO, trailMapTexture);
			break;
            case 83:	// S
            case 115:	// S to save settings
                saveSettings();
            break;
            case 76:	// L
            case 108:	// L to load settings
                loadSettings();
                reset(agentsSSBO, trailMapTexture);
				oldOption = -1;
				newOption = 0;
                display(oldOption, newOption, startX, startY);
            break;
			case 27:	// ESC to quit
				quit = 1;
			break;
		}
		
		// Reupload settings
		updateSpeciesSettings(speciesSettingsSSBO);
		updateSimulationSettings(simulationSettingsSSBO);
		
		/*----------------------------------*/
		
		// Calculate DeltaTime
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        glfwPollEvents();

		// Set max framerate
		if (deltaTime < 1.0 / simulationSettings.fps){
			if (!simulationSettings.fpsoff) continue;	// Skip if too fast
		}
		
		lastFrame = currentFrame;
		
		/*----------------------------------*/
		
		/*
		Bind our texture to binding point 1. This means we can access it in our shaders using
		"layout(binding = 1)"
		and we can both read and write from it. 
		*/
		glBindImageTexture(1, trailMapTexture, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);
		
		// Use Compute Shader to update the agents
		glUseProgram(computeProgram);
		// Set shader variable
		glUniform1i(uniformTime, time(NULL));
		// Specify number of workgroups: x, y, z --> can be optimized
		glDispatchCompute(simulationSettings.agents / 16, 1, 1);
		// If the special value GL_ALL_BARRIER_BITS is specified, all supported barriers for the corresponding command will be inserted.
		glMemoryBarrier(GL_ALL_BARRIER_BITS);

		// Use shader to draw trailMap
		glUseProgram(shaderProgram);
		// Set shader variable
		glUniform2i(uniformWindowSize, WIDTH, HEIGHT);
		
		// Draw two triangles to form a rectangle
		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);
		
		glBindVertexArray(VAO);
		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
		
		glMemoryBarrier(GL_ALL_BARRIER_BITS);
		
		// Swap buffers
		glfwSwapBuffers(window);
		
	}
	
	// Clean Up
	glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
	glDeleteBuffers(1, &EBO);
	glDeleteBuffers(1, &agentsSSBO);
	glDeleteBuffers(1, &speciesSettingsSSBO);
	glDeleteBuffers(1, &simulationSettingsSSBO);
	glDeleteTextures(1, &trailMapTexture);
	glDeleteProgram(shaderProgram);
	glDeleteProgram(computeProgram);
	
	// glfw: terminate, clearing all previously allocated GLFW resources.
    glfwTerminate();
	
	// Curses cleanup
	delwin(win);
    endwin();
	
	return 0;
}


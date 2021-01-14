#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

#include "a.h"

char mouse_data[3];
char is_mouse = 1;
long int t = 0; //this is time but I should have really given it a better name

int horiz;
int verti;
int pixel_size;
FILE* fb;
unsigned char* frame;
int scene_size;
struct object* scene;
struct point3 camera_position = {.x = 0, .y = 0, .z = 0};
struct point2 camera_rotation = {.x = 0, .y = 0};

struct render_queue_element
{
	struct triangle2 triangle;
	float depth;
	struct color color;
	char rendered;
}; //pretty self-explanatory, i guess
struct render_queue_element* render_queue; //a list of triangles to be rendered, sorted by size
int rqsize;

void *mouse_input(void*); //i need to run this as a separate thread
void *measure_framerate(void*); //the same deal here
static int cmprqe(const void*,const void*);  //"compare render queue elements"
float float_min(float* l, int c);

void init(int, char**);
void loop(int, char**);
void render(int, char**);

int main(int argc, char** argv)
{
	// i decided to fwrite the buffer instead of mmaping it, i probably just don't know how to configure the mmap so it's faster than the fwrite method
	fb = fopen("/dev/fb0", "w");

	{
		//I really hope everyone has this file on their computer
		FILE* resolution_data = fopen("/sys/class/graphics/fb0/virtual_size", "r");
		fscanf(resolution_data, "%d,%d", &horiz, &verti); //I don't actually need to read the first value into horiz, but it didn't work with a NULL pointer
		fclose(resolution_data);
		FILE* pixel_size_data = fopen("/sys/class/graphics/fb0/bits_per_pixel", "r");
		fscanf(resolution_data, "%d", &pixel_size);
		pixel_size /= 8;
		fclose(resolution_data);
		/*
		* okay, so the resolution in virtual_size isn't the actual
		* resoluion :). The *stride* file, however, contains how
		* many bytes a line actually has.
		*/
		FILE* stride_data = fopen("/sys/class/graphics/fb0/stride", "r");
		int stride;
		fscanf(stride_data, "%d", &stride);
		horiz = stride/pixel_size;
	}
	frame = malloc(horiz*verti*pixel_size);
	printf("allocated framebuffer of %dx%d pixels of size %d bytes\n", horiz, verti, pixel_size);
	//the init function takes care of loading the level and etc
	init(argc, argv);

        //threads for everything i need to run parallel to the main loop
	pthread_t thread, framerate_thing;
	pthread_create(&thread, NULL, mouse_input, NULL);
	pthread_create(&framerate_thing, NULL, measure_framerate, NULL);

	//now i also malloc the render queue
	//and by the way remember that I don't know anything about actual rendering at all, so an actual render queue is probably something absolutely different, this is just a list of triangles sorted by size so they're drawn in the correct order
	rqsize = 0;
	for(int i = 0; i < scene_size; i++)rqsize += scene[i].triangle_count;
	render_queue = malloc(rqsize * sizeof(struct render_queue_element));

	int previous_size = scene_size; //i need to keep track of the count of triangles in the scene, so I can resize the render queue if the scene is resized
	for(t = 0;;t++) //yay, the main loop
	{
		loop(argc, argv);
		//resize the render queue:
		if(previous_size != scene_size)
		{
			rqsize = 0;
			for(int i = 0; i < scene_size; i++)rqsize += scene[i].triangle_count;
			render_queue = realloc(render_queue, rqsize * sizeof(struct render_queue_element));
			previous_size = scene_size;
		}
		clear_frame(frame, horiz*verti*4); //a fun option to turn off :)
		render(argc, argv); //here we go, finally the rendering
		dump_frame(frame, horiz*verti*4, fb);
	}
}


void init(int argc, char** argv)
{
	FILE* level = fopen(argv[1], "r"); //the level file, this is where the level is stored

	/*int voxel_resolution;
	scene_size = 0;
	scene = malloc(scene_size);

	fscanf(level, "%d", voxel_resolution);
	char voxels[voxel_resolution][voxel_resolution][voxel_resolution];
	for(int i = 0; i < sizeof(voxels); i++)fscanf(level, "%c", voxels+i);
	for(int i = 0; i < voxel_resolution; i++)
		for(int j = 0; j < voxel_resolution; j++)
			for(int k = 0; k < voxel_resolution; k++)
				if(voxels[i][j][k])
	*/
	fscanf(level, "%d", &scene_size); //okay so the first byte of the level file is the number of objects in the scene
	printf("%d\n", scene_size);
	scene = malloc(scene_size * sizeof(struct object)); //ok so i'm putting the level in the heap so i can resize it later
	for(int i = 0; i < scene_size; i++) // more annoying file reding stuff
	{
		int r, g, b;
		fscanf(level, "%d%d%d", &r, &g, &b);
		scene[i].color = (struct color){.r = r, .g = g, .b = b};
		fscanf(level, "%d", &(scene[i].type));
		fscanf(level, "%f%f%f", &(scene[i].position.x), &(scene[i].position.y), &(scene[i].position.z));
		fscanf(level, "%f%f%f", &(scene[i].rotation.x), &(scene[i].rotation.y), &(scene[i].rotation.z));
		char model_name[128];
		fscanf(level, "%s", model_name);
		FILE* model = fopen(model_name, "r");
		fscanf(model, "%d", &(scene[i].triangle_count));
		scene[i].triangles = malloc(scene[i].triangle_count * sizeof(struct triangle3));
		for(int j = 0; j < scene[i].triangle_count; j++)
		{
			fscanf(model, "%f%f%f", &(scene[i].triangles[j].a.x), &(scene[i].triangles[j].a.y), &(scene[i].triangles[j].a.z));
			fscanf(model, "%f%f%f", &(scene[i].triangles[j].b.x), &(scene[i].triangles[j].b.y), &(scene[i].triangles[j].b.z));
			fscanf(model, "%f%f%f", &(scene[i].triangles[j].c.x), &(scene[i].triangles[j].c.y), &(scene[i].triangles[j].c.z));
		}
		fclose(model);
	}
	fclose(level);
	for(int i = 0;i < scene_size; i++)
	{
		printf("%d: %f %f %f\n", i, scene[i].position.x, scene[i].position.y, scene[i].position.z);
	}
}
void loop(int argc, char** argv)
{
	if(is_mouse) //this flag is activated by another thread
	{
		is_mouse = 0; //I need to reset the flag
		if(mouse_data[0]&1 == 1) //if the mouse is pressed
		{
			camera_position = point3_add(
				camera_position,
				point3_rotate(
					point3_create((float)(signed char)mouse_data[1] / 10, 0, (float)(signed char)mouse_data[2] / 10)
					,point3_create(camera_rotation.x, camera_rotation.y, 0)
				)
			);
		}
		else
		{
			camera_rotation.y += (float)(signed char)mouse_data[1] / 100;
			camera_rotation.x -= (float)(signed char)mouse_data[2] / 100;
		}
	}
	//I'll put something here later
	//for(int i = 0; i < scene_size; i++)
}

void render(int argc, char** argv)
{
	int f = 0;
	//ayyy variable names
	float mat[9];
	float mau[9];
	float mav[9];
	for(int i = 0; i < 9; i++){mat[i] = 0; mau[i] = 0; mav[i] = 0;}
	rotation_3x3(-camera_rotation.y, 1, mat); //rotate on y
	rotation_3x3(-camera_rotation.x, 0, mau); //rotate on x
	multiply_3x3(mat, mau, mav); //multiply the matrices
	for(int i = 0; i < scene_size; i++) //for every object in the scene
	{
		for(int j = 0; j < scene[i].triangle_count; j++) //for every triangle in the object
		{
			struct triangle3 almost_almost /*500 IQ variable naming*/ = triangle3_transform(triangle3_shift(triangle3_shift(triangle3_rotate(scene[i].triangles[j], scene[i].rotation),scene[i].position),point3_dot(camera_position,point3_create(-1, -1, -1))),mav); //hmmmmmmmmmmm
			struct triangle2 almost = perspective(almost_almost); //the 2d representation of our triangle
			//resize and shift the triangle to be directly rendered
			almost.a = point2_add(point2_dot(almost.a,point2_create(horiz/2/FOV_X, verti/2/FOV_Y)), point2_create(horiz/2, verti/2));
			almost.b = point2_add(point2_dot(almost.b,point2_create(horiz/2/FOV_X, verti/2/FOV_Y)), point2_create(horiz/2, verti/2));
			almost.c = point2_add(point2_dot(almost.c,point2_create(horiz/2/FOV_X, verti/2/FOV_Y)), point2_create(horiz/2, verti/2));
			//add the element to the render queue
			render_queue[f] = (struct render_queue_element)
			{
				.triangle = almost,
				.depth = almost_almost.a.z<almost_almost.b.z?almost_almost.a.z<almost_almost.c.z?almost_almost.a.z:almost_almost.c.z:almost_almost.b.z<almost_almost.c.z?almost_almost.b.z:almost_almost.c.z,
				.color = scene[i].color,
				.rendered = (almost_almost.a.z >= NEAR_PLANE &&almost_almost.b.z >= NEAR_PLANE &&almost_almost.c.z >= NEAR_PLANE)
			};
			//draw_triangle(frame, almost, scene[i].color);
			f++;
		}
	}
	qsort(render_queue, rqsize, sizeof(struct render_queue_element), cmprqe); //sort the render queue
	for(int i = 0; i < rqsize; i++)if(render_queue[i].rendered)
	{
		draw_triangle(frame, render_queue[i].triangle, render_queue[i].color); //draw the elements of the set in order
		draw_filled_triangle(frame, render_queue[i].triangle, render_queue[i].color); //draw the elements of the set in order
	}
}


void *mouse_input(void* vargp)
{
	printf("starting mouse\n");
	FILE* mouse = fopen("/dev/input/mice", "r");
	for(;;)
	{
		fread(mouse_data, 3, 1, mouse); //the data from my mouse is exactly 3 bytes, tweak the code if your mouse sends different events
		is_mouse = 1; //this is the flag at the beggining of the loop
	}
}

void *measure_framerate(void* vargp)
{
	int last = t;
	for(;;)
	{
		sleep(1);
		printf("%f\t%f\t%f\t\t%f\t%f\t", camera_position.x, camera_position.y, camera_position.z, camera_rotation.x, camera_rotation.y);
		printf("%f\n", (float)(t-last));
		last = t;
	}
}


static int cmprqe(const void* a,const void* b)
{
	return((*(const struct render_queue_element*)a).depth < (*(const struct render_queue_element*)b).depth) - ((*(const struct render_queue_element*)a).depth > (*(const struct render_queue_element*)b).depth);
}


float float_min(float* l, int c)
{
	float f = l[0];
	for(int i = 0; i < c; i++) if(l[i]<f) f = l[i];
	return f;
}

#include <stdio.h>
#include <math.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>
#include "a.h"

/*
* render.c contains functions for rendering, like drawing triangles and working with matrices
* actual putting-together of these functions takes place in a.c
*/

struct point3 point3_create(float x, float y, float z){return (struct point3){.x = x, .y = y, .z = z};}
struct point2 point2_create(float x, float y){return (struct point2){.x = x, .y = y};}

void draw_line(unsigned char* frame, int ax, int ay, int bx, int by, struct color col)
{// i've literally just stolen this function from wikipedia
	int dx = fabs(bx - ax);
	int sx = ax < bx ? 1 : -1;
	int dy = -fabs(by - ay);
	int sy = ay < by ? 1 : -1;
	int err = dx + dy;
	int e2;
	int fbp;
	while(ax != bx || ay != by)
	{
		if(!(ay > 0 && ay < verti && ax > 0 && ax < horiz)) break;
		fbp = (horiz * ay + ax) * pixel_size;
		frame[fbp + 0] = col.b;
		frame[fbp + 1] = col.g;
		frame[fbp + 2] = col.r;
		e2 = 2*err;
		if(e2 >= dy)
		{
			err += dy;
			ax += sx;
		}
		if(e2 <= dx)
		{
			err += dx;
			ay += sy;
		}
	}
}


void draw_triangle(unsigned char* frame, struct triangle2 triangle, struct color col)
{
	draw_line(frame, triangle.a.x, triangle.a.y, triangle.b.x, triangle.b.y, col);
	draw_line(frame, triangle.b.x, triangle.b.y, triangle.c.x, triangle.c.y, col);
	draw_line(frame, triangle.a.x, triangle.a.y, triangle.c.x, triangle.c.y, col);
}

#define FILL_AMOUNT 3
//I was lazy to create a proper algorithm for a filed triangle, and i also realized, that having the triangles filled without SHADING THEM (which I'm just lazy to do) would make the thing look much worse
//also, it's already really slow
void draw_filled_triangle(unsigned char* frame, struct triangle2 t, struct color col)
{
	struct point2 com = {.x = (t.a.x + t.b.x + t.c.x)/3, .y = (t.a.y + t.b.y + t.c.y)/3};
	for(int i = 1; i < FILL_AMOUNT; i++)
	{
		draw_triangle(frame, (struct triangle2){
			.a = (struct point2){.x = com.x + (t.a.x - com.x)*i/FILL_AMOUNT, .y = com.y + (t.a.y - com.y)*i/FILL_AMOUNT},
			.b = (struct point2){.x = com.x + (t.b.x - com.x)*i/FILL_AMOUNT, .y = com.y + (t.b.y - com.y)*i/FILL_AMOUNT},
			.c = (struct point2){.x = com.x + (t.c.x - com.x)*i/FILL_AMOUNT, .y = com.y + (t.c.y - com.y)*i/FILL_AMOUNT}
		}, col);
	}
}


struct color create_color(int r, int g, int b)
{
	struct color result;
	result.r = r;
	result.g = g;
	result.b = b;
	return result;
}

void clear_frame(unsigned char* frame, long int len)
{
	for(int i = 0; i < len; i++)
	{
		*(frame+i) = 0;
	}
}

void fill_frame(unsigned char* frame, long int len, struct color col)
{
	for(int i = 0; i < len; i += pixel_size)
	{
		*(frame+i) = col.b;
		*(frame+i+1) = col.g;
		*(frame+i+2) = col.r;
	}
}

void dump_frame(unsigned char* frame, long int size, FILE* framebuffer)
{
	fseek(framebuffer, 0, SEEK_SET);
	fwrite(frame, 1, size, framebuffer);
}


struct triangle2 perspective(struct triangle3 input)
{
	struct triangle2 output;
	output.a.x = input.a.x / input.a.z;
	output.a.y = input.a.y / input.a.z;
	output.b.x = input.b.x / input.b.z;
	output.b.y = input.b.y / input.b.z;
	output.c.x = input.c.x / input.c.z;
	output.c.y = input.c.y / input.c.z;
	return output;
}


struct point3 point3_dot(struct point3 a, struct point3 b)
{
	return (struct point3){.x = a.x * b.x, .y = a.y * b.y, .z = a.z * b.z};
}
struct point3 point3_add(struct point3 a, struct point3 b)
{
	return (struct point3){.x = a.x + b.x, .y = a.y + b.y, .z = a.z + b.z};
}
struct point2 point2_dot(struct point2 a, struct point2 b)
{
	return (struct point2){.x = a.x * b.x, .y = a.y * b.y};
}
struct point2 point2_add(struct point2 a, struct point2 b)
{
	return (struct point2){.x = a.x + b.x, .y = a.y + b.y};
}

struct point3 point3_rotate(struct point3 in, struct point3 rotation)
{
	if(rotation.x == 0 && rotation.y == 0 && rotation.z == 0) return in;
	float m[9], n[9], o[9], p[9], q[9];
	for(int i = 0; i < 9; i++){m[i] = 0; n[i] = 0; o[i] = 0; p[i] = 0; q[i] = 0;}
	rotation_3x3(rotation.x, 0, m);
	rotation_3x3(rotation.y, 1, n);
	rotation_3x3(rotation.z, 2, o);
	multiply_3x3(m, n, p);
	multiply_3x3(p, o, q);
	return point3_transform(in, q);
}

struct triangle3 triangle3_rotate(struct triangle3 in, struct point3 rotation)
{
	if(rotation.x == 0 && rotation.y == 0 && rotation.z == 0) return in;
	float m[9], n[9], o[9], p[9], q[9];
	for(int i = 0; i < 9; i++){m[i] = 0; n[i] = 0; o[i] = 0; p[i] = 0; q[i] = 0;}
	rotation_3x3(rotation.x, 0, m);
	rotation_3x3(rotation.y, 1, n);
	rotation_3x3(rotation.z, 2, o);
	multiply_3x3(m, n, p);
	multiply_3x3(p, o, q);
	return (struct triangle3){.a = point3_transform(in.a, q), .b = point3_transform(in.b, q), .c = point3_transform(in.c, q)};
}

struct triangle3 triangle3_shift(struct triangle3 in, struct point3 by)
{
	return (struct triangle3)
	{
		.a = point3_add(in.a, by),
		.b = point3_add(in.b, by),
		.c = point3_add(in.c, by)
	};
}

struct point3 point3_transform(struct point3 in, matrix m)
{
	return(struct point3)
	{
		.x = m[0]*in.x + m[1]*in.y + m[2]*in.z,
		.y = m[3]*in.x + m[4]*in.y + m[5]*in.z,
		.z = m[6]*in.x + m[7]*in.y + m[8]*in.z
	};
}

struct triangle3 triangle3_transform(struct triangle3 in, matrix m)
{
	return(struct triangle3)
	{
		.a = point3_transform(in.a, m),
		.b = point3_transform(in.b, m),
		.c = point3_transform(in.c, m)
	};
}

void rotation_3x3(float a, int axis, matrix m)
{
	switch(axis)
	{
		case 0:
			m[0] = 1;
			m[4] = cos(a);
			m[5] = sin(a);
			m[7] = -sin(a);
			m[8] = cos(a);
			break;
		case 1:
			m[0] = cos(a);
			m[2] = sin(a);
			m[4] = 1;
			m[6] = -sin(a);
			m[8] = cos(a);
			break;
		case 2:
			m[0] = cos(a);
			m[1] = sin(a);
			m[3] = -sin(a);
			m[4] = cos(a);
			m[8] = 1;
			break;
	}
}

void multiply_3x3(matrix in, matrix by, matrix out)
{
	out[0] = by[0]*in[0]+by[1]*in[3]+by[2]*in[6]; out[1] = by[0]*in[1]+by[1]*in[4]+by[2]*in[7]; out[2] = by[0]*in[2]+by[1]*in[5]+by[2]*in[8];
	out[3] = by[3]*in[0]+by[4]*in[3]+by[5]*in[6]; out[4] = by[3]*in[1]+by[4]*in[4]+by[5]*in[7]; out[5] = by[3]*in[2]+by[4]*in[5]+by[5]*in[8];
	out[6] = by[6]*in[0]+by[7]*in[3]+by[8]*in[6]; out[7] = by[6]*in[1]+by[7]*in[4]+by[8]*in[7]; out[8] = by[6]*in[2]+by[7]*in[5]+by[8]*in[8];
}

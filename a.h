#ifndef A_H
#define A_H

/*
* mostly defines functions defined in render.c, used in a.c
* also contains some settings
*/

//camera properties

#define NEAR_PLANE 0.1
#define FOV_X 1
#define FOV_Y 0.5

//framebuffer properties
extern int horiz;
extern int verti;
extern int pixel_size;

//structs and types

struct color{unsigned char r, g, b;};
struct point3{float x, y, z;};
struct point2{float x, y;};
struct triangle3{struct point3 a, b, c;};
struct triangle2{struct point2 a, b, c;};
struct object{
	struct point3 position;
	struct point3 rotation;
	struct triangle3 * triangles;
	int triangle_count;
	struct color color;
	int type;
	void* data; //this can be allocated differently for each object
};
typedef float* matrix;

//2d drawing functions

extern void draw_line(unsigned char*, int, int, int, int, struct color);
extern void draw_triangle(unsigned char*, struct triangle2, struct color);
extern void draw_filled_triangle(unsigned char*, struct triangle2, struct color);
extern struct color create_color(int, int, int);
extern void clear_frame(unsigned char*, long int);
extern void fill_frame(unsigned char* , long int, struct color);
extern void dump_frame(unsigned char* , long int, FILE*);

//triangle funtions

extern struct triangle2 perspective(struct triangle3);
extern struct triangle3 triangle3_rotate(struct triangle3, struct point3);
extern struct triangle3 triangle3_shift(struct triangle3, struct point3);
extern struct triangle3 triangle3_transform(struct triangle3, matrix);

//point funtions

extern struct point3 point3_create(float, float, float);
extern struct point2 point2_create(float, float);
extern struct point3 point3_dot(struct point3, struct point3);
extern struct point3 point3_add(struct point3, struct point3);
extern struct point2 point2_dot(struct point2, struct point2);
extern struct point2 point2_add(struct point2, struct point2);
extern struct point3 point3_transform(struct point3, matrix);
extern struct point3 point3_rotate(struct point3, struct point3);

//matrix operations

extern void rotation_3x3(float, int, matrix);
extern void multiply_3x3(matrix, matrix, matrix);
#endif //A_H

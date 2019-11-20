/*
 * Utility functions to create a program from a file.
 *
 * Author: infapi00 <infapi00@gmail.com>
 *
 */
#include <stdlib.h>

#include <GL/gl.h>
#include <GL/glext.h>

/* A complete structure with all the relevant information */
typedef struct {
  GLuint program_object;
  GLuint object_VS;
  GLuint object_FS;
}ProgramNode;

#define print_opengl_error() print_ogl_error(__FILE__, __LINE__)
void print_ogl_error(char *file, int line);

int aux_create_program (const char *vertex_file,
                        const char *fragment_file,
                        ProgramNode **result);
int aux_clean_program (ProgramNode *program);

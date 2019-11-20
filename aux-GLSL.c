#define GL_GLEXT_PROTOTYPES
#include "aux-GLSL.h"

#include <fcntl.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>

/*
 * Prints OpenGL error if one ocurred.
 */
void
print_ogl_error (char *file,
                 int line)
{
  GLenum gl_err;

  gl_err = glGetError();

  /* XXX: reimplement gluErrorString. I don't want to depend on glu
   * just for it. For now printing error code
   */
  while (gl_err != GL_NO_ERROR) {
    fprintf (stderr,"gl_error in file %s @ line %d: %i\n",
             file, line, gl_err);

      gl_err = glGetError();
  }
}

static void
print_shader_info_log (GLuint obj,
                       const char * file_name)
{
  int chars_written  = 0;
  GLchar logbuffer[1000];

  glGetShaderInfoLog (obj, sizeof(logbuffer), &chars_written, logbuffer);
  printf ("OpenGL Shader Info Log %s:\n%.*s",
          file_name, chars_written, logbuffer);

  print_opengl_error();
}

static void
print_program_info_log(GLuint obj,
                       const char * vertex_file_name,
                       const char * fragment_file_name)
{
  int chars_written  = 0;
  GLchar logbuffer[1000];

  glGetProgramInfoLog (obj, sizeof(logbuffer), &chars_written, logbuffer);
  printf ("OpenGL Program Info (after linking) Log (%s,%s):\n%.*s",
          vertex_file_name, fragment_file_name, chars_written, logbuffer);

  print_opengl_error();
}

/* Returns the size of @file_name content */
static int
shader_size (const char *file_name)
{
  int fd;
  int count = -1;

  fd = open(file_name, O_RDONLY);
  if (fd != -1) {
    count = lseek(fd, 0, SEEK_END) + 1;
    close(fd);
  }

  return count;
}


/*
 * Reads the content of @file_name file, and put it on
 * @shader_text. @size is the maximum size to be written.
 */
static int
read_shader (const char *file_name,
             char *shader_text,
             int size)
{
  FILE *fh;
  int count;

  fh = fopen(file_name, "r");
  if (!fh)
    return -1;

  fseek(fh, 0, SEEK_SET);
  count = fread(shader_text, 1, size, fh);
  shader_text[count] = '\0';

  if (ferror(fh))
    count = 0;

  fclose(fh);

  return count;
}

/*
 * Reads the content of @file_name, storing it on @shader.
 * Returns 0 if error. 1 otherwise.
 */
static int
read_shader_source (const char *file_name,
                    GLchar **shader)
{
  int size;

  size = shader_size (file_name);

  if (size == -1) {
    fprintf (stderr, "Cannot determine size of the file  %s\n", file_name );
    return 0;
  }

  *shader = (GLchar *) malloc(size);

  if (!read_shader(file_name, *shader, size)) {
    fprintf (stderr, "Cannot read file %s\n", file_name);
    return 0;
  }

  return 1;
}

/* Creates a @shaderType shader object from the content at @file_name
 *
 * Returns the id if got it compiled. Otherwise 0.
 */

static GLuint
get_shader_object (const char * file_name,
                   GLenum shader_type)
{
  GLuint object;
  GLint compiled;
  GLchar *text = NULL;

  if (!read_shader_source(file_name, &text))
    return 0;

  object = glCreateShader(shader_type);
  glShaderSource(object, 1,(const GLchar **) &text, NULL);
  print_opengl_error();

  glCompileShader(object);
  glGetShaderiv(object, GL_COMPILE_STATUS, &compiled);

  if (!compiled) {
     fprintf (stderr, "Shader %s didn't built. Using 0 as fallback default. \n",
              file_name);
     print_shader_info_log(object, file_name);
     object = 0;
  }

  if (text != NULL) {
      free (text);
  }

  return object;
}

/*
 * Creates a new shader program object
 *
 *   *vertex_file: vertex shader filename, if NULL fixed function is used
 *
 *   *fragment_file: fragment shader filename, if NULL fixed function is used
 *
 *   *result: Program Node obtained
 */
int aux_create_program (const char *vertex_file,
                        const char *fragment_file,
                        ProgramNode **result)


{
  GLint linked;
  GLuint object_VS = 0;
  GLuint object_FS = 0;
  GLuint object_prog = 0;

  if(vertex_file != NULL)
    object_VS = get_shader_object (vertex_file, GL_VERTEX_SHADER);

  if(fragment_file != NULL)
    object_FS = get_shader_object (fragment_file, GL_FRAGMENT_SHADER);

  object_prog = glCreateProgram();
  print_opengl_error();

  if (vertex_file != NULL) {
    glAttachShader(object_prog, object_VS);
    print_opengl_error();
  }

  if(fragment_file != NULL) {
    glAttachShader(object_prog, object_FS);
    print_opengl_error();
  }

  glLinkProgram (object_prog);
  print_opengl_error();
  glGetProgramiv(object_prog,
                 GL_LINK_STATUS, &linked);

  if (!linked)
     print_program_info_log (object_prog, vertex_file, fragment_file);

  if (!linked)
    return 0;

  *result = malloc (sizeof (ProgramNode));
  (*result)->object_VS = object_VS;
  (*result)->object_FS = object_FS;
  (*result)->program_object = object_prog;

  return 1;
}

int
aux_clean_program(ProgramNode *program)
{
   if (program->object_VS != 0)
      glDeleteShader(program->object_VS);
   program->object_VS = 0;

   if (program->object_FS != 0)
      glDeleteShader(program->object_FS);
   program->object_FS = 0;

   return 1;
}

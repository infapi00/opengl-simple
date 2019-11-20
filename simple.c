/*
 * Reference program. This one will be used as a reference of the
 * program that we want to execute without mesa libraries.
 *
 * Will be used to get a clif_dump to be used by the real smoke-test
 * program.
 */
#include <stdio.h>
#include <stdbool.h>
#include <string.h>

#define GL_GLEXT_PROTOTYPES

#include <GL/gl.h>
#include <GL/glext.h>
#include <GLFW/glfw3.h>

#include "aux-GLSL.h"

#define WIDTH    32
#define HEIGHT   32

struct Vertex {
  float position[2];
};

bool use_opengl_es = false;
bool print_info = false;
bool just_clear = false;

int window;
ProgramNode *program_node;
GLuint program;
GLuint vbo;

GLuint color_rb;
GLuint fbo;

static bool
check_correct_image()
{
   GLubyte pixels[WIDTH * HEIGHT * 4];
   GLubyte expected[4] = {0, 255, 0, 255};
   unsigned count_wrong = 0;

   glReadPixels(0, 0, WIDTH, HEIGHT, GL_RGBA, GL_UNSIGNED_BYTE, pixels);

   for (unsigned pixel = 0; pixel < WIDTH*HEIGHT; pixel++) {
      bool any_component_wrong = false;
      for (unsigned component = 0; component < 4; component++) {
         if (pixels[pixel*4 + component] != expected[component]) {
            any_component_wrong = true;
         }
      }
      if (any_component_wrong)
         count_wrong++;
   }

   if (count_wrong > 0) {
      fprintf(stderr, "%i pixels wrong out of %i total\n", count_wrong, WIDTH*HEIGHT);
   }

   return count_wrong==0;
}


static void
scene_render (void)
{
  glBindFramebuffer(GL_FRAMEBUFFER, fbo);

  if (just_clear) {
     glClearColor (0.0, 1.0, 0.0, 1.0);
  } else {
     glClearColor (1.0, 1.0, 1.0, 1.0);
  }

  glClear (GL_COLOR_BUFFER_BIT);
  glDisable(GL_DEPTH_TEST);

  if (!just_clear) {
     glEnableVertexAttribArray (0);

     glDrawArrays (GL_TRIANGLE_FAN, 0, 4);

     if (!check_correct_image()) {
        fprintf(stderr, "ERROR: image on FBO is wrong.\n");
        exit(EXIT_FAILURE);
     }
  }
}

static void
setup ()
{
  const struct Vertex data[4] = {
          {{-1.0, -1.0}},
          {{-1.0,  1.0}},
          {{ 1.0,  1.0}},
          {{ 1.0, -1.0}},
  };
  int shader_result = 0;
  GLenum fb_status;

  /* Shaders */
  if (use_opengl_es)
     shader_result = aux_create_program ("simple-es.vert", "simple-es.frag", &program_node);
  else
     shader_result = aux_create_program ("simple.vert", "simple.frag", &program_node);

  if (!shader_result) {
    fprintf (stderr, "Failed to compile the program!\n");
    exit(EXIT_FAILURE);
  }

  program = program_node->program_object;
  glUseProgramObjectARB(program);

  glViewport(0, 0, WIDTH, HEIGHT);

  /* Buffer data */
  glGenBuffers (1, &vbo);
  glBindBuffer (GL_ARRAY_BUFFER, vbo);

  glBufferData (GL_ARRAY_BUFFER,
                sizeof (struct Vertex) * 4,
                data, GL_DYNAMIC_DRAW);

  glVertexAttribPointer (0, 2,
                         GL_FLOAT, GL_FALSE,
                         sizeof (struct Vertex),
                         (const GLvoid *) (0));

  /* FBO */
  glGenFramebuffers(1, &fbo);
  glBindFramebuffer(GL_FRAMEBUFFER, fbo);

  glGenRenderbuffers(1, &color_rb);
  glBindRenderbuffer(GL_RENDERBUFFER, color_rb);
  glRenderbufferStorage(GL_RENDERBUFFER, GL_RGBA8, WIDTH, HEIGHT);

  glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, color_rb);

  fb_status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
  if (fb_status != GL_FRAMEBUFFER_COMPLETE) {
     fprintf(stderr, "Failed to create framebuffer!\n");
     exit(EXIT_FAILURE);
  }
}

static void
cleanup()
{
   /* Shaders */
   aux_clean_program(program_node);
   free(program_node);

   /* Buffer data*/
   glDeleteBuffers(1, &vbo);

   /* FBO */
   glDeleteRenderbuffers(1, &color_rb);
   glDeleteFramebuffers(1, &fbo);
}

static void error_callback(int error, const char* description)
{
   fprintf(stderr, "ERROR: %s\n", description);
}

static void
print_usage()
{
   fprintf(stderr, "./simple [--opengl-es] [--info] [--just-clear]\n");
}

static void
parse_args(int argc, char *argv[])
{
   for (unsigned arg = 1; arg < argc; arg++) {
      if (argc >= 2) {
         if (strcmp(argv[arg], "--opengl-es") == 0) {
            use_opengl_es = true;
         } else if (strcmp(argv[arg], "--info") == 0) {
            print_info = true;
         } else if (strcmp(argv[arg], "--just-clear") == 0) {
            just_clear = true;
         } else {
            print_usage();
            exit(EXIT_FAILURE);
         }
      }
   }
}

int
main(int argc,
     char *argv[])
{
   GLFWwindow* window;

   parse_args(argc, argv);

   glfwSetErrorCallback(error_callback);
   if (!glfwInit()) {
      exit(EXIT_FAILURE);
   }

   if (use_opengl_es) {
      glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
      glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
      glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
   } else {
      glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_API);
      glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
      glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
   }
   glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);

   window = glfwCreateWindow(WIDTH, HEIGHT, "Smoke test", NULL, NULL);
   if (!window) {
      glfwTerminate();
      exit(EXIT_FAILURE);
   }
   glfwMakeContextCurrent(window);

   if (print_info) {
      printf ("%s, %s, %s\n",
              (char *) glGetString (GL_RENDERER),
              (char *) glGetString (GL_VENDOR),
              (char *) glGetString (GL_VERSION));
   }

   setup ();

   scene_render();

   cleanup();

   glfwDestroyWindow(window);
   glfwTerminate();

   fprintf(stdout, "Success!\n");
   return EXIT_SUCCESS;
}

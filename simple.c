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

#define WIDTH    128
#define HEIGHT   128

int window;
ProgramNode *program_node;
GLuint color_rb;
GLuint fbo;

bool verify_result = false;
bool do_clear = false;

static bool
check_correct_image()
{
   GLubyte pixels[WIDTH * HEIGHT * 4];
   GLubyte expected[4] = {255, 255, 0, 255};
   unsigned count_wrong = 0;

   glReadPixels(0, 0, WIDTH, HEIGHT, GL_RGBA, GL_UNSIGNED_BYTE, pixels);

   for (unsigned pixel = 0; pixel < WIDTH*HEIGHT; pixel++) {
      bool any_component_wrong = false;
      for (unsigned component = 0; component < 4; component++) {
         if (pixels[pixel*4 + component] != expected[component]) {
            any_component_wrong = true;
            printf("%d.%d Expected: %u, Found: %u\n",
                   pixel, component,
                   expected[component], pixels[pixel*4 + component]);
         }
      }
      if (any_component_wrong)
         count_wrong++;
   }

   if (count_wrong > 0) {
      fprintf(stderr, "%i pixels wrong out of %i total\n",
              count_wrong, WIDTH*HEIGHT);
   }

   return count_wrong==0;
}

static void
write_pixels_to_file()
{
   GLubyte data[WIDTH * HEIGHT * 4];
   glReadPixels(0, 0, WIDTH, HEIGHT, GL_RGBA, GL_UNSIGNED_BYTE, data);

   FILE *out = fopen("out.tga","wb");
   uint8_t TGAhead[] = { 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                         WIDTH, 0, HEIGHT, 0, 24, 0 };
   fwrite(&TGAhead, sizeof(TGAhead), 1, out);
   for (int i = 0; i < WIDTH; i++) {
      for (int j = 0; j < HEIGHT; j++) {
          fwrite(&data[(i * WIDTH + j) * 4] + 2, 1, 1, out); // B
          fwrite(&data[(i * WIDTH + j) * 4] + 1, 1, 1, out); // G
          fwrite(&data[(i * WIDTH + j) * 4] + 0, 1, 1, out); // R
      }
   }
   fclose(out);
}


static void
scene_render (void)
{
  glBindFramebuffer(GL_FRAMEBUFFER, fbo);

  if (do_clear) {
     glClearColor(1.0f, 1.0f, 0.0f, 1.0f);
     glClear(GL_COLOR_BUFFER_BIT);
  } else {
     glDrawArrays (GL_TRIANGLE_STRIP, 0, 4);
  }

  write_pixels_to_file();

  if (verify_result && !check_correct_image()) {
    fprintf(stderr, "ERROR: image on FBO is wrong.\n");
    exit(EXIT_FAILURE);
  }
}

static void
setup ()
{
  int shader_result = 0;
  GLenum fb_status;

  /* Shaders */
  shader_result =
     aux_create_program ("simple-es.vert", "simple-es.frag", &program_node);

  if (!shader_result) {
    fprintf (stderr, "Failed to compile the program!\n");
    exit(EXIT_FAILURE);
  }

  glUseProgramObjectARB(program_node->program_object);

  glViewport(0, 0, WIDTH, HEIGHT);

  /* FBO */
  glGenFramebuffers(1, &fbo);
  glBindFramebuffer(GL_FRAMEBUFFER, fbo);
  glGenRenderbuffers(1, &color_rb);
  glBindRenderbuffer(GL_RENDERBUFFER, color_rb);
  glRenderbufferStorage(GL_RENDERBUFFER, GL_RGBA8, WIDTH, HEIGHT);
  glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                            GL_RENDERBUFFER, color_rb);
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

   /* FBO */
   glDeleteRenderbuffers(1, &color_rb);
   glDeleteFramebuffers(1, &fbo);
}

static void
error_callback(int error, const char* description)
{
   fprintf(stderr, "ERROR: %s\n", description);
}

static void
parse_args(int argc, char *argv[])
{
   for (unsigned arg = 1; arg < argc; arg++) {
      if (strcmp(argv[arg], "--verify") == 0) {
         verify_result = true;
      } else if (strcmp(argv[arg], "--clear") == 0) {
         do_clear = true;
      } else {
         printf("Incorrect options. Allowed options:"
                "\n\t --verify: check if the result image matches expectation."
                "\n\t --clear:  only clear the render target."
                "\n");
         exit(EXIT_FAILURE);
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

   glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
   glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
   glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
   glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);

   window = glfwCreateWindow(WIDTH, HEIGHT, "Small Test", NULL, NULL);
   if (!window) {
      glfwTerminate();
      exit(EXIT_FAILURE);
   }
   glfwMakeContextCurrent(window);

   setup();
   scene_render();
   cleanup();

   glfwDestroyWindow(window);
   glfwTerminate();

   fprintf(stdout, "Success!\n");
   return EXIT_SUCCESS;
}

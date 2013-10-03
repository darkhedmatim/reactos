/*
 * Mesa 3-D graphics library
 * Version:  7.5
 *
 * Copyright (C) 2009  VMware, Inc.  All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * BRIAN PAUL BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
 * AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

/**
 * \file shared.c
 * Shared-context state
 */

#include "imports.h"
#include "mfeatures.h"
#include "mtypes.h"
#include "hash.h"
#include "bufferobj.h"
#include "shared.h"
#include "program/program.h"
#include "dlist.h"
#include "shaderobj.h"


/**
 * Allocate and initialize a shared context state structure.
 * Initializes the display list, texture objects and vertex programs hash
 * tables, allocates the texture objects. If it runs out of memory, frees
 * everything already allocated before returning NULL.
 *
 * \return pointer to a gl_shared_state structure on success, or NULL on
 * failure.
 */
struct gl_shared_state *
_mesa_alloc_shared_state(struct gl_context *ctx)
{
   struct gl_shared_state *shared;
   GLuint i;

   shared = CALLOC_STRUCT(gl_shared_state);
   if (!shared)
      return NULL;

   _glthread_INIT_MUTEX(shared->Mutex);

   shared->DisplayList = _mesa_NewHashTable();
   shared->TexObjects = _mesa_NewHashTable();
   shared->Programs = _mesa_NewHashTable();

#if FEATURE_ARB_vertex_program
   shared->DefaultVertexProgram = (struct gl_vertex_program *)
      ctx->Driver.NewProgram(ctx, GL_VERTEX_PROGRAM_ARB, 0);
#endif

#if FEATURE_ARB_fragment_program
   shared->DefaultFragmentProgram = (struct gl_fragment_program *)
      ctx->Driver.NewProgram(ctx, GL_FRAGMENT_PROGRAM_ARB, 0);
#endif

#if FEATURE_ARB_shader_objects
   shared->ShaderObjects = _mesa_NewHashTable();
#endif

   shared->BufferObjects = _mesa_NewHashTable();

   /* Allocate the default buffer object */
   shared->NullBufferObj = ctx->Driver.NewBufferObject(ctx, 0, 0);

   /* Create default texture objects */
   for (i = 0; i < NUM_TEXTURE_TARGETS; i++) {
      /* NOTE: the order of these enums matches the TEXTURE_x_INDEX values */
      static const GLenum targets[NUM_TEXTURE_TARGETS] = {
         GL_TEXTURE_CUBE_MAP,
         GL_TEXTURE_3D,
         GL_TEXTURE_2D,
         GL_TEXTURE_1D
      };
      STATIC_ASSERT(Elements(targets) == NUM_TEXTURE_TARGETS);
      shared->DefaultTex[i] = ctx->Driver.NewTextureObject(ctx, 0, targets[i]);
   }

   /* sanity check */
   assert(shared->DefaultTex[TEXTURE_1D_INDEX]->RefCount == 1);

   /* Mutex and timestamp for texobj state validation */
   _glthread_INIT_MUTEX(shared->TexMutex);
   shared->TextureStateStamp = 0;

#if FEATURE_EXT_framebuffer_object
   shared->FrameBuffers = _mesa_NewHashTable();
   shared->RenderBuffers = _mesa_NewHashTable();
#endif

   return shared;
}


/**
 * Callback for deleting a display list.  Called by _mesa_HashDeleteAll().
 */
static void
delete_displaylist_cb(GLuint id, void *data, void *userData)
{
   struct gl_display_list *list = (struct gl_display_list *) data;
   struct gl_context *ctx = (struct gl_context *) userData;
   _mesa_delete_list(ctx, list);
}


/**
 * Callback for deleting a texture object.  Called by _mesa_HashDeleteAll().
 */
static void
delete_texture_cb(GLuint id, void *data, void *userData)
{
   struct gl_texture_object *texObj = (struct gl_texture_object *) data;
   struct gl_context *ctx = (struct gl_context *) userData;
   ctx->Driver.DeleteTexture(ctx, texObj);
}


/**
 * Callback for deleting a program object.  Called by _mesa_HashDeleteAll().
 */
static void
delete_program_cb(GLuint id, void *data, void *userData)
{
   struct gl_program *prog = (struct gl_program *) data;
   struct gl_context *ctx = (struct gl_context *) userData;
   if(prog != &_mesa_DummyProgram) {
      ASSERT(prog->RefCount == 1); /* should only be referenced by hash table */
      prog->RefCount = 0;  /* now going away */
      ctx->Driver.DeleteProgram(ctx, prog);
   }
}


/**
 * Callback for deleting a buffer object.  Called by _mesa_HashDeleteAll().
 */
static void
delete_bufferobj_cb(GLuint id, void *data, void *userData)
{
   struct gl_buffer_object *bufObj = (struct gl_buffer_object *) data;
   struct gl_context *ctx = (struct gl_context *) userData;
   if (_mesa_bufferobj_mapped(bufObj)) {
      ctx->Driver.UnmapBuffer(ctx, bufObj);
      bufObj->Pointer = NULL;
   }
   _mesa_reference_buffer_object(ctx, &bufObj, NULL);
}


/**
 * Callback for freeing shader program data. Call it before delete_shader_cb
 * to avoid memory access error.
 */
static void
free_shader_program_data_cb(GLuint id, void *data, void *userData)
{
   struct gl_context *ctx = (struct gl_context *) userData;
   struct gl_shader_program *shProg = (struct gl_shader_program *) data;

   if (shProg->Type == GL_SHADER_PROGRAM_MESA) {
       _mesa_free_shader_program_data(ctx, shProg);
   }
}


/**
 * Callback for deleting shader and shader programs objects.
 * Called by _mesa_HashDeleteAll().
 */
static void
delete_shader_cb(GLuint id, void *data, void *userData)
{
   struct gl_context *ctx = (struct gl_context *) userData;
   struct gl_shader *sh = (struct gl_shader *) data;
   if (sh->Type == GL_FRAGMENT_SHADER || sh->Type == GL_VERTEX_SHADER) {
      ctx->Driver.DeleteShader(ctx, sh);
   }
   else {
      struct gl_shader_program *shProg = (struct gl_shader_program *) data;
      ASSERT(shProg->Type == GL_SHADER_PROGRAM_MESA);
      ctx->Driver.DeleteShaderProgram(ctx, shProg);
   }
}


/**
 * Callback for deleting a framebuffer object.  Called by _mesa_HashDeleteAll()
 */
static void
delete_framebuffer_cb(GLuint id, void *data, void *userData)
{
   struct gl_framebuffer *fb = (struct gl_framebuffer *) data;
   /* The fact that the framebuffer is in the hashtable means its refcount
    * is one, but we're removing from the hashtable now.  So clear refcount.
    */
   /*assert(fb->RefCount == 1);*/
   fb->RefCount = 0;

   /* NOTE: Delete should always be defined but there are two reports
    * of it being NULL (bugs 13507, 14293).  Work-around for now.
    */
   if (fb->Delete)
      fb->Delete(fb);
}


/**
 * Callback for deleting a renderbuffer object. Called by _mesa_HashDeleteAll()
 */
static void
delete_renderbuffer_cb(GLuint id, void *data, void *userData)
{
   struct gl_renderbuffer *rb = (struct gl_renderbuffer *) data;
   rb->RefCount = 0;  /* see comment for FBOs above */
   if (rb->Delete)
      rb->Delete(rb);
}


/**
 * Deallocate a shared state object and all children structures.
 *
 * \param ctx GL context.
 * \param shared shared state pointer.
 * 
 * Frees the display lists, the texture objects (calling the driver texture
 * deletion callback to free its private data) and the vertex programs, as well
 * as their hash tables.
 *
 * \sa alloc_shared_state().
 */
static void
free_shared_state(struct gl_context *ctx, struct gl_shared_state *shared)
{
   GLuint i;

   /* Free the dummy/fallback texture object */
   if (shared->FallbackTex)
      ctx->Driver.DeleteTexture(ctx, shared->FallbackTex);

   /*
    * Free display lists
    */
   _mesa_HashDeleteAll(shared->DisplayList, delete_displaylist_cb, ctx);
   _mesa_DeleteHashTable(shared->DisplayList);

#if FEATURE_ARB_shader_objects
   _mesa_HashWalk(shared->ShaderObjects, free_shader_program_data_cb, ctx);
   _mesa_HashDeleteAll(shared->ShaderObjects, delete_shader_cb, ctx);
   _mesa_DeleteHashTable(shared->ShaderObjects);
#endif

   _mesa_HashDeleteAll(shared->Programs, delete_program_cb, ctx);
   _mesa_DeleteHashTable(shared->Programs);

#if FEATURE_ARB_vertex_program
   _mesa_reference_vertprog(ctx, &shared->DefaultVertexProgram, NULL);
#endif

#if FEATURE_ARB_fragment_program
   _mesa_reference_fragprog(ctx, &shared->DefaultFragmentProgram, NULL);
#endif

   _mesa_HashDeleteAll(shared->BufferObjects, delete_bufferobj_cb, ctx);
   _mesa_DeleteHashTable(shared->BufferObjects);

#if FEATURE_EXT_framebuffer_object
   _mesa_HashDeleteAll(shared->FrameBuffers, delete_framebuffer_cb, ctx);
   _mesa_DeleteHashTable(shared->FrameBuffers);
   _mesa_HashDeleteAll(shared->RenderBuffers, delete_renderbuffer_cb, ctx);
   _mesa_DeleteHashTable(shared->RenderBuffers);
#endif

   _mesa_reference_buffer_object(ctx, &shared->NullBufferObj, NULL);

   /*
    * Free texture objects (after FBOs since some textures might have
    * been bound to FBOs).
    */
   ASSERT(ctx->Driver.DeleteTexture);
   /* the default textures */
   for (i = 0; i < NUM_TEXTURE_TARGETS; i++) {
      ctx->Driver.DeleteTexture(ctx, shared->DefaultTex[i]);
   }

   /* all other textures */
   _mesa_HashDeleteAll(shared->TexObjects, delete_texture_cb, ctx);
   _mesa_DeleteHashTable(shared->TexObjects);

   _glthread_DESTROY_MUTEX(shared->Mutex);
   _glthread_DESTROY_MUTEX(shared->TexMutex);

   free(shared);
}


/**
 * gl_shared_state objects are ref counted.
 * If ptr's refcount goes to zero, free the shared state.
 */
void
_mesa_reference_shared_state(struct gl_context *ctx,
                             struct gl_shared_state **ptr,
                             struct gl_shared_state *state)
{
   if (*ptr == state)
      return;

   if (*ptr) {
      /* unref old state */
      struct gl_shared_state *old = *ptr;
      GLboolean delete;

      _glthread_LOCK_MUTEX(old->Mutex);
      assert(old->RefCount >= 1);
      old->RefCount--;
      delete = (old->RefCount == 0);
      _glthread_UNLOCK_MUTEX(old->Mutex);

      if (delete) {
         free_shared_state(ctx, old);
      }

      *ptr = NULL;
   }

   if (state) {
      /* reference new state */
      _glthread_LOCK_MUTEX(state->Mutex);
      state->RefCount++;
      *ptr = state;
      _glthread_UNLOCK_MUTEX(state->Mutex);
   }
}

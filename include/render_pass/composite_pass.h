#ifndef SS_COMPOSITE_PASS_H
#define SS_COMPOSITE_PASS_H

#include "render_pass/render_pass_i.h"

typedef struct composite_pass_data_t
{
	gs_handle(gs_graphics_shader_t) shader;
	gs_handle(gs_graphics_uniform_t) u_input_tex;
	gs_handle(gs_graphics_uniform_t) u_blur_tex;
	gs_handle(gs_graphics_uniform_t) u_exposure;
	gs_handle(gs_graphics_uniform_t) u_gamma;
	gs_handle(gs_graphics_uniform_t) u_bloom_scalar;
	gs_handle(gs_graphics_uniform_t) u_saturation;
	gs_handle(gs_graphics_texture_t) rt;
	gs_handle(gs_graphics_vertex_buffer_t) vbo;
	gs_handle(gs_graphics_index_buffer_t) ibo;
	gs_handle(gs_graphics_render_pass_t) rp;
	gs_handle(gs_graphics_pipeline_t) pip;
} composite_pass_data_t;

typedef struct composite_pass_t
{
	_base(render_pass_i);
	composite_pass_data_t data;
} composite_pass_t;

// Use this to pass in parameters for the pass ( will check for this)
typedef struct composite_pass_parameters_t 
{
	gs_handle(gs_graphics_texture_t) input_texture;
	gs_handle(gs_graphics_texture_t) blur_texture;
} composite_pass_parameters_t;

composite_pass_t composite_pass_ctor(gs_handle(gs_graphics_framebuffer_t) fb);

#endif
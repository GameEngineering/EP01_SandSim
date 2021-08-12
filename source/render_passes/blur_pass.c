
#include "render_pass/blur_pass.h"

// Forward Decls.
void calculate_blur_weights(blur_pass_t* pass);
void _blur_pass(gs_command_buffer_t* cb, render_pass_i* _pass, void* _params);

/*===============
// Blur Shaders
================*/

// Shaders
#if (defined GS_PLATFORM_WEB || defined GS_PLATFORM_ANDROID)
    #define GL_VERSION_STR "#version 300 es\n"
#else
    #define GL_VERSION_STR "#version 330 core\n"
#endif

const char* v_h_blur_src =
GL_VERSION_STR
"precision mediump float;\n"
"layout (location = 0) in vec2 a_position;\n"
"layout (location = 1) in vec2 a_uv;\n"
"out vec2 position;\n"
"out vec2 tex_coord;\n"
"out vec2 v_blur_tex_coods[11];\n"
"uniform vec2 u_tex_size;"
"void main() {\n"
"	gl_Position = vec4(a_position.x, a_position.y, 0.0, 1.0);\n"
"	tex_coord = a_uv;\n"
"	position = a_position;\n"
"	vec2 center_tex_coords = a_position * 0.5 + 0.5;\n"
"	float pixel_size = 1.0 / u_tex_size.x;\n"
"	for (int i = -5; i <= 5; ++i) {\n"
"		v_blur_tex_coods[i + 5] = center_tex_coords + vec2(pixel_size * float(i), 0.0);\n"
"	}\n"
"}";

const char* v_v_blur_src =
GL_VERSION_STR
"precision mediump float;\n"
"layout (location = 0) in vec2 a_position;\n"
"layout (location = 1) in vec2 a_uv;\n"
"out vec2 position;\n"
"out vec2 tex_coord;\n"
"out vec2 v_blur_tex_coods[11];\n"
"uniform vec2 u_tex_size;"
"void main() {\n"
"	gl_Position = vec4(a_position.x, a_position.y, 0.0, 1.0);\n"
"	tex_coord = a_uv;\n"
"	position = a_position;\n"
"	vec2 center_tex_coords = a_position * 0.5 + 0.5;\n"
"	float pixel_size = 1.0 / u_tex_size.y;\n"
"	for (int i = -5; i <= 5; ++i) {\n"
"		v_blur_tex_coods[i + 5] = center_tex_coords + vec2(0.0, pixel_size * float(i));\n"
"	}\n"
"}";

const char* f_blur_src =
GL_VERSION_STR
"precision mediump float;\n"
"in vec2 position;\n"
"in vec2 tex_coord;\n"
"in vec2 v_blur_tex_coods[11];\n"
"out vec4 frag_color;\n"
"uniform sampler2D u_tex;\n"
"void main() {\n"
"	frag_color = vec4(0.0, 0.0, 0.0, 1.0);\n"
"	frag_color += texture(u_tex, v_blur_tex_coods[0]) * 0.0093;\n"
"	frag_color += texture(u_tex, v_blur_tex_coods[1]) * 0.028002;\n"
"	frag_color += texture(u_tex, v_blur_tex_coods[2]) * 0.065984;\n"
"	frag_color += texture(u_tex, v_blur_tex_coods[3]) * 0.121703;\n"
"	frag_color += texture(u_tex, v_blur_tex_coods[4]) * 0.175713;\n"
"	frag_color += texture(u_tex, v_blur_tex_coods[5]) * 0.198596;\n"
"	frag_color += texture(u_tex, v_blur_tex_coods[6]) * 0.175713;\n"
"	frag_color += texture(u_tex, v_blur_tex_coods[7]) * 0.121703;\n"
"	frag_color += texture(u_tex, v_blur_tex_coods[8]) * 0.065984;\n"
"	frag_color += texture(u_tex, v_blur_tex_coods[9]) * 0.028002;\n"
"	frag_color += texture(u_tex, v_blur_tex_coods[10]) * 0.0093;\n"
"}\n";

// Vertex data layout for our mesh (for this shader, it's a single float2 attribute for position)
// gs_global gs_vertex_attribute_type layout[] = {
// 	gs_vertex_attribute_float2,
// 	gs_vertex_attribute_float2
// };
// // Count of our vertex attribute array
// gs_global u32 layout_count = sizeof(layout) / sizeof(gs_vertex_attribute_type); 

// Vertex data for triangle
gs_global f32 v_data[] = {
	// Positions  UVs
	-1.0f, -1.0f,  0.0f, 0.0f,	// Top Left
	 1.0f, -1.0f,  1.0f, 0.0f,	// Top Right 
	-1.0f,  1.0f,  0.0f, 1.0f,  // Bottom Left
	 1.0f,  1.0f,  1.0f, 1.0f   // Bottom Right
};

gs_global u32 i_data[] = {
	0, 2, 3,
	3, 1, 0
};

// All of these settings are being directly copied from my previous implementation in Enjon to save time
// GraphicsSubsystem: https://github.com/MrFrenik/Enjon/blob/master/Include/Graphics/GraphicsSubsystem.h
blur_pass_t blur_pass_ctor(gs_handle(gs_graphics_framebuffer_t) fb)
{
	blur_pass_t bp = {0};
	bp.data.iterations = 1;

	// Set pass for base
	bp._base.pass = &_blur_pass;

	// Initialize shaders and uniforms
	// bp.data.horizontal_blur_shader = gfx->construct_shader(v_h_blur_src, f_blur_src);

    // Create shader
    bp.data.horizontal_blur_shader = gs_graphics_shader_create (
        &(gs_graphics_shader_desc_t) {
            .sources = (gs_graphics_shader_source_desc_t[]) {
                {.type = GS_GRAPHICS_SHADER_STAGE_VERTEX, .source = v_h_blur_src},
                {.type = GS_GRAPHICS_SHADER_STAGE_FRAGMENT, .source = f_blur_src}
            }, 
            .size = 2 * sizeof(gs_graphics_shader_source_desc_t),
            .name = "hblur_shader"
        }
    );

	// bp.data.vertical_blur_shader = gfx->construct_shader(v_v_blur_src, f_blur_src);

    bp.data.vertical_blur_shader = gs_graphics_shader_create (
        &(gs_graphics_shader_desc_t) {
            .sources = (gs_graphics_shader_source_desc_t[]) {
                {.type = GS_GRAPHICS_SHADER_STAGE_VERTEX, .source = v_v_blur_src},
                {.type = GS_GRAPHICS_SHADER_STAGE_FRAGMENT, .source = f_blur_src}
            }, 
            .size = 2 * sizeof(gs_graphics_shader_source_desc_t),
            .name = "vblur_shader"
        }
    );

	// bp.data.u_input_tex = gfx->construct_uniform(bp.data.horizontal_blur_shader, "u_tex", gs_uniform_type_sampler2d);

	// Construct uniforms for shader
    bp.data.u_input_tex = gs_graphics_uniform_create (
        &(gs_graphics_uniform_desc_t) {
            .name = "u_tex",
            .layout = &(gs_graphics_uniform_layout_desc_t){.type = GS_GRAPHICS_UNIFORM_SAMPLER2D}
        }
    );

	// bp.data.u_tex_size = gfx->construct_uniform(bp.data.horizontal_blur_shader, "u_tex_size", gs_uniform_type_vec2);

    bp.data.u_tex_size = gs_graphics_uniform_create (
        &(gs_graphics_uniform_desc_t) {
            .name = "u_tex_size",
            .layout = &(gs_graphics_uniform_layout_desc_t){.type = GS_GRAPHICS_UNIFORM_VEC2}
        }
    );

	// Construct render target to render into
	gs_vec2 ws = gs_platform_window_sizev(gs_platform_main_window());

	gs_graphics_texture_desc_t t_desc = gs_default_val();
	t_desc.wrap_s = GS_GRAPHICS_TEXTURE_WRAP_CLAMP_TO_EDGE;
	t_desc.wrap_t = GS_GRAPHICS_TEXTURE_WRAP_CLAMP_TO_EDGE;
	t_desc.mag_filter = GS_GRAPHICS_TEXTURE_FILTER_LINEAR;
	t_desc.min_filter = GS_GRAPHICS_TEXTURE_FILTER_LINEAR;
	t_desc.format = GS_GRAPHICS_TEXTURE_FORMAT_RGBA16F;
	t_desc.num_mips = 0;
	t_desc.data = NULL;
	t_desc.width = (s32)(ws.x / 16);
	t_desc.height = (s32)(ws.y / 16);

	// Two render targets for double buffered separable blur (For now, just set to window's viewport)
	bp.data.blur_render_target_a = gs_graphics_texture_create(&t_desc);
	bp.data.blur_render_target_b = gs_graphics_texture_create(&t_desc);

    // Construct vertex buffer
    bp.data.vbo = gs_graphics_vertex_buffer_create(
        &(gs_graphics_vertex_buffer_desc_t) {
            .data = v_data,
            .size = sizeof(v_data)
        }
    );

    // Construct index buffer
    bp.data.ibo = gs_graphics_index_buffer_create(
        &(gs_graphics_index_buffer_desc_t) {
            .data = i_data,
            .size = sizeof(i_data)
        }
    );

	// Construct render passes
    bp.data.rp_vertical = gs_graphics_render_pass_create(
        &(gs_graphics_render_pass_desc_t){
            .fbo = fb,                      						// Frame buffer to bind for render pass
            .color = &bp.data.blur_render_target_a,                 // Color buffer array to bind to frame buffer    
            .color_size = sizeof(bp.data.blur_render_target_a)      // Size of color attachment array in bytes
        }
    );

    bp.data.rp_horizontal = gs_graphics_render_pass_create(
        &(gs_graphics_render_pass_desc_t){
            .fbo = fb,                      						// Frame buffer to bind for render pass
            .color = &bp.data.blur_render_target_b,                 // Color buffer array to bind to frame buffer    
            .color_size = sizeof(bp.data.blur_render_target_b)      // Size of color attachment array in bytes
        }
    );

    bp.data.pip_vertical = gs_graphics_pipeline_create (
        &(gs_graphics_pipeline_desc_t) {
            .raster = {
                .shader = bp.data.vertical_blur_shader
            },
            .blend = {
            	.func = GS_GRAPHICS_BLEND_EQUATION_ADD,
            	.src = GS_GRAPHICS_BLEND_MODE_SRC_ALPHA,
            	.dst = GS_GRAPHICS_BLEND_MODE_ONE_MINUS_SRC_ALPHA
            },
            .layout = {
                .attrs = (gs_graphics_vertex_attribute_desc_t[]){
                    {.format = GS_GRAPHICS_VERTEX_ATTRIBUTE_FLOAT2, .name = "a_position"}, // Named attribute required for lower GL versions / ES2 / ES3
                    {.format = GS_GRAPHICS_VERTEX_ATTRIBUTE_FLOAT2, .name = "a_uv"}        // Named attribute required for lower GL versions / ES2 / ES3
                },
                .size = 2 * sizeof(gs_graphics_vertex_attribute_desc_t)
            }
        }
    );

    bp.data.pip_horizontal = gs_graphics_pipeline_create (
        &(gs_graphics_pipeline_desc_t) {
            .raster = {
                .shader = bp.data.horizontal_blur_shader
            },
            .blend = {
            	.func = GS_GRAPHICS_BLEND_EQUATION_ADD,
            	.src = GS_GRAPHICS_BLEND_MODE_SRC_ALPHA,
            	.dst = GS_GRAPHICS_BLEND_MODE_ONE_MINUS_SRC_ALPHA
            },
            .layout = {
                .attrs = (gs_graphics_vertex_attribute_desc_t[]){
                    {.format = GS_GRAPHICS_VERTEX_ATTRIBUTE_FLOAT2, .name = "a_position"}, // Named attribute required for lower GL versions / ES2 / ES3
                    {.format = GS_GRAPHICS_VERTEX_ATTRIBUTE_FLOAT2, .name = "a_uv"}        // Named attribute required for lower GL versions / ES2 / ES3
                },
                .size = 2 * sizeof(gs_graphics_vertex_attribute_desc_t)
            }
        }
    );

	// bp.data.vb = gfx->construct_vertex_buffer(layout, layout_count, v_data, sizeof(v_data));
	// bp.data.ib = gfx->construct_index_buffer(i_data, sizeof(i_data));

	return bp;
}

void _blur_pass(gs_command_buffer_t* cb, render_pass_i* _pass, void* _params)
{

	blur_pass_t* bp = (blur_pass_t*)_pass;
	if (!bp) {
		return;
	} 

	// Can only use valid params
	blur_pass_parameters_t* params = (blur_pass_parameters_t*)_params;
	if (!params) {
		return;
	}
	
	gs_vec2 ws = gs_platform_window_sizev(gs_platform_main_window());
	gs_vec2 tex_size = (gs_vec2){ws.x / 16, ws.y / 16};

	// Need to actually re-create the texture if the size has changed
	gs_graphics_texture_desc_t t_desc = gs_default_val();
	t_desc.wrap_s = GS_GRAPHICS_TEXTURE_WRAP_CLAMP_TO_BORDER;
	t_desc.wrap_t = GS_GRAPHICS_TEXTURE_WRAP_CLAMP_TO_BORDER;
	t_desc.mag_filter = GS_GRAPHICS_TEXTURE_FILTER_LINEAR;
	t_desc.min_filter = GS_GRAPHICS_TEXTURE_FILTER_LINEAR;
	t_desc.format = GS_GRAPHICS_TEXTURE_FORMAT_RGBA16F;
	t_desc.num_mips = 0;
	t_desc.data = NULL;
	t_desc.width = (s32)(ws.x / 16);
	t_desc.height = (s32)(ws.y / 16);

	// Two render targets for double buffered separable blur (For now, just set to window's viewport)
	//gs_graphics_texture_request_update(cb, bp->data.blur_render_target_a, &t_desc);
	//gs_graphics_texture_request_update(cb, bp->data.blur_render_target_b, &t_desc);
	// gfx->update_texture_data(bp->data.blur_render_target_a, t_desc);
	// gfx->update_texture_data(bp->data.blur_render_target_b, t_desc);

    // Render pass action for clearing the screen
    gs_graphics_clear_desc_t clear = (gs_graphics_clear_desc_t){
        .actions = &(gs_graphics_clear_action_t){.color = {0.0f, 0.0f, 0.0f, 1.f}}
    };

	for (u32 i = 0; i < bp->data.iterations * 2; ++i) 
	{
		b32 is_even = (i % 2 == 0);
		gs_handle(gs_graphics_pipeline_t) pip = is_even ? bp->data.pip_vertical : bp->data.pip_horizontal;
		gs_handle(gs_graphics_render_pass_t) rp = is_even ? bp->data.rp_vertical : bp->data.rp_horizontal;
		gs_handle(gs_graphics_texture_t) tex = (i == 0) ? params->input_texture : is_even ? bp->data.blur_render_target_b : bp->data.blur_render_target_a;

	    // Uniform buffer array
	    gs_graphics_bind_uniform_desc_t uniforms[] = {
	        (gs_graphics_bind_uniform_desc_t){.uniform = bp->data.u_input_tex, .data = &tex, .binding = 0},
	        (gs_graphics_bind_uniform_desc_t){.uniform = bp->data.u_tex_size, .data = &tex_size},
	    };

	    // Binding descriptor for vertex buffer
	    gs_graphics_bind_desc_t binds = {
	        .vertex_buffers = {.desc = &(gs_graphics_bind_vertex_buffer_desc_t){.buffer = bp->data.vbo}},
	        .index_buffers = {.desc = &(gs_graphics_bind_index_buffer_desc_t){.buffer = bp->data.ibo}},
	        .uniforms = {.desc = uniforms, .size = sizeof(uniforms)}
	    };

		// Bind render pass, bind pipeline, set up everything...
		gs_graphics_begin_render_pass(cb, rp);
			gs_graphics_bind_pipeline(cb, pip);
			gs_graphics_set_viewport(cb, 0, 0, (u32)tex_size.x, (u32)tex_size.y);
			gs_graphics_clear(cb, &clear);
			gs_graphics_apply_bindings(cb, &binds);
			gs_graphics_draw(cb, &(gs_graphics_draw_desc_t){.start = 0, .count = 6});
		gs_graphics_end_render_pass(cb);

		// gs_resource(gs_texture)* target = is_even ? &bp->data.blur_render_target_a : &bp->data.blur_render_target_b;
		// gs_resource(gs_shader)* shader = is_even ? &bp->data.horizontal_blur_shader : &bp->data.vertical_blur_shader;
		// gs_resource(gs_texture)* tex = (i == 0) ? &params->input_texture : is_even ? &bp->data.blur_render_target_b : &bp->data.blur_render_target_a;

		// Set frame buffer attachment for rendering
		// gfx->set_frame_buffer_attachment(cb, *target, 0);

		// Set viewport 
		// gfx->set_view_port(cb, (u32)(tex_size.x), (u32)(tex_size.y));	
		// Clear
		// f32 cc[4] = {0.f, 0.f, 0.f, 1.f};
		// gfx->set_view_clear(cb, (f32*)&cc);

		// // Use the program
		// gfx->bind_shader(cb, *shader);
		// {
		// 	gfx->bind_texture(cb, bp->data.u_input_tex, *tex, 0);
		// 	gfx->bind_uniform(cb, bp->data.u_tex_size, &tex_size);
		// 	gfx->bind_vertex_buffer(cb, bp->data.vb);
		// 	gfx->bind_index_buffer(cb, bp->data.ib);
		// 	gfx->draw_indexed(cb, 6);
		// }
	}
}


















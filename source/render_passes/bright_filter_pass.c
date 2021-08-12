#include "render_pass/bright_filter_pass.h"

// Foward Decls
void bp_pass(gs_command_buffer_t* cb, struct render_pass_i* pass, void* paramters);

// Shaders
#if (defined GS_PLATFORM_WEB || defined GS_PLATFORM_ANDROID)
    #define GL_VERSION_STR "#version 300 es\n"
#else
    #define GL_VERSION_STR "#version 330 core\n"
#endif

const char* bp_v_src =
GL_VERSION_STR
"precision mediump float;\n"
"layout (location = 0) in vec2 a_position;\n"
"layout (location = 1) in vec2 a_uv;\n"
"out vec2 tex_coord;\n"
"void main() {\n"
"	gl_Position = vec4(a_position, 0.0, 1.0);\n"
"	tex_coord = a_uv;\n"
"}\n";

const char* bp_f_src =
GL_VERSION_STR
"precision mediump float;\n"
"in vec2 tex_coord;\n"
"out vec4 frag_color;\n"
"uniform sampler2D u_tex;\n"
"void main() {\n"
"	frag_color = vec4(0.0, 0.0, 0.0, 1.0);\n"
"	vec3 tex_color = texture(u_tex, tex_coord).rgb;\n"
"	float brightness = dot(tex_color, vec3(0.2126, 0.7152, 0.0722));\n"
"	if (tex_color.b < 0.2 && brightness > 0.4) {\n"
"		vec3 op = clamp(tex_color, vec3(0), vec3(255));\n"
"		frag_color = vec4(op * 0.1, 1.0);\n"
"	}\n"
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

bright_filter_pass_t bright_filter_pass_ctor(gs_handle(gs_graphics_framebuffer_t) fb)
{
	bright_filter_pass_t bp = {0};
	
	bp._base.pass = &bp_pass;

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

    bp.data.shader = gs_graphics_shader_create (
        &(gs_graphics_shader_desc_t) {
            .sources = (gs_graphics_shader_source_desc_t[]) {
                {.type = GS_GRAPHICS_SHADER_STAGE_VERTEX, .source = bp_v_src},
                {.type = GS_GRAPHICS_SHADER_STAGE_FRAGMENT, .source = bp_f_src}
            }, 
            .size = 2 * sizeof(gs_graphics_shader_source_desc_t),
            .name = "bright_shader"
        }
    );

	// bp.data.u_input_tex = gfx->construct_uniform(bp.data.shader, "u_tex", gs_uniform_type_sampler2d);

	// Construct uniforms for shader
    bp.data.u_input_tex = gs_graphics_uniform_create (
        &(gs_graphics_uniform_desc_t) {
            .name = "u_tex",
            .layout = &(gs_graphics_uniform_layout_desc_t){.type = GS_GRAPHICS_UNIFORM_SAMPLER2D}
        }
    );

	gs_vec2 ws = gs_platform_window_sizev(gs_platform_main_window());

	gs_graphics_texture_desc_t t_desc = gs_default_val();
	t_desc.wrap_s = GS_GRAPHICS_TEXTURE_WRAP_CLAMP_TO_EDGE;
	t_desc.wrap_t = GS_GRAPHICS_TEXTURE_WRAP_CLAMP_TO_EDGE;
	t_desc.mag_filter = GS_GRAPHICS_TEXTURE_FILTER_LINEAR;
	t_desc.min_filter = GS_GRAPHICS_TEXTURE_FILTER_LINEAR;
	t_desc.format = GS_GRAPHICS_TEXTURE_FORMAT_RGBA16F;
	t_desc.num_mips = 0;
	t_desc.data = NULL;
	t_desc.width = (s32)(ws.x / 8);
	t_desc.height = (s32)(ws.y / 8);
	bp.data.rt = gs_graphics_texture_create(&t_desc);

	// Construct render passes
    bp.data.rp = gs_graphics_render_pass_create(
        &(gs_graphics_render_pass_desc_t){
            .fbo = fb,                       // Frame buffer to bind for render pass
            .color = &bp.data.rt,             // Color buffer array to bind to frame buffer    
            .color_size = sizeof(bp.data.rt)  // Size of color attachment array in bytes
        }
    );

    bp.data.pip = gs_graphics_pipeline_create(
        &(gs_graphics_pipeline_desc_t) {
            .raster = {
                .shader = bp.data.shader
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

	return bp;
}

 void bp_pass(gs_command_buffer_t* cb, struct render_pass_i* _pass, void* _params)
 {
	bright_filter_pass_t* bp = (bright_filter_pass_t*)_pass;
	if (!bp) {
		return;
	} 

	// Can only use valid params
	bright_filter_pass_parameters_t* params = (bright_filter_pass_parameters_t*)_params;
	if (!params) {
		return;
	}

	gs_vec2 ws = gs_platform_window_sizev(gs_platform_main_window());

	gs_graphics_texture_desc_t t_desc = gs_default_val();
	t_desc.wrap_s = GS_GRAPHICS_TEXTURE_WRAP_CLAMP_TO_BORDER;
	t_desc.wrap_t = GS_GRAPHICS_TEXTURE_WRAP_CLAMP_TO_BORDER;
	t_desc.mag_filter = GS_GRAPHICS_TEXTURE_FILTER_LINEAR;
	t_desc.min_filter = GS_GRAPHICS_TEXTURE_FILTER_LINEAR;
	t_desc.format = GS_GRAPHICS_TEXTURE_FORMAT_RGBA16F;
	t_desc.num_mips = 0;
	t_desc.data = NULL;
	t_desc.width = (s32)(ws.x / 8);
	t_desc.height = (s32)(ws.y / 8);

	// Two render targets for double buffered separable blur (For now, just set to window's viewport)
	//gs_graphics_texture_request_update(cb, bp->data.rt, &t_desc);
	// gfx->update_texture_data(bp->data.render_target, t_desc);

    // Render pass action for clearing the screen
    gs_graphics_clear_desc_t clear = (gs_graphics_clear_desc_t){
        .actions = &(gs_graphics_clear_action_t){.color = {0.0f, 0.0f, 0.0f, 1.f}}
    };

    // Uniform buffer array
    gs_graphics_bind_uniform_desc_t uniforms[] = {
        (gs_graphics_bind_uniform_desc_t){.uniform = bp->data.u_input_tex, .data = &params->input_texture, .binding = 0}
    };

    // Binding descriptor for vertex buffer
    gs_graphics_bind_desc_t binds = {
        .vertex_buffers = {.desc = &(gs_graphics_bind_vertex_buffer_desc_t){.buffer = bp->data.vbo}},
        .index_buffers = {.desc = &(gs_graphics_bind_index_buffer_desc_t){.buffer = bp->data.ibo}},
        .uniforms = {.desc = uniforms, .size = sizeof(uniforms)}
    };

	// Bind render pass, bind pipeline, set up everything...
	gs_graphics_begin_render_pass(cb, bp->data.rp);
		gs_graphics_bind_pipeline(cb, bp->data.pip);
		gs_graphics_set_viewport(cb, 0, 0, (u32)(ws.x / 8), (u32)(ws.y / 8));
		gs_graphics_clear(cb, &clear);
		gs_graphics_apply_bindings(cb, &binds);
		gs_graphics_draw(cb, &(gs_graphics_draw_desc_t){.start = 0, .count = 6});
	gs_graphics_end_render_pass(cb);

	// Set frame buffer attachment for rendering
	// gfx->set_frame_buffer_attachment(cb, bp->data.render_target, 0);

	// Set viewport 
	// gfx->set_view_port(cb, (u32)(ws.x / 8), (u32)(ws.y / 8));	

	// Clear
	// f32 cc[4] = {0.f, 0.f, 0.f, 1.f};
	// gfx->set_view_clear(cb, (f32*)&cc);

	// Use the program
	// gfx->bind_shader(cb, bp->data.shader);
	// {
	// 	gfx->bind_texture(cb, bp->data.u_input_tex, params->input_texture, 0);
	// 	gfx->bind_vertex_buffer(cb, bp->data.vb);
	// 	gfx->bind_index_buffer(cb, bp->data.ib);
	// 	gfx->draw_indexed(cb, 6);
	// }
}




#include "render_pass/composite_pass.h"

// typedef struct composite_pass_data_t
// {
// 	gs_resource(gs_shader) shader;
// 	gs_resource (gs_uniform) u_input_tex;
// 	gs_resource(gs_texture) render_target;
// 	gs_resource(gs_vertex_buffer) vb;
// 	gs_resource(gs_index_buffer) ib;
//} composite_pass_data_t;

// typedef struct composite_pass_t
// {
// 	_base(render_pass_i);
// 	composite_pass_data_t data;
//} composite_pass_t;

// // Use this to pass in parameters for the pass (will check for this)
// typedef struct composite_pass_parameters_t 
// {
// 	gs_resource(gs_texture) input_texture;
//} composite_pass_parameters_t;

// Forward Decl.
void cp_pass(gs_command_buffer_t* cb, struct render_pass_i* pass, void* paramters);

// Shaders
#if (defined GS_PLATFORM_WEB || defined GS_PLATFORM_ANDROID)
    #define GL_VERSION_STR "#version 300 es\n"
#else
    #define GL_VERSION_STR "#version 330 core\n"
#endif

const char* cp_v_src =
GL_VERSION_STR
"precision mediump float;\n"
"layout (location = 0) in vec2 a_position;\n"
"layout (location = 1) in vec2 a_uv;\n"
"out vec2 tex_coord;\n"
"void main() {\n"
"	gl_Position = vec4(a_position, 0.0, 1.0);\n"
"	tex_coord = a_uv;\n"
"}\n";

const char* cp_f_src =
GL_VERSION_STR
"precision mediump float;\n"
"in vec2 tex_coord;\n"
"out vec4 frag_color;\n"
"uniform sampler2D u_tex;\n"
"uniform sampler2D u_blur_tex;\n"
"uniform float u_exposure;\n"
"uniform float u_gamma;\n"
"uniform float u_bloom_scalar;\n"
"uniform float u_saturation;\n"
"float A = 0.15;\n"
"float B = 0.50;\n"
"float C = 0.10;\n"
"float D = 0.20;\n"
"float E = 0.02;\n"
"float F = 0.30;\n"
"float W = 11.20;\n"
"vec3 uncharted_tone_map(vec3 x) {\n"
"	return ((x*(A*x+C*B)+D*E)/(x*(A*x+B)+D*F))-E/F;\n"
"}\n"
"void main() {\n"
"	vec3 hdr = max(vec3(0.0), texture(u_tex, tex_coord).rgb);\n"
"	vec3 bloom = texture(u_blur_tex, tex_coord).rgb;\n"
"	hdr += bloom * u_bloom_scalar;\n"
"	vec3 result = vec3(1.0) - exp(-hdr * u_exposure);\n"
"	result = pow(result, vec3(1.0 / u_gamma));\n"
"	float lum = result.r * 0.2 + result.g * 0.7 + result.b * 0.1;\n"
"	vec3 diff = result.rgb - vec3(lum);\n"
"	frag_color = vec4(vec3(diff) * u_saturation + lum, 1.0);\n"
"	frag_color = vec4(hdr, 1.0);\n"
"}\n";

// // Vertex data layout for our mesh (for this shader, it's a single float2 attribute for position)
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

composite_pass_t composite_pass_ctor(gs_handle(gs_graphics_framebuffer_t) fb)
{
	composite_pass_t bp = {0};

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
                {.type = GS_GRAPHICS_SHADER_STAGE_VERTEX, .source = cp_v_src},
                {.type = GS_GRAPHICS_SHADER_STAGE_FRAGMENT, .source = cp_f_src}
            }, 
            .size = 2 * sizeof(gs_graphics_shader_source_desc_t),
            .name = "bright_shader"
        }
    );

	// Construct shaders resources
	// p.data.vb = gfx->construct_vertex_buffer(layout, layout_count, cp_v_data, sizeof(cp_v_data));

	// p.data.ib = gfx->construct_index_buffer(cp_i_data, sizeof(cp_i_data));

	// p.data.shader = gfx->construct_shader(cp_v_src, cp_f_src);

	// Construct uniforms for shader
    bp.data.u_input_tex = gs_graphics_uniform_create (
        &(gs_graphics_uniform_desc_t) {
            .name = "u_tex",
            .layout = &(gs_graphics_uniform_layout_desc_t){.type = GS_GRAPHICS_UNIFORM_SAMPLER2D}
        }
    );

    bp.data.u_blur_tex = gs_graphics_uniform_create (
        &(gs_graphics_uniform_desc_t) {
            .name = "u_blur_tex",
            .layout = &(gs_graphics_uniform_layout_desc_t){.type = GS_GRAPHICS_UNIFORM_SAMPLER2D}
        }
    );

    bp.data.u_bloom_scalar = gs_graphics_uniform_create (
        &(gs_graphics_uniform_desc_t) {
            .name = "u_bloom_scalar",
            .layout = &(gs_graphics_uniform_layout_desc_t){.type = GS_GRAPHICS_UNIFORM_FLOAT}
        }
    );

    bp.data.u_exposure = gs_graphics_uniform_create (
        &(gs_graphics_uniform_desc_t) {
            .name = "u_exposure",
            .layout = &(gs_graphics_uniform_layout_desc_t){.type = GS_GRAPHICS_UNIFORM_FLOAT}
        }
    );

    bp.data.u_saturation = gs_graphics_uniform_create (
        &(gs_graphics_uniform_desc_t) {
            .name = "u_saturation",
            .layout = &(gs_graphics_uniform_layout_desc_t){.type = GS_GRAPHICS_UNIFORM_FLOAT}
        }
    );

    bp.data.u_gamma = gs_graphics_uniform_create (
        &(gs_graphics_uniform_desc_t) {
            .name = "u_gamma",
            .layout = &(gs_graphics_uniform_layout_desc_t){.type = GS_GRAPHICS_UNIFORM_FLOAT}
        }
    );

	// p.data.u_input_tex = gfx->construct_uniform(p.data.shader, "u_tex", gs_uniform_type_sampler2d);

	// p.data.u_blur_tex = gfx->construct_uniform(p.data.shader, "u_blur_tex", gs_uniform_type_sampler2d);

	// p.data.u_exposure = gfx->construct_uniform(p.data.shader, "u_exposure", gs_uniform_type_float);
	// p.data.u_gamma = gfx->construct_uniform(p.data.shader, "u_gamma", gs_uniform_type_float);
	// p.data.u_bloom_scalar = gfx->construct_uniform(p.data.shader, "u_bloom_scalar", gs_uniform_type_float);
	// p.data.u_saturation = gfx->construct_uniform(p.data.shader, "u_saturation", gs_uniform_type_float);

	bp._base.pass = &cp_pass;

	// Construct render target to render into
	gs_vec2 ws = gs_platform_window_sizev(gs_platform_main_window());

	// gs_texture_parameter_desc t_desc = gs_texture_parameter_desc_default();
	// t_desc.texture_wrap_s = gs_clamp_to_border;
	// t_desc.texture_wrap_t = gs_clamp_to_border;
	// t_desc.mag_filter = gs_linear;
	// t_desc.min_filter = gs_linear;
	// t_desc.texture_format = gs_texture_format_rgba16f;
	// t_desc.generate_mips = false;
	// t_desc.num_comps = 4;
	// t_desc.data = NULL;
	// t_desc.width = (s32)(ws.x);
	// t_desc.height = (s32)(ws.y);

	gs_graphics_texture_desc_t t_desc = gs_default_val();
	t_desc.wrap_s = GS_GRAPHICS_TEXTURE_WRAP_CLAMP_TO_EDGE;
	t_desc.wrap_t = GS_GRAPHICS_TEXTURE_WRAP_CLAMP_TO_EDGE;
	t_desc.mag_filter = GS_GRAPHICS_TEXTURE_FILTER_LINEAR;
	t_desc.min_filter = GS_GRAPHICS_TEXTURE_FILTER_LINEAR;
	t_desc.format = GS_GRAPHICS_TEXTURE_FORMAT_RGBA16F;
	t_desc.num_mips = 0;
	t_desc.data = NULL;
	t_desc.width = (s32)(ws.x);
	t_desc.height = (s32)(ws.y);
	bp.data.rt = gs_graphics_texture_create(&t_desc);

	// Two render targets for double buffered separable blur (For now, just set to window's viewport)
	// p.data.render_target = gfx->construct_texture(t_desc);

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

void cp_pass(gs_command_buffer_t* cb, struct render_pass_i* _pass, void* _params)
{
	composite_pass_t* p = (composite_pass_t*)_pass;
	if (!p) {
		return;
	} 

	// Can only use valid params
	composite_pass_parameters_t* params = (composite_pass_parameters_t*)_params;
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
	t_desc.width = (s32)(ws.x);
	t_desc.height = (s32)(ws.y);

	// Two render targets for double buffered separable blur (For now, just set to window's viewport)
	// gfx->update_texture_data(p->data.render_target, t_desc);
	//gs_graphics_texture_request_update(cb, p->data.rt, &t_desc);

    // Render pass action for clearing the screen
    gs_graphics_clear_desc_t clear = (gs_graphics_clear_desc_t){
        .actions = &(gs_graphics_clear_action_t){.color = {0.0f, 0.0f, 0.0f, 1.f}}
    };

	f32 saturation = 2.0f;
	f32 gamma = 2.2f;
	f32 exposure = 0.5f;
	f32 bloom_scalar = 1.0f;

    // Uniform buffer array
    gs_graphics_bind_uniform_desc_t uniforms[] = {
        (gs_graphics_bind_uniform_desc_t){.uniform = p->data.u_input_tex, .data = &params->input_texture, .binding = 0},
        (gs_graphics_bind_uniform_desc_t){.uniform = p->data.u_blur_tex, .data = &params->blur_texture, .binding = 1},
        (gs_graphics_bind_uniform_desc_t){.uniform = p->data.u_saturation, .data = &saturation},
        (gs_graphics_bind_uniform_desc_t){.uniform = p->data.u_gamma, .data = &gamma},
        (gs_graphics_bind_uniform_desc_t){.uniform = p->data.u_exposure, .data = &exposure},
        (gs_graphics_bind_uniform_desc_t){.uniform = p->data.u_bloom_scalar, .data = &bloom_scalar},
    };

    // Binding descriptor for vertex buffer
    gs_graphics_bind_desc_t binds = {
        .vertex_buffers = {.desc = &(gs_graphics_bind_vertex_buffer_desc_t){.buffer = p->data.vbo}},
        .index_buffers = {.desc = &(gs_graphics_bind_index_buffer_desc_t){.buffer = p->data.ibo}},
        .uniforms = {.desc = uniforms, .size = sizeof(uniforms)}
    };

	// Bind render pass, bind pipeline, set up everything...
	gs_graphics_begin_render_pass(cb, p->data.rp);
		gs_graphics_bind_pipeline(cb, p->data.pip);
		gs_graphics_set_viewport(cb, 0, 0, (u32)(ws.x), (u32)(ws.y));
		gs_graphics_clear(cb, &clear);
		gs_graphics_apply_bindings(cb, &binds);
		gs_graphics_draw(cb, &(gs_graphics_draw_desc_t){.start = 0, .count = 6});
	gs_graphics_end_render_pass(cb);

	// Set frame buffer attachment for rendering
	// gfx->set_frame_buffer_attachment(cb, p->data.render_target, 0);

	// // Set viewport 
	// gfx->set_view_port(cb, (u32)(ws.x), (u32)(ws.y));	

	// // Clear
	// f32 cc[4] = {0.f, 0.f, 0.f, 1.f};
	// gfx->set_view_clear(cb, (f32*)&cc);

	// // Use the program
	// gfx->bind_shader(cb, p->data.shader);
	// {
	// 	f32 saturation = 2.0f;
	// 	f32 gamma = 2.2f;
	// 	f32 exposure = 0.5f;
	// 	f32 bloom_scalar = 1.0f;

	// 	gfx->bind_texture(cb, p->data.u_input_tex, params->input_texture, 0);
	// 	gfx->bind_texture(cb, p->data.u_blur_tex, params->blur_texture, 1);
	// 	gfx->bind_uniform(cb, p->data.u_saturation, &saturation);
	// 	gfx->bind_uniform(cb, p->data.u_gamma, &gamma);
	// 	gfx->bind_uniform(cb, p->data.u_exposure, &exposure);
	// 	gfx->bind_uniform(cb, p->data.u_bloom_scalar, &bloom_scalar);
	// 	gfx->bind_vertex_buffer(cb, p->data.vb);
	// 	gfx->bind_index_buffer(cb, p->data.ib);
	// 	gfx->draw_indexed(cb, 6);
	// }
}

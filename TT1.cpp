#include "model.h"
#include "gl.h"

constexpr int width = 1000;
constexpr int height = 1000;

const vec3 light = vec3(1, 1, 3);
const vec3 eye = vec3(4, 1, 1);
const vec3 center = vec3(0, 0, 0);
const vec3 up = vec3(0, 1, 0);

extern mat<4, 4> modelView;
extern mat<4, 4> projection;
extern mat<4, 4> viewport;

struct Shader : IShader
{
	const Model& model;
	vec3 uniform_l;
	mat<2, 3> varying_uv;
	mat<3, 3> varying_nrm;
	mat<3, 3> view_tri;
	mat<4, 4> shadow_matrix;
	mat<4, 4> matrix_invert_transpose;
	std::vector<double> shadowbuffer;
	Shader(const Model& m, const mat<4, 4> shadowMatrix, const mat<4, 4> matrix_invert_transpose, const std::vector<double> shadowbuffer) : model(m), shadow_matrix(shadowMatrix), matrix_invert_transpose(matrix_invert_transpose), shadowbuffer(shadowbuffer){
		uniform_l = proj<3>(modelView * embed<4>(light, 0.)).normalize();
	}

	virtual void vertex(const int iface, const int nthvert, vec4& gl_Position) {
		varying_uv.set_col(nthvert, model.uv(iface, nthvert));
		
		varying_nrm.set_col(nthvert, proj<3>(modelView.invert_transpose() * embed<4>(model.normal(iface, nthvert))));
		gl_Position = modelView * embed<4>(model.vert(iface, nthvert));
		view_tri.set_col(nthvert, proj<3>(gl_Position));
		gl_Position = projection * gl_Position;
	}

	virtual bool fragment(const vec3 bar, const vec3 pts1[3], TGAColor& gl_FragColor) {
		vec3 bn = (varying_nrm * bar).normalize();
		vec2 uv = varying_uv * bar;
		mat<4, 4> mvp = viewport * projection * modelView;
		mat<4, 4> invert = mvp.invert();
		vec4 p = (shadow_matrix * invert * embed<4>((pts1[0] * bar.x + pts1[1] * bar.y + pts1[2] * bar.z)));
		p = p / p[3];
		mat<3, 3> AI = mat<3, 3>{ {view_tri.col(1) - view_tri.col(0), view_tri.col(2) - view_tri.col(0), bn} }.invert();
		vec3 i = AI * vec3(varying_uv[0][1] - varying_uv[0][0], varying_uv[0][2] - varying_uv[0][0], 0);
		vec3 j = AI * vec3(varying_uv[1][1] - varying_uv[1][0], varying_uv[1][2] - varying_uv[1][0], 0);
		mat<3, 3> B = mat<3, 3>{ {i.normalize(), j.normalize(), bn} }.transpose();

		vec3 n = (B * model.normal(uv)).normalize(); // transform the normal from the texture to the tangent space

		float diffuse = std::max(0., n * uniform_l);
		vec3 r = (n * ((uniform_l * n) * 2) - uniform_l).normalize();
		double spec = std::pow(std::max(-r.z, 0.), 1 + sample2D(model.specular(), uv)[0]);
		TGAColor color = sample2D(model.diffuse(), uv);

		//shadow
		int idx = (int)(p[0]) + (int)(p[1]) * width;
		std::cout << p << std::endl;
		int flag = 0;
		if (shadowbuffer[idx] > p[2])
		{
			flag = 1;
		}
		double shadow = .3 + .7 * flag; 
		//double shadow = 1.;
		
		for ( int i = 0; i < 3; ++i)
		{
			gl_FragColor[i] = std::min(255., 5 + color[i] * shadow * (diffuse + 0.6* spec) * 1.);
		}
		return false;

		
	}
};

struct ShadowShader : IShader
{
	const Model& model;
	ShadowShader(const Model& m) : model(m)
	{
		
	}

	virtual void vertex(const int iface, const int nthvert, vec4& gl_Position) {
		gl_Position = projection * modelView * embed<4>(model.vert(iface, nthvert));
	}

	virtual bool fragment(const vec3 bar, const vec3 pts1[3], TGAColor& gl_FragColor) {
		gl_FragColor = TGAColor(255, 255, 255);
		return false;
	}
};

int main()
{	
	//model
	Model m("obj/african_head/african_head.obj");

	//matrix
	lookat(light, center, up);
	Projection((light - center).norm());
	Viewport(width / 8, height / 8, width * 3 / 4, height * 3 / 4);
	
	mat<4, 4> light_mvp = viewport * projection * modelView;
	//shadow-pass
	TGAImage shadowouput(width, height, TGAImage::RGB);
	TGAImage shadowzBuffer(width, height, TGAImage::RGB);
	std::vector<double> shadowbuffer(width * height, std::numeric_limits<double>::max());
	ShadowShader shadowshader(m);
	for (int i = 0; i < m.nfaces(); i++)
	{
		vec4 clip_vert[3];
		for (int j = 0; j < 3; j++)
		{
			shadowshader.vertex(i, j, clip_vert[j]);
		}
		triangle(clip_vert, shadowshader, shadowouput, shadowzBuffer, shadowbuffer);
	}
	shadowzBuffer.write_tga_file("shadowzbuffer.tga");

	//renderer-pass
	TGAImage framebuffer(width, height, TGAImage::RGB);
	TGAImage framezbuffer(width, height, TGAImage::RGB);
	std::vector<double> zbuffer(width * height, std::numeric_limits<double>::max());
	
	lookat(eye, center, up);
	Projection((eye - center).norm());
	
	Shader shader(m, light_mvp, (viewport * projection * modelView).invert(), shadowbuffer);
	for (int i = 0; i < m.nfaces(); i++)
	{
		vec4 clip_vert[3];
		for (int j = 0; j < 3; j++)
		{
			shader.vertex(i, j, clip_vert[j]);
		}
		triangle(clip_vert, shader, framebuffer, framezbuffer, zbuffer);
	}
	framebuffer.write_tga_file("output.tga");
	framezbuffer.write_tga_file("zbuffer.tga");
	return 0;
}




#include "tgaimage.h"
#include "geometry.h"

struct  boundingBox;
void lookat(vec3 eye, vec3 center, vec3 up);
void Projection(float f);
void Viewport(int x, int y, int w, int h);

struct IShader {
	static TGAColor sample2D(const TGAImage& img, vec2& uvf) {
		return img.get(uvf[0] * img.width(), uvf[1] * img.height());
	}

	virtual bool fragment(const vec3 bar, const vec3 pts[3], TGAColor& color) = 0;
};

//void triangle(const vec4 clip_verts[3], IShader& shader, TGAImage& image, std::vector<double>& zbuffer);
void triangle(const vec4* verts, IShader& shader, TGAImage &image, TGAImage &bufferImage, std::vector<double>& zbuffer);
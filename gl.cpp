#include "gl.h"
#include <limits>

mat<4, 4> projection;
mat<4, 4> viewport;
mat<4, 4> modelView;

struct  boundingBox
{
	vec2 min;
	vec2 max;
};

boundingBox getBoundingbox(vec2* verts, TGAImage& image)
{
	boundingBox bbox;
	bbox.min = vec2(image.width() - 1., image.height() - 1.);
	bbox.max = vec2(0, 0);
	vec2 clamp = vec2(image.width() - 1., image.height() - 1.);
	for (int i = 0; i < 3; i++)
	{
		bbox.min.x = std::max(0., std::min(bbox.min.x, verts[i].x));
		bbox.min.y = std::max(0., std::min(bbox.min.y, verts[i].y));
		bbox.max.x = std::min(clamp.x, std::max(bbox.max.x, verts[i].x));
		bbox.max.y = std::min(clamp.y, std::max(bbox.max.y, verts[i].y));
	}
	return bbox;
}

vec3 baryCentric(const vec2 tri[3], const vec2 P) {
	mat<3, 3> ABC = { {embed<3>(tri[0]), embed<3>(tri[1]), embed<3>(tri[2])} };
	if (ABC.det() < 1e-3) return vec3(-1, 1, 1);
	return ABC.invert_transpose() * embed<3>(P);
}

void lookat(vec3 eye, vec3 center, vec3 up)
{
	vec3 z = (center - eye).normalize();
	vec3 x = cross(up, z).normalize();
	vec3 y = cross(z, x).normalize();
	mat<4, 4> M = mat<4, 4>::identity();
	mat<4, 4> V = mat<4, 4>::identity();
	for (int i = 0; i < 3; i++)
	{
		M[0][i] = x[i];
		M[1][i] = y[i];
		M[2][i] = z[i];
		V[i][3] = -eye[i];
	}
	modelView = M * V;
}

void Projection(float f)
{
	projection = mat<4, 4>::identity();
	projection[1][1] = -1;
	projection[3][2] = -1 / f;
	projection[3][3] = 0;
}

void Viewport(int x, int y, int w, int h)
{
	viewport = mat<4, 4>::identity();
	viewport[0][0] = w / 2;
	viewport[1][1] = h / 2;
	viewport[0][3] = w / 2. + x;
	viewport[1][3] = h / 2. + y;
}

void triangle(const vec4* verts, IShader &shader, TGAImage &image, TGAImage &bufferImage, std::vector<double>& zbuffer)
{
	vec4 pts[3] = { viewport * verts[0],    viewport * verts[1],    viewport * verts[2] };  // triangle screen coordinates before persp. division
	vec3 pts1[3] = { proj<3>(pts[0] / pts[0][3]), proj<3>(pts[1] / pts[1][3]), proj<3>(pts[2] / pts[2][3]) };
	vec2 pts2[3] = { proj<2>(pts[0] / pts[0][3]), proj<2>(pts[1] / pts[1][3]), proj<2>(pts[2] / pts[2][3]) };  // triangle screen 

	boundingBox bbox = getBoundingbox(pts2, image);
	for (int x = bbox.min.x; x < bbox.max.x; x++)
	{
		for (int y = bbox.min.y; y < bbox.max.y; y++)
		{
			vec3 bc_screen = baryCentric(pts2, vec2(x, y));
			vec3 bc_clip = vec3(bc_screen.x / pts[0][3], bc_screen.y / pts[1][3], bc_screen.z / pts[2][3]);
			bc_clip = bc_clip / (bc_clip.x + bc_clip.y + bc_clip.z);
			double p_depth = vec3(verts[0][2], verts[1][2], verts[2][2]) * bc_clip;
		
			if (bc_screen.x >= 0 && bc_screen.y >= 0 && bc_screen.z >= 0 && p_depth < zbuffer[x + y * image.width()]) {
				//std::cout << p_depth << std::endl;
				TGAColor color;
				if (shader.fragment(bc_clip, pts1, color)) continue;
				zbuffer[x + y * image.width()] = p_depth;
				image.set(x, y, color);
				float depthColor = (3 - p_depth) * 100;
				bufferImage.set(x, y, TGAColor(depthColor, depthColor, depthColor));
			};

		}
	}
}


#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <chrono>
#include <thread>

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include "atlas.h"
#include "misc.h"

void CalculateAccuTextureU32(uint32_t* pixelsu32,int width,int height,uint8_t* pixels) {
	for(int y=0;y!=height;y++) {
		for(int x=0;x!=width;x++) {
			pixelsu32[y*width+x]=(int)pixels[y*width+x];
		}
	}
	for(int x=0;x!=width-1;x++) {
		int x1=x+1;
		for(int y=0;y!=height;y++) {
			pixelsu32[y*width+x1+0]+=pixelsu32[y*width+x];
		}
	}
	for(int y=0;y!=height-1;y++) {
		int y1=y+1;
		for(int x=0;x!=width;x++) {
			pixelsu32[y1*width+x]+=pixelsu32[y*width+x];
		}
	}
}

const char* vertexShaderTex=R"(
	uniform mat4 ProjMtx;
	in vec2 in_position;
	in vec2 UV;
	in vec4 Color;
	out vec2 frag_UV;
	out vec4 frag_Color;
	void main() {
		frag_UV=UV;
		frag_Color=Color;
        gl_Position=ProjMtx*vec4(in_position.xy,0,1);
	}
)";
const char* fragmentShaderTex=R"(
	uniform sampler2D Texture;
    in vec2 frag_UV;
	in vec4 frag_Color;
	out vec4 FragColor;
	void main() {
		FragColor=frag_Color*texture(Texture,frag_UV);
	}
)";

const GLchar* vertexShaderAccu=R"(
	uniform mat4 ProjMtx;
	in vec2 in_position;
	in vec2 UV;
	in vec2 UVScale;
	in vec4 Color;
	in vec4 in_region;
	out vec2 fragPix;
	out vec2 fragPixScale;
	out vec4 textColor;
	flat out ivec4 frag_region;
	void main() {
		fragPix=UV;
		fragPixScale=UVScale;
		textColor=Color;
		vec4 region=in_region;
		frag_region=ivec4(region);
		gl_Position=ProjMtx*vec4(in_position.xy,0,1);
	}
)";

const GLchar* fragmentShaderAccu=R"(
	uniform usampler2D Texture;
	uniform int mode;
	in vec2 fragPix;
	in vec2 fragPixScale;
	in vec4 textColor;
	flat in ivec4 frag_region;
	layout (location=0,index=0) out vec4 vFragColor;
	layout (location=0,index=1) out vec4 vFragBlend;
	float readRectAccuCoverage(ivec2 pix0in,ivec2 pix1) {
		float cnt=(pix1.x-pix0in.x)*(pix1.y-pix0in.y);
		ivec2 pix0=pix0in;//-ivec2(1,1);

		//if(pix0.x<frag_region.x||pix0.y<frag_region.y)			//Debug padding
		//	return 1.0;
		//if(pix1.x>frag_region.z||pix0.y>frag_region.w)
		//	return 1.0;

		pix0=clamp(pix0,frag_region.xy,frag_region.zw);
		pix1=clamp(pix1,frag_region.xy,frag_region.zw);
		uint tl=texelFetch(Texture,pix0,0).r;
		uint bl=texelFetch(Texture,ivec2(pix0.x,pix1.y),0).r;
		uint br=texelFetch(Texture,pix1,0).r;
		uint tr=texelFetch(Texture,ivec2(pix1.x,pix0.y),0).r;
		uint usum=br-bl-tr+tl;
		float sum=usum;
		return sum/(cnt*255);
	}
	vec4 calculateRectAverageAccuSubPixel(vec2 pix0,vec2 pix1) {
		float subPixelWidth=(pix1.x-pix0.x)/3.0;

		vec2 pixTop0=vec2(pix0.x-subPixelWidth*1,pix0.y);
		vec2 pixTop1=vec2(pix0.x+subPixelWidth*0,pix0.y);
		vec2 pixTop2=vec2(pix0.x+subPixelWidth*1,pix0.y);
		vec2 pixTop3=vec2(pix0.x+subPixelWidth*2,pix0.y);
		vec2 pixTop4=vec2(pix0.x+subPixelWidth*3,pix0.y);

		vec2 pixBot1=vec2(pixTop1.x,pix1.y);
		vec2 pixBot2=vec2(pixTop2.x,pix1.y);
		vec2 pixBot3=vec2(pixTop3.x,pix1.y);
		vec2 pixBot4=vec2(pixTop4.x,pix1.y);
		vec2 pixBot5=vec2(pix0.x+subPixelWidth*4,pix1.y);

		float cov01=readRectAccuCoverage(ivec2(pixTop0),ivec2(pixBot1));
		float cov12=readRectAccuCoverage(ivec2(pixTop1),ivec2(pixBot2));
		float cov23=readRectAccuCoverage(ivec2(pixTop2),ivec2(pixBot3));
		float cov34=readRectAccuCoverage(ivec2(pixTop3),ivec2(pixBot4));
		float cov45=readRectAccuCoverage(ivec2(pixTop4),ivec2(pixBot5));

		float w0=0.2;		//Weight current subpixel
		float w1=0.6;		//Weight neighbour subpixels

		float r=cov01*w0+cov12*w1+cov23*w0;
		float g=cov12*w0+cov23*w1+cov34*w0;
		float b=cov23*w0+cov34*w1+cov45*w0;
		return vec4(r,g,b,1);
	}
	vec4 calculateRectAverageAccuSubPixelOpti(vec2 pix0,vec2 pix1) {
		float subPixelWidth=(pix1.x-pix0.x)/3.0;

		vec2 pixTop0=vec2(pix0.x-subPixelWidth*1,pix0.y);
		vec2 pixTop1=vec2(pix0.x+subPixelWidth*0,pix0.y);
		vec2 pixTop2=vec2(pix0.x+subPixelWidth*1,pix0.y);
		vec2 pixTop3=vec2(pix0.x+subPixelWidth*2,pix0.y);
		vec2 pixTop4=vec2(pix0.x+subPixelWidth*3,pix0.y);

		vec2 pixBot1=vec2(pixTop1.x,pix1.y);
		vec2 pixBot2=vec2(pixTop2.x,pix1.y);
		vec2 pixBot3=vec2(pixTop3.x,pix1.y);
		vec2 pixBot4=vec2(pixTop4.x,pix1.y);
		vec2 pixBot5=vec2(pix0.x+subPixelWidth*4,pix1.y);

		float area01=(pixBot1.x-pixTop0.x)*(pixBot1.y-pixTop0.y);
		float area12=(pixBot2.x-pixTop1.x)*(pixBot2.y-pixTop1.y);
		float area23=(pixBot3.x-pixTop2.x)*(pixBot3.y-pixTop2.y);
		float area34=(pixBot4.x-pixTop3.x)*(pixBot4.y-pixTop3.y);
		float area45=(pixBot5.x-pixTop4.x)*(pixBot5.y-pixTop4.y);

		vec2 pixTop0C=clamp(pixTop0,frag_region.xy,frag_region.zw);
		vec2 pixTop1C=clamp(pixTop1,frag_region.xy,frag_region.zw);
		vec2 pixTop2C=clamp(pixTop2,frag_region.xy,frag_region.zw);
		vec2 pixTop3C=clamp(pixTop3,frag_region.xy,frag_region.zw);
		vec2 pixTop4C=clamp(pixTop4,frag_region.xy,frag_region.zw);

		vec2 pixBot1C=clamp(pixBot1,frag_region.xy,frag_region.zw);
		vec2 pixBot2C=clamp(pixBot2,frag_region.xy,frag_region.zw);
		vec2 pixBot3C=clamp(pixBot3,frag_region.xy,frag_region.zw);
		vec2 pixBot4C=clamp(pixBot4,frag_region.xy,frag_region.zw);
		vec2 pixBot5C=clamp(pixBot5,frag_region.xy,frag_region.zw);

		uint txT0=texelFetch(Texture,ivec2(pixTop0C),0).r;
		uint txT1=texelFetch(Texture,ivec2(pixTop1C),0).r;
		uint txT2=texelFetch(Texture,ivec2(pixTop2C),0).r;
		uint txT3=texelFetch(Texture,ivec2(pixTop3C),0).r;
		uint txT4=texelFetch(Texture,ivec2(pixTop4C),0).r;

		uint txB1=texelFetch(Texture,ivec2(pixBot1C),0).r;
		uint txB2=texelFetch(Texture,ivec2(pixBot2C),0).r;
		uint txB3=texelFetch(Texture,ivec2(pixBot3C),0).r;
		uint txB4=texelFetch(Texture,ivec2(pixBot4C),0).r;
		uint txB5=texelFetch(Texture,ivec2(pixBot5C),0).r;

		uint txT0B1=texelFetch(Texture,ivec2(pixTop0C.x,pixBot1C.y),0).r;
		uint txB5T4=texelFetch(Texture,ivec2(pixBot5C.x,pixTop4C.y),0).r;

		float cov01=(txB1-txT0B1-txT1+txT0)/(area01*255);
		float cov12=(txB2-txB1-txT2+txT1)/(area12*255);
		float cov23=(txB3-txB2-txT3+txT2)/(area23*255);
		float cov34=(txB4-txB3-txT4+txT3)/(area34*255);
		float cov45=(txB5-txB4-txB5T4+txT4)/(area45*255);

		float w0=0.2;		//Weight current subpixel
		float w1=0.6;		//Weight neighbour subpixels

		float r=cov01*w0+cov12*w1+cov23*w0;
		float g=cov12*w0+cov23*w1+cov34*w0;
		float b=cov23*w0+cov34*w1+cov45*w0;
		return vec4(r,g,b,1);
	}
	vec4 calculateRectAverageAccu(ivec2 pix0,ivec2 pix1) {
		float cov01=readRectAccuCoverage(pix0,pix1);
		return vec4(cov01,cov01,cov01,1);
	}
	void main() {																			// texelFetch uin32_t does not support bilinear filtering so coverage will be including one texel too much
		vec4 aveColor;
		if(mode==0) {
			ivec2 ipix0=ivec2(fragPix);
			ivec2 ipix1=ivec2(fragPix+fragPixScale);
			aveColor=calculateRectAverageAccu(ipix0,ipix1);
		}else{
			aveColor=calculateRectAverageAccuSubPixelOpti(fragPix,fragPix+fragPixScale);	// 12 texelFetch unrolled
			//aveColor=calculateRectAverageAccuSubPixel(fragPix,fragPix+fragPixScale);		// 20 texelFetch 5*4 subpixel corners
		}
		aveColor=pow(aveColor,vec4(1.0/1.2));		//Gamma
		vFragColor=textColor;
		vFragBlend=textColor.a*aveColor;
	}
)";

struct Vertex {
	V2 pos;
	V2 uv;
	V2 uvscale;
	V4 region;
	uint32_t col;
};

void AddQuadVertices(std::vector<Vertex>* vertices,std::vector<uint16_t>* indices,const V2& xy0,const V2& xy1,const V2& st0,const V2& st1,const V2& uvScale,const V4& region,uint32_t col=0xffffffff) {
	int ix=(int)indices->size();
	indices->resize(indices->size()+6);
	uint16_t* p=indices->data()+ix;
	ix=(int)vertices->size();
	p[0]=ix+0;
	p[1]=ix+1;
	p[2]=ix+2;
	p[3]=ix+2;
	p[4]=ix+3;
	p[5]=ix+0;
	vertices->resize(vertices->size()+4);
	Vertex* v=&vertices->at(vertices->size()-4);
	v->pos=xy0;
	v->uv=st0;
	v->uvscale=uvScale;
	v->region=region;
	v->col=col;
	v=&vertices->at(vertices->size()-3);
	v->pos=V2(xy1.x,xy0.y);
	v->uv=V2(st1.x,st0.y);
	v->uvscale=uvScale;
	v->region=region;
	v->col=col;
	v=&vertices->at(vertices->size()-2);
	v->pos=xy1;
	v->uv=st1;
	v->uvscale=uvScale;
	v->region=region;
	v->col=col;
	v=&vertices->at(vertices->size()-1);
	v->pos=V2(xy0.x,xy1.y);
	v->uv=V2(st0.x,st1.y);
	v->uvscale=uvScale;
	v->region=region;
	v->col=col;
}

struct Program {
	GLuint m_shaderHandle;
	GLint m_attribTex;
	GLint m_attribProjMtx;
	GLint m_attribMode;
	GLuint m_attribVtxPos;
	GLuint m_attribVtxUV;
	GLuint m_attribVtxUVScale;
	GLuint m_attribVtxColor;
	GLuint m_attribRegion;
};

V2 g_mouse={0,0};
V2 g_mousePressStart={0,0};
bool g_mouseButtons[2]={false,false};
V2 g_atlasPosStart={0,0};
V2 g_atlasPos={0,0};
bool g_subpixel=false;

unsigned int CompileShader(const char* vertexShaderGLSL,const char* fragmentShaderGLSL) {
    const GLchar* vertex_shader_with_version[2]={"#version 330\n",vertexShaderGLSL};
	unsigned int vertexShader=glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertexShader,2,vertex_shader_with_version,NULL);
	glCompileShader(vertexShader);
	int success;
	char infoLog[512];
	glGetShaderiv(vertexShader,GL_COMPILE_STATUS,&success);
	if(!success) {
		glGetShaderInfoLog(vertexShader,512,NULL,infoLog);
		uprintf("ERROR: Vertex shader compilation failed\n%s\n",infoLog);
		exit(0);
	}
    const GLchar* fragment_shader_with_version[2]={"#version 330\n",fragmentShaderGLSL};
	unsigned int fragmentShader=glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragmentShader,2,fragment_shader_with_version,NULL);
	glCompileShader(fragmentShader);
	glGetShaderiv(fragmentShader,GL_COMPILE_STATUS,&success);
	if(!success) {
		glGetShaderInfoLog(fragmentShader,512,NULL,infoLog);
		uprintf("ERROR: Fragment shader compilation failed\n%s\n",infoLog);
		exit(0);
	}
	unsigned int shaderProgram=glCreateProgram();
	glAttachShader(shaderProgram,vertexShader);
	glAttachShader(shaderProgram,fragmentShader);
	glLinkProgram(shaderProgram);
	glGetProgramiv(shaderProgram,GL_LINK_STATUS,&success);
	if (!success) {
		glGetProgramInfoLog(shaderProgram,512,NULL,infoLog);
		uprintf("ERROR: Program compilation failed\n%s\n",infoLog);
		exit(0);
	}
	glDeleteShader(vertexShader);
	glDeleteShader(fragmentShader);
	return shaderProgram;
}

void CreateProgram(Program* program,const char* vertexShaderGLSL,const char* fragmentShaderGLSL) {
	int shaderHandle=CompileShader(vertexShaderGLSL,fragmentShaderGLSL);
	program->m_shaderHandle=shaderHandle;
	program->m_attribProjMtx=glGetUniformLocation(shaderHandle,"ProjMtx");
	program->m_attribMode=glGetUniformLocation(shaderHandle,"mode");
	program->m_attribTex=glGetUniformLocation(shaderHandle,"Texture");
	program->m_attribVtxPos=(GLuint)glGetAttribLocation(shaderHandle,"in_position");
	program->m_attribVtxUV=(GLuint)glGetAttribLocation(shaderHandle,"UV");
	program->m_attribVtxUVScale=(GLuint)glGetAttribLocation(shaderHandle,"UVScale");
	program->m_attribVtxColor=(GLuint)glGetAttribLocation(shaderHandle,"Color");
	program->m_attribRegion=(GLuint)glGetAttribLocation(shaderHandle,"in_region");
}

void RunProgram(const Program& program,const std::vector<Vertex>& vertices,const std::vector<uint16_t>& indices,const float* projection,GLuint texture) {
	if(!indices.size())
		return;
	glUseProgram(program.m_shaderHandle);
	glBufferData(GL_ARRAY_BUFFER,sizeof(Vertex)*(int)vertices.size(),&vertices[0],GL_STATIC_DRAW);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER,sizeof(indices[0])*(int)indices.size(),&indices[0],GL_STATIC_DRAW);
	glEnableVertexAttribArray(program.m_attribVtxPos);
	glEnableVertexAttribArray(program.m_attribVtxUV);
	glEnableVertexAttribArray(program.m_attribVtxUVScale);
	glEnableVertexAttribArray(program.m_attribRegion);
	glEnableVertexAttribArray(program.m_attribVtxColor);
	glVertexAttribPointer(program.m_attribVtxUV,2,GL_FLOAT,GL_FALSE,sizeof(Vertex),(GLvoid*)offsetof(Vertex,uv));
	glVertexAttribPointer(program.m_attribVtxUVScale,2,GL_FLOAT,GL_FALSE,sizeof(Vertex),(GLvoid*)offsetof(Vertex,uvscale));
	glVertexAttribPointer(program.m_attribVtxPos,2,GL_FLOAT,GL_FALSE,sizeof(Vertex),(GLvoid*)offsetof(Vertex,pos));
	glVertexAttribPointer(program.m_attribRegion,4,GL_FLOAT,GL_FALSE,sizeof(Vertex),(GLvoid*)offsetof(Vertex,region));
	glVertexAttribPointer(program.m_attribVtxColor,4,GL_UNSIGNED_BYTE,GL_TRUE,sizeof(Vertex),(GLvoid*)offsetof(Vertex,col));
	glUniformMatrix4fv(program.m_attribProjMtx,1,GL_FALSE,projection);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D,texture);
	glUniform1i(program.m_attribTex,0);
	glUniform1i(program.m_attribMode,g_subpixel?1:0);
	glDrawElements(GL_TRIANGLES,(int)indices.size(),GL_UNSIGNED_SHORT,0);
	glDisableVertexAttribArray(program.m_attribVtxPos);
	glDisableVertexAttribArray(program.m_attribVtxUV);
	glDisableVertexAttribArray(program.m_attribVtxUVScale);
	glDisableVertexAttribArray(program.m_attribRegion);
	glDisableVertexAttribArray(program.m_attribVtxColor);
}

void SetPixel(uint8_t* pixels,int width,int height,int x,int y,uint32_t col) {
	assert(x>=0&&x<width&&y>=0&&y<height);
	pixels[(y*width+x)*4+0]=(col>>24)&0xff;
	pixels[(y*width+x)*4+1]=(col>>16)&0xff;
	pixels[(y*width+x)*4+2]=(col>>8)&0xff;
	pixels[(y*width+x)*4+3]=col&0xff;
}
void AddPixel(uint8_t* pixels,int width,int height,int x,int y,uint32_t col) {
	assert(x>=0&&x<width&&y>=0&&y<height);
	uint32_t r=(uint32_t)pixels[(y*width+x)*4+0]+((col>>24)&0xff);
	uint32_t g=(uint32_t)pixels[(y*width+x)*4+1]+((col>>16)&0xff);
	uint32_t b=(uint32_t)pixels[(y*width+x)*4+2]+((col>>8)&0xff);
	uint32_t a=(uint32_t)pixels[(y*width+x)*4+3]+(col&0xff);
	pixels[(y*width+x)*4+0]=r>0xff ? 0xff:(uint8_t)r;
	pixels[(y*width+x)*4+1]=g>0xff ? 0xff:(uint8_t)g;
	pixels[(y*width+x)*4+2]=b>0xff ? 0xff:(uint8_t)b;
	pixels[(y*width+x)*4+3]=a>0xff ? 0xff:(uint8_t)a;
}

void AddRect(uint8_t* pixels,int width,int height,const VI2& p0,const VI2& p1,uint32_t col) {
	for(int x=p0.x;x<=p1.x;x++) {
		AddPixel(pixels,width,height,x,p0.y,col);
		AddPixel(pixels,width,height,x,p1.y,col);
	}
	for(int y=p0.y+1;y<p1.y;y++) {
		AddPixel(pixels,width,height,p0.x,y,col);
		AddPixel(pixels,width,height,p1.x,y,col);
	}
}
void AddRectFilled(uint8_t* pixels,int width,int height,const VI2& p0,const VI2& p1,uint32_t col) {
	for(int y=p0.y+1;y<p1.y;y++) {
		for(int x=p0.x;x<=p1.x;x++) {
			AddPixel(pixels,width,height,x,y,col);
			AddPixel(pixels,width,height,x,y,col);
		}
	}
}

void DrawTextLine(std::vector<Vertex>* vertices,std::vector<uint16_t>* indices,const char* string,const V2& pos,float sizePixels,const Atlas& atlas,uint32_t col) {
	float scale=sizePixels/atlas.m_sizePixels;
	const char* p=string;
	float linePosX=0;
	while(*p) {
		Glyph glyph;
		if(!atlas.FindGlyph(&glyph,*p)) {
			uprintf("ERROR: Glyph not found\n");
			continue;
		}
		if(!glyph.m_visible) {			//No visible character, could be space
			linePosX+=glyph.m_advanceX*scale;
			p++;
			continue;
		}
		V2 xy0=glyph.xy0;
		V2 xy1=glyph.xy1;
		V2 tex0=V2((float)glyph.tex0.x,(float)glyph.tex0.y);
		V2 tex1=V2((float)glyph.tex1.x,(float)glyph.tex1.y);
		xy0=xy0*scale;
		xy1=xy1*scale;
		V2 cell=(tex1-tex0)/(xy1-xy0);
#if 1
		//Center to get sharp edges around baseline
		V2 xypivot=V2(glyph.xy1.x,lerp(glyph.xy0.y,glyph.xy1.y,glyph.m_baseline));
		V2 texpivot=V2(tex1.x,lerp(tex0.y,tex1.y,glyph.m_baseline));
		xypivot=xypivot*scale;
		//Center pixel
		float frag=fmodf(xypivot.y,1.0f)-0.49f;
		xy0.y-=frag;
		xy1.y-=frag;
		xypivot.y-=frag;
		//Snap to baseline
		V2 sz=V2(ceilf(xypivot.x-xy0.x),ceilf(xypivot.y-xy0.y));
		xy0=xypivot-sz;
		tex0=texpivot-sz*cell;
		//Extend to full size
		sz=V2(ceilf(xy1.x-xy0.x),ceilf(xy1.y-xy0.y));
		xy1=xy0+sz;
		tex1=tex0+sz*cell;
#endif
		//Extend with one texture cell to get edge anti-aliasing correct
		xy0=xy0-V2(1,1)*1;
		xy1=xy1+V2(1,1)*1;
		tex0=tex0-cell*1;
		tex1=tex1+cell*1;

		V2 p0=pos+xy0+V2(linePosX,0);
		V2 p1=pos+xy1+V2(linePosX,0);

		V4 region=V4(glyph.tex0.x-1.0f,glyph.tex0.y-1.0f,glyph.tex1.x+1.0f,glyph.tex1.y+1.0f);
		AddQuadVertices(vertices,indices,p0,p1,tex0,tex1,cell,region,col);

		linePosX+=glyph.m_advanceX*scale;
		p++;
	}
}

int main() {

	Atlas atlas;
	if(!CreateFontAtlas(&atlas))
		return -1;
	uprintf("Atlas created %d,%d glyphs %d\n",atlas.m_width,atlas.m_height,(int)atlas.m_glyphs.size());

	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR,3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR,3);
	glfwWindowHint(GLFW_OPENGL_PROFILE,GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT,GL_TRUE);
#endif
	GLFWwindow* window=glfwCreateWindow(1280,720,"Summed Area Font",NULL,NULL);
	if (window==NULL) {
		printf("Failed to create GLFW window\n");
		glfwTerminate(); 
		return -1;
	}
	glfwMakeContextCurrent(window);
	glfwSetCursorPosCallback(window,[](GLFWwindow* window,double x,double y) {
		g_mouse=V2((float)x,(float)y);
		if(g_mouseButtons[0]) {
			V2 d=g_mousePressStart-g_mouse;
			g_atlasPos=g_atlasPosStart+d;
		}
	});
	glfwSetKeyCallback(window,[](GLFWwindow* window, int key, int scancode, int action, int mods) {
		if(action==GLFW_PRESS && key==GLFW_KEY_SPACE)
			g_subpixel=!g_subpixel;
	});

	glfwSetMouseButtonCallback(window,[](GLFWwindow* window,int button,int action,int mods){
		if(button==GLFW_MOUSE_BUTTON_LEFT) {
			if(action==GLFW_PRESS) {
				g_atlasPosStart=g_atlasPos;
				g_mouseButtons[0]=true;
				g_mousePressStart=g_mouse;
			}
			if(action==GLFW_RELEASE) {
				g_mouseButtons[0]=false;
				g_mousePressStart={0,0};
				g_atlasPosStart={0,0};
			}
		}
	});
	glewInit();

	Program paccu;
	CreateProgram(&paccu,vertexShaderAccu,fragmentShaderAccu);
	Program ptex;
	CreateProgram(&ptex,vertexShaderTex,fragmentShaderTex);
	
	GLuint vertexArrayID;
	glGenVertexArrays(1,&vertexArrayID);
	glBindVertexArray(vertexArrayID);

	int texWidth=atlas.m_width;
	int texHeight=atlas.m_height;
	uint8_t* pixelsRGBA=new uint8_t[texWidth*texHeight*4];
	for(int y=0;y!=texHeight;y++) {
		for(int x=0;x!=texWidth;x++) {
			char col=atlas.m_pixels[y*texWidth+x];
			pixelsRGBA[(y*texWidth+x)*4+0]=col;
			pixelsRGBA[(y*texWidth+x)*4+1]=col;
			pixelsRGBA[(y*texWidth+x)*4+2]=col;
			pixelsRGBA[(y*texWidth+x)*4+3]=col;
		}
	}

	for(auto g:atlas.m_glyphs) {
		if(!g.m_visible)
			continue;
		float texbaseline=lerp((float)g.tex0.y,(float)g.tex1.y,g.m_baseline)-1.0f;
		g.m_baseline=(texbaseline-g.tex0.y)/(g.tex1.y-g.tex0.y);
		for(int x=g.tex0.x;x<=g.tex1.x;x++) {
			float y=lerp((float)g.tex0.y,(float)g.tex1.y,g.m_baseline);
			AddPixel(pixelsRGBA,texWidth,texHeight,(int)x,(int)y,0x008000ff);
		}
	}

	GLuint textureId;
	glGenTextures(1,&textureId);
	glBindTexture(GL_TEXTURE_2D,textureId);

	float color[]={.2f,.2f,.2f,0};
	glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR,color);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);

	glPixelStorei(GL_UNPACK_ROW_LENGTH,0);
	glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA,texWidth,texHeight,0,GL_RGBA,GL_UNSIGNED_BYTE,pixelsRGBA);

	GLuint texU32Id;
	glGenTextures(1,&texU32Id);
	glBindTexture(GL_TEXTURE_2D,texU32Id);

	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);

	uint32_t* pixelsu32=new uint32_t[texWidth*texHeight*1];
	CalculateAccuTextureU32(pixelsu32,texWidth,texHeight,atlas.m_pixels);
	glTexImage2D(GL_TEXTURE_2D,0,GL_R32UI,texWidth,texHeight,0,GL_RED_INTEGER,GL_UNSIGNED_INT,pixelsu32);
	delete [] pixelsu32;

	delete [] pixelsRGBA;

	GLuint vertexBuffer;
	GLuint indexBuffer;
	glGenBuffers(1,&vertexBuffer);
	glGenBuffers(1,&indexBuffer);

	while(!glfwWindowShouldClose(window)) {
		auto time=std::chrono::system_clock::now();
		if(glfwGetKey(window,GLFW_KEY_ESCAPE)==GLFW_PRESS)
			glfwSetWindowShouldClose(window,true);

		int windowWidth,windowHeight;
		glfwGetWindowSize(window,&windowWidth,&windowHeight);

		float atlasSplit=(float)windowHeight/2;
		glViewport(0,0,windowWidth,windowHeight);
		glClearColor(.9,.9,.9,1);
		glClear(GL_COLOR_BUFFER_BIT);

		float L=0;
		float R=(float)windowWidth;
		float T=0;
		float B=(float)windowHeight;
		const float projection[4][4]={
			{2.0f/(R-L),0.0f,       0.0f,0.0f},
			{0.0f,      2.0f/(T-B), 0.0f,0.0f},
			{0.0f,      0.0f,      -1.0f,0.0f},
			{(R+L)/(L-R),(T+B)/(B-T),0.0f,1.0f},
		};
		std::vector<Vertex> vertices;
		std::vector<uint16_t> indices;

		glBindBuffer(GL_ARRAY_BUFFER,vertexBuffer);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,indexBuffer);

		float sizePixels=64.0f;
		const char* string="Subpixel RGB rendering is OFF. Press space to toggle";
		if(g_subpixel)
			string="Subpixel RGB rendering is ON. Press space to toggle";
		static float dt=0;
		dt+=0.01f;
		sizePixels=lerp(8,64,(sinf(dt)+1.0f)/2.0f);
		DrawTextLine(&vertices,&indices,string,V2(4,40),sizePixels,atlas,0xff803020);
		V2 pos=V2(8+dt,64);
		sizePixels=8;
		string="The quick brown fox jumps over the lazy dog. 1234567890";
		for(int i=0;i!=10;i++) {
			DrawTextLine(&vertices,&indices,string,pos,sizePixels,atlas,0xff000000);
			pos.y+=sizePixels+4;
			sizePixels+=4;
		}
		//Draw text
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC1_COLOR, GL_ONE_MINUS_SRC1_COLOR);
		RunProgram(paccu,vertices,indices,&projection[0][0],texU32Id);

		//Draw atlas
		glDisable(GL_BLEND);
		V2 st0=g_atlasPos;
		V2 st1=g_atlasPos+V2((float)windowWidth,(float)windowHeight-atlasSplit);
		vertices.clear();
		indices.clear();
		V2 tex((float)texWidth,(float)texHeight);
		AddQuadVertices(&vertices,&indices,V2(0,atlasSplit)+V2(0,0),V2((float)windowWidth,(float)windowHeight),st0/tex,st1/tex,V2(0,0),V4(0,0,0,0),0xffffffff);
		RunProgram(ptex,vertices,indices,&projection[0][0],textureId);

		//Swap drawbuffers
		std::this_thread::sleep_until(time+std::chrono::microseconds(1000000/120));		//limit frame rate
		glfwSwapBuffers(window);
		glfwPollEvents();
	}
	glDeleteBuffers(1,&indexBuffer);
	glDeleteBuffers(1,&vertexBuffer);
	glDeleteVertexArrays(1,&vertexArrayID);
	glDeleteProgram(paccu.m_shaderHandle);
	glDeleteProgram(ptex.m_shaderHandle);

	DestroyFontAtlas(&atlas);
	glfwDestroyWindow(window);
	glfwTerminate();
	return 0;
}


#ifdef _WIN32
#undef APIENTRY
#include <windows.h>
#include "debugapi.h"
#include <crtdbg.h>
int WINAPI WinMain(HINSTANCE hInstance,HINSTANCE hPrevInstance,PSTR lpCmdLine,INT nCmdShow) {
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
	//_CrtSetBreakAlloc(104);
	return main();
}
#endif

void uprintf(const char* format,...) {
    va_list v;
    va_start(v,format);
    int len=vsnprintf(NULL,0,format,v);
    va_end(v);
	char str[1024];
	if(len>=(int)sizeof(str))
		exit(0);
    va_start(v,format);
    vsprintf(str,format,v);
    va_end(v);
    str[len]=0;
#ifdef WIN32
	OutputDebugStringA(str);
#else
	printf("%s",str);
#endif
}

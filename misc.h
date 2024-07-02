#pragma once

#include <assert.h>

void uprintf(const char* format,...);

struct V2 {
	float x,y;
	V2(){x=y=0.0f;}
	V2(float _x,float _y){x=_x;y=_y;}
};
inline V2 operator+(const V2& a,const V2& b) {
	return V2(a.x+b.x,a.y+b.y);
}
inline V2 operator-(const V2& a,const V2& b) {
	return V2(a.x-b.x,a.y-b.y);
}
inline V2 operator/(const V2& a,const V2& b) {
	return V2(a.x/b.x,a.y/b.y);
}
inline V2 operator*(const V2& a,const V2& b) {
	return V2(a.x*b.x,a.y*b.y);
}
inline V2 operator*(const V2& a,float b) {
	return V2(a.x*b,a.y*b);
}
inline float lerp(float a,float b,float t){return a+(b-a)*t;}
inline V2 lerp(const V2& a,const V2& b,float t){return a+(b-a)*V2(t,t);}

struct V4 {
	float x,y,z,w;
	V4(){x=y=z=w=0.0f;}
	V4(float _x,float _y,float _z,float _w){x=_x;y=_y;z=_z;w=_w;}
};

struct VI2 {
	int x,y;
	VI2(){x=y=0;}
	VI2(int _x,int _y){x=_x;y=_y;}
};
struct VI4 {
	int x,y,z,w;
	VI4(){x=y=z=w=0;}
	VI4(int _x,int _y,int _z,int _w){x=_x;y=_y;z=_z;w=_w;}
};


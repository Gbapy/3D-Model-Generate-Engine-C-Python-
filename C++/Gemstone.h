#pragma once
#include "ImgCvt.h"

typedef struct _PRONG_
{
	double		size;
	double		rotation;
	VERTEX		pos;
	VERTEX		uve[3];
	vector<int>	nGems;
}PRONG, *PPRONG;

typedef struct _GEM_
{
	VERTEX		pos;
	VERTEX		uve[3];
	double		radius;
	double		holeSize;
	double		prongSize;
}GEM, *PGEM;

typedef struct _RING_
{
	VERTEX		pos;
	VERTEX		uve[3];
	double		radius;
}RING, *PRING;


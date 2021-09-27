#pragma once

#include <vector>
#include <stdint.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <limits.h>
#include <time.h>

using namespace std;

typedef unsigned long long int tick64_t;
typedef unsigned long int tick32_t;

#define		MARGIN	30
#define		PI		3.1415926535897932384626433832795
typedef		unsigned char	UCHAR;
typedef		uint32_t		UINT;
typedef		unsigned long	DWORD;


typedef struct _MESH_INFO_
{
	char	name[256];
	int		vertCount;
	int		meshCount;
}MESHINFO, *PMESHINFO;

typedef struct _GEM_SIZE_PATTERN_
{
	double	gemSize;
	double	prongSize;
	double	holeSize;
}GEM_SIZE_PATTERN, *PGEM_SIZE_PATTERN;

typedef struct _MATRIX_
{
	double val[4][4];
}MATRIX, *PMATRIX;

typedef struct _VECTOR2I_
{
	int val[2];

	_VECTOR2I_() {
		val[0] = 0; val[1] = 0;
	}

	_VECTOR2I_(int i, int j) {
		val[0] = i; val[1] = j;
	}
}Vec2i, *PVec2i;

typedef struct _VECTOR3D_
{
	_VECTOR3D_() {
	
	}

	_VECTOR3D_(double x, double y, double z) {
		val[0] = x; val[1] = y; val[2] = z;
	}
	double	val[3];

	double getMagnitude() {
		return sqrt(val[0] * val[0] + val[1] * val[1] + val[2] * val[2]);
	}

	void doNormalize() {
		double m = this->getMagnitude();
		if (m == 0) {
			val[0] = 0;
			val[1] = 0;
			val[2] = 0;
		}
		else {
			val[0] /= m;
			val[1] /= m;
			val[2] /= m;
		}
	}
}VECTOR3D, *PVECTOR3D;

typedef struct _VERTEX_
{
	_VERTEX_() {
		val[0] = 0; val[1] = 0; val[2] = 0; val[3] = 1;
		nor = VECTOR3D(0, 0, 0);
		ntriCnt = 0;
		optIndex = -1;
		tag = 0;
	}

	_VERTEX_(double x, double y, double z) {
		val[0] = x; val[1] = y; val[2] = z; val[3] = 1;
		nor = VECTOR3D(0, 0, 0);
		ntriCnt = 0;
		optIndex = -1;
		tag = 0;
	}
	double		val[4];
	VECTOR3D	nor;
	int			ntriCnt;
	int			optIndex;
	int			tag;
	void inverseDirection() {
		this->val[0] = -this->val[0];
		this->val[1] = -this->val[1];
		this->val[2] = -this->val[2];
	}

	double getMagnitude() {
		return sqrt(val[0] * val[0] + val[1] * val[1] + val[2] * val[2]);
	}

	void doNormalize() {
		double m = this->getMagnitude();
		if (m == 0) {
			val[0] = 0;
			val[1] = 0;
			val[2] = 0;
		}
		else {
			val[0] /= m;
			val[1] /= m;
			val[2] /= m;
		}
	}
}VERTEX, *PVERTEX;

VERTEX doMinus(VERTEX a, VERTEX b);
VECTOR3D doMinus(VECTOR3D a, VECTOR3D b);
VERTEX doAdd(VERTEX a, VERTEX b);
VERTEX doCross(VERTEX a, VERTEX b);
VECTOR3D doCross(VECTOR3D a, VECTOR3D b);
MATRIX getUVEMatrix(VERTEX *axies, VERTEX eyePos);
VERTEX rotateVertex(VERTEX v, MATRIX mat);
bool isInside(VERTEX *p, VERTEX a);

typedef struct _TRIANGLE_
{
	_TRIANGLE_() {
		idx[0] = -1; idx[1] = -1; idx[2] = -1;
	}

	_TRIANGLE_(int idx1, int idx2, int idx3) {
		idx[0] = idx1; idx[1] = idx2; idx[2] = idx3;
	}
	int		idx[3];
	double	param[4];

	void flip() {
		int tmp = idx[1]; idx[1] = idx[2]; idx[2] = tmp;
	}

	bool isInnerPoint(vector<VERTEX> *sb, VERTEX pt) {
		VERTEX axis[3];
		VERTEX p[3];

		p[0] = sb->at(idx[0]);
		p[1] = sb->at(idx[1]);
		p[2] = sb->at(idx[2]);
		VERTEX v1 = doMinus(p[1], p[0]);
		VERTEX v2 = doMinus(p[2], p[0]);
		axis[2] = doCross(v1, v2);
		axis[2].inverseDirection();
		axis[1] = v1;
		axis[0] = doCross(axis[2], axis[1]);
		axis[0].inverseDirection();

		MATRIX mat = getUVEMatrix(axis, pt);
		p[0] = rotateVertex(p[0], mat);
		if (p[0].val[2] <= 0) return false;
		p[1] = rotateVertex(p[1], mat);
		p[2] = rotateVertex(p[2], mat);
		p[0].val[2] = 0; p[1].val[2] = 0; p[2].val[2] = 0;
		return isInside(p, VERTEX(0, 0, 0));
	}
}TRIANGLE, *PTRIANGLE;

typedef struct _SUBNODE_
{
	vector<VERTEX>		vertex;
	vector<TRIANGLE>	index;
	char				name[256];

	bool isInsidePoint(VERTEX p) {
		for (UINT i = 0; i < index.size(); i++) {
			if (index[i].isInnerPoint(&(this->vertex), p) == false) return false;
		}
		return true;
	}

	void putName(const char *str) {
		memset(name, 0, 256);
		sprintf(name, "%s", str);
	}

	void putName(const char *str, int idx) {
		memset(name, 0, 256);
		sprintf(name, "%s%d", str, idx);
	}
}SUBNODE, *PSUBNODE;

enum FBXENUM
{
	SUCCESS_IMPORT,
	INVALID_FBX_FILE,
	UNSURPPORTED_FBX_VERSION,
	UNSURPPORTED_PROPERTY_TYPE,
	INVALID_VERTEX_PAIR,
	INVALID_POLYGON_PAIR,
	COMPRESSED_PROPERTY_FOUND
};

extern Vec2i	neighbor[8];
extern SUBNODE	gem;
extern SUBNODE	prong;
extern SUBNODE	lowProng;
extern SUBNODE	ring;
extern VERTEX	prongTool[4];

extern int RESIZED_WIDTH;
extern int RESIZED_HEIGHT;
extern int ROI;

extern double	*depth;
extern int		*indexMap1;
extern int		*indexMap2;
extern VERTEX	*vertMap;
extern unsigned char	*renderImg;

extern double	roi_Depth;
extern int		gridInv;
extern double	maxDepth;
extern double	minDepth;
extern double	x_Rotate;
extern double	y_Rotate;
extern double	z_Rotate;
extern double	zoomFactor;
extern double	normalFactor;

extern vector<SUBNODE>	tNodes;
extern vector<SUBNODE>	eNodes;

extern VECTOR3D tDirV;

void normalizeDepth();
void doProjection(VERTEX *a);

VERTEX doAverage(VERTEX a, VERTEX b);
void doProjection(VECTOR3D *a);

void OptimizeMesh(vector<SUBNODE> *model, bool optFlag, float optRate);
void OptimizeMesh(SUBNODE *node, bool optFlag, float optRate);
void GetBound(VERTEX *mx, VERTEX *mn, SUBNODE *node);
void GetBound(VERTEX *mx, VERTEX *mn, vector<SUBNODE> *nodes);
void Move2Center(SUBNODE *node);
void DoScale(SUBNODE *node, double scaleFactor);
void Move2Center(vector<SUBNODE> *nodes);
void DoScale(vector<SUBNODE> *nodes, double scaleFactor);
void cloneNodes(SUBNODE *src, SUBNODE *dst);
void cloneNodes(vector<SUBNODE> *src, vector<SUBNODE> *dst);
void Move2Position(SUBNODE *node, double x, double y, double z);
void Move2Position(vector<SUBNODE> *nodes, double x, double y, double z);
void doSetVertexNormal(vector<SUBNODE> *model);
void doSetVertexNormal(SUBNODE *model);
MATRIX xRotateMatrix(double alpha);
MATRIX yRotateMatrix(double alpha);
MATRIX zRotateMatrix(double alpha);
void doSubnodeTransform(PSUBNODE node, MATRIX mat);
void doTransform(vector<SUBNODE> *nodes, MATRIX mat);


void InitRenderBuffer(int width, int height);
void ReleaseRenderBuffer();

int ReadGem(char *path);
int ReadProng(char *path);
int generateCurvedModel(UCHAR *raw, int raw_width, int raw_height, PGEM_SIZE_PATTERN gemPat, int gemPatSize, 
	double thickness, double xTwist, double yTwist,	vector<SUBNODE> *nodes, double *info, int option);
int generateCircleRingModel(UCHAR *raw, int raw_width, int raw_height, PGEM_SIZE_PATTERN gemPat, int gemPatSize, 
	double thickness, double xRadius, double yRadius, vector<SUBNODE> *nodes, double *info, int option);
int generateEllipseRingModel(UCHAR *raw, int raw_width, int raw_height, PGEM_SIZE_PATTERN gemPat, int gemPatSize,
	double thickness, double xRadius1, double xRadius2,	double yRadiu1, double yRadius2, 
	vector<SUBNODE> *nodes, double *info, int option);

int exportGemModel(char *exportPath, vector<SUBNODE> *nodes);
void cvt2Gray(UCHAR *raw, int width, int height, UCHAR *src);
void generateBitmapImage(unsigned char* image, int height, int width, char* imageFileName);

int getGlbFileBuffer(vector<SUBNODE> *model, char *fileBuffer, int glbSize);
int getGlbBufferSize(vector<SUBNODE> *model);
void initProngTool();
void cutProng(SUBNODE *prg, VERTEX gemPos, double gemScale);

tick32_t get_tick_count();
void writeImage(UCHAR *src, int width, int height, char *path);
void writeMonoImage(UCHAR *src, int width, int height, char *path);
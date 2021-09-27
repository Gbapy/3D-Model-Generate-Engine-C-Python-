#include "ImgCvt.h"

MATRIX TMat;

VERTEX center;

vector<SUBNODE>	tNodes;
vector<SUBNODE>	eNodes;

int RESIZED_WIDTH = 300;
int RESIZED_HEIGHT = 300;
int ROI = 5;

double welding = 1e-1;
double maxDepth = 0;
double minDepth = 1e+10;
double x_Rotate = 0.52359877559829887307710723054658;
double y_Rotate = 0.52359877559829887307710723054658;
double z_Rotate = 0.52359877559829887307710723054658;
double normalFactor = 9.0f;
double zoomFactor = 1.1f;
double coverRadius = 0.0f;
double deltaZ = 0;

double		*depth = NULL;
int			*indexMap1 = NULL;
int			*indexMap2 = NULL;
VERTEX		*vertMap = NULL;
unsigned char	*renderImg = NULL;

double		roi_Depth = 1.0f;
int			gridInv = 0;

VECTOR3D dirV;
VECTOR3D tDirV;

void cloneNodes(SUBNODE *src, SUBNODE *dst) {
	dst->vertex.clear();
	dst->index.clear();
	memcpy(dst->name, src->name, 256);

	for (UINT j = 0; j < src->index.size(); j++) {
		dst->index.push_back(src->index[j]);
	}
	for (UINT j = 0; j < src->vertex.size(); j++) {
		dst->vertex.push_back(src->vertex[j]);
	}
}

void cloneNodes(vector<SUBNODE> *src, vector<SUBNODE> *dst) {
	for (UINT i = 0; i < src->size(); i++) {
		SUBNODE subNode;

		cloneNodes(&(src->at(i)), &subNode);
		dst->push_back(subNode);
	}
}

void normalizeDepth() {
	maxDepth = -1;
	minDepth = 1e+10;
	for (int i = 0; i < RESIZED_HEIGHT; i++) {
		for (int j = 0; j < RESIZED_WIDTH; j++) {
			if (depth[i * RESIZED_WIDTH + j] > 0) {
				if (depth[i * RESIZED_WIDTH + j] > maxDepth) maxDepth = depth[i * RESIZED_WIDTH + j];
				if (depth[i * RESIZED_WIDTH + j] < minDepth) minDepth = depth[i * RESIZED_WIDTH + j];
			}
		}
	}

	for (int i = 0; i < RESIZED_HEIGHT; i++) {
		for (int j = 0; j < RESIZED_WIDTH; j++) {
			if (depth[i * RESIZED_WIDTH + j] > 0) {
				depth[i * RESIZED_WIDTH + j] = 255 - (depth[i * RESIZED_WIDTH + j] - minDepth) * 255.0f / (maxDepth - minDepth);
			}
		}
	}
}

double doMagnitude(VECTOR3D v) {
	return sqrt(v.val[0] * v.val[0] + v.val[1] * v.val[1] + v.val[2] * v.val[2]);
}

double doMagnitude(VERTEX v) {
	return sqrt(v.val[0] * v.val[0] + v.val[1] * v.val[1] + v.val[2] * v.val[2]);
}

void GetBound(VERTEX *mx, VERTEX *mn, SUBNODE *node) {
	for (UINT i = 0; i < node->vertex.size(); i++) {
		VERTEX v = node->vertex[i];
		if (i == 0)
		{
			*mn = v;
			*mx = v;
		}
		else {
			if (mn->val[0] > v.val[0]) mn->val[0] = v.val[0];
			if (mn->val[1] > v.val[1]) mn->val[1] = v.val[1];
			if (mn->val[2] > v.val[2]) mn->val[2] = v.val[2];
			if (mx->val[0] < v.val[0]) mx->val[0] = v.val[0];
			if (mx->val[1] < v.val[1]) mx->val[1] = v.val[1];
			if (mx->val[2] < v.val[2]) mx->val[2] = v.val[2];
		}
	}
}

void GetBound(VERTEX *mx, VERTEX *mn, vector<SUBNODE> *nodes) {
	for (UINT n = 0; n < nodes->size(); n++) {
		for (UINT i = 0; i < nodes->at(n).vertex.size(); i++) {
			VERTEX v = nodes->at(n).vertex[i];
			if (n == 0 && i == 0)
			{
				*mn = v;
				*mx = v;
			}
			else{
				if (mn->val[0] > v.val[0]) mn->val[0] = v.val[0];
				if (mn->val[1] > v.val[1]) mn->val[1] = v.val[1];
				if (mn->val[2] > v.val[2]) mn->val[2] = v.val[2];
				if (mx->val[0] < v.val[0]) mx->val[0] = v.val[0];
				if (mx->val[1] < v.val[1]) mx->val[1] = v.val[1];
				if (mx->val[2] < v.val[2]) mx->val[2] = v.val[2];
			}
		}
	}
}

void OptimizeMesh(SUBNODE *node, bool optFlag, float optRate) {
	vector<SUBNODE>	nd;
	VERTEX mx, mn;
	VERTEX delta;

	GetBound(&mx, &mn, node);
	delta = doMinus(mx, mn);
	normalFactor = doMagnitude(delta) / 20.0f;

	welding = 0.0f; //normalFactor / optRate;
	if (!optFlag) return;
	SUBNODE sn;

	for (UINT i = 0; i < node->vertex.size(); i++) {
		VECTOR3D v1 = VECTOR3D(node->vertex[i].val[0], node->vertex[i].val[1], node->vertex[i].val[2]);
		node->vertex[i].optIndex = (int)sn.vertex.size();
		bool flag = false;
		for (UINT j = 0; j < sn.vertex.size(); j++) {
			VECTOR3D v2 = VECTOR3D(sn.vertex[j].val[0], sn.vertex[j].val[1], sn.vertex[j].val[2]);
			v2.val[0] -= v1.val[0];
			v2.val[1] -= v1.val[1];
			v2.val[2] -= v1.val[2];

			if (fabs(v2.val[0]) > welding || fabs(v2.val[1]) > welding || fabs(v2.val[2]) > welding) continue;
			node->vertex[i].optIndex = j;
			flag = true;
			break;
		}
		if (!flag) {
			sn.vertex.push_back(VERTEX(v1.val[0], v1.val[1], v1.val[2]));
		}
	}
	for (UINT i = 0; i < node->index.size(); i++) {
		TRIANGLE t = node->index[i];

		t.idx[0] = node->vertex[t.idx[0]].optIndex;
		t.idx[1] = node->vertex[t.idx[1]].optIndex;
		t.idx[2] = node->vertex[t.idx[2]].optIndex;

		if (t.idx[0] != t.idx[1] && t.idx[1] != t.idx[2] && t.idx[0] != t.idx[2])
		{
			sn.index.push_back(t);
		}
	}

	node->index.clear();
	node->vertex.clear();
	for (UINT i = 0; i < sn.vertex.size(); i++) {
		node->vertex.push_back(sn.vertex[i]);
	}
	for (UINT i = 0; i < sn.index.size(); i++) {
		node->index.push_back(sn.index[i]);
	}
}

void OptimizeMesh(vector<SUBNODE> *model, bool optFlag, float optRate) {
	vector<SUBNODE>	nd;
	VERTEX mx, mn;
	VERTEX delta;

	GetBound(&mx, &mn, model);
	delta = doMinus(mx, mn);
	normalFactor = doMagnitude(delta) / 20.0f;

	welding = 0.0f;// normalFactor / optRate;
	if (!optFlag) return;
	for (UINT n = 0; n < model->size(); n++) {
		SUBNODE sn;

		for (UINT i = 0; i < model->at(n).vertex.size(); i++) {
			VECTOR3D v1 = VECTOR3D(model->at(n).vertex[i].val[0], model->at(n).vertex[i].val[1], model->at(n).vertex[i].val[2]);
			model->at(n).vertex[i].optIndex = (int)sn.vertex.size();
			bool flag = false;
			for (UINT j = 0; j < sn.vertex.size(); j++) {
				VECTOR3D v2 = VECTOR3D(sn.vertex[j].val[0], sn.vertex[j].val[1], sn.vertex[j].val[2]);
				v2.val[0] -= v1.val[0];
				v2.val[1] -= v1.val[1];
				v2.val[2] -= v1.val[2];

				if (fabs(v2.val[0]) > welding || fabs(v2.val[1]) > welding || fabs(v2.val[2]) > welding) continue;
				model->at(n).vertex[i].optIndex = j;
				flag = true;
				break;
			}
			if (!flag) {
				sn.vertex.push_back(VERTEX(v1.val[0], v1.val[1], v1.val[2]));
			}
		}
		for (UINT i = 0; i < model->at(n).index.size(); i++) {
			TRIANGLE t = model->at(n).index[i];

			t.idx[0] = model->at(n).vertex[t.idx[0]].optIndex;
			t.idx[1] = model->at(n).vertex[t.idx[1]].optIndex;
			t.idx[2] = model->at(n).vertex[t.idx[2]].optIndex;

			if (t.idx[0] != t.idx[1] && t.idx[1] != t.idx[2] && t.idx[0] != t.idx[2]) 
			{
				sn.index.push_back(t);
			}
		}

		model->at(n).index.clear();
		model->at(n).vertex.clear();
		for (UINT i = 0; i < sn.vertex.size(); i++) {
			model->at(n).vertex.push_back(sn.vertex[i]);
		}
		for (UINT i = 0; i < sn.index.size(); i++) {
			model->at(n).index.push_back(sn.index[i]);
		}
	}
}

void InitRenderBuffer(int width, int height) {
	depth = (double *)malloc(height * width * sizeof(double));
	indexMap1 = (int *)malloc(height * width * sizeof(int));
	indexMap2 = (int *)malloc(height * width * sizeof(int));
	vertMap = (VERTEX *)malloc(height * width * sizeof(VERTEX));
	renderImg = (unsigned char *)malloc(height * width * 4);
}

void ReleaseRenderBuffer() {
	if (depth) free(depth);
	if (indexMap1) free(indexMap1); 
	if (indexMap2) free(indexMap2);
	if (vertMap) free(vertMap);
	if (renderImg) free(renderImg);
	depth = NULL; indexMap1 = NULL; indexMap2 = NULL; vertMap = NULL; renderImg = NULL;
}

VERTEX doMinus(VERTEX a, VERTEX b) {
	return VERTEX(a.val[0] - b.val[0], a.val[1] - b.val[1], a.val[2] - b.val[2]);
}

VERTEX doAdd(VERTEX a, VERTEX b) {
	return VERTEX(a.val[0] + b.val[0], a.val[1] + b.val[1], a.val[2] + b.val[2]);
}

VECTOR3D doMinus(VECTOR3D a, VECTOR3D b) {
	VECTOR3D r = VECTOR3D(a.val[0] - b.val[0], a.val[1] - b.val[1], a.val[2] - b.val[2]);

	return r;
}

VERTEX doAverage(VERTEX a, VERTEX b) {
	VERTEX r = VERTEX((a.val[0] + b.val[0]) / 2.0f, (a.val[1] + b.val[1]) / 2.0f, (a.val[2] + b.val[2]) / 2.0f);

	return r;
}

VERTEX doRotateVector(VERTEX cord, VERTEX v, double alpha) {
	double mag = cord.val[0] * v.val[0] + cord.val[1] * v.val[1] + cord.val[2] * v.val[2];
	VERTEX v0 = VERTEX(cord.val[0] * mag, cord.val[1] * mag, cord.val[2] * mag);
	VERTEX v1 = VERTEX(v.val[0] - v0.val[0], v.val[1] - v0.val[1], v.val[2] - v0.val[2]);

	VERTEX rV = doCross(cord, v1);
	rV.doNormalize();
	mag = doMagnitude(v1);
	VERTEX a = VERTEX(v1.val[0] * cos(alpha), v1.val[1] * cos(alpha), v1.val[2] * cos(alpha));
	VERTEX b = VERTEX(-rV.val[0] * mag * sin(alpha), -rV.val[1] * mag * sin(alpha), -rV.val[2] * mag * sin(alpha));
	a.val[0] += (b.val[0] + v0.val[0]); a.val[1] += (b.val[1] + v0.val[1]); a.val[2] += (b.val[2] + v0.val[2]);
	//doNormalize(&a);
	return a;
}

VERTEX doCross(VERTEX a, VERTEX b) {
	VERTEX r = VERTEX(a.val[1] * b.val[2] - a.val[2] * b.val[1], 
		a.val[2] * b.val[0] - a.val[0] * b.val[2],
		a.val[0] * b.val[1] - a.val[1] * b.val[0]);

	return r;
}

VECTOR3D doCross(VECTOR3D a, VECTOR3D b) {
	VECTOR3D r = VECTOR3D(a.val[1] * b.val[2] - a.val[2] * b.val[1],
		a.val[2] * b.val[0] - a.val[0] * b.val[2],
		a.val[0] * b.val[1] - a.val[1] * b.val[0]);

	return r;
}

VECTOR3D doCross(VECTOR3D *a, VERTEX *b) {
	VECTOR3D r = VECTOR3D(a->val[1] * b->val[2] - a->val[2] * b->val[1],
		a->val[2] * b->val[0] - a->val[0] * b->val[2],
		a->val[0] * b->val[1] - a->val[1] * b->val[0]);

	return r;
}

MATRIX xRotateMatrix(double alpha) {
	MATRIX mat;
	for (int i = 0; i < 4; i++) {
		for (int j = 0; j < 4; j++) {
			if (i == j) mat.val[i][j] = 1; else mat.val[i][j] = 0;
		}
	}
	mat.val[1][1] = cos(alpha); mat.val[1][2] = sin(alpha);
	mat.val[2][1] = -sin(alpha); mat.val[2][2] = cos(alpha);
	return mat;
}

MATRIX yRotateMatrix(double alpha) {
	MATRIX mat;

	for (int i = 0; i < 4; i++) {
		for (int j = 0; j < 4; j++) {
			if (i == j) mat.val[i][j] = 1; else mat.val[i][j] = 0;
		}
	}
	mat.val[0][0] = cos(alpha); mat.val[0][2] = sin(alpha);
	mat.val[2][0] = -sin(alpha); mat.val[2][2] = cos(alpha);
	return mat;
}

MATRIX zRotateMatrix(double alpha) {
	MATRIX mat;

	for (int i = 0; i < 4; i++) {
		for (int j = 0; j < 4; j++) {
			if (i == j) mat.val[i][j] = 1; else mat.val[i][j] = 0;
		}
	}
	mat.val[0][0] = cos(alpha); mat.val[0][1] = sin(alpha);
	mat.val[1][0] = -sin(alpha); mat.val[1][1] = cos(alpha);
	return mat;
}

MATRIX tMatrix(VECTOR3D t) {
	MATRIX mat;
	for (int i = 0; i < 4; i++) {
		for (int j = 0; j < 4; j++) {
			if (i == j) mat.val[i][j] = 1; else mat.val[i][j] = 0;
		}
	}
	mat.val[3][0] = t.val[0]; mat.val[3][1] = t.val[1]; mat.val[3][2] = t.val[2];
	return mat;
}

MATRIX sMatrix(VECTOR3D s) {
	MATRIX mat;
	for (int i = 0; i < 4; i++) {
		for (int j = 0; j < 4; j++) {
			if (i == j) mat.val[i][j] = 1; else mat.val[i][j] = 0;
		}
	}
	mat.val[0][0] = s.val[0]; mat.val[1][1] = s.val[1]; mat.val[2][2] = s.val[2];
	return mat;
}

MATRIX makeConcatination(MATRIX *a, MATRIX *b) {
	MATRIX ret;

	for (int i = 0; i < 4; i++) {
		for (int j = 0; j < 4; j++) {
			ret.val[i][j] = 0;
			for (int k = 0; k < 4; k++) {
				ret.val[i][j] += a->val[i][k] * b->val[k][j];
			}
		}
	}
	return ret;
}

void makeTransformMatrix(double rx, double ry, double rz) {
	TMat = xRotateMatrix(rx);
	MATRIX yMat = yRotateMatrix(ry);
	MATRIX zMat = yRotateMatrix(rz);
	TMat = makeConcatination(&TMat, &yMat);
	TMat = makeConcatination(&TMat, &zMat);
}

void doSubnodeTransform(PSUBNODE node, MATRIX mat) {
	for (UINT m = 0; m < node->vertex.size(); m++) {
		VERTEX v = node->vertex[m];
		VERTEX r;
		r.nor = v.nor;
		r.val[0] = 0; r.val[1] = 0; r.val[2] = 0; r.val[3] = 0;
		for (int i = 0; i < 4; i++) {
			for (int j = 0; j < 4; j++) {
				r.val[i] += v.val[j] * mat.val[j][i];
			}
		}
		node->vertex[m] = r;
	}
}

void doTransformCoordinate(MATRIX *mat) {
	tDirV.val[0] = 0; tDirV.val[1] = 0; tDirV.val[2] = 0;
	for (int i = 0; i < 3; i++) {
		for (int j = 0; j < 3; j++) {
			tDirV.val[i] += dirV.val[j] * mat->val[j][i];
		}
	}
	dirV = tDirV;
}

void doTransform(vector<SUBNODE> *nodes, MATRIX mat) {
	for (UINT n = 0; n < nodes->size(); n++) {
		doSubnodeTransform(&nodes->at(n), mat);
	}
}

VERTEX rotateVertex(VERTEX v, MATRIX mat) {
	VERTEX r = v;

	r.val[0] = 0; r.val[1] = 0; r.val[2] = 0; r.val[3] = 0;

	for (int i = 0; i < 4; i++) {
		for (int j = 0; j < 4; j++) {
			r.val[i] += v.val[j] * mat.val[j][i];
		}
	}
	return r;
}

MATRIX getUVEMatrix(VERTEX *axies, VERTEX eyePos) {
	MATRIX m;

	axies[0].doNormalize();
	axies[1].doNormalize();
	axies[2].doNormalize();
	m.val[0][0] = axies[1].val[0]; m.val[0][1] = axies[1].val[1]; m.val[0][2] = axies[1].val[2];
	m.val[0][3] = -(axies[1].val[0] * eyePos.val[0] + axies[1].val[1] * eyePos.val[1] + axies[1].val[2] * eyePos.val[2]);
	m.val[1][0] = axies[0].val[0]; m.val[1][1] = axies[0].val[1]; m.val[1][2] = axies[0].val[2];
	m.val[1][3] = -(axies[0].val[0] * eyePos.val[0] + axies[0].val[1] * eyePos.val[1] + axies[0].val[2] * eyePos.val[2]);
	m.val[2][0] = axies[2].val[0]; m.val[2][1] = axies[2].val[1]; m.val[2][2] = axies[2].val[2];
	m.val[2][3] = -(axies[2].val[0] * eyePos.val[0] + axies[2].val[1] * eyePos.val[1] + axies[2].val[2] * eyePos.val[2]);
	m.val[3][0] = 0; m.val[3][1] = 0; m.val[3][2] = 0; m.val[3][3] = 1;
	return m;
}

void Move2Position(SUBNODE *node, double x, double y, double z) {
	for (UINT j = 0; j < node->vertex.size(); j++) {
		VERTEX v = node->vertex[j];
		v.val[0] += x; v.val[1] += y; v.val[2] += z;
		node->vertex[j] = v;
	}
}

void Move2Position(vector<SUBNODE> *nodes, double x, double y, double z) {
	for (UINT i = 0; i < nodes->size(); i++) {
		for (UINT j = 0; j < nodes->at(i).vertex.size(); j++) {
			VERTEX v = nodes->at(i).vertex[j];
			v.val[0] += x; v.val[1] += y; v.val[2] += z;
			nodes->at(i).vertex[j] = v;
		}
	}
}

bool isValid(VERTEX *e, VERTEX *p) {
	double d = e->val[0] * p->val[0] + e->val[1] * p->val[1] + e->val[2] * p->val[2] + e->val[3];
	if (d == 0)
		return true;
	else
		return false;
}

void doSetVertexNormal(SUBNODE *model) {
	for (UINT m = 0; m < model->vertex.size(); m++) {
		model->vertex[m].nor = VECTOR3D(0, 0, 0);
		model->vertex[m].ntriCnt = 0;
	}
	for (UINT m = 0; m < model->index.size(); m++) {
		VERTEX p1 = model->vertex[model->index[m].idx[0]];
		VERTEX p2 = model->vertex[model->index[m].idx[1]];
		VERTEX p3 = model->vertex[model->index[m].idx[2]];
		VERTEX v1 = doMinus(p2, p1);
		VERTEX v2 = doMinus(p3, p1);
		VERTEX e = doCross(v1, v2);
		e.doNormalize();
		for (int p = 0; p < 3; p++) {
			model->vertex[model->index[m].idx[p]].nor.val[0] += e.val[0];
			model->vertex[model->index[m].idx[p]].nor.val[1] += e.val[1];
			model->vertex[model->index[m].idx[p]].nor.val[2] += e.val[2];
			model->vertex[model->index[m].idx[p]].ntriCnt++;
		}
	}
	for (UINT m = 0; m < model->vertex.size(); m++) {
		model->vertex[m].nor.val[0] /= (float)(model->vertex[m].ntriCnt);
		model->vertex[m].nor.val[1] /= (float)(model->vertex[m].ntriCnt);
		model->vertex[m].nor.val[2] /= (float)(model->vertex[m].ntriCnt);
		model->vertex[m].nor.doNormalize();
	}
}

void doSetVertexNormal(vector<SUBNODE> *model) {
	for (UINT n = 0; n < model->size(); n++) {
		for (UINT m = 0; m < model->at(n).vertex.size(); m++) {
			model->at(n).vertex[m].nor = VECTOR3D(0, 0, 0);
			model->at(n).vertex[m].ntriCnt = 0;
		}
	}
	for (UINT n = 0; n < model->size(); n++) {
		for (UINT m = 0; m < model->at(n).index.size(); m++) {
			VERTEX p1 = model->at(n).vertex[model->at(n).index[m].idx[0]];
			VERTEX p2 = model->at(n).vertex[model->at(n).index[m].idx[1]];
			VERTEX p3 = model->at(n).vertex[model->at(n).index[m].idx[2]];
			VERTEX v1 = doMinus(p2, p1);
			VERTEX v2 = doMinus(p3, p1);
			VERTEX e = doCross(v1, v2);
			e.doNormalize();
			for (int p = 0; p < 3; p++) {
				model->at(n).vertex[model->at(n).index[m].idx[p]].nor.val[0] += e.val[0];
				model->at(n).vertex[model->at(n).index[m].idx[p]].nor.val[1] += e.val[1];
				model->at(n).vertex[model->at(n).index[m].idx[p]].nor.val[2] += e.val[2];
				model->at(n).vertex[model->at(n).index[m].idx[p]].ntriCnt++;
			}
		}
	}
	for (UINT n = 0; n < model->size(); n++) {
		for (UINT m = 0; m < model->at(n).vertex.size(); m++) {
			model->at(n).vertex[m].nor.val[0] /= (float)(model->at(n).vertex[m].ntriCnt);
			model->at(n).vertex[m].nor.val[1] /= (float)(model->at(n).vertex[m].ntriCnt);
			model->at(n).vertex[m].nor.val[2] /= (float)(model->at(n).vertex[m].ntriCnt);
			model->at(n).vertex[m].nor.doNormalize();
		}
	}
}

void doProjection(VERTEX *a) {
	double c = sqrt(3);
	a->val[0] = (RESIZED_WIDTH / 2) + (a->val[0] * c / a->val[2]) * (RESIZED_WIDTH / 2);
	a->val[1] = (RESIZED_HEIGHT / 2) + (a->val[1] * c / a->val[2]) * (RESIZED_HEIGHT / 2);
	a->val[2] = 1;
}

void doProjection(VECTOR3D *a) {
	double c = sqrt(3);
	a->val[0] = (RESIZED_WIDTH / 2) + (a->val[0] * c / a->val[2]) * (RESIZED_WIDTH / 2);
	a->val[1] = (RESIZED_HEIGHT / 2) + (a->val[1] * c / a->val[2]) * (RESIZED_HEIGHT / 2);
}

VERTEX RotateVertex(VERTEX v, double rX, double rY, double rZ) {
	VERTEX vX = VERTEX(1, 0, 0);
	VERTEX vY = VERTEX(0, 1, 0);
	VERTEX vZ = VERTEX(0, 0, 1);

	v = doRotateVector(vX, v, rX);
	vY = doRotateVector(vX, vY, rX);
	vZ = doRotateVector(vX, vZ, rX);

	v = doRotateVector(vY, v, rY);
	vX = doRotateVector(vY, vX, rY);
	vZ = doRotateVector(vY, vZ, rY);

	v = doRotateVector(vZ, v, rZ);
	vX = doRotateVector(vZ, vX, rZ);
	vY = doRotateVector(vZ, vY, rZ);
	return v;
}

bool isInside(VERTEX *p, VERTEX a) {
	VERTEX pp[3];
	VERTEX v1 = doMinus(p[1], p[0]);
	VERTEX v2 = doMinus(a, p[0]);
	pp[0] = doCross(v1, v2);
	v1 = doMinus(p[2], p[1]);
	v2 = doMinus(a, p[1]);
	pp[1] = doCross(v1, v2);
	v1 = doMinus(p[0], p[2]);
	v2 = doMinus(a, p[2]);
	pp[2] = doCross(v1, v2);
	if ((pp[0].val[2] > 0 && pp[1].val[2] > 0 && pp[2].val[2] > 0) ||
		(pp[0].val[2] < 0 && pp[1].val[2] < 0 && pp[2].val[2] < 0)) 
		return true;
	return false; 
}

void InitBuffer() {
	minDepth = 1e+10;
	maxDepth = 0;

	for (int i = 0; i < RESIZED_HEIGHT; i++) {
		for (int j = 0; j < RESIZED_WIDTH; j++) {
			indexMap1[i * RESIZED_WIDTH + j] = -1;
			indexMap2[i * RESIZED_WIDTH + j] = -1;
		}
	}
}

void Move2Center() {
	VERTEX mn, mx;

	for (UINT n = 0; n < tNodes.size(); n++) {
		for (UINT m = 0; m < tNodes[n].vertex.size(); m++) {
			VERTEX v = tNodes[n].vertex[m];
			v.val[0] -= center.val[0]; v.val[1] -= center.val[1]; v.val[2] -= center.val[2];
			tNodes[n].vertex[m] = v;
		}
	}
}

void Move2Center(SUBNODE *node) {
	VERTEX mn, mx;
	VERTEX ct;

	GetBound(&mx, &mn, node);

	ct.val[0] = (mx.val[0] + mn.val[0]) / 2;
	ct.val[1] = (mx.val[1] + mn.val[1]) / 2;
	ct.val[2] = (mx.val[2] + mn.val[2]) / 2;


	for (UINT m = 0; m < node->vertex.size(); m++) {
		VERTEX v = node->vertex[m];
		v.val[0] -= ct.val[0]; v.val[1] -= ct.val[1]; v.val[2] -= ct.val[2];
		node->vertex[m] = v;
	}
}

void Move2Center(vector<SUBNODE> *nodes) {
	VERTEX mn, mx;
	VERTEX ct;

	GetBound(&mx, &mn, nodes);

	ct.val[0] = (mx.val[0] + mn.val[0]) / 2;
	ct.val[1] = (mx.val[1] + mn.val[1]) / 2;
	ct.val[2] = (mx.val[2] + mn.val[2]) / 2;


	for (UINT n = 0; n < nodes->size(); n++) {
		for (UINT m = 0; m < nodes->at(n).vertex.size(); m++) {
			VERTEX v = nodes->at(n).vertex[m];
			v.val[0] -= ct.val[0]; v.val[1] -= ct.val[1]; v.val[2] -= ct.val[2];
			nodes->at(n).vertex[m] = v;
		}
	}
}


void DoScale(SUBNODE *node, double scaleFactor) {
	MATRIX mat = sMatrix(VECTOR3D(scaleFactor, scaleFactor, scaleFactor));
	doSubnodeTransform(node, mat);
}

void DoScale(vector<SUBNODE> *nodes, double scaleFactor) {
	MATRIX mat = sMatrix(VECTOR3D(scaleFactor, scaleFactor, scaleFactor));
	doTransform(nodes, mat);
}

void Move2Orig() {
	for (UINT n = 0; n < tNodes.size(); n++) {
		for (UINT m = 0; m < tNodes[n].vertex.size(); m++) {
			VERTEX v = tNodes[n].vertex[m];
			v.val[0] += center.val[0]; v.val[1] += center.val[1]; v.val[2] += center.val[2];
			tNodes[n].vertex[m] = v;
		}
	}
}

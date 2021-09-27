#include "ImgCvt.h"

#define EP			1E-3
#define	M_INFINITE	1E+10
#define GEMHOLE_DEPTH	0.2f

typedef struct _TERMINAL_
{
	double x;
	double y;
	bool isValid = true;
	double nor[2];
	double uv[2];
	double maxOffset = 0;

	_TERMINAL_() {
		x = 0;
		y = 0;
		isValid = true;
	}

	_TERMINAL_(double ix, double iy) {
		x = ix; y = iy;
		isValid = true;
	}

	_TERMINAL_(int ix, int iy) {
		x = (double)ix; y = (double)iy;

		isValid = true;
	}

	bool isEqual(_TERMINAL_ p) {
		if (fabs(p.x - x) < EP && fabs(p.y - y) < EP) return true;
		return false;
	}

	void compareBoundary(_TERMINAL_ *mn, _TERMINAL_ *mx) {
		if (x < mn->x) mn->x = x;
		if (y < mn->y) mn->y = y;
		if (x > mx->x) mx->x = x;
		if (y > mx->y) mx->y = y;
	}

	double getMagnitude() {
		return sqrt(x * x + y * y);
	}

	void doNormalize() {
		double m = this->getMagnitude();
		if (m == 0) {
			x = 0;
			y = 0;
		}
		else {
			x /= m;
			y /= m;
		}
	}

	VERTEX toVertex() {
		return VERTEX(x, y, 0);
	}
}TERMINAL, *PTERMINAL;


struct LINE
{
	TERMINAL	terms[2];
	TERMINAL	offsets[2];
	TERMINAL	center;
	double		radius;
	bool		isValid;
	int			neighbourIndex;
	double		maxOffset;
	double		maxLifeSpan;
	LINE();

	virtual ~LINE() = default;

	LINE(TERMINAL t1, TERMINAL t2);

	LINE(TERMINAL t1, TERMINAL t2, TERMINAL of1, TERMINAL of2);

	LINE *clone();

	VERTEX getPositiveDirection();

	VERTEX getNegativeDirection();

	bool isFliped();

	VERTEX getGradient();

	void swapTerminals();

	bool hasSamePivot(TERMINAL p);

	double getDistance(PTERMINAL t, PTERMINAL ret);

	bool isContainedPoint(PTERMINAL p);

	VERTEX getTangent(TERMINAL p);

	TERMINAL getPositiveOffset(double offset);

	TERMINAL getNegativeOffset(double offset);

	VERTEX getPositiveDelta();

	VERTEX getNegativeDelta();

	LINE* tryOffset(double offset);

	void doOffset();

	double getMagnitude();

	double getOffetMagnitude();

	void getBound(PTERMINAL mn, PTERMINAL mx);

	void getBound(PVERTEX mn, PVERTEX mx);
};

typedef struct SHAPE
{
	vector<LINE *> prims;
	bool isValid = true;
	bool isIntersected = true;
	bool isCompleted = false;
	bool isPositive = false;
	bool isGemhole = false;
	double maxOffset;
	double depth = 0;

	int color;
	SHAPE();

	SHAPE *clone();

	int findFrozenTerm();

	int findFrozenPrimitive();

	int getFreeTerm(int index, SHAPE *other);

	bool isSortedShape();

	bool sortprimitives();

	bool isPositiveShape();

	void reversePrimitive();

	bool makePositive();

	void update();

	bool isInsidePoint(LINE *pr, int index);

	TERMINAL getCenter();

	bool isInsidePoint(TERMINAL p);

	bool isInsideShape(SHAPE *shp);

	void InsertPrimitive(LINE *pr, int index);

	void removeInvalidPrimitives();

	bool getIntersection(LINE *pr, PTERMINAL ret, int *retIndex);

	void doOffset();

	void clear();
}SHAPE, *PSHAPE; 

bool isCrossed(LINE *l1, LINE *l2);
int isConflict(LINE *obj1, LINE *obj2, TERMINAL *p1);
int isConflict(VERTEX v1, VERTEX v2, double val, VERTEX *ret);
double GetAngleBetweenTerminal(TERMINAL t1, TERMINAL t2);

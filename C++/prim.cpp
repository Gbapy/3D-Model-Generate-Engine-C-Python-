#include "prim.h"

TERMINAL doMinus(TERMINAL a, TERMINAL b) {
	TERMINAL r = TERMINAL(a.x - b.x, a.y - b.y);

	return r;
}

double GetDistance(TERMINAL p1, TERMINAL p2) {
	double x = p2.x - p1.x;
	double y = p2.y - p1.y;
	return sqrt(x * x + y * y);
}

double GetDistance(VERTEX p1, VERTEX p2) {
	double x = p2.val[0] - p1.val[0];
	double y = p2.val[1] - p1.val[1];
	double z = p2.val[2] - p1.val[2];
	return sqrt(x * x + y * y + z * z);
}

double GetMagnitude(PTERMINAL p) {
	return sqrt(p->x * p->x + p->y * p->y);
}

double GetMagnitude(VERTEX p) {
	return sqrt(p.val[0] * p.val[0] + p.val[1] * p.val[1] + p.val[2] * p.val[2]);
}

double GetMagnitude(TERMINAL p) {
	return sqrt(p.x * p.x + p.y * p.y);
}

double GetAngleBetweenTerminal(TERMINAL t1, TERMINAL t2) {
	double x = t2.x - t1.x;
	double y = t2.y - t1.y;

	if (x == 0) {
		return y > 0 ? 90.0f : 270.0f;
	}
	else {
		if (y == 0) {
			return x > 0 ? 0 : 180.0f;
		}
		else {
			if (x > 0) {
				return y > 0 ? atan(y / x) * 180.0f / M_PI : 360.0f + atan(y / x) * 180.0f / M_PI;
			}
			else {
				return 180.0f + atan(y / x) * 180.0f / M_PI;
			}
		}
	}
	return atan(y / x) * 180.0f / M_PI;
}

int GetSharePoint(LINE *l1, LINE *l2, PTERMINAL p1) {
	VERTEX v11 = VERTEX(l1->terms[0].x, l1->terms[0].y, 0);
	VERTEX v12 = VERTEX(l1->terms[1].x, l1->terms[1].y, 0);
	VERTEX v21 = VERTEX(l2->terms[0].x, l2->terms[0].y, 0);
	VERTEX v22 = VERTEX(l2->terms[1].x, l2->terms[1].y, 0);
	VERTEX v1 = doMinus(v12, v11);
	VERTEX v2 = doMinus(v22, v21);
	VERTEX v3 = doCross(v1, v2);
	if (fabs(v3.val[2]) < 1E-10) return 0;
	VERTEX vt1 = doMinus(v21, v11);
	VERTEX vt2 = doMinus(v22, v11);
	vt1 = doCross(vt1, v1);
	vt2 = doCross(v1, vt2);

	double m1 = vt1.val[2];
	double m2 = vt2.val[2];
	double m = m1 + m2;
	p1->x = (v21.val[0] * m2 + v22.val[0] * m1) / m;
	p1->y = (v21.val[1] * m2 + v22.val[1] * m1) / m;
	return 1;
}

bool isCrossed(LINE *l1, LINE *l2) {
	VERTEX mn1, mn2, mx1, mx2;

	l1->getBound(&mn1, &mx1); l2->getBound(&mn2, &mx2);
	if (mx1.val[0] < mn2.val[0]) return false;
	if (mx1.val[1] < mn2.val[1]) return false;
	if (mx2.val[0] < mn1.val[0]) return false;
	if (mx2.val[1] < mn1.val[1]) return false;
	return true;
}

int isConflict(LINE *obj1, LINE *obj2, TERMINAL *p1) {
	if (isCrossed(obj1, obj2) == false) return 0;
	int ret = GetSharePoint(obj1, obj2, p1);
	if (ret == 0 || ret == 2) return ret;

	ret = obj1->isContainedPoint(p1) && obj2->isContainedPoint(p1) ? 1 : 0;
	return ret;
}

int isConflict(VERTEX v1, VERTEX v2, double val, VERTEX *ret) {
	double x0 = v2.val[0] - v1.val[0];
	double x1 = val - v1.val[0];
	double x2 = val - v2.val[0];
	if ((x1 < 0 && x2 > 0) || (x1 > 0 && x2 < 0)) {
		ret->val[0] = val;
		ret->val[1] = v1.val[1] + (v2.val[1] - v1.val[1]) * x1 / x0;
		ret->val[2] = v1.val[2] + (v2.val[2] - v1.val[2]) * x1 / x0;
		return 1;
	}
	return 0;
}

LINE::LINE() {

}

LINE::LINE(TERMINAL t1, TERMINAL t2) {
	this->terms[0] = t1;
	this->terms[1] = t2;
	this->terms[0].isValid = t1.isValid;
	this->terms[1].isValid = t2.isValid;
	this->center.x = (t1.x + t2.x) / 2.0f;
	this->center.y = (t1.y + t2.y) / 2.0f;
	this->radius = M_INFINITE;
	this->maxOffset = M_INFINITE;
	this->isValid = true;
}

LINE::LINE(TERMINAL t1, TERMINAL t2, TERMINAL of1, TERMINAL of2) {
	this->terms[0] = t1;
	this->terms[1] = t2;
	this->terms[0].isValid = t1.isValid;
	this->terms[1].isValid = t2.isValid;
	this->offsets[0] = of1;
	this->offsets[1] = of2;
	this->center.x = (t1.x + t2.x) / 2.0f;
	this->center.y = (t1.y + t2.y) / 2.0f;
	this->radius = M_INFINITE;
	this->maxOffset = M_INFINITE;
	this->isValid = true;
}

LINE *LINE::clone()
{
	LINE *prim = new LINE(this->terms[0], this->terms[1]);
	prim->maxOffset = this->maxOffset;
	prim->maxLifeSpan = this->maxLifeSpan;
	return prim;
}

VERTEX LINE::getPositiveDirection()
{
	VERTEX v = VERTEX(this->terms[1].x - this->terms[0].x,
		this->terms[1].y - this->terms[0].y, 0.0f);
	v.doNormalize();
	return v;
}

VERTEX LINE::getNegativeDirection()
{
	VERTEX v = VERTEX(this->terms[0].x - this->terms[1].x,
		this->terms[0].y - this->terms[1].y, 0.0f);
	v.doNormalize();
	return v;
}

bool LINE::isFliped() {
	VERTEX v1 = VERTEX(terms[1].x - terms[0].x, terms[1].y - terms[0].y, 0);
	VERTEX v2 = VERTEX(offsets[1].x - offsets[0].x, offsets[1].y - offsets[0].y, 0);
	v1.doNormalize();
	v2.doNormalize();
	v1.val[0] -= v2.val[0]; v1.val[1] -= v2.val[1];
	if (GetMagnitude(v1) > 0.01f) return true;
	return false;
}

double LINE::getMagnitude()
{
	VERTEX v = VERTEX(this->terms[0].x - this->terms[1].x,
		this->terms[0].y - this->terms[1].y, 0.0f);
	return GetMagnitude(v);
}

double LINE::getOffetMagnitude()
{
	VERTEX v = VERTEX(this->offsets[0].x - this->offsets[1].x,
		this->offsets[0].y - this->offsets[1].y, 0.0f);
	return GetMagnitude(v);
}

void LINE::getBound(PTERMINAL mn, PTERMINAL mx) {
	if (this->terms[0].x > this->terms[1].x) {
		mx->x = this->terms[0].x;
		mn->x = this->terms[1].x;
	}
	else {
		mx->x = this->terms[1].x;
		mn->x = this->terms[0].x;
	}
	if (this->terms[0].y > this->terms[1].y) {
		mx->y = this->terms[0].y;
		mn->y = this->terms[1].y;
	}
	else {
		mx->y = this->terms[1].y;
		mn->y = this->terms[0].y;
	}
}

void LINE::getBound(PVERTEX mn, PVERTEX mx) {
	if (this->terms[0].x > this->terms[1].x) {
		mx->val[0] = this->terms[0].x;
		mn->val[0] = this->terms[1].x;
	}else{
		mx->val[0] = this->terms[1].x;
		mn->val[0] = this->terms[0].x;
	}
	if (this->terms[0].y > this->terms[1].y) {
		mx->val[1] = this->terms[0].y;
		mn->val[1] = this->terms[1].y;
	}
	else {
		mx->val[1] = this->terms[1].y;
		mn->val[1] = this->terms[0].y;
	}
}

void LINE::swapTerminals()
{
	TERMINAL t = this->terms[1];
	this->terms[1] = this->terms[0];
	this->terms[0] = t;
}

bool LINE::hasSamePivot(TERMINAL p)
{
	if (this->terms[0].isEqual(p)) {
		return true;
	}
	if (this->terms[1].isEqual(p)) {
		this->swapTerminals();
		return true;
	}
	return false;
}

double LINE::getDistance(PTERMINAL t, PTERMINAL ret)
{
	VERTEX v1 = VERTEX(t->x - this->terms[1].x, t->y - this->terms[1].y, 0);
	VERTEX v0 = VERTEX(t->x - this->terms[0].x, t->y - this->terms[0].y, 0);
	VERTEX v2 = VERTEX(this->terms[1].x - this->terms[0].x,
		this->terms[1].y - this->terms[0].y, 0);
	VERTEX v3 = doCross(v1, v0);
	double a = GetMagnitude(v2);
	double b = GetMagnitude(v0);
	double c = GetMagnitude(v1);
	double h = fabs(v3.val[2]) / a;

	if (h > b) h = b;
	if (h > c) h = c;

	double l0 = sqrt(b * b - h * h);
	double l1 = sqrt(c * c - h * h);
	if (l0 <= a && l1 <= a) {
		VERTEX v = this->getPositiveDirection();
		ret->x = this->terms[0].x + v.val[0] * l0;
		ret->y = this->terms[0].y + v.val[1] * l0;
	}
	else {
		if (l0 > l1) {
			VERTEX v = this->getPositiveDirection();
			ret->x = terms[0].x + v.val[0] * l0;
			ret->y = terms[0].y + v.val[1] * l0;
		}
		else {
			VERTEX v = this->getNegativeDirection();
			ret->x = terms[1].x + v.val[0] * l1;
			ret->y = terms[1].y + v.val[1] * l1;
		}
	}
	return h;
}

bool LINE::isContainedPoint(PTERMINAL p)
{
	TERMINAL v1 = TERMINAL(p->x - this->terms[0].x, p->y - this->terms[0].y);
	TERMINAL v2 = TERMINAL(p->x - this->terms[1].x, p->y - this->terms[1].y);
	TERMINAL v3 = TERMINAL(this->terms[1].x - this->terms[0].x,
		this->terms[1].y - this->terms[0].y);
	double f1 = GetMagnitude(&v1);
	double f2 = GetMagnitude(&v2);
	double f3 = GetMagnitude(&v3);

	if (fabs(f3 - f1 - f2) <= EP) return true;
	return false;
}

VERTEX LINE::getGradient() {
	VERTEX v = this->getPositiveDirection();
	v = doCross(v, VERTEX(0, 0, 1));
	v.doNormalize();

	return v;
}

LINE* LINE::tryOffset(double offset)
{
	LINE *ret = new LINE(this->terms[0], this->terms[1]);
	VERTEX v = this->getGradient();
	ret->terms[0].x += (v.val[0] * offset); ret->terms[0].y += (v.val[1] * offset);
	ret->terms[1].x += (v.val[0] * offset); ret->terms[1].y += (v.val[1] * offset);
	return ret;
}

void LINE::doOffset() {
	this->terms[0] = offsets[0];
	this->terms[1] = offsets[1];
}

VERTEX LINE::getTangent(TERMINAL p)
{
	return this->getPositiveDirection();
}

TERMINAL LINE::getPositiveOffset(double offset) {
	return TERMINAL((this->terms[1].x - this->terms[0].x) * offset + this->terms[0].x,
		(this->terms[1].y - this->terms[0].y) * offset + this->terms[0].y);
}

TERMINAL LINE::getNegativeOffset(double offset) {
	return TERMINAL((this->terms[0].x - this->terms[1].x) * offset + this->terms[1].x,
		(this->terms[0].y - this->terms[1].y) * offset + this->terms[1].y);
}

VERTEX LINE::getPositiveDelta()
{
	return VERTEX(terms[1].x - terms[0].x, terms[1].y - terms[0].y, 0);
}

VERTEX LINE::getNegativeDelta()
{
	return VERTEX(terms[0].x - terms[1].x, terms[0].y - terms[1].y, 0);
}

SHAPE::SHAPE() {
	this->prims.clear();
	this->isValid = true;
	this->isIntersected = true;
	this->isCompleted = false;
	this->isPositive = false;
}

SHAPE *SHAPE::clone() {
	SHAPE *shp = new SHAPE();
	for (std::size_t i = 0; i < this->prims.size(); i++) {
		shp->prims.push_back(this->prims[i]->clone());
	}
	shp->isValid = this->isValid;
	shp->isIntersected = this->isIntersected;
	shp->isCompleted = this->isCompleted;
	shp->isPositive = this->isPositive;
	return shp;
}

int SHAPE::getFreeTerm(int index, SHAPE *other)
{
	for (UINT i = 0; i < this->prims.size(); i++) {
		if (this->prims[i]->terms[0].isValid == false) continue;
		LINE ln = LINE(other->prims[index]->terms[0], this->prims[i]->terms[0]);
		TERMINAL p;
		int retIndex;

		if (getIntersection(&ln, &p, &retIndex) == true) {
			continue;
		}
		if (other->getIntersection(&ln, &p, &retIndex) == true) {
			continue;
		}
		return i;
	}
	return -1;
}

int SHAPE::findFrozenTerm() {
	for (std::size_t i = 0; i < this->prims.size(); i++) {
		TERMINAL t1 = this->prims[i]->terms[0];
		TERMINAL t2 = this->prims[i]->terms[1];
		int n1 = 0;
		int n2 = 0;
		for (std::size_t j = 0; j < this->prims.size(); j++) {
			if (i == j) continue;
			if (this->prims[j]->terms[0].isEqual(t1) || this->prims[j]->terms[1].isEqual(t1)) n1++;
			if (this->prims[j]->terms[0].isEqual(t2) || this->prims[j]->terms[1].isEqual(t2)) n2++;
		}
		if (n1 == 0 || n2 == 0) return (int)i;
		if (n1 > 1 || n2 > 1) return (int)i;
	}
	return -1;
}

int SHAPE::findFrozenPrimitive() {
	for (std::size_t i = 0; i < this->prims.size(); i++) {
		LINE *pr = this->prims[i]->clone();
		TERMINAL t;
		int index;

		if (this->getIntersection(pr, &t, &index) == true) {
			delete pr;
			return (int)i;
		}
		delete pr;
	}
	return -1;
}

bool SHAPE::isSortedShape() {
	for (std::size_t i = 0; i < this->prims.size(); i++) {
		int n = (int)i + 1;
		if (n == (int)(this->prims.size())) n = 0;
		if (this->prims[i]->terms[1].isEqual(this->prims[n]->terms[0]) == false) {
			return false;
		}
	}
	return true;
}

bool SHAPE::sortprimitives() {
	if (this->findFrozenTerm() != -1) return false;
	if (this->prims.size() < 2) return false;
	int n = 0;
	TERMINAL t = this->prims[0]->terms[1];
	vector<LINE *> ps;

	ps.push_back(this->prims[0]);

	while (true) {
		bool flag = false;
		for (std::size_t i = 0; i < this->prims.size(); i++) {
			if (n == (int)i) continue;
			if (this->prims[i]->hasSamePivot(t)) {

				ps.push_back(this->prims[i]);
				t = this->prims[i]->terms[1];
				n = (int)i;
				if (ps.size() == this->prims.size()) {
					flag = false;
				}
				else {
					flag = true;
				}
				break;
			}
		}
		if (flag == false) break;
	}
	this->prims.clear();
	for (std::size_t i = 0; i < ps.size(); i++) {
		this->prims.push_back(ps[i]);
	}

	return true;
}

bool SHAPE::isPositiveShape() {
	for (std::size_t i = 0; i < this->prims.size(); i++) {
		int n = (int)i - 1;
		if (n < 0) n = (int)this->prims.size() - 1;
		TERMINAL t = this->prims[i]->terms[0];
		VERTEX v1 = this->prims[i]->getPositiveDirection();
		VERTEX v2T = this->prims[n]->getNegativeDirection();
		VERTEX v2 = VERTEX(-v2T.val[0], -v2T.val[1], 0);

		v2T.val[0] += v1.val[0]; v2T.val[1] += v1.val[1];
		v2T.doNormalize();
		v1 = doCross(v2, v1);
		if (v1.val[2] == 0) {
			v2T = doCross(VERTEX(0, 0, 1), v2);
		}
		else if (v1.val[2] < 0) {
			v2T.val[0] = -v2T.val[0]; v2T.val[1] = -v2T.val[1];
		}
		v2T.doNormalize();

		LINE l1 = LINE(t, TERMINAL(t.x + v2T.val[0] * M_INFINITE, t.y + v2T.val[1] * M_INFINITE));
		TERMINAL p;
		double mn = M_INFINITE;
		int mnI = -1;

		for (std::size_t j = 0; j < this->prims.size(); j++) {
			TERMINAL p1, p2;
			if (isConflict(&l1, this->prims[j], &p1) == 1) {
				if (p1.isValid && !p1.isEqual(t)) {
					double m = GetDistance(t, p1);
					if (m < mn) {
						p = p1;
						mn = m;
						mnI = (int)j;
					}
				}
			}
		}

		if (mnI == -1)
			return false;
		v1 = this->prims[mnI]->getTangent(p);
		v2.val[0] = t.x - p.x; v2.val[1] = t.y - p.y; v2.val[2] = 0;
		v2.doNormalize();
		v1 = doCross(v1, v2);
		if (v1.val[2] < 0)
			return false;
	}
	return true;
}

void SHAPE::reversePrimitive() {
	vector<LINE *> ps;
	for (int i = (int)this->prims.size() - 1; i >= 0; i--) {
		this->prims[i]->swapTerminals();
		ps.push_back(this->prims[i]);
	}
	this->prims.clear();
	for (std::size_t i = 0; i < ps.size(); i++) {
		this->prims.push_back(ps[i]);
	}
}

bool SHAPE::makePositive() {
	if (this->isPositiveShape() == false) {
		reversePrimitive();
	}
	this->isPositive = true;
	return true;
}

void SHAPE::update() {
	if (this->isSortedShape() == false) {
		if (!sortprimitives()) return;
		this->makePositive();
	}
	else {
		this->isPositive = this->isPositiveShape();
	}
	if (this->findFrozenPrimitive() != -1) return;
	this->isCompleted = true;
}

bool SHAPE::isInsidePoint(LINE *pr, int index) {
	VERTEX v1 = this->prims[index]->getTangent(pr->terms[1]);
	VERTEX v2 = pr->getNegativeDirection();
	v1 = doCross(v1, v2);
	return v1.val[2] < 0 ? false : true;
}

TERMINAL SHAPE::getCenter() {
	int n = (int)this->prims.size();
	double mnx = M_INFINITE;
	double mxx = -M_INFINITE;
	double mny = M_INFINITE;
	double mxy = -M_INFINITE;

	for (int i = 0; i < n; i++) {
		double x = this->prims[i]->terms[0].x;
		double y = this->prims[i]->terms[0].y;;
		if (x > mxx) mxx = x;
		if (x < mnx) mnx = x;
		if (y > mxy) mxy = y;
		if (y < mny) mny = y;
	}

	return TERMINAL((mnx + mxx) / 2.0f, (mny + mxy) / 2.0f);
}

bool SHAPE::isInsidePoint(TERMINAL p) {
	TERMINAL t1, t2;
	int index1, index2;
	LINE ln1 = LINE(TERMINAL(p.x, p.y), TERMINAL(p.x + M_INFINITE, p.y));
	LINE ln2 = LINE(TERMINAL(p.x, p.y), TERMINAL(p.x - M_INFINITE, p.y));
	bool ret = this->getIntersection(&ln1, &t1, &index1) &&
		this->getIntersection(&ln2, &t2, &index2) ? true : false;

	if (ret == false) {
		return this->isPositive ? false : true;
	}
	ln1 = LINE(p, t1);
	ln2 = LINE(p, t2);
	ret = this->isInsidePoint(&ln1, index1) || this->isInsidePoint(&ln2, index2) ? true : false;
	return ret;
}

bool SHAPE::isInsideShape(SHAPE *shp) {
	for (std::size_t i = 0; i < shp->prims.size(); i++) {
		if (this->isInsidePoint(shp->prims[i]->terms[0])) return true;
	}
	return false;
}

void SHAPE::InsertPrimitive(LINE *pr, int index) {
	SHAPE sp;

	for (std::size_t i = 0; i < this->prims.size(); i++) {
		if ((int)i == index) {
			sp.prims.push_back(pr);
		}
		sp.prims.push_back(this->prims[i]);
	}
	this->prims.clear();
	for (std::size_t i = 0; i < sp.prims.size(); i++) {
		this->prims.push_back(sp.prims[i]);
	}
}

void SHAPE::removeInvalidPrimitives() {
	SHAPE sp;

	for (std::size_t i = 0; i < this->prims.size(); i++) {
		if (this->prims[i]->isValid) {
			sp.prims.push_back(this->prims[i]);
		}
		else {
			delete this->prims[i];
		}

	}
	this->prims.clear();
	for (std::size_t i = 0; i < sp.prims.size(); i++) {
		this->prims.push_back(sp.prims[i]);
	}
}

bool SHAPE::getIntersection(LINE *pr, PTERMINAL ret, int *retIndex) {
	bool flag = false;
	double mn = 0;

	*retIndex = -1;
	for (std::size_t i = 0; i < this->prims.size(); i++) {
		TERMINAL p;
		if (isConflict(pr, this->prims[i], &p) == 1) {
			if (!(p.isEqual(pr->terms[0])) && !(p.isEqual(pr->terms[1]))) {
				VERTEX v = VERTEX(p.x - pr->terms[0].x, p.y - pr->terms[0].y, 0);
				double m = GetMagnitude(v);
				if (flag == false) {
					flag = true;
					mn = m;
					ret->x = p.x;
					ret->y = p.y;
					*retIndex = (int)i;
				}
				else {
					if (m < mn) {
						ret->x = p.x;
						ret->y = p.y;
						*retIndex = (int)i;
						mn = m;
					}
				}
			}
		}
	}
	return flag;
}

void SHAPE::doOffset() {
	for (UINT i = 0; i < this->prims.size(); i++) {
		this->prims[i]->doOffset();
	}
}

void SHAPE::clear() {
	for (std::size_t i = 0; i < this->prims.size(); i++) {
		delete this->prims[i];
	}
	prims.clear();
}
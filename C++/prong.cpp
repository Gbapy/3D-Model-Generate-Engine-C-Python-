#include "ImgCvt.h"
#include "prim.h"

VERTEX			prongTool[4];

void initProngTool() {
	prongTool[0] = VERTEX(1.0f, 0, 0.0f);
	prongTool[1] = VERTEX(1.0f, 0, 0.15f);
	prongTool[2] = VERTEX(0.85f, 0, 0.25f);
	prongTool[3] = VERTEX(0.85f, 0, 10.0f);
}

double getProngToolRadius(VERTEX *prTool, double z) {
	double ret = 0;
	
	for (int i = 0; i < 3; i++) {
		int j = i + 1;
		if (z >= prTool[i].val[2] && z < prTool[j].val[2]) {
			double rate = prTool[j].val[2] - prTool[i].val[2];
			rate = (z - prTool[i].val[2]) / rate;
			ret = prTool[i].val[0] + rate * (prTool[j].val[0] - prTool[i].val[0]);
		}
	}
	return ret;
}

bool isInsidePrTool(VERTEX *prTool, VERTEX p, VERTEX gemPos, double gemScale) {
	VERTEX v = doMinus(p, gemPos);
	double z = v.val[2];
	double radius = getProngToolRadius(prTool, z);
	v.val[2] = 0;
	if (v.getMagnitude() < radius) {
		return true;
	}
	return false;
}

int getConflictPoint(VERTEX *prTool, VERTEX v1, VERTEX v2, VERTEX gemPos, double gemScale, VERTEX *ret) {
	bool r1 = isInsidePrTool(prTool, v1, gemPos, gemScale);
	bool r2 = isInsidePrTool(prTool, v2, gemPos, gemScale);
	if (!r1 && !r2) return -1;
	if (r1 && r2) return 0;
	
	VERTEX p1 = doMinus(v1, gemPos);
	VERTEX p2 = doMinus(v2, gemPos);
	p1.val[2] = 0; p2.val[2] = 0;
	double rd1 = getProngToolRadius(prTool, v1.val[2]);
	double rd2 = getProngToolRadius(prTool, v2.val[2]);
	double rate = 0;
	double d1 = p1.getMagnitude();
	double d2 = p2.getMagnitude();
	if (v1.val[2] != v2.val[2]) {
		
		LINE ln1 = LINE(TERMINAL(v1.val[2], rd1), TERMINAL(v2.val[2], rd2));
		LINE ln2 = LINE(TERMINAL(v1.val[2], d1), TERMINAL(v2.val[2], d2));
		TERMINAL t;

		if (isConflict(&ln1, &ln2, &t) != 1) {
			return -1;
		}
		rate = (t.x - v1.val[2]) / (v2.val[2] - v1.val[2]);
	}
	else {
		rate = (rd1 - d1) / (d2 - d1);
	}

	p1 = doMinus(v2, v1);
	*ret = VERTEX(v1.val[0] + p1.val[0] * rate, v1.val[1] + p1.val[1] * rate, v1.val[2] + p1.val[2] * rate);
	return r1 ? 1 : 2;
}

void cutProng(SUBNODE *prg, VERTEX gemPos, double gemScale) {
	VERTEX prTool[4];
	int m = (int)(prg->index.size());

	for (int i = 0; i < 4; i++) {
		prTool[i] = VERTEX(prongTool[i].val[0] * gemScale, 0, prongTool[i].val[2] * gemScale);
	}
	
	for (UINT i = 0; i < prg->vertex.size(); i++) {
		prg->vertex[i].optIndex = 0;
	}

	for (int i = 0; i < m; i++) {
		TRIANGLE tr = prg->index[i];
		vector<VERTEX> sharePoints;
		int unsharedIndex = -1;
		VERTEX v1 = prg->vertex[tr.idx[0]];
		VERTEX v2 = prg->vertex[tr.idx[1]];
		VERTEX v3 = prg->vertex[tr.idx[2]];
		v2 = doMinus(v2, v1);
		v3 = doMinus(v3, v1);
		VERTEX v0 = doCross(v2, v3);
		v0.doNormalize();

		for (int j = 0; j < 3; j++) {
			int k = j + 1 == 3 ? 0 : j + 1;
			v1 = prg->vertex[tr.idx[j]];
			v2 = prg->vertex[tr.idx[k]];
			VERTEX ctp;
			int ret = getConflictPoint(prTool, v1, v2, gemPos, gemScale, &ctp);
			if (ret > 0) {
				sharePoints.push_back(ctp);
				if (ret == 1) {
					prg->vertex[tr.idx[j]].optIndex = 100;
				}
				else {
					prg->vertex[tr.idx[k]].optIndex = 100;
				}
			}
			else{
				unsharedIndex = j;
				if (ret == 0) {
					prg->vertex[tr.idx[j]].optIndex = 100;
					prg->vertex[tr.idx[k]].optIndex = 100;
				}
			}
		}
		if (sharePoints.size() == 2 && unsharedIndex != -1) {
			int p = unsharedIndex + 2 >= 3 ? unsharedIndex - 1 : unsharedIndex + 2;
			int k = unsharedIndex + 1 == 3 ? 0 : unsharedIndex + 1;
			v1 = sharePoints[0];
			v2 = sharePoints[1];
			v3 = prg->vertex[tr.idx[p]];
			v1 = doMinus(v1, v3);
			v2 = doMinus(v2, v3);
			v1 = doCross(v1, v2);
			v1.doNormalize();
			v1 = doMinus(v1, v0);
			if (v1.getMagnitude() < 0.001f) {
				prg->vertex.push_back(sharePoints[0]);
				prg->vertex.push_back(sharePoints[1]);
			}
			else {
				prg->vertex.push_back(sharePoints[1]);
				prg->vertex.push_back(sharePoints[0]);
			}
			int vc = (int)(prg->vertex.size());
			prg->index[i].idx[0] = vc - 2;
			prg->index[i].idx[1] = vc - 1;
			prg->index[i].idx[2] = tr.idx[p];
			prg->index.push_back(TRIANGLE(vc - 2, tr.idx[k], vc - 1));
			prg->index.push_back(TRIANGLE(vc - 2, tr.idx[unsharedIndex], tr.idx[k]));
		}
	}

	for (int i = 0; i < (int)(prg->vertex.size()); i++) {
		if (prg->vertex[i].optIndex != 100) continue;
		VERTEX v = prg->vertex[i];
		double radius = getProngToolRadius(prTool, v.val[2]);
		VERTEX ct = VERTEX(gemPos.val[0], gemPos.val[1], v.val[2]);
		VERTEX p = doMinus(v, ct);
		//if (p.getMagnitude() < radius) 
		{
			p.doNormalize();
			v = VERTEX(ct.val[0] + p.val[0] * radius, ct.val[1] + p.val[1] * radius, ct.val[2]);
			prg->vertex[i] = v;
		}
	}
}
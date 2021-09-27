#include "ImgCvt.h"
#include <iostream>
#include <sys/mman.h>
#include <errno.h>

tick32_t get_tick_count()
{
	tick32_t tick = 0ul;

#if defined(WIN32) || defined(WIN64)
	tick = GetTickCount();
#else
	struct timespec tp;

	clock_gettime(CLOCK_MONOTONIC, &tp);

	tick = (tp.tv_sec * 1000ul) + (tp.tv_nsec / 1000ul / 1000ul);
#endif

	return tick;
}

int dumpData(vector<SUBNODE> *nodes, double **vertBuffer, double **normBuffer, int **meshBuffer, MESHINFO **meshInfo, double *extraInfo) {
	int vCount = 0;
	int mCount = 0;

	for (UINT i = 0; i < nodes->size(); i++) {
		vCount += (int)(nodes->at(i).vertex.size());
		mCount += (int)(nodes->at(i).index.size());
	}

	extraInfo[13] = (double)(nodes->size());
	MESHINFO *mesh_info = (MESHINFO *)mmap(NULL, sizeof(MESHINFO) * (int)(nodes->size()), 
		PROT_READ | PROT_WRITE | PROT_EXEC, MAP_ANON | MAP_PRIVATE, 0, 0);
	double *vBuffer = (double *)mmap(NULL, sizeof(double) * vCount * 3,
		PROT_READ | PROT_WRITE | PROT_EXEC, MAP_ANON | MAP_PRIVATE, 0, 0);
	double *nBuffer = (double *)mmap(NULL, sizeof(double) * vCount * 3,
		PROT_READ | PROT_WRITE | PROT_EXEC, MAP_ANON | MAP_PRIVATE, 0, 0);
	int *mBuffer = (int *)mmap(NULL, sizeof(int) * mCount * 3,
		PROT_READ | PROT_WRITE | PROT_EXEC, MAP_ANON | MAP_PRIVATE, 0, 0);

	if (mesh_info == MAP_FAILED || vBuffer == MAP_FAILED || 
		nBuffer == MAP_FAILED || mBuffer == MAP_FAILED) {
		cout << "Fail to allocation memories for mesh buffer!" << endl;
		return -40;
	}

	vCount = 0; mCount = 0;
	for (UINT i = 0; i < nodes->size(); i++) {
		memcpy(mesh_info[i].name, nodes->at(i).name, 256);
		mesh_info[i].vertCount = (int)(nodes->at(i).vertex.size());
		mesh_info[i].meshCount = (int)(nodes->at(i).index.size());

		for (UINT j = 0; j < nodes->at(i).index.size(); j++) {
			mBuffer[mCount * 3] = nodes->at(i).index[j].idx[0];
			mBuffer[mCount * 3 + 1] = nodes->at(i).index[j].idx[1];
			mBuffer[mCount * 3 + 2] = nodes->at(i).index[j].idx[2];
			mCount++;
		}

		for (UINT j = 0; j < nodes->at(i).vertex.size(); j++) {
			vBuffer[vCount * 3] = nodes->at(i).vertex[j].val[0];
			vBuffer[vCount * 3 + 1] = nodes->at(i).vertex[j].val[1];
			vBuffer[vCount * 3 + 2] = nodes->at(i).vertex[j].val[2];

			nBuffer[vCount * 3] = nodes->at(i).vertex[j].nor.val[0];
			nBuffer[vCount * 3 + 1] = nodes->at(i).vertex[j].nor.val[1];
			nBuffer[vCount * 3 + 2] = nodes->at(i).vertex[j].nor.val[2];

			vCount++;
		}
	}
	*meshInfo = mesh_info;
	*vertBuffer = vBuffer;
	*normBuffer = nBuffer;
	*meshBuffer = mBuffer;
	return 0;
}

extern "C" int ReadBmpFile(char* filename, int *width, int *height, UCHAR **bmpDataPtr)
{
	cout << "Reading Bmp..." << endl;
	FILE* f = fopen(filename, "rb");
	if (f == NULL) {
		cout << "Fail to open file" << endl;
		return -1;
	}
	unsigned char info[54];
	fread(info, sizeof(unsigned char), 54, f); // read the 54-byte header

	// extract image height and width from header
	uint32_t offset = *(uint32_t *)&info[10];
	*width = *(int*)&info[18];
	*height = *(int*)&info[22];

	if (*width == 0 || *height == 0) {
		cout << "Invalid width or height!" << endl;
		return -2;
	}
	uint16_t biCount = *(uint16_t *)(info + 28);
	biCount /= 8;

	int row_padded = 0;
	if (biCount == 3) {
		row_padded = (*width * 3 + 3) & (~3);
	}
	else
	{
		row_padded = *width * 4;
	}
	if (*height < 0) *height *= -1;
	*bmpDataPtr = (UCHAR *)mmap(NULL, *height * (*width) * 3,
		PROT_READ | PROT_WRITE | PROT_EXEC, MAP_ANON | MAP_PRIVATE, 0, 0);
	if (*bmpDataPtr == NULL) {
		cout << "Fail to allocating a buffer for bmp data" << endl;
		return -3;
	}

	UCHAR *tmp = (UCHAR *)mmap(NULL, row_padded,
		PROT_READ | PROT_WRITE | PROT_EXEC, MAP_ANON | MAP_PRIVATE, 0, 0);

	if (tmp == NULL) {
		munmap(*bmpDataPtr, *height * (*width) * 3);
		cout << "Fail to allocating a buffer for row data" << endl;
		return -4;
	}
	UCHAR *bmpData = (UCHAR *)(*bmpDataPtr);

	fseek(f, offset, SEEK_SET);
	for (int i = 0; i < *height; i++)
	{
		UCHAR *ptr = bmpData + i * (*width * 3);
		fread(tmp, sizeof(UCHAR), row_padded, f);
		for (int j = 0; j < *width; j++)
		{
			ptr[j * 3] = tmp[j * biCount];
			ptr[j * 3 + 1] = tmp[j * biCount + 1];
			ptr[j * 3 + 2] = tmp[j * biCount + 2];
		}
	}

	munmap(tmp, row_padded);
	fclose(f);
	return 0;
}

UCHAR* ReadBMP(char* filename, int *width, int *height)
{
	FILE* f = fopen(filename, "rb");
	UCHAR* data = NULL;
	if (f == NULL)	return NULL;

	unsigned char info[54];
	fread(info, sizeof(unsigned char), 54, f); // read the 54-byte header

	// extract image height and width from header
	uint32_t offset = *(uint32_t *)&info[10];
	*width = *(int*)&info[18];
	*height = *(int*)&info[22];
	uint16_t biCount = *(uint16_t *)(info + 28);
	biCount /= 8;

	int row_padded = 0;
	if (biCount == 3) {
		row_padded = (*width * 3 + 3) & (~3);
	}
	else
	{
		row_padded = *width * 4;
	}
	if (*height < 0) *height *= -1;
	data = new unsigned char[*height * (*width) * 3];
	UCHAR* tmp = new unsigned char[row_padded];
	fseek(f, offset, SEEK_SET);
	for (int i = 0; i < *height; i++)
	{
		UCHAR *ptr = data + i * (*width * 3);
		fread(tmp, sizeof(UCHAR), row_padded, f);
		for (int j = 0; j < *width; j++)
		{
			ptr[j * 3] = tmp[j * biCount];
			ptr[j * 3 + 1] = tmp[j * biCount + 1];
			ptr[j * 3 + 2] = tmp[j * biCount + 2];
		}
	}

	free(tmp);
	fclose(f);
	cout << data << endl;
	return data;
}

extern "C" int parseBmp(char* fileBuffer, int *width, int *height, UCHAR **bmpDataPtr)
{
	cout << "Parsing Bmp...";
	char *fileData = fileBuffer;
	// extract image height and width from header
	uint32_t offset = *(uint32_t *)&fileData[10];
	*width = *(int*)&fileData[18];
	*height = *(int*)&fileData[22];

	if (*width == 0 || *height == 0) {
		cout << "Invalid width or height! (" << *width << ", " << *height << ")" << endl;
		return -2;
	}

	cout << "(Width = " << *width << ", Height = " << *height << ") ";

	uint16_t biCount = *(uint16_t *)(fileData + 28);
	biCount /= 8;

	int row_padded = 0;
	if (biCount == 3) {
		row_padded = (*width * 3 + 3) & (~3);
	}
	else
	{
		row_padded = *width * 4;
	}
	if (*height < 0) *height *= -1;
	*bmpDataPtr = (UCHAR *)mmap(NULL, *height * (*width) * 3,
		PROT_READ | PROT_WRITE | PROT_EXEC, MAP_ANON | MAP_PRIVATE, 0, 0);
	if (*bmpDataPtr == NULL) {
		cout << "Fail to allocating a buffer for bmp data" << endl;
		return -3;
	}

	UCHAR *bmpData = (UCHAR *)(*bmpDataPtr);

	fileData += offset;
	for (int i = 0; i < *height; i++)
	{
		UCHAR *ptr = bmpData + i * (*width * 3);
		for (int j = 0; j < *width; j++)
		{
			ptr[j * 3] = fileData[j * biCount];
			ptr[j * 3 + 1] = fileData[j * biCount + 1];
			ptr[j * 3 + 2] = fileData[j * biCount + 2];
		}
		fileData += row_padded;
	}

	cout << "Success!" << endl;
	return 0;
}

extern "C" int readFile(char *fileName, UCHAR **data, int *dataSize) {
	FILE *pFile = fopen(fileName, "rb");

	cout << "Reading file...";

	if (pFile == NULL) {
		cout << "Fail to open file (" << fileName << ")" << endl;
		return -1;
	}
	fseek(pFile, 0, SEEK_END);
	*dataSize = (int)ftell(pFile);
	
	if (*dataSize == 0) {
		cout << "Invalid File Size" << endl;
		return -1;
	}

	fseek(pFile, 0, SEEK_SET);
	*data = (UCHAR *)mmap(NULL, *dataSize,
		PROT_READ | PROT_WRITE | PROT_EXEC, MAP_ANON | MAP_PRIVATE, 0, 0);

	if (*data == NULL) {
		cout << "Fail to memory allocation when reading file." << endl;
	}

	fread(*data, 1, *dataSize, pFile);
	fclose(pFile);
	cout << "Success!" << endl;

	return 0;
}

extern "C" int writeFile(char *fileName, char *data, int dataSize) {
	FILE *pFile = fopen(fileName, "wb");
	
	if (pFile == NULL) return -1;
	fwrite(data, 1, dataSize, pFile);
	fclose(pFile);
	return 0;
}

int ReadGem(char *path) {
	VERTEX mx, mn;
	double width;
	char modelPath[PATH_MAX];

	sprintf(modelPath, "%s/jewel.bin", path);
	int n, m;
	FILE *pFile = fopen(modelPath, "rb");
	if (pFile == NULL) return -1;
	fread(&n, sizeof(int), 1, pFile);
	fread(&m, sizeof(int), 1, pFile);
	gem.vertex.clear();
	gem.index.clear();
	for (int i = 0; i < n; i++) {
		VERTEX v;
		for (int j = 0; j < 3; j++) {
			fread(&(v.val[j]), sizeof(double), 1, pFile);
		}
		v.val[2] = -v.val[2];
		gem.vertex.push_back(v);
	}
	for (int i = 0; i < m; i++) {
		TRIANGLE tr;
		for (int j = 0; j < 3; j++) {
			fread(&(tr.idx[j]), sizeof(int), 1, pFile);
		}
		tr.flip();
		gem.index.push_back(tr);
	}
	fclose(pFile);
	//OptimizeMesh(&gem, true, 50.0f);
	Move2Center(&gem);
	GetBound(&mx, &mn, &gem);
	width = mx.val[0] - mn.val[0];
	DoScale(&gem, 1.0f / width);
	return 0;
}

int ReadProng(char *path) {
	VERTEX mx, mn;
	double width;
	char modelPath[PATH_MAX];

	memset(modelPath, 0, PATH_MAX);

	sprintf(modelPath, "%s/prong.bin", path);
	int n, m;
	FILE *pFile = fopen(modelPath, "rb");
	if (pFile == NULL) return -1;
	fread(&n, sizeof(int), 1, pFile);
	fread(&m, sizeof(int), 1, pFile);
	for (int i = 0; i < n; i++) {
		VERTEX v;
		for (int j = 0; j < 3; j++) {
			fread(&(v.val[j]), sizeof(double), 1, pFile);
		}
		v.val[2] = -v.val[2];
		prong.vertex.push_back(v);
	}
	for (int i = 0; i < m; i++) {
		TRIANGLE tr;
		for (int j = 0; j < 3; j++) {
			fread(&(tr.idx[j]), sizeof(int), 1, pFile);
		}
		tr.flip();
		prong.index.push_back(tr);
	}
	fclose(pFile);
	OptimizeMesh(&prong, true, 500.0f);
	Move2Center(&prong);
	GetBound(&mx, &mn, &prong);
	width = mx.val[0] - mn.val[0];
	Move2Position(&prong, 0, 0, -mn.val[2]);
	DoScale(&prong, 1.0f / width);
	return 0;
}

int ReadProngLow(char *path) {
	VERTEX mx, mn;
	double width;
	char modelPath[PATH_MAX];

	memset(modelPath, 0, PATH_MAX);

	sprintf(modelPath, "%s/prong_low.bin", path);
	int n, m;
	FILE *pFile = fopen(modelPath, "rb");
	if (pFile == NULL) return -1;
	fread(&n, sizeof(int), 1, pFile);
	fread(&m, sizeof(int), 1, pFile);
	for (int i = 0; i < n; i++) {
		VERTEX v;
		for (int j = 0; j < 3; j++) {
			fread(&(v.val[j]), sizeof(double), 1, pFile);
		}
		//v.val[2] = -v.val[2];
		lowProng.vertex.push_back(v);
	}
	for (int i = 0; i < m; i++) {
		TRIANGLE tr;
		for (int j = 0; j < 3; j++) {
			fread(&(tr.idx[j]), sizeof(int), 1, pFile);
		}
		tr.flip();
		lowProng.index.push_back(tr);
	}
	fclose(pFile);
	OptimizeMesh(&lowProng, true, 500.0f);
	Move2Center(&lowProng);
	GetBound(&mx, &mn, &lowProng);
	width = mx.val[0] - mn.val[0];
	Move2Position(&lowProng, 0, 0, -mn.val[2]);
	DoScale(&lowProng, 1.0f / width);
	return 0;
}

int ReadRing(char *path) {
	VERTEX mx, mn;
	double width;
	char modelPath[PATH_MAX];

	memset(modelPath, 0, PATH_MAX);

	sprintf(modelPath, "%s/ring.bin", path);
	int n, m;
	FILE *pFile = fopen(modelPath, "rb");
	if (pFile == NULL) return -1;
	fread(&n, sizeof(int), 1, pFile);
	fread(&m, sizeof(int), 1, pFile);
	for (int i = 0; i < n; i++) {
		VERTEX v;
		for (int j = 0; j < 3; j++) {
			fread(&(v.val[j]), sizeof(double), 1, pFile);
		}
		double m = v.val[2]; v.val[2] = v.val[1]; v.val[1] = m;
		ring.vertex.push_back(v);
	}
	for (int i = 0; i < m; i++) {
		TRIANGLE tr;
		for (int j = 0; j < 3; j++) {
			fread(&(tr.idx[j]), sizeof(int), 1, pFile);
		}

		tr.flip();
		ring.index.push_back(tr);
	}
	fclose(pFile);
	Move2Center(&ring);
	GetBound(&mx, &mn, &ring);
	width = (mx.val[2] + mn.val[2]) * 0.5f;
	Move2Position(&ring, 0, 0, -width);
	width = mx.val[0] - mn.val[0];
	DoScale(&ring, 1.0f / width);
	OptimizeMesh(&ring, true, 5.0f);
	doSetVertexNormal(&ring);
	return 0;
}

extern "C" int getCurvedGemModel(char *bmpPath, int length, PGEM_SIZE_PATTERN gemPat, int gemPatSize, double thickness, double xTwist, double yTwist,
	double **vertBuffer, double **normBuffer, int **meshBuffer, PMESHINFO *meshInfo, double *extraInfo, int option)
{
	UCHAR *raw;
	vector<SUBNODE> nodes;
	int width = 0;
	int height = 0;
	int ret = 0;
	char *filename = (char *)malloc(length + 1);
	tick32_t et = get_tick_count();

	if (filename == NULL) {
		cout << "Fail to allocate buffer for filename..." << endl;
		return -1;
	}
	memset(filename, 0, length + 1);
	memcpy(filename, bmpPath, length);

	raw = ReadBMP(filename, &width, &height);
	free(filename);
	if (raw == NULL || width == 0 || height == 0) return -1;

	ret = generateCurvedModel(raw, width, height, gemPat, gemPatSize, thickness, xTwist, yTwist, &nodes, extraInfo, option);
	if (ret != 0) goto _final;
	if (nodes.size() == 0) {
		ret = -20;
		goto _final;
	}

	for (UINT i = 0; i < nodes.size(); i++) {
		if (nodes[i].vertex.size() == 0 || nodes[i].index.size() == 0) {
			ret = -30;
			goto _final;
		}
	}

	ret = dumpData(&nodes, vertBuffer, normBuffer, meshBuffer, meshInfo, extraInfo);
_final:
	free(raw);
	if (ret == 0) {
		et = get_tick_count() - et;
		cout << "Ellapsed time of mesh generating is " << et << " ms." << endl;
	}
	return ret;
}

extern "C" int getCircularRingModel(char *bmpPath, int length, PGEM_SIZE_PATTERN gemPat, int gemPatSize, double thickness, double xRadius, double yRadius
	, double **vertBuffer, double **normBuffer, int **meshBuffer, PMESHINFO *meshInfo, double *extraInfo, int option) {
	UCHAR *raw;
	vector<SUBNODE> nodes;
	int width = 0;
	int height = 0;
	int ret = 0;
	char *filename = (char *)malloc(length + 1);
	tick32_t et = get_tick_count();

	if (filename == NULL) {
		cout << "Fail to allocate buffer for filename..." << endl;
		return -1;
	}
	memset(filename, 0, length + 1);
	memcpy(filename, bmpPath, length);

	raw = ReadBMP(filename, &width, &height);
	free(filename);
	if (raw == NULL || width == 0 || height == 0) return -1;

	ret = generateCircleRingModel(raw, width, height, gemPat, gemPatSize, thickness, xRadius, yRadius, 
		&nodes, extraInfo, option);
	if (ret != 0) goto _final;
	if (nodes.size() == 0) {
		ret = -20;
		goto _final;
	}

	for (UINT i = 0; i < nodes.size(); i++) {
		if (nodes[i].vertex.size() == 0 || nodes[i].index.size() == 0) {
			ret = -30;
			goto _final;
		}
	}

	ret = dumpData(&nodes, vertBuffer, normBuffer, meshBuffer, meshInfo, extraInfo);
_final:
	free(raw);
	if (ret == 0) {
		et = get_tick_count() - et;
		cout << "Ellapsed time of mesh generating is " << et << " ms." << endl;
	}
	return ret;
}

extern "C" int getEllipticalRingModel(char *bmpPath, int length, PGEM_SIZE_PATTERN gemPat, int gemPatSize, double thickness, double xRadius1, double xRadius2, double yRadius1, double yRadius2,
	double **vertBuffer, double **normBuffer, int **meshBuffer, PMESHINFO *meshInfo, double *extraInfo, int option) {
	UCHAR *raw;
	vector<SUBNODE> nodes;
	int width = 0;
	int height = 0;
	int ret = 0;
	char *filename = (char *)malloc(length + 1);
	tick32_t et = get_tick_count();

	if (filename == NULL) {
		cout << "Fail to allocate buffer for filename..." << endl;
		return -1;
	}
	memset(filename, 0, length + 1);
	memcpy(filename, bmpPath, length);

	raw = ReadBMP(filename, &width, &height);
	free(filename);

	if (raw == NULL || width == 0 || height == 0) return -1;

	ret = generateEllipseRingModel(raw, width, height, gemPat, gemPatSize, thickness, xRadius1, xRadius2, 
		yRadius1, yRadius2, &nodes, extraInfo, option);
	if (ret != 0) goto _final;
	if (nodes.size() == 0) {
		ret = -20;
		goto _final;
	}

	for (UINT i = 0; i < nodes.size(); i++) {
		if (nodes[i].vertex.size() == 0 || nodes[i].index.size() == 0) {
			ret = -30;
			goto _final;
		}
	}
	ret = dumpData(&nodes, vertBuffer, normBuffer, meshBuffer, meshInfo, extraInfo);
_final:
	free(raw);
	if (ret == 0) {
		et = get_tick_count() - et;
		cout << "Ellapsed time of mesh generating is " << et << " ms." << endl;
	}
	return ret;
}

extern "C" int getCurvedGemModelFromBuffer(unsigned char *raw, int width, int height, PGEM_SIZE_PATTERN gemPat, int gemPatSize, double thickness, double xTwist, double yTwist,
	double **vertBuffer, double **normBuffer, int **meshBuffer, PMESHINFO *meshInfo, double *extraInfo, int option)
{
	vector<SUBNODE> nodes;
	int ret = 0;
	tick32_t et = get_tick_count();

	if (raw == NULL || width == 0 || height == 0) return -1;

	ret = generateCurvedModel(raw, width, height, gemPat, gemPatSize, thickness, xTwist, yTwist, &nodes, extraInfo, option);
	if (ret != 0) goto _final;
	if (nodes.size() == 0) {
		ret = -20;
		goto _final;
	}

	for (UINT i = 0; i < nodes.size(); i++) {
		if (nodes[i].vertex.size() == 0 || nodes[i].index.size() == 0) {
			ret = -30;
			goto _final;
		}
	}
	ret = dumpData(&nodes, vertBuffer, normBuffer, meshBuffer, meshInfo, extraInfo);
_final:
	if (ret == 0) {
		et = get_tick_count() - et;
		cout << "Ellapsed time of mesh generating is " << et << " ms." << endl;
	}
	return ret;
}

extern "C" int getCircularRingModelFromBuffer(unsigned char *raw, int width, int height, PGEM_SIZE_PATTERN gemPat, int gemPatSize, double thickness, double xRadius, double yRadius
	, double **vertBuffer, double **normBuffer, int **meshBuffer, PMESHINFO *meshInfo, double *extraInfo, int option) {
	vector<SUBNODE> nodes;
	int ret = 0;
	tick32_t et = get_tick_count();

	if (raw == NULL || width == 0 || height == 0) return -1;

	ret = generateCircleRingModel(raw, width, height, gemPat, gemPatSize, thickness, xRadius, yRadius, &nodes, extraInfo, option);
	if (ret != 0) goto _final;
	if (nodes.size() == 0) {
		ret = -20;
		goto _final;
	}

	for (UINT i = 0; i < nodes.size(); i++) {
		if (nodes[i].vertex.size() == 0 || nodes[i].index.size() == 0) {
			ret = -30;
			goto _final;
		}
	}

	ret = dumpData(&nodes, vertBuffer, normBuffer, meshBuffer, meshInfo, extraInfo);
_final:
	if (ret == 0) {
		et = get_tick_count() - et;
		cout << "Ellapsed time of mesh generating is " << et << " ms." << endl;
	}
	return ret;
}

extern "C" int getEllipticalRingModelFromBuffer(unsigned char *raw, int width, int height, PGEM_SIZE_PATTERN gemPat, int gemPatSize, double thickness, double xRadius1, double xRadius2, double yRadius1, double yRadius2,
	double **vertBuffer, double **normBuffer, int **meshBuffer, PMESHINFO *meshInfo, double *extraInfo, int option) {
	vector<SUBNODE> nodes;
	int ret = 0;
	tick32_t et = get_tick_count();

	if (raw == NULL || width == 0 || height == 0) return -1;

	ret = generateEllipseRingModel(raw, width, height, gemPat, gemPatSize, thickness, 
		xRadius1, xRadius2, yRadius1, yRadius2, &nodes, extraInfo, option);
	if (ret != 0) goto _final;
	if (nodes.size() == 0) {
		ret = -20;
		goto _final;
	}

	for (UINT i = 0; i < nodes.size(); i++) {
		if (nodes[i].vertex.size() == 0 || nodes[i].index.size() == 0) {
			ret = -30;
			goto _final;
		}
	}
	ret = dumpData(&nodes, vertBuffer, normBuffer, meshBuffer, meshInfo, extraInfo);
_final:
	if (ret == 0) {
		et = get_tick_count() - et;
		cout << "Ellapsed time of mesh generating is " << et << " ms." << endl;
	}
	return ret;
}

extern "C" int initOperation(char *initPath, int length) {
	char *path = (char *)malloc(length + 1);
	if (path == NULL) {
		cout << "Fail to allocate buffer for inittialization path..." << endl;
		return -1;
	}
	memset(path, 0, length + 1);
	memcpy(path, initPath, length);

	neighbor[0] = Vec2i(-1, 0);
	neighbor[1] = Vec2i(-1, 1);
	neighbor[2] = Vec2i(0, 1);
	neighbor[3] = Vec2i(1, 1);
	neighbor[4] = Vec2i(1, 0);
	neighbor[5] = Vec2i(1, -1);
	neighbor[6] = Vec2i(0, -1);
	neighbor[7] = Vec2i(-1, -1);
	initProngTool();

	if (ReadGem(path) != 0) return -1;
	if (ReadProng(path) != 0) return -2;
	if (ReadRing(path) != 0) return -3;
	if (ReadProngLow(path) != 0) return -4;
	free(path);
	return 0;
}

extern "C" int getGlbFileSize(double *vertBuffer, double *normBuffer, int *meshBuffer,
	MESHINFO *meshInfo, double *extraInfo) {
	vector<SUBNODE> nodes;
	int nodeCount = (int)(extraInfo[13]);
	int vCount = 0;
	int mCount = 0;

	cout << "Getting GLB File Size...";

	for (int i = 0; i < nodeCount; i++) {
		SUBNODE sb;

		memcpy(sb.name, meshInfo[i].name, 256);
		for (int j = 0; j < meshInfo[i].vertCount; j++) {
			VERTEX v;

			v.val[0] = vertBuffer[vCount * 3];
			v.val[1] = vertBuffer[vCount * 3 + 1];
			v.val[2] = vertBuffer[vCount * 3 + 2];

			v.nor.val[0] = normBuffer[vCount * 3];
			v.nor.val[1] = normBuffer[vCount * 3 + 1];
			v.nor.val[2] = normBuffer[vCount * 3 + 2];
			v.nor.val[2] = normBuffer[vCount * 3 + 2];

			sb.vertex.push_back(v);
			vCount++;
		}

		for (int j = 0; j < meshInfo[i].meshCount; j++) {
			TRIANGLE tr;

			tr.idx[0] = meshBuffer[mCount * 3];
			tr.idx[1] = meshBuffer[mCount * 3 + 1];
			tr.idx[2] = meshBuffer[mCount * 3 + 2];

			sb.index.push_back(tr);
			mCount++;
		}

		nodes.push_back(sb);
	}

	int ret = getGlbBufferSize(&nodes);

	if (ret > 0) {
		cout << "(" << ret << ") Success!" << endl;
	}
	return ret;
}

extern "C" int getGlbFileAsBuffer(double *vertBuffer, double *normBuffer, int *meshBuffer,
	MESHINFO *meshInfo, double *extraInfo, char *fileBuffer, int glbSize) {
	vector<SUBNODE> nodes;
	int nodeCount = (int)(extraInfo[13]);
	int vCount = 0;
	int mCount = 0;
	int ret = 0;
	long fileSize = 0;

	cout << "Getting GLB File as Buffer...";

	for (int i = 0; i < nodeCount; i++) {
		SUBNODE sb;

		memcpy(sb.name, meshInfo[i].name, 256);
		for (int j = 0; j < meshInfo[i].vertCount; j++) {
			VERTEX v;

			v.val[0] = vertBuffer[vCount * 3];
			v.val[1] = vertBuffer[vCount * 3 + 1];
			v.val[2] = vertBuffer[vCount * 3 + 2];

			v.nor.val[0] = normBuffer[vCount * 3];
			v.nor.val[1] = normBuffer[vCount * 3 + 1];
			v.nor.val[2] = normBuffer[vCount * 3 + 2];
			v.nor.val[2] = normBuffer[vCount * 3 + 2];

			sb.vertex.push_back(v);
			vCount++;
		}

		for (int j = 0; j < meshInfo[i].meshCount; j++) {
			TRIANGLE tr;

			tr.idx[0] = meshBuffer[mCount * 3];
			tr.idx[1] = meshBuffer[mCount * 3 + 1];
			tr.idx[2] = meshBuffer[mCount * 3 + 2];

			sb.index.push_back(tr);
			mCount++;
		}

		nodes.push_back(sb);
	}

	ret = getGlbFileBuffer(&nodes, fileBuffer, glbSize);
	if (ret == 0) {
		cout << "Success!" << endl;
	}
	return ret;
}

extern "C" int exportMesh(char *filePath, int length, double *vertBuffer, double *normBuffer, int *meshBuffer,
	MESHINFO *meshInfo, double *extraInfo) {
	cout << "Experting mesh..." << endl;

	char *filename = (char *)malloc(length + 1);

	if (filename == NULL) {
		cout << "Fail to allocate buffer for filename..." << endl;
		return -1;
	}

	memset(filename, 0, length + 1);
	memcpy(filename, filePath, length);

	int nodeCount = (int)(extraInfo[13]);

	vector<SUBNODE> nodes;

	int vCount = 0;
	int mCount = 0;

	for (int i = 0; i < nodeCount; i++) {
		SUBNODE sb;

		memcpy(sb.name, meshInfo[i].name, 256);
		for (int j = 0; j < meshInfo[i].vertCount; j++) {
			VERTEX v;

			v.val[0] = vertBuffer[vCount * 3];
			v.val[1] = vertBuffer[vCount * 3 + 1];
			v.val[2] = vertBuffer[vCount * 3 + 2];

			v.nor.val[0] = normBuffer[vCount * 3];
			v.nor.val[1] = normBuffer[vCount * 3 + 1];
			v.nor.val[2] = normBuffer[vCount * 3 + 2];
			v.nor.val[2] = normBuffer[vCount * 3 + 2];

			sb.vertex.push_back(v);
			vCount++;
		}

		for (int j = 0; j < meshInfo[i].meshCount; j++) {
			TRIANGLE tr;

			tr.idx[0] = meshBuffer[mCount * 3];
			tr.idx[1] = meshBuffer[mCount * 3 + 1];
			tr.idx[2] = meshBuffer[mCount * 3 + 2];

			sb.index.push_back(tr);
			mCount++;
		}

		nodes.push_back(sb);
	}

	int ret = exportGemModel(filename, &nodes);
	free(filename);
	return ret;
}

extern "C" int deallocateMeshData(double *vertBuffer, double *normBuffer, int *meshBuffer, MESHINFO *meshInfo, double *extraInfo) {
	cout << "Deallocating Mesh Data...";
	int nodeCount = (int)extraInfo[13];
	int vCount = 0;
	int mCount = 0;
	int r1 = 0;
	int r2 = 0;
	int r3 = 0;
	int r4 = 0;
	for (int i = 0; i < nodeCount; i++) {
		vCount += meshInfo[i].vertCount;
		mCount += meshInfo[i].meshCount;
	}
	if (vertBuffer != NULL) r1 = munmap(vertBuffer, sizeof(double) * vCount * 3);
	if (normBuffer != NULL) r2 = munmap(normBuffer, sizeof(double) * vCount * 3);
	if (meshBuffer != NULL) r3 = munmap(meshBuffer, sizeof(int) * mCount * 3);
	if (meshInfo != NULL) r4 = munmap(meshInfo, sizeof(MESHINFO) * nodeCount);
	if (!r1 && !r2 && !r3 && !r4) {
		cout << "Success!" << endl;
		return 0;
	}
	else
	{
		cout << "Fail!" << endl;
		return -1;
	}
}

extern "C" void deallocateBuffer(unsigned char *buffer) {
	cout << "Deallocating buffers...Success!" << endl;
	if (buffer != NULL) free(buffer);
}

extern "C" void deallocateBuffer64(unsigned char *buffer, int sz) {
	cout << "Deallocating buffers(x64)...Success!" << endl;
	if (buffer != NULL) munmap(buffer, sz);
}

void initGemPattern(GEM_SIZE_PATTERN *gemPat) {
	gemPat[0].gemSize = 0.0008f; gemPat[0].holeSize = gemPat[0].gemSize * 0.75f; gemPat[0].prongSize = 0.4f;
	gemPat[1].gemSize = 0.0009f; gemPat[1].holeSize = gemPat[1].gemSize * 0.75f; gemPat[1].prongSize = 0.4f;
	gemPat[2].gemSize = 0.001f; gemPat[2].holeSize = gemPat[2].gemSize * 0.75f; gemPat[2].prongSize = 0.4f;
	gemPat[3].gemSize = 0.00115f; gemPat[3].holeSize = gemPat[3].gemSize * 0.75f; gemPat[3].prongSize = 0.43f;
	gemPat[4].gemSize = 0.0012f; gemPat[4].holeSize = gemPat[4].gemSize * 0.75f; gemPat[4].prongSize = 0.43f;
	gemPat[5].gemSize = 0.0013f; gemPat[5].holeSize = gemPat[5].gemSize * 0.75f; gemPat[5].prongSize = 0.45f;
}

/*
int main() {
	GEM_SIZE_PATTERN gemPat[6];
	char *bmpPath = "/mnt/test.bmp";
	char *modelPath = "/mnt/out.glb";
	double **vertBuffer = (double **)malloc(sizeof(double *));
	double **normBuffer = (double **)malloc(sizeof(double *));
	int **meshBuffer = (int **)malloc(sizeof(int *));
	double extraInfo[14];
	MESHINFO **meshInfo = (MESHINFO **)malloc(sizeof(MESHINFO *));

	initGemPattern(gemPat);

	getCurvedGemModel(bmpPath, strlen(bmpPath), gemPat, 6, 0.0013f, 0, 0, vertBuffer, normBuffer, meshBuffer, meshInfo, extraInfo, 2);

	free(vertBuffer);
	free(normBuffer);
	free(meshBuffer);
	free(meshInfo);
	return 0;
}
*/
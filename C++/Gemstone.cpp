// Gemstone.cpp : Defines the entry point for the application.
//

#include "prim.h"
#include "ImgCvt.h"
#include "Gemstone.h"
#include <iostream>
#include <sys/mman.h>

#define MAX_LOADSTRING 100
#define DEPTH_RATE		5.0f

SUBNODE			gem;
SUBNODE			prong;
SUBNODE			lowProng;
SUBNODE			ring;
UCHAR			*gemHole;
Vec2i			neighbor[8];

int		quad = 1;
char	renderText[256];
UCHAR	*matrix = NULL;
long	*intImg = NULL;

char magic[] = { 0x67, 0x6C, 0x54, 0x46, 0x02, 0x00, 0x00, 0x00, 0x04, 0x0C, 0x00, 0x00, 0x10, 0x08, 0x00, 0x00, 0x4A, 0x53, 0x4F, 0x4E };
char space[] = { 0x20, 0x20, 0x20, 0x20 };
char binMark[] = { 0x42, 0x49, 0x4E, 0x00 };
char endMark[] = { 0x00, 0x00, 0x00, 0x00 };

const char *header = "{\"asset\":{\"version\":\"2.0\", \"generator\":\"Microsoft GLTF Exporter 2.4.2.8\"},";
const char *materials = "\"materials\":[{\"pbrMetallicRoughness\":{\"baseColorTexture\":{\"index\":2},\"metallicRoughnessTexture\":{\"index\":3}},"
"\"alphaMode\":\"MASK\","
"\"name\":\"126\","
"\"doubleSided\":true,"
"\"extensions\":{\"KHR_materials_pbrSpecularGlossiness\":{\"diffuseTexture\":{\"index\":0},\"specularGlossinessTexture\":{\"index\":1}}},"
"\"extras\":{\"MSFT_sRGBFactors\":false}}],";

const char *texture = "\"textures\":[{\"name\":\"texture383\",\"sampler\" : 0,\"source\":0},"
"{\"name\":\"texture384\",\"sampler\":0,\"source\":1},"
"{\"name\":\"126_bc\",\"sampler\":0,\"source\":2},"
"{\"name\":\"126_mr\",\"sampler\":0,\"source\":3 }],";
const char *sampler = "\"samplers\":[{\"minFilter\":9985}],";
const char *scene = "\"scenes\":[{\"nodes\":[0]}],\"scene\":0,";
const char *extension = "\"extensionsUsed\":[\"KHR_materials_pbrSpecularGlossiness\"]}";

void ClearBuffer() {
	if (matrix) free(matrix);
	if (intImg) free(intImg);
	matrix = NULL;
	intImg = NULL;
}

void writeBuffer(char **dst, char *src, int size) {
	memcpy(*dst, src, size);
	*dst = *dst + size;
}

void writeBuffer(char **dst, char *src, size_t size) {
	memcpy(*dst, src, size);
	*dst = *dst + size;
}

void PutColor(UCHAR *src, int width, int height, double x, double y, UCHAR r, UCHAR g, UCHAR b) {
	if (x < 0 || x >= width) return;
	if (y < 0 || y >= height) return;
	src[(int)y * width * 3 + (int)x * 3] = r;
	src[(int)y * width * 3 + (int)x * 3 + 1] = g;
	src[(int)y * width * 3 + (int)x * 3 + 2] = b;
}

void DrawLine(UCHAR *src, int width, int height, double x1, double y1, double x2, double y2, UCHAR r, UCHAR g, UCHAR b) {
	double m = sqrtf((float)((x2 - x1) * (x2 - x1) + (y2 - y1) * (y2 - y1)));
	double stpX = (x2 - x1) / (double)m;
	double stpY = (y2 - y1) / (double)m;
	double sX = x1;
	double sY = y1;
	for (int i = 0; i < m; i++) {
		sX += stpX;
		sY += stpY;
		PutColor(src, width, height, sX, sY, r, g, b);
	}
}

void writeContourImage(SHAPE *shp, int width, int height, char *path) {
	UCHAR *dmp = (UCHAR *)malloc(width * height * 3);
	memset(dmp, 0, width * height * 3);
	int p = (int)(shp->prims.size());
	for (int i = 0; i < p; i++) {
		TERMINAL t1 = shp->prims[i]->terms[0];
		TERMINAL t2 = shp->prims[i]->terms[1];

		DrawLine(dmp, width, height, t1.x, t1.y, t2.x, t2.y, 255, 255, 255);
	}

	generateBitmapImage(dmp, height, width, path);
	free(dmp);
}

void writeContourImage(vector<SHAPE> *conts, int width, int height, char *path) {
	UCHAR *dmp = (UCHAR *)malloc(width * height * 3);
	memset(dmp, 0, width * height * 3);
	for (int n = 0; n < (int)(conts->size()); n++) {
		int p = (int)(conts->at(n).prims.size());
		for (int i = 0; i < p; i++) {
			TERMINAL t1 = conts->at(n).prims[i]->terms[0];
			TERMINAL t2 = conts->at(n).prims[i]->terms[1];

			DrawLine(dmp, width, height, t1.x, t1.y, t2.x, t2.y, 255, 255, 255);
		}
	}

	generateBitmapImage(dmp, height, width, path);
	free(dmp);
}

void writeImage(UCHAR *src, int width, int height, char *path) {
	generateBitmapImage(src, height, width, path);
}

void writeMonoImage(UCHAR *src, int width, int height, char *path) {
	UCHAR *dmp = (UCHAR *)malloc(width * height * 3);
	for (int i = 0; i < height; i++) {
		UCHAR *p1 = dmp + i * width * 3;
		UCHAR *p2 = src + i * width;
		for (int j = 0; j < width; j++) {
			p1[j * 3] = p2[j];
			p1[j * 3 + 1] = p2[j];
			p1[j * 3 + 2] = p2[j];
		}
	}
	generateBitmapImage(dmp, height, width, path);
	free(dmp);
}

void IntegralImage(UCHAR *src, long *integral, int width, int height)
{
	for (int j = 0; j < height; j++) {
		for (int i = 0; i < width; i++) {
			long a, b, c;

			if (j > 0) a = integral[(j - 1) * width + i]; else a = 0;
			if (i > 0) b = integral[j * width + (i - 1)]; else b = 0;
			if (i > 0 && j > 0) c = integral[(j - 1) * width + (i - 1)]; else c = 0;

			a = a + b + src[j * width + i] - c;
			integral[j * width + i] = a;
		}
	}
}

UINT getBufferViewLength(vector<SUBNODE> *model, long *length) {
	int n, m;
	const char *suffix = "], ";
	char buf[1024];
	const char *bufferView = "{\"buffer\":0,\"byteOffset\":%d,\"byteLength\":%d%s}";
	const char *bufferViewforImage = "{\"buffer\":0,\"byteOffset\":%d,\"byteLength\":163}";
	const char *bufferviewPrefix = "\"bufferViews\":[";
	const char *images = "\"images\":[{\"bufferView\":%d,\"mimeType\":\"image/png\"},"
		"{\"bufferView\":%d,\"mimeType\":\"image/png\"},"
		"{\"bufferView\":%d,\"mimeType\":\"image/png\"},"
		"{\"bufferView\":%d,\"mimeType\":\"image/png\"}],";

	*length = *length + strlen(bufferviewPrefix);
	n = 0; m = 0;
	for (UINT i = 0; i < model->size(); i++) {
		if (i > 0) *length = *length + 1;

		memset(buf, 0, 1024);
		sprintf(buf, bufferView, m, model->at(i).index.size() * sizeof(UINT) * 3, ",\"target\":34963");
		*length = *length + strlen(buf);

		m += (int)(model->at(i).index.size() * sizeof(UINT) * 3);
		memset(buf, 0, 1024);
		sprintf(buf, bufferView, m, model->at(i).vertex.size() * sizeof(float) * 3, ",\"target\":34962");
		*length = *length + 1;
		*length = *length + strlen(buf);

		m += (int)(model->at(i).vertex.size() * sizeof(float) * 3);
		memset(buf, 0, 1024);
		sprintf(buf, bufferView, m, model->at(i).vertex.size() * sizeof(float) * 3, ",\"target\":34962");
		*length = *length + 1;
		*length = *length + strlen(buf);

		m += (int)(model->at(i).vertex.size() * sizeof(float) * 3);
		memset(buf, 0, 1024);
		sprintf(buf, bufferView, m, model->at(i).vertex.size() * sizeof(float) * 2, ",\"target\":34962");
		*length = *length + 1;
		*length = *length + strlen(buf);

		m += (int)(model->at(i).vertex.size() * sizeof(float) * 2);
		n += 4;
	}

	for (int i = 0; i < 4; i++) {
		memset(buf, 0, 1024);
		sprintf(buf, bufferViewforImage, m);
		*length = *length + 1;
		*length = *length + strlen(buf);
		m += 163;
	}
	
	*length = *length + strlen(suffix);

	memset(buf, 0, 1024);
	sprintf(buf, "\"buffers\":[{\"byteLength\":%d}],", m + 4);
	*length = *length + strlen(buf);

	memset(buf, 0, 1024);
	sprintf(buf, images, n, n + 1, n + 2, n + 3);
	*length = *length + strlen(buf);
	return m + 4;
}

UINT writeBufferView(vector<SUBNODE> *model, char **buffer) {
	int n, m;
	const char *suffix = "], ";
	char buf[1024];
	const char *bufferView = "{\"buffer\":0,\"byteOffset\":%d,\"byteLength\":%d%s}";
	const char *bufferViewforImage = "{\"buffer\":0,\"byteOffset\":%d,\"byteLength\":163}";
	const char *bufferviewPrefix = "\"bufferViews\":[";
	const char *images = "\"images\":[{\"bufferView\":%d,\"mimeType\":\"image/png\"},"
		"{\"bufferView\":%d,\"mimeType\":\"image/png\"},"
		"{\"bufferView\":%d,\"mimeType\":\"image/png\"},"
		"{\"bufferView\":%d,\"mimeType\":\"image/png\"}],";

	writeBuffer(buffer, (char *)bufferviewPrefix, strlen(bufferviewPrefix));
	n = 0; m = 0;
	for (UINT i = 0; i < model->size(); i++) {
		if (i > 0) {
			writeBuffer(buffer, (char *)",", 1);
		}

		memset(buf, 0, 1024);
		sprintf(buf, bufferView, m, model->at(i).index.size() * sizeof(UINT) * 3, ",\"target\":34963");
		writeBuffer(buffer, buf, strlen(buf));

		m += (int)(model->at(i).index.size() * sizeof(UINT) * 3);
		memset(buf, 0, 1024);
		sprintf(buf, bufferView, m, model->at(i).vertex.size() * sizeof(float) * 3, ",\"target\":34962");
		writeBuffer(buffer, (char *)",", 1);
		writeBuffer(buffer, buf, strlen(buf));

		m += (int)(model->at(i).vertex.size() * sizeof(float) * 3);
		memset(buf, 0, 1024);
		sprintf(buf, bufferView, m, model->at(i).vertex.size() * sizeof(float) * 3, ",\"target\":34962");
		writeBuffer(buffer, (char *)",", 1);
		writeBuffer(buffer, buf, strlen(buf));

		m += (int)(model->at(i).vertex.size() * sizeof(float) * 3);
		memset(buf, 0, 1024);
		sprintf(buf, bufferView, m, model->at(i).vertex.size() * sizeof(float) * 2, ",\"target\":34962");
		writeBuffer(buffer, (char *)",", 1);
		writeBuffer(buffer, buf, strlen(buf));

		m += (int)(model->at(i).vertex.size() * sizeof(float) * 2);
		n += 4;
	}

	for (int i = 0; i < 4; i++) {
		memset(buf, 0, 1024);
		sprintf(buf, bufferViewforImage, m);
		writeBuffer(buffer, (char *)",", 1);
		writeBuffer(buffer, buf, strlen(buf));
		m += 163;
	}
	writeBuffer(buffer, (char *)suffix, strlen(suffix));

	memset(buf, 0, 1024);
	sprintf(buf, "\"buffers\":[{\"byteLength\":%d}],", m + 4);
	writeBuffer(buffer, buf, strlen(buf));

	memset(buf, 0, 1024);
	sprintf(buf, images, n, n + 1, n + 2, n + 3);
	writeBuffer(buffer, buf, strlen(buf));
	return m + 4;
}

void getAccessorBufferLength(vector<SUBNODE> *model, long *length) {
	int n;
	const char *suffix = "],";
	char buf[1024];
	const char *accessor = "{\"bufferView\":%d,\"componentType\":%d,\"count\":%d,\"type\":%s}";
	const char *accessorPrefix = "\"accessors\":[";

	*length = *length + strlen(accessorPrefix);
	n = 0;
	for (UINT i = 0; i < model->size(); i++) {
		if (i > 0) *length = *length + 1;

		memset(buf, 0, 1024);
		sprintf(buf, accessor, n, 5125, model->at(i).index.size() * 3, "\"SCALAR\"");
		*length = *length + strlen(buf);

		memset(buf, 0, 1024);
		sprintf(buf, accessor, n + 1, 5126, model->at(i).vertex.size(), "\"VEC3\"");
		*length = *length + 1;
		*length = *length + strlen(buf);

		memset(buf, 0, 1024);
		sprintf(buf, accessor, n + 2, 5126, model->at(i).vertex.size(), "\"VEC3\"");
		*length = *length + 1;
		*length = *length + strlen(buf);

		memset(buf, 0, 1024);
		sprintf(buf, accessor, n + 3, 5126, model->at(i).vertex.size(), "\"VEC2\"");
		*length = *length + 1;
		*length = *length + strlen(buf);
		n += 4;
	}
	*length = *length + strlen(suffix);
}

void writeAccessor(vector<SUBNODE> *model, char **buffer) {
	int n;
	const char *suffix = "],";
	char buf[1024];
	const char *accessor = "{\"bufferView\":%d,\"componentType\":%d,\"count\":%d,\"type\":%s}";
	const char *accessorPrefix = "\"accessors\":[";

	writeBuffer(buffer, (char *)accessorPrefix, strlen(accessorPrefix));
	n = 0;
	for (UINT i = 0; i < model->size(); i++) {
		if (i > 0) {
			writeBuffer(buffer, (char *)",", 1);
		}

		memset(buf, 0, 1024);
		sprintf(buf, accessor, n, 5125, model->at(i).index.size() * 3, "\"SCALAR\"");
		writeBuffer(buffer, buf, strlen(buf));

		memset(buf, 0, 1024);
		sprintf(buf, accessor, n + 1, 5126, model->at(i).vertex.size(), "\"VEC3\"");
		writeBuffer(buffer, (char *)",", 1);
		writeBuffer(buffer, buf, strlen(buf));

		memset(buf, 0, 1024);
		sprintf(buf, accessor, n + 2, 5126, model->at(i).vertex.size(), "\"VEC3\"");
		writeBuffer(buffer, (char *)",", 1);
		writeBuffer(buffer, buf, strlen(buf));

		memset(buf, 0, 1024);
		sprintf(buf, accessor, n + 3, 5126, model->at(i).vertex.size(), "\"VEC2\"");
		writeBuffer(buffer, (char *)",", 1);
		writeBuffer(buffer, buf, strlen(buf));
		n += 4;
	}
	writeBuffer(buffer, (char *)suffix, strlen(suffix));
}

void getMeshBufferSize(vector<SUBNODE> *model, long *length) {
	int n = 0;
	const char *suffix = "],";
	char buf[1024];
	const char *meshprefix = "\"meshes\":[";
	//const char *mesh = "{\"name\":\"mesh_id%d\",\"primitives\":[{\"attributes\":{\"POSITION\":%d,\"NORMAL\":%d,\"TEXCOORD_0\":%d},\"indices\":%d,\"material\":0}]}";
	//const char *mesh = "{\"name\":\"%s\",\"primitives\":[{\"attributes\":{\"POSITION\":%d,\"NORMAL\":%d,\"TEXCOORD_0\":%d},\"indices\":%d,\"material\":0}]}";
	const char *mesh = "{\"name\":\"%s\",\"primitives\":[{\"attributes\":{\"POSITION\":%d,\"NORMAL\":%d,\"TEXCOORD_0\":%d},\"indices\":%d}]}";

	*length = *length + strlen(meshprefix);
	for (UINT i = 0; i < model->size(); i++) {
		if (i > 0) *length = *length + 1;

		memset(buf, 0, 1024);
		sprintf(buf, mesh, model->at(i).name, n + 1, n + 2, n + 3, n);
		*length = *length + strlen(buf);
		n += 4;
	}

	*length = *length + strlen(suffix);
}

void writeMesh(vector<SUBNODE> *model, char **buffer) {
	int n = 0;
	const char *suffix = "],";
	char buf[1024];
	const char *meshprefix = "\"meshes\":[";
	//const char *mesh = "{\"name\":\"mesh_id%d\",\"primitives\":[{\"attributes\":{\"POSITION\":%d,\"NORMAL\":%d,\"TEXCOORD_0\":%d},\"indices\":%d,\"material\":0}]}";
	//const char *mesh = "{\"name\":\"%s\",\"primitives\":[{\"attributes\":{\"POSITION\":%d,\"NORMAL\":%d,\"TEXCOORD_0\":%d},\"indices\":%d,\"material\":0}]}";
	const char *mesh = "{\"name\":\"%s\",\"primitives\":[{\"attributes\":{\"POSITION\":%d,\"NORMAL\":%d,\"TEXCOORD_0\":%d},\"indices\":%d}]}";

	writeBuffer(buffer, (char *)meshprefix, strlen(meshprefix));
	for (UINT i = 0; i < model->size(); i++) {
		if (i > 0) writeBuffer(buffer, (char *)",", 1);

		memset(buf, 0, 1024);
		sprintf(buf, mesh, model->at(i).name, n + 1, n + 2, n + 3, n);
		writeBuffer(buffer, buf, strlen(buf));
		n += 4;
	}

	writeBuffer(buffer, (char *)suffix, strlen(suffix));
}

void makeChildrenNodeString(size_t n, char *str, int length) {
	char *tmp = str;

	memset(str, 0, length);
	str[0] = '[';

	for (size_t i = 1; i <= n; i++) {
		tmp = str + strlen(str);

		if (i > 1) {
			sprintf(tmp, ", %d", i);
		} else{
			sprintf(tmp, "%d", i);
		}
	}
	tmp = str + strlen(str);
	tmp[0] = ']';
}

void getNodeBufferSize(vector<SUBNODE> *model, long *length) {
	int n = 0;
	char buf[10000];
	char nodeChildrenString[10000];
	const char *suffix = "],";
	const char *nodeprefix = "\"nodes\":[{\"children\":%s,\"name\":\"root\"},";
	//const char *nodes = "{\"translation\":[0.0,0.0,0.0],\"scale\":[1.0,1.0,1.0],\"mesh\":%d,\"name\":\"node_id%d\"}";
	const char *nodes = "{\"translation\":[0.0,0.0,0.0],\"scale\":[1.0,1.0,1.0],\"mesh\":%d,\"name\":\"%s\"}";

	makeChildrenNodeString(model->size(), nodeChildrenString, 10000);
	memset(buf, 0, 10000);
	sprintf(buf, nodeprefix, nodeChildrenString);
	*length = *length + strlen(buf);

	for (UINT i = 0; i < model->size(); i++)
	{
		if (i > 0) *length = *length + 1;

		memset(buf, 0, 1024);
		sprintf(buf, nodes, n, model->at(i).name);
		*length = *length + strlen(buf);
		n++;
	}
	*length = *length + strlen(suffix);
}

void writeNodes(vector<SUBNODE> *model, char **buffer) {
	int n = 0;
	char buf[10000];
	char nodeChildrenString[10000];
	const char *suffix = "],";
	const char *nodeprefix = "\"nodes\":[{\"children\":%s,\"name\":\"root\"},";
	//const char *nodes = "{\"translation\":[0.0,0.0,0.0],\"scale\":[1.0,1.0,1.0],\"mesh\":%d,\"name\":\"node_id%d\"}";
	const char *nodes = "{\"translation\":[0.0,0.0,0.0],\"scale\":[1.0,1.0,1.0],\"mesh\":%d,\"name\":\"%s\"}";

	makeChildrenNodeString(model->size(), nodeChildrenString, 10000);
	memset(buf, 0, 10000);
	sprintf(buf, nodeprefix, nodeChildrenString);
	writeBuffer(buffer, buf, strlen(buf));

	for (UINT i = 0; i < model->size(); i++)
	{
		if (i > 0) writeBuffer(buffer, (char *)",", 1);

		memset(buf, 0, 1024);
		sprintf(buf, nodes, n, model->at(i).name);
		writeBuffer(buffer, buf, strlen(buf));
		n++;
	}
	writeBuffer(buffer, (char *)suffix, strlen(suffix));
}

void writeJSONSize(FILE *pFile) {
	long	orgPos = ftell(pFile);
	long	jsonSize = orgPos - 20;
	fseek(pFile, 0x0C, SEEK_SET);
	fwrite(&jsonSize, 1, sizeof(uint32_t), pFile);
	fseek(pFile, orgPos, SEEK_SET);
}


int getBinaryDataSize(vector<SUBNODE> *model, long *length) {
	for (UINT i = 0; i < model->size(); i++) {
		*length = *length + model->at(i).index.size() * sizeof(UINT) * 3;
		*length = *length + model->at(i).vertex.size() * sizeof(float) * 3;
		*length = *length + model->at(i).vertex.size() * sizeof(float) * 3;
		*length = *length + model->at(i).vertex.size() * sizeof(float) * 2;
	}

	*length = *length + 652;
	return 0;
}

int writeBinaryData(vector<SUBNODE> *model, char **buffer) {
	int n = 0;
	
	for (UINT i = 0; i < model->size(); i++) {
		void *buf = (void *)malloc(model->at(i).index.size() * sizeof(UINT) * 3);
		if (buf == NULL) {
			cout << "Fail to memory allocation (writeBinatyData)" << endl;
			return -1;
		}
		n = 0;
		for (UINT j = 0; j < model->at(i).index.size(); j++) {
			TRIANGLE t = model->at(i).index[j];
			UINT p0 = t.idx[0]; UINT p1 = t.idx[1]; UINT p2 = t.idx[2];
			((UINT *)buf)[n] = p0; ((UINT *)buf)[n + 1] = p1; ((UINT *)buf)[n + 2] = p2;
			n += 3;
		}
		writeBuffer(buffer, (char *)buf, model->at(i).index.size() * sizeof(UINT) * 3);
		free(buf);

		n = 0;
		buf = (void *)malloc(model->at(i).vertex.size() * sizeof(float) * 3);
		if (buf == NULL) {
			cout << "Fail to memory allocation (writeBinatyData)" << endl;
			return -1;
		}
		for (UINT j = 0; j < model->at(i).vertex.size(); j++) {
			VERTEX t = model->at(i).vertex[j];
			float p0 = (float)t.val[0]; float p1 = (float)t.val[1]; float p2 = (float)t.val[2];
			((float *)buf)[n] = p0; ((float *)buf)[n + 1] = p1; ((float *)buf)[n + 2] = p2;
			n += 3;
		}
		writeBuffer(buffer, (char *)buf, model->at(i).vertex.size() * sizeof(float) * 3);
		n = 0;
		for (UINT j = 0; j < model->at(i).vertex.size(); j++) {
			VECTOR3D t = model->at(i).vertex[j].nor;
			float p0 = (float)t.val[0]; float p1 = (float)t.val[1]; float p2 = (float)t.val[2];
			((float *)buf)[n] = p0; ((float *)buf)[n + 1] = p1; ((float *)buf)[n + 2] = p2;
			n += 3;
		}
		writeBuffer(buffer, (char *)buf, model->at(i).vertex.size() * sizeof(float) * 3);
		n = 0;
		for (UINT j = 0; j < model->at(i).vertex.size(); j++) {
			((float *)buf)[n] = 1; ((float *)buf)[n + 1] = 1;
			n += 2;
		}
		writeBuffer(buffer, (char *)buf, model->at(i).vertex.size() * sizeof(float) * 2);
		free(buf);
	}

	UCHAR buf[652];
	writeBuffer(buffer, (char *)buf, 652);
	return 0;
}

void writeGLBSize(FILE  *pFile) {
	long sz = ftell(pFile);
	fseek(pFile, 0x08, SEEK_SET);
	fwrite(&sz, 1, sizeof(UINT), pFile);
}

int getGlbBufferSize(vector<SUBNODE> *model) {
	if (model->size() == 0) {
		cout << "Invalid Model" << endl;
		return -1;
	}
	///////////////////
	long fileSize = 20 + strlen(header);

	getAccessorBufferLength(model, &fileSize);

	getBufferViewLength(model, &fileSize);

	fileSize += strlen(materials);

	getMeshBufferSize(model, &fileSize);

	getNodeBufferSize(model, &fileSize);

	fileSize += strlen(sampler);

	fileSize += strlen(scene);

	fileSize += strlen(texture);

	fileSize += strlen(extension);

	fileSize += 4; 

	fileSize += sizeof(UINT);

	fileSize += 4;

	getBinaryDataSize(model, &fileSize);

	fileSize += 4; 
	return (int)fileSize;
}

int getGlbFileBuffer(vector<SUBNODE> *model, char *fileBuffer, int glbSize)
{
	UINT bufferSize = 0;
	long jsonSize = 0;
	long fileSize = 20 + strlen(header);

	if (model->size() == 0) {
		cout << "Invalid Model" << endl;
		return -1;
	}
	char *fileData = NULL;

	if (fileBuffer == NULL) {
		cout << "Invalid Buffer for GLB file" << endl;
		return -1;
	}


	getAccessorBufferLength(model, &fileSize);

	getBufferViewLength(model, &fileSize);

	fileSize += strlen(materials);

	getMeshBufferSize(model, &fileSize);

	getNodeBufferSize(model, &fileSize);

	fileSize += strlen(sampler);

	fileSize += strlen(scene);

	fileSize += strlen(texture);

	fileSize += strlen(extension);

	fileSize += 4; jsonSize = (fileSize - 20);

	fileData = fileBuffer;
	////////////////////////////////////////
	//Move2Center(model);

	writeBuffer(&fileData, magic, 20);
	writeBuffer(&fileData, (char *)header, strlen(header));
	
	writeAccessor(model, &fileData);

	bufferSize = writeBufferView(model, &fileData);

	writeBuffer(&fileData, (char *)materials, strlen(materials));

	writeMesh(model, &fileData);

	writeNodes(model, &fileData);

	writeBuffer(&fileData, (char *)sampler, strlen(sampler));

	writeBuffer(&fileData, (char *)scene, strlen(scene));

	writeBuffer(&fileData, (char *)texture, strlen(texture));

	writeBuffer(&fileData, (char *)extension, strlen(extension));

	writeBuffer(&fileData, (char *)space, 4);

	memcpy(fileBuffer + 0x0C, (char *)&jsonSize, sizeof(uint32_t));

	writeBuffer(&fileData, (char *)&bufferSize, sizeof(UINT));

	writeBuffer(&fileData, binMark, 4);

	if (writeBinaryData(model, &fileData) != 0) {
		return -1;
	}

	writeBuffer(&fileData, endMark, 4);
	
	memcpy(fileBuffer + 0x08, (char *)&glbSize, sizeof(uint32_t));

	return 0;
}

int exportSTL(vector<SUBNODE> *model, char *fileName) {
	if (model->size() == 0) return -3;
	char header[80];
	FILE *pFile = fopen(fileName, "wb");
	UINT facets = 0;

	if (pFile == NULL) return -1;
	Move2Center(model);
	for (UINT i = 0; i < model->size(); i++) {
		facets += (UINT)(model->at(i).index.size());
	}
	memset(header, 0, 80);
	strcpy(header, "Gemstone Generator");
	fwrite(header, 1, 80, pFile);
	fwrite(&facets, 1, sizeof(UINT), pFile);

	for (UINT i = 0; i < model->size(); i++) {
		for (UINT j = 0; j < model->at(i).index.size(); j++) {
			char buf[50];
			float *facet = (float *)buf;

			facet[0] = (float)model->at(i).index[j].param[0];
			facet[1] = (float)model->at(i).index[j].param[1];
			facet[2] = (float)model->at(i).index[j].param[2];
			for (int k = 0; k < 3; k++) {
				int n = model->at(i).index[j].idx[k];
				facet[2 + k * 3 + 1] = (float)model->at(i).vertex[n].val[0];
				facet[2 + k * 3 + 2] = (float)model->at(i).vertex[n].val[1];
				facet[2 + k * 3 + 3] = (float)model->at(i).vertex[n].val[2];
			}
			buf[48] = 0; buf[49] = 0;
			fwrite(buf, 1, 50, pFile);
		}
	}
	fclose(pFile);
	return 0;
}

void threshold(UCHAR *frm, int width, int height, int bwThreshold) {
	for (int i = 0; i < height; i++) {
		UCHAR *ptr = frm + i * width;
		for (int j = 0; j < width; j++) {
			if (ptr[j] <= bwThreshold) {
				ptr[j] = 0;
			}
		}
	}
}

bool toThicker(UCHAR *src, int width, int height, int thres, int iterate) {
	UCHAR *tmp = NULL;
	try
	{
		tmp = (UCHAR *)malloc(width * height * 4);
		if (tmp == NULL) return false;
	}
	catch (const std::exception&)
	{
		return false;
	}
	for (int n = 0; n < iterate; n++) {
		memset(tmp, 0, width * height * 4);

		for (int i = 1; i < height - 1; i++) {
			UCHAR *p1 = src + (i - 1) * width;
			UCHAR *p2 = src + i * width;
			UCHAR *p3 = src + (i + 1) * width;
			UCHAR *ptrDst = tmp + i * width;

			for (int j = 1; j < width - 1; j++) {
				if (p2[j] > thres) continue;
				int n = 0;

				n += p1[j] > thres ? 1 : 0;
				n += p2[j - 1] > thres ? 1 : 0;
				n += p2[j + 1] > thres ? 1 : 0;
				n += p3[j] > thres ? 1 : 0;
				if (n > 0) ptrDst[j] = 255;
			}
		}

		for (int i = 0; i < height; i++) {
			UCHAR *ptr = src + i * width;
			UCHAR *ptrTmp = tmp + i * width;
			for (int j = 0; j < width; j++) {
				if (ptrTmp[j] > 0) ptr[j] = 255;
			}
		}
	}
	free(tmp);
	return true;
}

bool toThinner(UCHAR *src, int width, int height, int thres, int iterate)
{
	UCHAR *tmp = NULL;
	try
	{
		tmp = (UCHAR *)malloc(width * height);
		if (tmp == NULL) return false;
	}
	catch (const std::exception&)
	{
		return false;
	}

	for (int n = 0; n < iterate; n++) {
		memset(tmp, 0, width * height);

		for (int i = 1; i < height - 1; i++) {
			UCHAR *p1 = src + (i - 1) * width;
			UCHAR *p2 = src + i * width;
			UCHAR *p3 = src + (i + 1) * width;
			UCHAR *ptrDst = tmp + i * width;

			for (int j = 1; j < width - 1; j++) {
				if (p2[j] == 0) continue;
				int n = 0;

				n += p1[j] == 0 ? 1 : 0;
				n += p2[j - 1] == 0 ? 1 : 0;
				n += p2[j + 1] == 0 ? 1 : 0;
				n += p3[j] == 0 ? 1 : 0;
				if (n > 0) ptrDst[j] = 255;
			}
		}

		for (int i = 0; i < height; i++) {
			UCHAR *ptr = src + i * width;
			UCHAR *ptrTmp = tmp + i * width;
			for (int j = 0; j < width; j++) {
				if (ptrTmp[j] > 0) ptr[j] = 0;
			}
		}
	}
	free(tmp);
	return true;
}

void Clamp(UCHAR *src, int width, int height) {
	for (int i = 0; i < height; i++) {
		UCHAR *ptr = src + i * width;
		for (int j = 0; j < width; j++) {
			if (ptr[j] > 230) ptr[j] = 230;
		}
	}
}

void ExtractGemProngPosition(vector<GEM> *gems, vector<PRONG> *prongs, vector<RING> *rings, UCHAR *src, int width, int height) {
	gems->clear();
	prongs->clear();
	double mn = 0;
	vector<Vec2i> prB;
	vector<Vec2i> gmB;

	for (int i = 0; i < height; i++) {
		UCHAR *p = src + i * (width * 3);
		for (int j = 0; j < width; j++) {
			int r = (int)p[j * 3 + 2];
			int g = (int)p[j * 3 + 1];
			int b = (int)p[j * 3];

			if (r > 50 && g > 50 && b < 10) {
				prB.push_back(Vec2i(j, i));
			}

			if (r < 10 && g > 100 && b < 10) {
				gmB.push_back(Vec2i(j, i));
			}

			if (b > 50 && r < 10 && g < 10) {
				GEM gm;

				gm.pos.val[0] = (float)j; gm.pos.val[1] = (float)i; gm.pos.val[2] = 0;
				gm.uve[0] = VERTEX(0, 1, 0); gm.uve[1] = VERTEX(1, 0, 0); gm.uve[2] = VERTEX(0, 0, 1);
				gems->push_back(gm);

				for (int x = -ROI; x <= ROI; x++) {
					UCHAR *tPtr = src + ((i + x) * (width * 3));
					for (int y = -ROI; y <= ROI; y++) {
						tPtr[(j + y) * 3 + 2] = 0;
						tPtr[(j + y) * 3 + 1] = 0;
						tPtr[(j + y) * 3] = 0;
					}
				}
			}

			if (r > 50 && g < 10 && b < 10) {
				PRONG pr;
				pr.pos.val[0] = (float)j; pr.pos.val[1] = (float)i; pr.pos.val[2] = 0;
				pr.uve[0] = VERTEX(0, 1, 0); pr.uve[1] = VERTEX(1, 0, 0); pr.uve[2] = VERTEX(0, 0, 1);
				prongs->push_back(pr);

				for (int x = -ROI; x <= ROI; x++) {
					UCHAR *tPtr = src + ((i + x) * (width * 3));
					for (int y = -ROI; y <= ROI; y++) {

						tPtr[(j + y) * 3 + 2] = 0;
						tPtr[(j + y) * 3 + 1] = 0;
						tPtr[(j + y) * 3] = 0;
					}
				}
			}

			if (r < 10 && g > 50 && b > 50) {
				RING rg;
				rg.pos = VERTEX((double)j, (double)i, 0.0f);
				rg.uve[0] = VERTEX(0, 1, 0); rg.uve[1] = VERTEX(1, 0, 0); rg.uve[2] = VERTEX(0, 0, 1);
				rings->push_back(rg);

				for (int x = -ROI; x <= ROI; x++) {
					UCHAR *tPtr = src + ((i + x) * (width * 3));
					for (int y = -ROI; y <= ROI; y++) {
						int r = (int)tPtr[(j + y) * 3 + 2];
						int g = (int)tPtr[(j + y) * 3 + 1];
						int b = (int)tPtr[(j + y) * 3];

						if (r < 10 && g > 50 && b > 50) {
							tPtr[(j + y) * 3 + 2] = 0;
							tPtr[(j + y) * 3 + 1] = 0;
							tPtr[(j + y) * 3] = 0;
						}
					}
				}
			}
		}
	}

	for (UINT i = 0; i < prongs->size(); i++) {
		mn = M_INFINITE;
		double mnX = -1;
		double mnY = -1;
		double x1 = prongs->at(i).pos.val[0];
		double y1 = prongs->at(i).pos.val[1];

		for (UINT j = 0; j < prB.size(); j++) {
			double x2 = (double)prB[j].val[0];
			double y2 = (double)prB[j].val[1];
			double m = sqrt((x2 - x1) * (x2 - x1) + (y2 - y1) * (y2 - y1));
			if (mn > m) {
				mn = m;
				mnX = x2; mnY = y2;
			}
		}
		if (mnX > 0 && mnY > 0) {
			prongs->at(i).rotation = GetAngleBetweenTerminal(TERMINAL(x1, y1), TERMINAL(mnX, mnY));
			prongs->at(i).size = mn;
		}
	}

	for (UINT i = 0; i < gems->size(); i++) {
		mn = M_INFINITE;
		double mnX = -1;
		double mnY = -1;
		double x1 = gems->at(i).pos.val[0];
		double y1 = gems->at(i).pos.val[1];

		for (UINT j = 0; j < gmB.size(); j++) {
			double x2 = (double)gmB[j].val[0];
			double y2 = (double)gmB[j].val[1];
			double m = sqrt((x2 - x1) * (x2 - x1) + (y2 - y1) * (y2 - y1));
			if (mn > m) {
				mn = m;
				mnX = x2; mnY = y2;
			}
		}
		if (mnX > 0 && mnY > 0) {
			gems->at(i).radius = mn * 1.1f;
		}
	}
	
	for (UINT i = 0; i < gems->size(); i++) {
		for (UINT j = 0; j < prongs->size(); j++) {
			VERTEX v1 = gems->at(i).pos;
			VERTEX v2 = prongs->at(j).pos;
			v1 = doMinus(v1, v2);
			double s = v1.getMagnitude();
			if (s < gems->at(i).radius + prongs->at(j).size * 1.5f) {
				prongs->at(j).nGems.push_back(i);
			}
		}
	}
}

void placeGems(vector<GEM> *gems, vector<SUBNODE> *nodes, double scaleFactor) {
	tick32_t et = get_tick_count();

	const char *meshName = "GemStone";
	
	for (UINT i = 0; i < gems->size(); i++)
	{
		SUBNODE p;
		cloneNodes(&gem, &p);
		DoScale(&p, gems->at(i).radius * 2.0f);
		//cout << "Gem " << i << " size =" << gems->at(i).radius * scaleFactor * 2000.0f << "mm" << endl;
		MATRIX mat = getUVEMatrix(gems->at(i).uve, VERTEX(0, 0, 0));
		doSubnodeTransform(&p, mat);

		double offset = gems->at(i).radius * 0.18f;
		VERTEX gd = gems->at(i).uve[2];
		VERTEX v = gems->at(i).pos;

		gd.doNormalize();
		v.val[0] -= gd.val[0] * offset;
		v.val[1] -= gd.val[1] * offset;
		v.val[2] -= gd.val[2] * offset;


		Move2Position(&p, v.val[0], v.val[1], v.val[2]);
		SUBNODE subnode;
		for (UINT j = 0; j < p.index.size(); j++) {
			subnode.index.push_back(p.index[j]);
		}
		for (UINT j = 0; j < p.vertex.size(); j++) {
			subnode.vertex.push_back(p.vertex[j]);
		}
		subnode.putName(meshName, i);
		nodes->push_back(subnode);
	}
	et = get_tick_count() - et;
	cout << "Placing Gems..." << et << " ms." << endl;
}

void cutRings(vector<SHAPE> *conts, vector<RING> *rings, vector<SUBNODE> *ringNodes) {
	tick32_t et = get_tick_count();

	cout << "Cutting Rings...";
	for (UINT n = 0; n < rings->size(); n++) {
		SUBNODE subnode;
		SUBNODE p;

		cloneNodes(&ring, &p);
		DoScale(&p, rings->at(n).radius * 2.0f);
		Move2Position(&p, rings->at(n).pos.val[0], rings->at(n).pos.val[1], rings->at(n).pos.val[2]);
		for (UINT j = 0; j < p.index.size(); j++) {
			bool flag = false;

			for (int k = 0; k < 3; k++) {
				int idx = p.index[j].idx[k];
				TERMINAL t = TERMINAL(p.vertex[idx].val[0], p.vertex[idx].val[1]);
				
				if (conts->at(0).isInsidePoint(t) == false) {
					flag = true;
					break;
				}
			}

			if (flag) {
				TRIANGLE tr;

				for (int k = 0; k < 3; k++) {
					int idx = ring.index[j].idx[k];
					subnode.vertex.push_back(ring.vertex[idx]);
					tr.idx[k] = (int)subnode.vertex.size() - 1;
				}

				subnode.index.push_back(tr);
			}
		}
		OptimizeMesh(&subnode, true, 50.0f);
		ringNodes->push_back(subnode);
	}

	et = get_tick_count() - et;
	cout << et << "ms." << endl;
}

void cutPorngs(vector<GEM> *gems, vector<PRONG> *prongs, vector<SUBNODE> *prongNodes, int option) {
	tick32_t et = get_tick_count();

	for (UINT i = 0; i < prongs->size(); i++)
	{
		SUBNODE p;
		if (option == 1) {
			cloneNodes(&lowProng, &p);
		}
		else {
			cloneNodes(&prong, &p);
		}
		DoScale(&p, prongs->at(i).size * 2.0f);

		MATRIX mat = zRotateMatrix(prongs->at(i).rotation * PI / 180.0f);
		doSubnodeTransform(&p, mat);

		if (option == 2 || option == 3) {
			for (UINT j = 0; j < prongs->at(i).nGems.size(); j++) {
				double x1 = gems->at(prongs->at(i).nGems[j]).pos.val[0] - prongs->at(i).pos.val[0];
				double y1 = gems->at(prongs->at(i).nGems[j]).pos.val[1] - prongs->at(i).pos.val[1];
				cutProng(&p, VERTEX(x1, y1, 0), gems->at(prongs->at(i).nGems[j]).radius * 1.05f);
			}
		}
		prongNodes->push_back(p);
	}
	et = get_tick_count() - et;
}

void placeProngs(vector<PRONG> *prongs, vector<SUBNODE> *prongNodes, vector<SUBNODE> *nodes) {
	tick32_t et = get_tick_count();

	const char *meshName = "Prong";
	cout << "Placing Prongs ...";
	for (UINT i = 0; i < prongs->size(); i++)
	{
		MATRIX mat = getUVEMatrix(prongs->at(i).uve, VERTEX(0, 0, 0));
		doSubnodeTransform(&(prongNodes->at(i)), mat);

		Move2Position(&(prongNodes->at(i)), prongs->at(i).pos.val[0], prongs->at(i).pos.val[1], prongs->at(i).pos.val[2]);
		SUBNODE subnode;
		for (UINT j = 0; j < prongNodes->at(i).index.size(); j++) {
			subnode.index.push_back(prongNodes->at(i).index[j]);
		}
		for (UINT j = 0; j < prongNodes->at(i).vertex.size(); j++) {
			subnode.vertex.push_back(prongNodes->at(i).vertex[j]);
		}
		subnode.putName(meshName, i);
		nodes->push_back(subnode);
	}
	et = get_tick_count() - et;
	cout << et << " ms." << endl;
}

void placeRings(vector<RING> *rings, vector<SUBNODE> *ringNodes, vector<SUBNODE> *nodes, double depth) {
	tick32_t et = get_tick_count();

	cout << "Placing Chain Rings ...";
	const char *meshName = "Ring";
	double offset = depth * 0.5f;
	for (UINT i = 0; i < rings->size(); i++)
	{
		DoScale(&ringNodes->at(i), rings->at(i).radius * 2.0f);
		MATRIX mat = getUVEMatrix(rings->at(i).uve, VERTEX(0, 0, 0));
		doSubnodeTransform(&ringNodes->at(i), mat);

		VERTEX gd = rings->at(i).uve[2];
		VERTEX v = rings->at(i).pos;
		
		gd.doNormalize();
		v.val[0] -= gd.val[0] * offset;
		v.val[1] -= gd.val[1] * offset;
		v.val[2] -= gd.val[2] * offset;

		Move2Position(&ringNodes->at(i), v.val[0], v.val[1], v.val[2]);
		SUBNODE subnode;

		for (UINT j = 0; j < ringNodes->at(i).vertex.size(); j++) {
			subnode.vertex.push_back(ringNodes->at(i).vertex[j]);
		}

		for (UINT j = 0; j < ringNodes->at(i).index.size(); j++) {
			subnode.index.push_back(ringNodes->at(i).index[j]);
		}

		subnode.putName(meshName, i);
		nodes->push_back(subnode);
	}
	et = get_tick_count() - et;
	cout << et << " ms." << endl;
}

void reSizeModel(vector<GEM> *gems, vector<SUBNODE> *nodes) {
	double mn = 0;

	for (UINT i = 0; i < gems->size(); i++) {
		if (i == 0) {
			mn = gems->at(i).radius;
		}
		else if (mn > gems->at(i).radius * 2.0f) {
			mn = gems->at(i).radius;
		}
	}
	DoScale(nodes, 0.8f / mn);
}	

void revertImage(UCHAR *src, int width, int height) {
	for (int i = 0; i < height; i++) {
		UCHAR *ptr = src + i * width;
		for (int j = 0; j < width; j++) {
			ptr[j] = (UCHAR)(255 - ptr[j]);
		}
	}
}

bool crop_extend(UCHAR *src, int width, int height,
	UCHAR **dst, int *newWidth, int *newHeight, int margin) {
	int mnI = height;
	int mnJ = width;
	int mxI = 0;
	int mxJ = 0;

	for (int i = 0; i < height; i++) {
		UCHAR *ptr = src + i * width * 3;
		for (int j = 0; j < width; j++) {
			if (ptr[j * 3] == 255 && ptr[j * 3 + 1] == 255 && ptr[j * 3 + 2] == 255) continue;
			if (i < mnI) mnI = i;
			if (j < mnJ) mnJ = j;
			if (i > mxI) mxI = i;
			if (j > mxJ) mxJ = j;
		}
	}

	*newWidth = mxJ - mnJ + 1 + margin * 2;
	*newHeight = mxI - mnI + 1 + margin * 2;

	if (*newWidth < 1 || *newHeight < 1) return false;
	*dst = (UCHAR *)malloc((*newWidth) * (*newHeight) * 3);
	if (*dst == NULL) return false;
	memset(*dst, 255, (*newWidth) * (*newHeight) * 3);

	for (int i = mnI; i <= mxI; i++) {
		UCHAR *pSrc = src + i * width * 3;
		UCHAR *pDst = *dst + (i + margin - mnI) * (*newWidth) * 3;
		for (int j = mnJ; j <= mxJ; j++) {
			pDst[(j - mnJ + margin) * 3] = pSrc[j * 3];
			pDst[(j - mnJ + margin) * 3 + 1] = pSrc[j * 3 + 1];
			pDst[(j - mnJ + margin) * 3 + 2] = pSrc[j * 3 + 2];
		}
	}
	return true;
}


void extractContourPixels(UCHAR *src, int width, int height) {
	UCHAR *tmp = (UCHAR *)malloc(width * height);
	memset(tmp, 0, width * height);

	for (int i = 1; i < height - 1; i++) {
		UCHAR *p1 = src + (i - 1) * width;
		UCHAR *p2 = src + i * width;
		UCHAR *p3 = src + (i + 1) * width;
		UCHAR *ptrDst = tmp + i * width;

		for (int j = 1; j < width - 1; j++) {
			if (p2[j] == 0) continue;
			int n = 0;

			n += p1[j] == 0 ? 1 : 0;
			n += p2[j - 1] == 0 ? 1 : 0;
			n += p2[j + 1] == 0 ? 1 : 0;
			n += p3[j] == 0 ? 1 : 0;
			if (n > 0) ptrDst[j] = 255;
		}
	}

	memcpy(src, tmp, width * height);
	free(tmp);
}

void unifyContour(vector<SHAPE> *contGroup) {
	vector<SHAPE> shps;
	TERMINAL t;
	int retIdx;
	tick32_t et = get_tick_count();

	cout << "Unifying Contour...";

	for (UINT n = 0; n < contGroup->size(); n++) {
		contGroup->at(n).isValid = true;
	}
	for (UINT n = 1; n < contGroup->size(); n++) {
		if (contGroup->at(n).isValid == false) continue;
		int primCount = (int)(contGroup->at(n).prims.size());
		for (int i = 0; i < primCount; i++) {
			TERMINAL t1 = contGroup->at(n).prims[i]->terms[0];
			if (t1.isValid == false) continue;
			bool flag = true;
			
			for (UINT m = 0; m < contGroup->size(); m++) {
				if (contGroup->at(m).isValid == false) continue;
				if (m == n) continue;
				for (UINT j = 0; j < contGroup->at(m).prims.size(); j++) {
					TERMINAL t2 = contGroup->at(m).prims[j]->terms[0];
					if (t2.isValid == false) continue;
					LINE ln = LINE(t1, t2);
					flag = true;
					for (UINT p = 0; p < contGroup->size(); p++) {
						if (contGroup->at(p).isValid == false) continue;
						if (contGroup->at(p).getIntersection(&ln, &t, &retIdx) == true) {
							flag = false;
							break;
						}
					}
					if (flag) {
						contGroup->at(n).prims[i]->terms[0].isValid = false;
						contGroup->at(m).prims[j]->terms[0].isValid = false;
						t1.isValid = false;
						t2.isValid = false;
						contGroup->at(m).prims.insert(contGroup->at(m).prims.begin() + j,
							new LINE(t1, t2));
						int pCount = (int)(contGroup->at(n).prims.size());
						SHAPE shp;
						for (int p = 0; p < pCount; p++) {
							int k = i + p >= pCount ? i + p - pCount : i + p;
							shp.prims.push_back(contGroup->at(n).prims[k]);
						}
						contGroup->at(m).prims.insert(contGroup->at(m).prims.begin() + j,
							shp.prims.begin(), shp.prims.end());
						contGroup->at(m).prims.insert(contGroup->at(m).prims.begin() + j,
							new LINE(t2, t1));
						contGroup->at(n).isValid = false;
						break;
					}
				}
				if (flag) break;
			}
			if (flag) break;
		}
	}

	et = get_tick_count() - et;
	cout << et << " ms." << endl;
}

int excludeModel(vector<SUBNODE> *nodes1, vector<SUBNODE> *nodes2) {
	return 0;
}

int getClosedContour(UCHAR *src, int width, vector<Vec2i> *path, int pi, int pj, bool apartFlag, int nCount) {
	if (nCount > 1000) return -1;
	int ci = -1; int cj = -1;
	for (int k = 0; k < 8; k++) {
		int i1 = pi + neighbor[k].val[0];
		int j1 = pj + neighbor[k].val[1];
		if (src[i1 * width + j1] == 0) continue;
		vector<Vec2i> curPath;
		bool flag = abs(ci - path->at(0).val[0]) > 1 || abs(cj - path->at(0).val[1]) > 1 ?
			true : false;
		ci = i1; cj = j1;
		path->push_back(Vec2i(ci, cj));
		src[ci * width + cj] = 0;
		if (apartFlag && abs(ci - path->at(0).val[0]) < 2 && abs(cj - path->at(0).val[1]) < 2) {
			return 1;
		}
		int ret = getClosedContour(src, width, path, ci, cj, flag, nCount + 1);
		if (ret != 0) return ret;
		if (ret == 0) path->pop_back();
	}
	return 0;
}

void generateFlatMeshes(SHAPE *cont, SUBNODE *sb) {
	vector<TERMINAL> verts;
	vector<int>		vertIndexs;
	int prevVertCount = (int)sb->vertex.size();
	//tick32_t et = get_tick_count();

	//cout << "Generating Flat Mesh...";

	for (UINT i = 0; i < cont->prims.size(); i++) {
		verts.push_back(cont->prims[i]->terms[0]);
		VERTEX v = VERTEX(cont->prims[i]->terms[0].x, cont->prims[i]->terms[0].y, 0);
		sb->vertex.push_back(v);
		vertIndexs.push_back(i);
	}

	while (verts.size() > 2) {
		int vCount = (int)vertIndexs.size();

		for (int i = 0; i < (int)vertIndexs.size() - 1; i++) {
			TRIANGLE tr;
			int p = (int)i - 1 < 0 ? (int)vertIndexs.size() - 1 : i - 1;
			int j = i + 1;
			int k = i + 2 >= (int)vertIndexs.size() ? 0 : i + 2;
			int m = i + 3 >= (int)vertIndexs.size() ? i + 3 - (int)vertIndexs.size() : i + 3;
			LINE ln1 = LINE(verts[vertIndexs[i]], verts[vertIndexs[j]]);
			LINE ln2 = LINE(verts[vertIndexs[j]], verts[vertIndexs[k]]);
			LINE ln3 = LINE(verts[vertIndexs[k]], verts[vertIndexs[i]]);
			SHAPE shp;
			TERMINAL t;
			int retIndex;

			shp.prims.push_back(&ln1);
			shp.prims.push_back(&ln2);
			shp.prims.push_back(&ln3);
			shp.isPositive = true;

			if (cont->getIntersection(&ln3, &t, &retIndex)) continue;

			VERTEX v1 = ln1.getPositiveDirection();
			VERTEX v2 = ln3.getNegativeDirection();

			v1 = doCross(v1, v2);
			if (v1.val[2] < 0 || shp.isInsidePoint(verts[m]) || shp.isInsidePoint(verts[p])) continue;

			tr.idx[0] = prevVertCount + vertIndexs[k];
			tr.idx[1] = prevVertCount + vertIndexs[i];
			tr.idx[2] = prevVertCount + vertIndexs[j];
			sb->index.push_back(tr);
			vertIndexs.erase(vertIndexs.begin() + j);
		}
		if (vCount < 3) break;
		if (vCount == (int)(vertIndexs.size())) break;
	}
	verts.clear();
	vertIndexs.clear();
	//et = get_tick_count() - et;
	//cout << "Vertics : " << sb->vertex.size() << ", Triangles : " << sb->index.size() << ", " << et << " ms." << endl;
}

void reverse(UCHAR *src, int width, int height) {
	for (int i = 0; i < height; i++) {
		UCHAR *p = src + i * width;
		for (int j = 0; j < width; j++) {
			p[j] = (UCHAR)(255 - p[j]);
		}
	}
}

void cvt2Gray(UCHAR *src, int width, int height, UCHAR *dst) {
	for (int i = 0; i < height; i++) {
		UCHAR *p1 = src + i * width * 3;
		UCHAR *p2 = dst + i * width;
		for (int j = 0; j < width; j++) {
			UCHAR r = p1[j * 3 + 2];
			UCHAR g = p1[j * 3 + 1];
			UCHAR b = p1[j * 3];
			float s = (float)(r + g + b);

			s /= 3.0f;
			p2[j] = (UCHAR)s;
		}
	}
}

TERMINAL curvePoint(double x, double y, TERMINAL tCenter, int width, double xTwist) {
	if (xTwist == 0) return TERMINAL(x, y);
	double radius = tCenter.y - y;
	double length = x - tCenter.x;
	double delta = length * (xTwist * 0.5f) / ((double)width / 2.0f);
	return TERMINAL(tCenter.x + radius * sin(delta * PI / 180.0f), tCenter.y - radius * cos(delta * PI / 180.0f));
}

void curvePath(vector<Vec2i> *path, TERMINAL tCenter, int width, double xTwist) {
	if (xTwist == 0) return;
	for (int i = 0; i < (int)(path->size()); i++) {
		double x = path->at(i).val[1];
		double y = path->at(i).val[0];
		TERMINAL t = curvePoint(x, y, tCenter, width, xTwist);
		path->at(i).val[1] = (int)(t.x);
		path->at(i).val[0] = (int)(t.y);
	}
}

void curveGemProng(vector<GEM> *gems, vector<PRONG> *prongs, vector<RING> *rings, TERMINAL tCenter, int width, double xTwist) {
	for (UINT i = 0; i < gems->size(); i++) {
		TERMINAL t = curvePoint(gems->at(i).pos.val[0], gems->at(i).pos.val[1], tCenter, width, xTwist);
		gems->at(i).pos.val[0] = (float)t.x;
		gems->at(i).pos.val[1] = (float)t.y;
	}
	for (UINT i = 0; i < prongs->size(); i++) {
		TERMINAL t = curvePoint(prongs->at(i).pos.val[0], prongs->at(i).pos.val[1], tCenter, width, xTwist);
		prongs->at(i).pos.val[0] = (float)t.x;
		prongs->at(i).pos.val[1] = (float)t.y;
	}
	for (UINT i = 0; i < rings->size(); i++) {
		TERMINAL t = curvePoint(rings->at(i).pos.val[0], rings->at(i).pos.val[1], tCenter, width, xTwist);
		rings->at(i).pos.val[0] = (float)t.x;
		rings->at(i).pos.val[1] = (float)t.y;
	}
}

TERMINAL curvePointEllipse(double x, double y, TERMINAL tCenter, int width,
	double r1, double r2, double xTwist) {
	if (xTwist == 0) return TERMINAL(x, y);
	double radius = tCenter.y - y;
	double length = x - tCenter.x;
	double delta = length * (xTwist * 0.5f) / ((double)width / 2.0f);
	delta = 90.0f - delta;
	return TERMINAL(tCenter.x + (r2 + radius - r1) * cos(delta * PI / 180.0f), tCenter.y - radius * sin(delta * PI / 180.0f));
}

void curvePathEllipse(vector<Vec2i> *path, TERMINAL tCenter, int width, double r1, double r2, 
	double xTwist) {
	if (xTwist == 0) return;
	for (int i = 0; i < (int)(path->size()); i++) {
		double x = path->at(i).val[1];
		double y = path->at(i).val[0];
		TERMINAL t = curvePointEllipse(x, y, tCenter, width, r1, r2, xTwist);
		path->at(i).val[1] = (int)(t.x);
		path->at(i).val[0] = (int)(t.y);
	}
}

void curveGemProngEllipse(vector<GEM> *gems, vector<PRONG> *prongs, vector<RING> *rings, TERMINAL tCenter, int width,
	double r1, double r2, double xTwist) {
	for (UINT i = 0; i < gems->size(); i++) {
		TERMINAL t = curvePointEllipse(gems->at(i).pos.val[0], gems->at(i).pos.val[1], tCenter, width, r1, r2, xTwist);
		gems->at(i).pos.val[0] = (float)t.x;
		gems->at(i).pos.val[1] = (float)t.y;
	}
	for (UINT i = 0; i < prongs->size(); i++) {
		TERMINAL t = curvePointEllipse(prongs->at(i).pos.val[0], prongs->at(i).pos.val[1], tCenter, width, r1, r2, xTwist);
		prongs->at(i).pos.val[0] = (float)t.x;
		prongs->at(i).pos.val[1] = (float)t.y;
	}

	for (UINT i = 0; i < rings->size(); i++) {
		TERMINAL t = curvePointEllipse(rings->at(i).pos.val[0], rings->at(i).pos.val[1], tCenter, width, r1, r2, xTwist);
		rings->at(i).pos.val[0] = (float)t.x;
		rings->at(i).pos.val[1] = (float)t.y;
	}
}

void smoothenContour(vector<SHAPE> *conts) {
	tick32_t et = get_tick_count();

	cout << "Smoothing Contour...";
	//Smoothen contours
	for (int n = 0; n < (int)conts->size(); n++) {
		for (int i = 0; i < (int)(conts->at(n).prims.size()); i++) {
			int j = i + 1 >= (int)(conts->at(n).prims.size()) ? 0 : i + 1;
			double m1 = conts->at(n).prims[i]->getMagnitude();
			double m2 = conts->at(n).prims[j]->getMagnitude();
			VERTEX g1 = conts->at(n).prims[i]->getGradient();
			VERTEX g2 = conts->at(n).prims[j]->getGradient();
			VERTEX d1 = conts->at(n).prims[i]->getPositiveDirection();
			VERTEX d2 = conts->at(n).prims[j]->getPositiveDirection();
			d2 = doCross(d1, d2);
			if (d2.val[2] < 0) {
				g1.inverseDirection(); g2.inverseDirection();
			}
			double m = m1 < m2 ? m1 : m2;
			m = m / 3.0f;
			TERMINAL t1 = conts->at(n).prims[i]->getNegativeOffset(m / m1);
			TERMINAL t2 = conts->at(n).prims[j]->getPositiveOffset(m / m2);
			conts->at(n).prims[i]->terms[1] = t1;
			conts->at(n).prims[j]->terms[0] = t2;
			conts->at(n).prims.insert(conts->at(n).prims.begin() + j, new LINE(t1, t2));
			i++;
		}
	}
	et = get_tick_count() - et;
	cout << et << " ms." << endl;
}

void buildBackWall(double depth, vector<SUBNODE> *nodes) {
	tick32_t et = get_tick_count();

	cout << "Generating Back-Wall...";
	int nCount = (int)(nodes->size());
	for (int i = 0; i < nCount; i++) {
		SUBNODE sb;
		for (UINT j = 0; j < nodes->at(i).vertex.size(); j++) {
			VERTEX v = nodes->at(i).vertex[j];
			if (v.tag == 100) {
				v.val[2] = -depth - v.val[2];
			}
			else {
				v.val[2] = -depth;
			}
			sb.vertex.push_back(v);
		}	
		for (UINT j = 0; j < nodes->at(i).index.size(); j++) {
			TRIANGLE tr = nodes->at(i).index[j];
			tr.flip();
			sb.index.push_back(tr);
		}
		nodes->push_back(sb);
	}
	et = get_tick_count() - et;
	cout << et << " ms." << endl;
}

void placeGemHole(vector<GEM> *gems, vector<SHAPE> *conts) {
	tick32_t et = get_tick_count();

	cout << "Placing Gem Hole...";

	for (UINT n = 0; n < gems->size(); n++) {
		double r = gems->at(n).radius * 0.86f;
		double r1 = gems->at(n).holeSize;
		double x = gems->at(n).pos.val[0];
		double y = gems->at(n).pos.val[1];

		SHAPE shp;
		TERMINAL t = TERMINAL(x + r * cos(PI * 0.5f), y + r * sin(PI * 0.5f));
		TERMINAL tt = TERMINAL(x + r1 * cos(PI * 0.5f), y + r1 * sin(PI * 0.5f));
		for (int a = 105; a < 450; a += 15) {
			TERMINAL t1 = TERMINAL(x + r * cos(a * PI / 180.0f), y + r * sin(a * PI / 180.0f));
			TERMINAL tt1 = TERMINAL(x + r1 * cos(a * PI / 180.0f), y + r1 * sin(a * PI / 180.0f));
			shp.prims.push_back(new LINE(t, t1, tt, tt1));
			t = t1;
			tt = tt1;
		}
		shp.prims.push_back(new LINE(t, shp.prims[0]->terms[0], tt, shp.prims[0]->offsets[0]));
		shp.isGemhole = true;
		shp.depth = r * GEMHOLE_DEPTH;
		conts->push_back(shp);
	}
	et = get_tick_count() - et;
	cout << et << " ms." << endl;
}

void buildGemInnerHole(vector<SHAPE> *conts, vector<SUBNODE> *nodes) {
	SUBNODE sb;
	int n = 0;
	tick32_t et = get_tick_count();

	cout << "Generating Gem-Hole...";

	for (UINT i = 0; i < conts->size(); i++) {
		if (!conts->at(i).isGemhole) continue;
		int pCount = (int)(conts->at(i).prims.size());
		for (int j = 0; j < pCount; j++) {
			int k = j + 1 >= pCount ? 0 : j + 1;
			TERMINAL t1 = conts->at(i).prims[j]->terms[0];
			TERMINAL t2 = conts->at(i).prims[j]->offsets[0];
			sb.vertex.push_back(VERTEX(t1.x, t1.y, 0));
			sb.vertex.push_back(VERTEX(t2.x, t2.y, -conts->at(i).depth));
			TRIANGLE tr1 = TRIANGLE(2 * (n + j), 2 * (n + j) + 1, 2 * (n + k) + 1);
			TRIANGLE tr2 = TRIANGLE(2 * (n + j), 2 * (n + k) + 1, 2 * (n + k));
			sb.index.push_back(tr1);
			sb.index.push_back(tr2);
			conts->at(i).prims[j]->terms[0] = conts->at(i).prims[j]->offsets[0];
			conts->at(i).prims[j]->terms[1] = conts->at(i).prims[j]->offsets[1];
		}
		n += pCount;
	}
	if(n > 0) nodes->push_back(sb);
	et = get_tick_count() - et;
	cout << et << " ms." << endl;
}

double getSquareMeter(vector<SUBNODE> *nodes) {
	double s = 0.0f;

	for (UINT i = 0; i < nodes->size(); i++) {
		for (UINT j = 0; j < nodes->at(i).index.size(); j++) {
			TRIANGLE tr = nodes->at(i).index[j];
			VERTEX v1 = nodes->at(i).vertex[tr.idx[0]];
			VERTEX v2 = nodes->at(i).vertex[tr.idx[1]];
			VERTEX v3 = nodes->at(i).vertex[tr.idx[2]];
			v2 = doMinus(v2, v1);
			v3 = doMinus(v3, v1);
			v1 = doCross(v2, v3);
			s += v1.getMagnitude() / 2.0f;
		}
	}
	return s;
}

void regroupContour(vector<SHAPE> *conts, vector<vector<SHAPE> > *contourGroup) {
	tick32_t et = get_tick_count();

	cout << "Grouping Contour...";

	for (UINT pi = 0; pi < conts->size(); pi++) {
		vector<SHAPE> contGroup;
		if (conts->at(pi).isValid == false) continue;
		contGroup.push_back(conts->at(pi));
		for (UINT pj = pi + 1; pj < conts->size(); pj++) {
			if (conts->at(pi).isInsideShape(&(conts->at(pj))) == true) {
					conts->at(pj).isValid = false;
					conts->at(pj).reversePrimitive();
					conts->at(pj).isPositive = false;
					contGroup.push_back(conts->at(pj));
			}
		}
		contourGroup->push_back(contGroup);
	}

	et = get_tick_count() - et;
	cout << et << " ms." << endl;
}

double makeRoundEdge(vector<SHAPE> *conts, vector<SUBNODE> *nodes, double depth) {
	tick32_t et = get_tick_count();
	double rndEdge = depth * 0.05f;

	cout << "Rounding Edge...";
	for (UINT n = 0; n < conts->size(); n++) {
		SUBNODE sb;
		if (conts->at(n).isGemhole == true) continue;
		int m = (int)(conts->at(n).prims.size());
		for (int i = 0; i < m; i++) {
			int j = i - 1 < 0 ? m - 1 : i - 1;
			VERTEX v1 = conts->at(n).prims[j]->getPositiveDirection();
			VERTEX v2 = conts->at(n).prims[i]->getPositiveDirection();
			VERTEX v3 = conts->at(n).prims[j]->getNegativeDirection();
			VERTEX v0 = doAverage(v3, v2);

			v1 = doCross(v1, v2);
			if (v1.val[2] > 0) {
				v0.inverseDirection();
			}

			v0.doNormalize();
			conts->at(n).prims[i]->offsets[0].x = v0.val[0];
			conts->at(n).prims[i]->offsets[0].y = v0.val[1];
			conts->at(n).prims[j]->offsets[1].x = v0.val[0];
			conts->at(n).prims[j]->offsets[1].y = v0.val[1];
		}

		for (int a = 90; a >= 0; a -= 10) {
			double al = a * PI / 180.0f;
			int vertCount = (int)(sb.vertex.size());
			for (int i = 0; i < m; i++) {
				double x = rndEdge * cos(al);
				double y = rndEdge - rndEdge * sin(al);
				TERMINAL t = conts->at(n).prims[i]->terms[0];
				TERMINAL o = conts->at(n).prims[i]->offsets[0];
				VERTEX v = VERTEX(t.x + o.x * x, t.y + o.y * x, -y);
				v.tag = 100;
				sb.vertex.push_back(v);
			}
			if (a > 0) {
				for (int i = 0; i < m; i++) {
					int j = i + 1 >= m ? 0 : i + 1;
					TRIANGLE tr1 = TRIANGLE(vertCount + i, vertCount + i + m, vertCount + j + m);
					TRIANGLE tr2 = TRIANGLE(vertCount + i, vertCount + j + m, vertCount + j);
					//if (conts->at(n).isPositive == false) {
					//	tr1.flip(); tr2.flip();
					//}
					sb.index.push_back(tr1);
					sb.index.push_back(tr2);
				}
			}
		}

		nodes->push_back(sb);

		for (int i = 0; i < m; i++)
		{
			conts->at(n).prims[i]->terms[0].x += conts->at(n).prims[i]->offsets[0].x * rndEdge;
			conts->at(n).prims[i]->terms[0].y += conts->at(n).prims[i]->offsets[0].y * rndEdge;
			conts->at(n).prims[i]->terms[1].x += conts->at(n).prims[i]->offsets[1].x * rndEdge;
			conts->at(n).prims[i]->terms[1].y += conts->at(n).prims[i]->offsets[1].y * rndEdge;
		}
		conts->at(n).depth = rndEdge;
	}
	et = get_tick_count() - et;
	cout << et << " ms." << endl;

	return getSquareMeter(nodes);
}

void buildFrontWall(vector<vector<SHAPE> > *contourGroup, vector<SUBNODE> *nodes) {
	nodes->clear();

	for (UINT n = 0; n < contourGroup->size(); n++) {
		SUBNODE sb;
		unifyContour(&(contourGroup->at(n)));
		generateFlatMeshes(&(contourGroup->at(n)[0]), &sb);
		nodes->push_back(sb);
	}
}

void buildSideWall(vector<SHAPE> *conts, double depth, vector<SUBNODE> *nodes) {
	tick32_t et = get_tick_count();
	SUBNODE sb;
	const char *meshName = "MetalBody";

	cout << "Generating Side-Wall...";

	for (int i = 0; i < (int)(conts->size()); i++) {
		int p = (int)(conts->at(i).prims.size());
		int vCount = (int)(sb.vertex.size());
		for (int j = 0; j < p; j++) {
			int k = j + 1 >= p ? 0 : j + 1;
			TERMINAL t = conts->at(i).prims[j]->terms[0];
			sb.vertex.push_back(VERTEX(t.x, t.y, -conts->at(i).depth));
			if (conts->at(i).isGemhole == true) {
				sb.vertex.push_back(VERTEX(t.x, t.y, -depth));
			}
			else {
				sb.vertex.push_back(VERTEX(t.x, t.y, -depth + conts->at(i).depth));
			}
			TRIANGLE tr1 = TRIANGLE(vCount + j * 2, vCount + j * 2 + 1, vCount + k * 2 + 1);
			TRIANGLE tr2 = TRIANGLE(vCount + j * 2, vCount + k * 2 + 1, vCount + k * 2);
			//if (conts->at(i).isPositive == false) {
			//	tr1.flip(); tr2.flip();
			//}
			sb.index.push_back(tr1);
			sb.index.push_back(tr2);
		}
	}
	nodes->push_back(sb);

	sb.vertex.clear();
	sb.index.clear();

	for (UINT i = 0; i < nodes->size(); i++) {
		int vertCount = (int)(sb.vertex.size());

		for (UINT j = 0; j < nodes->at(i).vertex.size(); j++) {
			sb.vertex.push_back(nodes->at(i).vertex[j]);
		}
		for (UINT j = 0; j < nodes->at(i).index.size(); j++) {
			TRIANGLE tr = nodes->at(i).index[j];
			sb.index.push_back(TRIANGLE(tr.idx[0] + vertCount, tr.idx[1] + vertCount, tr.idx[2] + vertCount));
		}
		nodes->at(i).vertex.clear();
		nodes->at(i).index.clear();
	}
	sb.putName(meshName);

	nodes->clear();
	nodes->push_back(sb);
	et = get_tick_count() - et;
	cout << et << " ms." << endl;
}

void simplifyContour(vector<SHAPE> *conts) {
	tick32_t et = get_tick_count();

	cout << "Simplifying Contour...";

	for (UINT n = 0; n < conts->size(); n++) {
		for (int i = 0; i < (int)conts->at(n).prims.size(); i++) {
			int k = i + 1 == (int)conts->at(n).prims.size() ? 0 : i + 1;
			int j = i - 1 >= 0 ? i - 1 : (int)conts->at(n).prims.size() - 1;
			if (conts->at(n).prims[i]->getMagnitude() < 2.0) {
				conts->at(n).prims[j]->terms[1] = conts->at(n).prims[k]->terms[0];
				conts->at(n).prims.erase(conts->at(n).prims.begin() + i);
				i--;
			}
			else {
				VERTEX v1 = conts->at(n).prims[i]->getPositiveDirection();
				VERTEX v2 = conts->at(n).prims[k]->getPositiveDirection();
				v1 = doCross(v1, v2);
				if (fabs(v1.val[2]) < 0.1f) {
					conts->at(n).prims[i]->terms[1] = conts->at(n).prims[k]->terms[1];
					conts->at(n).prims.erase(conts->at(n).prims.begin() + k);
					i--;
				}
			}
		}
		conts->at(n).isValid = true;
	}
	et = get_tick_count() - et;
	cout << et << " ms." << endl;
}

VERTEX curveVertex(VERTEX v, VERTEX ct, double length, double curve, double mnX, double stp) {
	int s1 = (int)((v.val[0] - mnX) / stp);
	double x1 = mnX + s1 * stp;
	double x2 = x1 + stp;
	double rate = (v.val[0] - x1) / stp;
	double r = ct.val[2] - v.val[2];
	x1 = x1 - ct.val[0];
	x2 = x2 - ct.val[0];
	x1 = x1 * curve * 0.5f / length;
	x2 = x2 * curve * 0.5f / length;
	double z1 = ct.val[2] - r * cos(x1 * PI / 180.0f);
	double z2 = ct.val[2] - r * cos(x2 * PI / 180.0f);
	x1 = ct.val[0] + r * sin(x1 * PI / 180.0f);
	x2 = ct.val[0] + r * sin(x2 * PI / 180.0f);
	v.val[0] = x1 + (x2 - x1) * rate;
	v.val[2] = z1 + (z2 - z1) * rate;
	return v;
}

VERTEX curveVertexEllipse(VERTEX v, VERTEX ct, double length, double curve, double mnX, double stp, double r1, double r2) {
	int s1 = (int)((v.val[0] - mnX) / stp);
	double x1 = mnX + s1 * stp;
	double x2 = x1 + stp;
	double rate = (v.val[0] - x1) / stp;
	double r = ct.val[2] - v.val[2];
	x1 = x1 - ct.val[0];
	x2 = x2 - ct.val[0];
	x1 = x1 * curve * 0.5f / length;
	x2 = x2 * curve * 0.5f / length;
	x1 = 90.0f - x1;
	x2 = 90.0f - x2;
	double z1 = ct.val[2] - r * sin(x1 * PI / 180.0f);
	double z2 = ct.val[2] - r * sin(x2 * PI / 180.0f);
	x1 = ct.val[0] + (r2 + r - r1) * cos(x1 * PI / 180.0f);
	x2 = ct.val[0] + (r2 + r - r1) * cos(x2 * PI / 180.0f);
	v.val[0] = x1 + (x2 - x1) * rate;
	v.val[2] = z1 + (z2 - z1) * rate;
	return v;
}

void curveNode(vector<GEM> *gems, vector<PRONG> *prongs, vector<RING> *rings, vector<SUBNODE> *nodes, double curve) {
	tick32_t et = get_tick_count();

	if (curve == 0.0f) return;
	cout << "Curving at ...";

	for (UINT i = 0; i < gem.index.size(); i++) {
		gem.index[i].flip();
	}
	double mnX = M_INFINITE;
	double mxX = -M_INFINITE;
	double mnZ = M_INFINITE;
	double mxZ = -M_INFINITE;

	for (UINT i = 0; i < nodes->size(); i++) {
		for (UINT j = 0; j < nodes->at(i).vertex.size(); j++) {
			VERTEX v = nodes->at(i).vertex[j];
			if (mnX > v.val[0]) mnX = v.val[0];
			if (mxX < v.val[0]) mxX = v.val[0];
			if (mnZ > v.val[2]) mnZ = v.val[2];
			if (mxZ < v.val[2]) mxZ = v.val[2];
		}
	}

	int segCount = (int)(fabs(curve) / 5.0f) + 1;
	double radius = (mxX - mnX) / (curve * PI / 180.0f);
	double stp = (mxX - mnX) / (double)segCount;
	for (int n = 1; n < segCount; n++) {
		double x = mnX + stp * n;
		for (UINT i = 0; i < nodes->size(); i++) {
			int m = (int)(nodes->at(i).index.size());
			for (int j = 0; j < m; j++) {
				TRIANGLE tr = nodes->at(i).index[j];
				VERTEX v1 = nodes->at(i).vertex[tr.idx[0]];
				VERTEX v2 = nodes->at(i).vertex[tr.idx[1]];
				VERTEX v3 = nodes->at(i).vertex[tr.idx[2]];
				v2 = doMinus(v2, v1);
				v3 = doMinus(v3, v1);
				VERTEX v0 = doCross(v2, v3);
				v0.doNormalize();
				vector<VERTEX> sharePoints;
				int unsharedIndex = -1;
				for (int k = 0; k < 3; k++) {
					int p = k + 1 == 3 ? 0 : k + 1;
					v1 = nodes->at(i).vertex[tr.idx[k]];
					v2 = nodes->at(i).vertex[tr.idx[p]];
					if (isConflict(v1, v2, x, &v3) == 1) {
						sharePoints.push_back(v3);
					}
					else {
						unsharedIndex = k;
					}
				}
				if (sharePoints.size() == 2 && unsharedIndex != -1) {
					int p = unsharedIndex + 2 >= 3 ? unsharedIndex - 1 : unsharedIndex + 2;
					int k = unsharedIndex + 1 == 3 ? 0 : unsharedIndex + 1;
					v1 = sharePoints[0];
					v2 = sharePoints[1];
					v3 = nodes->at(i).vertex[tr.idx[p]];
					v1 = doMinus(v1, v3);
					v2 = doMinus(v2, v3);
					v1 = doCross(v1, v2);
					v1.doNormalize();
					v1 = doMinus(v1, v0);
					if (v1.getMagnitude() < EP) {
						nodes->at(i).vertex.push_back(sharePoints[0]);
						nodes->at(i).vertex.push_back(sharePoints[1]);
					}
					else {
						nodes->at(i).vertex.push_back(sharePoints[1]);
						nodes->at(i).vertex.push_back(sharePoints[0]);
					}
					int vc = (int)(nodes->at(i).vertex.size());
					nodes->at(i).index[j].idx[0] = vc - 2;
					nodes->at(i).index[j].idx[1] = vc - 1;
					nodes->at(i).index[j].idx[2] = tr.idx[p];
					nodes->at(i).index.push_back(TRIANGLE(vc - 2, tr.idx[k], vc - 1));
					nodes->at(i).index.push_back(TRIANGLE(vc - 2, tr.idx[unsharedIndex], tr.idx[k]));
				}
			}
		}
	}

	double ctX = (mnX + mxX) / 2.0f;
	double ctZ = radius > 0 ? mxZ + radius : mnZ + radius;

	double length = (mxX - mnX) / 2.0f;
	for (UINT i = 0; i < nodes->size(); i++) {
		int m = (int)(nodes->at(i).vertex.size());
		for (int j = 0; j < m; j++) {
			VERTEX v = nodes->at(i).vertex[j];
			
			nodes->at(i).vertex[j] = curveVertex(v, VERTEX(ctX, 0, ctZ), length, curve, mnX, stp);
		}
	}

	for (UINT i = 0; i < gems->size(); i++) {
		VERTEX v = doAdd(gems->at(i).pos, VERTEX(0, 0, 10));
		gems->at(i).uve[2] = curveVertex(v, VERTEX(ctX, 0, ctZ), length, curve, mnX, stp);
		gems->at(i).pos = curveVertex(gems->at(i).pos, VERTEX(ctX, 0, ctZ), length, curve, mnX, stp);
		gems->at(i).uve[2] = doMinus(gems->at(i).uve[2], gems->at(i).pos);
		gems->at(i).uve[0] = VERTEX(0, 1, 0);
		gems->at(i).uve[1] = doCross(gems->at(i).uve[2], gems->at(i).uve[0]);
	}

	for (UINT i = 0; i < prongs->size(); i++) {
		VERTEX v = doAdd(prongs->at(i).pos, VERTEX(0, 0, 10));
		prongs->at(i).uve[2] = curveVertex(v, VERTEX(ctX, 0, ctZ), length, curve, mnX, stp);
		prongs->at(i).pos = curveVertex(prongs->at(i).pos, VERTEX(ctX, 0, ctZ), length, curve, mnX, stp);
		prongs->at(i).uve[2] = doMinus(prongs->at(i).uve[2], prongs->at(i).pos);
		prongs->at(i).uve[0] = VERTEX(0, 1, 0);
		prongs->at(i).uve[1] = doCross(prongs->at(i).uve[0], prongs->at(i).uve[2]);
	}

	for (UINT i = 0; i < rings->size(); i++) {
		VERTEX v = doAdd(rings->at(i).pos, VERTEX(0, 0, 10));
		rings->at(i).uve[2] = curveVertex(v, VERTEX(ctX, 0, ctZ), length, curve, mnX, stp);
		rings->at(i).pos = curveVertex(rings->at(i).pos, VERTEX(ctX, 0, ctZ), length, curve, mnX, stp);
		rings->at(i).uve[2] = doMinus(rings->at(i).uve[2], rings->at(i).pos);
		rings->at(i).uve[0] = VERTEX(0, 1, 0);
		rings->at(i).uve[1] = doCross(rings->at(i).uve[0], rings->at(i).uve[2]);
	}
	et = get_tick_count() - et;
	cout << et << " ms." << endl;
}

void curveNodeEllipse(vector<GEM> *gems, vector<PRONG> *prongs, vector<RING> *rings, vector<SUBNODE> *nodes, double r1, double r2, double curve) {
	if (curve == 0.0f) return;
		
	cout << "Curving at ..." << endl;
	
	double mnX = M_INFINITE;
	double mxX = -M_INFINITE;
	double mnZ = M_INFINITE;
	double mxZ = -M_INFINITE;

	for (UINT i = 0; i < gem.index.size(); i++) {
		gem.index[i].flip();
	}

	for (UINT i = 0; i < nodes->size(); i++) {
		for (UINT j = 0; j < nodes->at(i).vertex.size(); j++) {
			VERTEX v = nodes->at(i).vertex[j];
			if (mnX > v.val[0]) mnX = v.val[0];
			if (mxX < v.val[0]) mxX = v.val[0];
			if (mnZ > v.val[2]) mnZ = v.val[2];
			if (mxZ < v.val[2]) mxZ = v.val[2];
		}
	}

	int segCount = (int)(fabs(curve) / 5.0f) + 1;
	double radius = (mxX - mnX) / (curve * PI / 180.0f);
	double stp = (mxX - mnX) / (double)segCount;
	for (int n = 1; n < segCount; n++) {
		double x = mnX + stp * n;
		for (UINT i = 0; i < nodes->size(); i++) {
			int m = (int)(nodes->at(i).index.size());
			for (int j = 0; j < m; j++) {
				TRIANGLE tr = nodes->at(i).index[j];
				VERTEX v1 = nodes->at(i).vertex[tr.idx[0]];
				VERTEX v2 = nodes->at(i).vertex[tr.idx[1]];
				VERTEX v3 = nodes->at(i).vertex[tr.idx[2]];
				v2 = doMinus(v2, v1);
				v3 = doMinus(v3, v1);
				VERTEX v0 = doCross(v2, v3);
				v0.doNormalize();
				vector<VERTEX> sharePoints;
				int unsharedIndex = -1;
				for (int k = 0; k < 3; k++) {
					int p = k + 1 == 3 ? 0 : k + 1;
					v1 = nodes->at(i).vertex[tr.idx[k]];
					v2 = nodes->at(i).vertex[tr.idx[p]];
					if (isConflict(v1, v2, x, &v3) == 1) {
						sharePoints.push_back(v3);
					}
					else {
						unsharedIndex = k;
					}
				}
				if (sharePoints.size() == 2 && unsharedIndex != -1) {
					int p = unsharedIndex + 2 >= 3 ? unsharedIndex - 1 : unsharedIndex + 2;
					int k = unsharedIndex + 1 == 3 ? 0 : unsharedIndex + 1;
					v1 = sharePoints[0];
					v2 = sharePoints[1];
					v3 = nodes->at(i).vertex[tr.idx[p]];
					v1 = doMinus(v1, v3);
					v2 = doMinus(v2, v3);
					v1 = doCross(v1, v2);
					v1.doNormalize();
					v1 = doMinus(v1, v0);
					if (v1.getMagnitude() < EP) {
						nodes->at(i).vertex.push_back(sharePoints[0]);
						nodes->at(i).vertex.push_back(sharePoints[1]);
					}
					else {
						nodes->at(i).vertex.push_back(sharePoints[1]);
						nodes->at(i).vertex.push_back(sharePoints[0]);
					}
					int vc = (int)(nodes->at(i).vertex.size());
					nodes->at(i).index[j].idx[0] = vc - 2;
					nodes->at(i).index[j].idx[1] = vc - 1;
					nodes->at(i).index[j].idx[2] = tr.idx[p];
					nodes->at(i).index.push_back(TRIANGLE(vc - 2, tr.idx[k], vc - 1));
					nodes->at(i).index.push_back(TRIANGLE(vc - 2, tr.idx[unsharedIndex], tr.idx[k]));
				}
			}
		}
	}

	double ctX = (mnX + mxX) / 2.0f;
	double ctZ = radius > 0 ? mxZ + radius : mnZ + radius;

	double length = (mxX - mnX) / 2.0f;
	for (UINT i = 0; i < nodes->size(); i++) {
		int m = (int)(nodes->at(i).vertex.size());
		for (int j = 0; j < m; j++) {
			VERTEX v = nodes->at(i).vertex[j];

			nodes->at(i).vertex[j] = curveVertexEllipse(v, VERTEX(ctX, 0, ctZ), length, curve, mnX, stp, r1, r2);
		}
	}

	for (UINT i = 0; i < gems->size(); i++) {
		VERTEX v = doAdd(gems->at(i).pos, VERTEX(0, 0, 10));
		gems->at(i).uve[2] = curveVertexEllipse(v, VERTEX(ctX, 0, ctZ), length, curve, mnX, stp, r1, r2);
		gems->at(i).pos = curveVertexEllipse(gems->at(i).pos, VERTEX(ctX, 0, ctZ), length, curve, mnX, stp, r1, r2);
		gems->at(i).uve[2] = doMinus(gems->at(i).uve[2], gems->at(i).pos);
		gems->at(i).uve[0] = VERTEX(0, 1, 0);
		gems->at(i).uve[1] = doCross(gems->at(i).uve[2], gems->at(i).uve[0]);
	}

	for (UINT i = 0; i < prongs->size(); i++) {
		VERTEX v = doAdd(prongs->at(i).pos, VERTEX(0, 0, 10));
		prongs->at(i).uve[2] = curveVertexEllipse(v, VERTEX(ctX, 0, ctZ), length, curve, mnX, stp, r1, r2);
		prongs->at(i).pos = curveVertexEllipse(prongs->at(i).pos, VERTEX(ctX, 0, ctZ), length, curve, mnX, stp, r1, r2);
		prongs->at(i).uve[2] = doMinus(prongs->at(i).uve[2], prongs->at(i).pos);
		prongs->at(i).uve[0] = VERTEX(0, 1, 0);
		prongs->at(i).uve[1] = doCross(prongs->at(i).uve[0], prongs->at(i).uve[2]);
	}

	for (UINT i = 0; i < rings->size(); i++) {
		VERTEX v = doAdd(rings->at(i).pos, VERTEX(0, 0, 10));
		rings->at(i).uve[2] = curveVertexEllipse(v, VERTEX(ctX, 0, ctZ), length, curve, mnX, stp, r1, r2);
		rings->at(i).pos = curveVertexEllipse(rings->at(i).pos, VERTEX(ctX, 0, ctZ), length, curve, mnX, stp, r1, r2);
		rings->at(i).uve[2] = doMinus(rings->at(i).uve[2], rings->at(i).pos);
		rings->at(i).uve[0] = VERTEX(0, 1, 0);
		rings->at(i).uve[1] = doCross(rings->at(i).uve[0], rings->at(i).uve[2]);
	}
}

double getScaleFactor(vector<GEM> *gems, vector<PRONG> *prongs, PGEM_SIZE_PATTERN gemPat, int gemPatSize) {
	double mn = 0;
	double m = 0;
	double scaleFactor = 0.0f;

	for (int i = 0; i < gemPatSize; i++) {
		for (int j = i + 1; j < gemPatSize; j++) {
			if (gemPat[i].gemSize > gemPat[j].gemSize) {
				GEM_SIZE_PATTERN gm = gemPat[i];
				gemPat[i] = gemPat[j];
				gemPat[j] = gm;
			}
		}
	}

	for (int i = 0; i < gemPatSize; i++) {
		if (i == 0) {
			mn = gemPat[i].gemSize;
		}
		else {
			if (mn > gemPat[i].gemSize) {
				mn = gemPat[i].gemSize;
			}
		}
	}

	for (UINT i = 0; i < gems->size(); i++) {
		if (i == 0) {
			m = gems->at(i).radius * 2.0f;
		}
		else if (m > gems->at(i).radius * 2.0f) {
			m = gems->at(i).radius * 2.0f;
		}
	}

	scaleFactor = m == 0 ? 1 : mn / m;

	for (UINT i = 0; i < gems->size(); i++) {
		int mni = -1;
		gems->at(i).radius *= scaleFactor;

		for (int j = 0; j < gemPatSize; j++) {
			double delta = fabs(gemPat[j].gemSize - gems->at(i).radius * 2.0f);
			if (j == 0) {
				m = delta; mni = 0;
			}
			else {
				if (m > delta) {
					m = delta; mni = j;
				}
			}
		}
		gems->at(i).radius = gemPat[mni].gemSize * 0.5f / scaleFactor;
		gems->at(i).holeSize = gemPat[mni].holeSize * 0.5f / scaleFactor;
		gems->at(i).prongSize = gemPat[mni].prongSize * 0.5f / scaleFactor;
	}

	for (UINT i = 0; i < prongs->size(); i++) {
		for (UINT j = 0; j < prongs->at(i).nGems.size(); j++) {
			int n = prongs->at(i).nGems[j];
			if (j == 0) {
				prongs->at(i).size = gems->at(n).prongSize;
			}
			else {
				if (prongs->at(i).size < gems->at(n).prongSize) {
					prongs->at(i).size = gems->at(n).prongSize;
				}
			}
		}
	}

	return scaleFactor;
}

void adjustRingCenter(vector<RING> *rings, vector<SHAPE> *conts, double scaleFactor, int width) {

	for (UINT n = 0; n < rings->size(); n++) {
		rings->at(n).radius = 0.001f / scaleFactor;
		if (rings->at(n).pos.val[0] > (float)width * 0.5f) {
			rings->at(n).pos.val[0] += rings->at(n).radius * 0.75f;
		}
		else {
			rings->at(n).pos.val[0] -= rings->at(n).radius * 0.75f;
		}
	}
	/*
	for (UINT n = 0; n < rings->size(); n++) {
		double mn = M_INFINITE;
		TERMINAL mnt;
		for (UINT i = 0; i < conts->size(); i++) {
			for (UINT j = 0; j < conts->at(i).prims.size(); j++) {
				TERMINAL t;
				VERTEX v = conts->at(i).prims[j]->getGradient();
				LINE ln = LINE(TERMINAL(rings->at(n).pos.val[0], rings->at(n).pos.val[1]),
					TERMINAL(rings->at(n).pos.val[0] + v.val[0] * M_INFINITE, rings->at(n).pos.val[1] + v.val[1] * M_INFINITE));
				if (isConflict(&ln, conts->at(i).prims[j], &t) == false) continue;
				double x = t.x - rings->at(n).pos.val[0];
				double y = t.y - rings->at(n).pos.val[1];
				double m = sqrt(x * x + y * y);
				if (m < mn) {
					mn = m;
					mnt = t;
				}
			}
		}
		mnt.x -= rings->at(n).pos.val[0];
		mnt.y -= rings->at(n).pos.val[1];
		mnt.doNormalize();
		rings->at(n).radius = 0.001f / scaleFactor;
		rings->at(n).pos.val[0] += mnt.x * rings->at(n).radius * 0.75f;
		rings->at(n).pos.val[1] += mnt.y * rings->at(n).radius * 0.75f;
	}
	*/
}

void sortTerminalsbyX(vector<TERMINAL> *terms) {
	for (UINT i = 0; i < terms->size(); i++) {
		for (UINT j = i + 1; j < terms->size(); j++) {
			if (terms->at(i).x > terms->at(j).x) {
				TERMINAL t = terms->at(i);
				terms->at(i) = terms->at(j);
				terms->at(j) = t;
			}
		}
	}
}

void sortTerminalsbyY(vector<TERMINAL> *terms) {
	for (UINT i = 0; i < terms->size(); i++) {
		for (UINT j = i + 1; j < terms->size(); j++) {
			if (terms->at(i).y > terms->at(j).y) {
				TERMINAL t = terms->at(i);
				terms->at(i) = terms->at(j);
				terms->at(j) = t;
			}
		}
	}
}

void findClosedContours(SHAPE *shp, vector<SHAPE> *conts) {
	int cur = 0;
	bool flag = true;
	SHAPE sp;

	for (UINT i = 0; i < shp->prims.size(); i++) {
		shp->prims[i]->isValid = true;
	}

	sp.prims.push_back(shp->prims[cur]->clone());
	shp->prims[cur]->isValid = false;

	while (flag) {
		TERMINAL t = shp->prims[cur]->terms[1];

		flag = false;
		for (UINT i = 0; i < shp->prims.size(); i++) {
			bool found = false;
			if (shp->prims[i]->isValid == false) continue;

			flag = true;
			if (shp->prims[i]->terms[0].isEqual(t)) {
				found = true;
			}
			if (shp->prims[i]->terms[1].isEqual(t)) {
				shp->prims[i]->swapTerminals();
				found = true;
			}
			if(found) {
				sp.prims.push_back(shp->prims[i]->clone());
				shp->prims[i]->isValid = false;
				if (shp->prims[i]->terms[1].isEqual(sp.prims[0]->terms[0])) {
					sp.makePositive();
					conts->push_back(sp);
					sp.prims.clear();
					cur = -1;
					for (UINT j = 0; j < shp->prims.size(); j++) {
						if (shp->prims[j]->isValid == false) continue;
						cur = j;
					}
					if (cur == -1) {
						flag = false;
					}
					break;
				}
				else {
					cur = i;
					t = shp->prims[cur]->terms[1];
				}
			}
		}
	}
}

void splitCoutourGroup(vector<vector<SHAPE> > *contGroup, vector<SHAPE> *conts) {
	tick32_t et = get_tick_count();

	for (int n = 0; n < (int)contGroup->size(); n++) {
		vector<TERMINAL> ctrs;
		vector<TERMINAL> spts;

		if (contGroup->at(n).size() == 1) {
			SHAPE shp;

			for (int i = 0; i < (int)contGroup->at(n)[0].prims.size(); i++) {
				shp.prims.push_back(contGroup->at(n)[0].prims[i]->clone());
			}
			conts->push_back(shp);
			continue;
		}

		for (int i = 1; i < (int)contGroup->at(n).size(); i++) {
			ctrs.push_back(contGroup->at(n)[i].getCenter());
		}

		sortTerminalsbyX(&ctrs);

		spts.push_back(ctrs[0]);

		for (int i = 1; i < (int)ctrs.size(); i++) {
			int j = (int)spts.size() - 1;
			
			if (spts[j].isEqual(ctrs[i]) == false) {
				spts.push_back(ctrs[i]);
			}
		}

		spts.insert(spts.begin(), TERMINAL(-M_INFINITE, 0.0f));
		spts.push_back(TERMINAL(M_INFINITE, 0.0f));

		for (int i = 0; i < (int)spts.size() - 1; i++) {
			int j = i + 1;
			vector<TERMINAL> ta;
			vector<TERMINAL> tb;
			SHAPE shp;
			LINE la = LINE(TERMINAL(spts[i].x, M_INFINITE), TERMINAL(spts[i].x, -M_INFINITE));
			LINE lb = LINE(TERMINAL(spts[j].x, M_INFINITE), TERMINAL(spts[j].x, -M_INFINITE));
			
			for (UINT k = 0; k < contGroup->at(n).size(); k++) {
				for (UINT m = 0; m < contGroup->at(n)[k].prims.size(); m++) {
					TERMINAL mx, mn;
					TERMINAL t;
					TERMINAL t1 = contGroup->at(n)[k].prims[m]->terms[0];
					TERMINAL t2 = contGroup->at(n)[k].prims[m]->terms[1];

					if (t1.x == spts[i].x) ta.push_back(t1);
					if (t2.x == spts[i].x) ta.push_back(t2);
					if (t1.x == spts[j].x) tb.push_back(t1);
					if (t2.x == spts[j].x) tb.push_back(t2);

					contGroup->at(n)[k].prims[m]->getBound(&mn, &mx);

					if (mn.x >= spts[i].x && mx.x <= spts[j].x) {
						shp.prims.push_back(contGroup->at(n)[k].prims[m]->clone());
					}
					if (mn.x < spts[i].x && mx.x <= spts[j].x && mx.x > spts[i].x) {
						isConflict(contGroup->at(n)[k].prims[m], &la, &t);
						if (t1.x >= spts[i].x && t1.x <= spts[j].x) {
							shp.prims.push_back(new LINE(t1, t));
						}
						else {
							shp.prims.push_back(new LINE(t2, t));
						}
						ta.push_back(t);
					}
					if (mn.x >= spts[i].x && mn.x < spts[j].x && mx.x > spts[j].x) {
						isConflict(contGroup->at(n)[k].prims[m], &lb, &t);
						if (t1.x >= spts[i].x && t1.x <= spts[j].x) {
							shp.prims.push_back(new LINE(t1, t));
						}
						else {
							shp.prims.push_back(new LINE(t2, t));
						}
						tb.push_back(t);
					}
					if (mn.x < spts[i].x && mx.x > spts[j].x) {
						isConflict(contGroup->at(n)[k].prims[m], &la, &t1);
						isConflict(contGroup->at(n)[k].prims[m], &lb, &t2);
						shp.prims.push_back(new LINE(t1, t2));
						ta.push_back(t1); tb.push_back(t2);
					}
				}
			}
			sortTerminalsbyY(&ta); sortTerminalsbyY(&tb);
			for (int k = 0; k < (int)ta.size() - 1; k++) {
				if (ta[k].isEqual(ta[k + 1])) continue;
				bool flag = false;
				TERMINAL t = TERMINAL(ta[k].x, (ta[k].y + ta[k + 1].y) * 0.5f);
				for (UINT m = 0; m < contGroup->at(n).size(); m++) {
					if (contGroup->at(n)[m].isInsidePoint(t) == false) {
						flag = true;
						break;
					}
				}
				if (flag == false) {
					shp.prims.push_back(new LINE(ta[k], ta[k + 1]));
				}
			}
			for (int k = 0; k < (int)tb.size() - 1; k++) {
				if (tb[k].isEqual(tb[k + 1])) continue;
				bool flag = false;
				TERMINAL t = TERMINAL(tb[k].x, (tb[k].y + tb[k + 1].y) * 0.5f);
				for (UINT m = 0; m < contGroup->at(n).size(); m++) {
					if (contGroup->at(n)[m].isInsidePoint(t) == false) {
						flag = true;
						break;
					}
				}
				if (flag == false) {
					shp.prims.push_back(new LINE(tb[k], tb[k + 1]));
				}
			}
			findClosedContours(&shp, conts);
			shp.clear();
		}
	}

	et = get_tick_count() - et;
	cout << "Spliting Contours..." << et << " ms." << endl;
}

int generateCurvedModel(UCHAR *raw, int raw_width, int raw_height, PGEM_SIZE_PATTERN gemPat, int gemPatSize, double thickness,
	double xTwist, double yTwist, vector<SUBNODE> *nodes, double *info, int option) {
	SUBNODE subnode;
	vector<GEM>		gems;
	vector<PRONG>	prongs;
	vector<RING>	rings;
	vector<SHAPE> conts;
	UCHAR *crop = NULL;
	UCHAR *src = NULL;
	int width = 0;
	int height = 0;
	double depth = 0;
	double scaleFactor = 1.0f;
	vector<vector<SHAPE> > contourGroup;
	vector<SHAPE> spContour;
	vector<SUBNODE> prongNodes;
	vector<SUBNODE> ringNodes;

	crop_extend(raw, raw_width, raw_height, &crop, &width, &height, MARGIN);
	if (crop == NULL || width == 0 || height == 0) return -1;
	ExtractGemProngPosition(&gems, &prongs, &rings, crop, width, height);
	scaleFactor = getScaleFactor(&gems, &prongs, gemPat, gemPatSize);
	src = (UCHAR *)malloc(width * height);
	cvt2Gray(crop, width, height, src);
	reverse(src, width, height);
	threshold(src, width, height, 30);
	toThinner(src, width, height, 0, 3);
	extractContourPixels(src, width, height);
	depth = thickness / scaleFactor;

	double length = 0; 
	double radius = 0; 
	TERMINAL tCenter;

	if (xTwist != 0) {
		length = (double)(width - MARGIN * 2);
		radius = length / (xTwist * PI / 180.0f);
		if (xTwist > 0) {
			tCenter = TERMINAL((double)width / 2.0f, (double)height - MARGIN + radius);
		}
		else if (xTwist < 0) {
			tCenter = TERMINAL((double)width / 2.0f, (double)MARGIN + radius);
		}
		curveGemProng(&gems, &prongs, &rings, tCenter, width - MARGIN * 2, xTwist);
	}

	for (int i = 0; i < height; i++) {
		UCHAR *p = src + i * width;
		for (int j = 0; j < width; j++) {
			if (p[j] == 0) continue;
			int ci = i; int cj = j;
			int primCount = 0;

			SHAPE shp;
			vector<Vec2i> path;
			path.push_back(Vec2i(ci, cj));
			p[j] = 0;

		_retry:
			int ret = getClosedContour(src, width, &path, ci, cj, false, 0);
			if (ret == -1) {
				ci = path[path.size() - 1].val[0];
				cj = path[path.size() - 1].val[1];

				goto _retry;
			}

			curvePath(&path, tCenter, width - MARGIN * 2, xTwist);

			int n = 0;
			int m = (int)path.size();
			for (int pi = 1; pi < m; pi++) {
				VERTEX v_xy = VERTEX(path[pi].val[0] - path[n].val[0], path[pi].val[1] - path[n].val[1], 0);
				double length_xy = v_xy.getMagnitude();
				bool f = false;

				for (int pj = n; pj < pi; pj++) {
					VERTEX v1_xy = VERTEX(path[pj].val[0] - path[n].val[0], path[pj].val[1] - path[n].val[1], 0);
					VERTEX v2_xy = doCross(v_xy, v1_xy);

					double s = fabs(v2_xy.val[2]) / length_xy;
					if (s >= 1.0f) {
						f = true;
						break;
					}
				}
				if (f) {
					LINE *ln = new LINE(TERMINAL(path[n].val[1], path[n].val[0]),
						TERMINAL(path[pi - 1].val[1], path[pi - 1].val[0]));
					shp.prims.push_back(ln);
					primCount++;
					n = pi - 1;
				}
			}

			LINE *ln = new LINE(TERMINAL(path[n].val[1], path[n].val[0]),
				TERMINAL(path[m - 1].val[1], path[m - 1].val[0]));
			shp.prims.push_back(ln);
			primCount++;

			if (primCount > 2) {
				if (shp.prims[primCount - 1]->terms[1].isEqual(shp.prims[0]->terms[0]) == false) {
					shp.prims[primCount - 1]->terms[1] = shp.prims[0]->terms[0];
				}
				shp.isGemhole = false;
				shp.depth = 0;
				shp.makePositive();
				conts.push_back(shp);
			}
		}
	}

	simplifyContour(&conts);

	smoothenContour(&conts);

	adjustRingCenter(&rings, &conts, scaleFactor, width);

	placeGemHole(&gems, &conts);

	regroupContour(&conts, &contourGroup);

	splitCoutourGroup(&contourGroup, &spContour);

	for (UINT n = 0; n < spContour.size(); n++) 
	{
		generateFlatMeshes(&(spContour[n]), &subnode);
		spContour[n].clear();
	}
	if (subnode.vertex.size() > 0 && subnode.index.size() > 0) nodes->push_back(subnode);

	OptimizeMesh(nodes, true, 50.0f);

	double sMeter = makeRoundEdge(&conts, nodes, depth);

	buildGemInnerHole(&conts, nodes);

	buildBackWall(depth, nodes);

	buildSideWall(&conts, depth, nodes);
	
	cutPorngs(&gems, &prongs, &prongNodes, option);

	curveNode(&gems, &prongs, &rings, nodes, yTwist);

	if (option != 3) {
		placeGems(&gems, nodes, scaleFactor);
	}

	placeProngs(&prongs, &prongNodes, nodes);

	cutRings(&conts, &rings, &ringNodes);

	placeRings(&rings, &ringNodes, nodes, depth);
	
	doSetVertexNormal(nodes);

	DoScale(nodes, scaleFactor);

	for (UINT i = 0; i < rings.size(); i++) {
		rings[i].pos.val[0] *= scaleFactor;
		rings[i].pos.val[1] *= scaleFactor;
		rings[i].pos.val[2] *= scaleFactor;
	}

	VERTEX mx, mn;
	VERTEX center;

	GetBound(&mx, &mn, nodes);
	center = doAverage(mx, mn);

	Move2Center(nodes);

	info[0] = sMeter * depth * scaleFactor * scaleFactor;
	info[1] = mx.val[0] - center.val[0]; info[2] = mx.val[1] - center.val[1]; info[3] = mx.val[2] - center.val[2];
	info[4] = mn.val[0] - center.val[0]; info[5] = mn.val[1] - center.val[1]; info[6] = mn.val[2] - center.val[2];
	if (rings.size() == 2) {
		info[7] = rings[0].pos.val[0] - center.val[0]; 
		info[8] = rings[0].pos.val[1] - center.val[1];
		info[9] = rings[0].pos.val[2] - center.val[2];
		info[10] = rings[1].pos.val[0] - center.val[0];
		info[11] = rings[1].pos.val[1] - center.val[1];
		info[12] = rings[1].pos.val[2] - center.val[2];
	}
	
	free(src);
	free(crop);

	for (UINT i = 0; i < conts.size(); i++) {
		conts[i].clear();
	}

	conts.clear();
	contourGroup.clear();
	return 0;
}

int generateCircleRingModel(UCHAR *raw, int raw_width, int raw_height, PGEM_SIZE_PATTERN gemPat, int gemPatSize, double thickness, double xRadius, double yRadius, 
	vector<SUBNODE> *nodes, double *info, int option) {
	SUBNODE subnode;
	vector<GEM>		gems;
	vector<PRONG>	prongs;
	vector<RING>	rings;
	vector<SHAPE> conts;
	UCHAR *crop = NULL;
	UCHAR *src = NULL;
	int width = 0;
	int height = 0;
	double depth = 0;
	double scaleFactor = 1.0f;
	TERMINAL tCenter;
	vector<vector<SHAPE> > contourGroup;
	vector<SHAPE> spContour;
	vector<SUBNODE> prongNodes;
	vector<SUBNODE> ringNodes;

	crop_extend(raw, raw_width, raw_height, &crop, &width, &height, MARGIN);
	if (crop == NULL || width == 0 || height == 0) return -1;
	ExtractGemProngPosition(&gems, &prongs, &rings, crop, width, height);
	scaleFactor = getScaleFactor(&gems, &prongs, gemPat, gemPatSize);
	src = (UCHAR *)malloc(width * height);
	cvt2Gray(crop, width, height, src);
	reverse(src, width, height);
	threshold(src, width, height, 30);
	toThinner(src, width, height, 0, 3);
	extractContourPixels(src, width, height);

	depth = thickness / scaleFactor;
	double length = (double)(width - MARGIN * 2);
	double virtualXR = xRadius / scaleFactor;
	double virtualYR = yRadius / scaleFactor;

	double xTwist = virtualXR != 0 ? length / virtualXR : 0.0f;
	double yTwist = virtualYR != 0 ? length / virtualYR : 0.0f;
	xTwist *= 180.0f / PI;
	yTwist *= 180.0f / PI;

	if (xTwist != 0) {
		if (xTwist > 0) {
			tCenter = TERMINAL((double)width / 2.0f, (double)height - MARGIN + virtualXR);
		}
		else if (xTwist < 0) {
			tCenter = TERMINAL((double)width / 2.0f, (double)MARGIN + virtualXR);
		}
		curveGemProng(&gems, &prongs, &rings, tCenter, width - MARGIN * 2, xTwist);
	}
	for (int i = 0; i < height; i++) {
		UCHAR *p = src + i * width;
		for (int j = 0; j < width; j++) {
			if (p[j] == 0) continue;
			int ci = i; int cj = j;
			int primCount = 0;

			SHAPE shp;
			vector<Vec2i> path;
			path.push_back(Vec2i(ci, cj));
			p[j] = 0;

		_retry:
			int ret = getClosedContour(src, width, &path, ci, cj, false, 0);
			if (ret == -1) {
				ci = path[path.size() - 1].val[0];
				cj = path[path.size() - 1].val[1];

				goto _retry;
			}
			curvePath(&path, tCenter, width - MARGIN * 2, xTwist);
			int n = 0;
			int m = (int)path.size();
			for (int pi = 1; pi < m; pi++) {
				VERTEX v_xy = VERTEX(path[pi].val[0] - path[n].val[0], path[pi].val[1] - path[n].val[1], 0);
				double length_xy = v_xy.getMagnitude();
				bool f = false;

				for (int pj = n; pj < pi; pj++) {
					VERTEX v1_xy = VERTEX(path[pj].val[0] - path[n].val[0], path[pj].val[1] - path[n].val[1], 0);
					VERTEX v2_xy = doCross(v_xy, v1_xy);

					double s = fabs(v2_xy.val[2]) / length_xy;
					if (s >= 1.0f) {
						f = true;
						break;
					}
				}
				if (f) {
					LINE *ln = new LINE(TERMINAL(path[n].val[1], path[n].val[0]),
						TERMINAL(path[pi - 1].val[1], path[pi - 1].val[0]));
					shp.prims.push_back(ln);
					primCount++;
					n = pi - 1;
				}
			}

			LINE *ln = new LINE(TERMINAL(path[n].val[1], path[n].val[0]),
				TERMINAL(path[m - 1].val[1], path[m - 1].val[0]));
			shp.prims.push_back(ln);
			primCount++;

			if (primCount > 2) {
				if (shp.prims[primCount - 1]->terms[1].isEqual(shp.prims[0]->terms[0]) == false) {
					shp.prims[primCount - 1]->terms[1] = shp.prims[0]->terms[0];
				}
				shp.isGemhole = false;
				shp.depth = 0;
				shp.makePositive();
				conts.push_back(shp);
			}
		}
	}

	simplifyContour(&conts);

	smoothenContour(&conts);

	adjustRingCenter(&rings, &conts, scaleFactor, width);

	placeGemHole(&gems, &conts);

	regroupContour(&conts, &contourGroup);

	splitCoutourGroup(&contourGroup, &spContour);

	for (UINT n = 0; n < spContour.size(); n++) {
		generateFlatMeshes(&(spContour[n]), &subnode);
		spContour[n].clear();
	}
	
	if (subnode.vertex.size() > 0 && subnode.index.size() > 0) nodes->push_back(subnode);

	OptimizeMesh(nodes, true, 50.0f);

	double sMeter = makeRoundEdge(&conts, nodes, depth);

	buildGemInnerHole(&conts, nodes);

	buildBackWall(depth, nodes);

	buildSideWall(&conts, depth, nodes);

	cutPorngs(&gems, &prongs, &prongNodes, option);
	
	curveNode(&gems, &prongs, &rings, nodes, yTwist);

	if (option != 3) {
		placeGems(&gems, nodes, scaleFactor);
	}

	placeProngs(&prongs, &prongNodes, nodes);

	cutRings(&conts, &rings, &ringNodes);

	placeRings(&rings, &ringNodes, nodes, depth);

	doSetVertexNormal(nodes);

	DoScale(nodes, scaleFactor);

	for (UINT i = 0; i < rings.size(); i++) {
		rings[i].pos.val[0] *= scaleFactor;
		rings[i].pos.val[1] *= scaleFactor;
		rings[i].pos.val[2] *= scaleFactor;
	}

	VERTEX mx, mn;
	VERTEX center;

	GetBound(&mx, &mn, nodes);
	center = doAverage(mx, mn);

	Move2Center(nodes);

	info[0] = sMeter * depth * scaleFactor * scaleFactor;
	info[1] = mx.val[0] - center.val[0]; info[2] = mx.val[1] - center.val[1]; info[3] = mx.val[2] - center.val[2];
	info[4] = mn.val[0] - center.val[0]; info[5] = mn.val[1] - center.val[1]; info[6] = mn.val[2] - center.val[2];
	if (rings.size() == 2) {
		info[7] = rings[0].pos.val[0] - center.val[0];
		info[8] = rings[0].pos.val[1] - center.val[1];
		info[9] = rings[0].pos.val[2] - center.val[2];
		info[10] = rings[1].pos.val[0] - center.val[0];
		info[11] = rings[1].pos.val[1] - center.val[1];
		info[12] = rings[1].pos.val[2] - center.val[2];
	}

	free(src);
	free(crop);

	for (UINT i = 0; i < conts.size(); i++) {
		conts[i].clear();
	}

	conts.clear();
	contourGroup.clear();
	return 0;
}

int generateEllipseRingModel(UCHAR *raw, int raw_width, int raw_height, PGEM_SIZE_PATTERN gemPat, int gemPatSize, double thickness, double xRadius1, double xRadius2,
	double yRadius1, double yRadius2, vector<SUBNODE> *nodes, double *info, int option) {
	SUBNODE subnode;
	vector<GEM>		gems;
	vector<PRONG>	prongs;
	vector<RING>	rings;
	vector<SHAPE> conts;
	UCHAR *crop = NULL;
	UCHAR *src = NULL;
	int width = 0;
	int height = 0;
	double depth = 0;
	double scaleFactor = 1.0f;
	TERMINAL tCenter;
	VERTEX pf[180];
	double xTwist = 0;
	double yTwist = 0;
	vector<vector<SHAPE> > contourGroup;
	vector<SHAPE> spContour;
	vector<SUBNODE> prongNodes;
	vector<SUBNODE> ringNodes;

	if (xRadius1 * xRadius2 == 0) {
		if (xRadius1 != 0 || xRadius2 != 0) return -4;
	}
	if (yRadius1 * yRadius2 == 0) {
		if (yRadius1 != 0 || yRadius2 != 0) return -5;
	}

	xRadius2 = fabs(xRadius2);
	yRadius2 = fabs(yRadius2);

	crop_extend(raw, raw_width, raw_height, &crop, &width, &height, MARGIN);
	if (crop == NULL || width == 0 || height == 0) return -1;
	ExtractGemProngPosition(&gems, &prongs, &rings, crop, width, height);
	scaleFactor = getScaleFactor(&gems, &prongs, gemPat, gemPatSize);
	src = (UCHAR *)malloc(width * height);
	cvt2Gray(crop, width, height, src);
	reverse(src, width, height);
	threshold(src, width, height, 30);
	toThinner(src, width, height, 0, 3);
	extractContourPixels(src, width, height);
	depth = thickness / scaleFactor;
	double length = (double)(width - MARGIN * 2);

	//writeMonoImage(src, width, height);
	double virtualXR1 = xRadius1 / scaleFactor;
	double virtualXR2 = xRadius2 / scaleFactor;
	double virtualYR1 = yRadius1 / scaleFactor;
	double virtualYR2 = yRadius2 / scaleFactor;

	if (xRadius1 * xRadius2 != 0) {
		double m = 0;
		for (int i = 0; i < 180; i++) {
			pf[i].val[0] = virtualXR2 * cos((double)(90 - i) * PI / 180.0f);
			pf[i].val[1] = virtualXR1 * sin((double)(90 - i) * PI / 180.0f);
			pf[i].val[2] = 0.0f;
			if (i > 0) {
				VERTEX v = doMinus(pf[i], pf[i - 1]);
				m += v.getMagnitude();
				if (m > length * 0.5f) {
					xTwist = (double)(i - 1) + (length * 0.5f - pf[i - 1].val[3]) / (m - pf[i - 1].val[3]);
					break;
				}
			}
			pf[i].val[3] = m;
		}

		if (xTwist == 0) return -2;
		xTwist *= 2.0f;
		xTwist = xRadius1 > 0 ? xTwist : -1.0f * xTwist;
	}
	if (yRadius1 * yRadius2 != 0) {
		double m = 0;
		for (int i = 0; i < 180; i++) {
			pf[i].val[0] = virtualYR2 * cos((double)(90 - i) * PI / 180.0f);
			pf[i].val[1] = virtualYR1 * sin((double)(90 - i) * PI / 180.0f);
			if (i > 0) {
				VERTEX v = doMinus(pf[i], pf[i - 1]);
				m += v.getMagnitude();
				if (m > length * 0.5f) {
					yTwist = (double)(i - 1) + (length * 0.5f - pf[i - 1].val[3]) / (m - pf[i - 1].val[3]);
					break;
				}
			}
			pf[i].val[3] = m;
		}
		if (yTwist == 0) return -3;
		yTwist *= 2.0f;
		yTwist = yRadius1 > 0 ? yTwist : -1.0f * yTwist;
	}

	if (xTwist != 0) {
		if (xTwist > 0) {
			tCenter = TERMINAL((double)width / 2.0f, (double)height - MARGIN + virtualXR1);
		}
		else if (xTwist < 0) {
			tCenter = TERMINAL((double)width / 2.0f, (double)MARGIN + virtualXR1);
		}
		curveGemProngEllipse(&gems, &prongs, &rings, tCenter, width - MARGIN * 2, virtualXR1, virtualXR2, xTwist);
	}

	for (int i = 0; i < height; i++) {
		UCHAR *p = src + i * width;
		for (int j = 0; j < width; j++) {
			if (p[j] != 255) continue;
			int ci = i; int cj = j;
			int primCount = 0;

			SHAPE shp;
			vector<Vec2i> path;
			path.push_back(Vec2i(ci, cj));
			p[j] = 0;

		_retry:
			int ret = getClosedContour(src, width, &path, ci, cj, false, 0);
			if (ret == -1) {
				ci = path[path.size() - 1].val[0];
				cj = path[path.size() - 1].val[1];

				goto _retry;
			}
			curvePathEllipse(&path, tCenter, width - MARGIN * 2, virtualXR1, virtualXR2, xTwist);
			
			int n = 0;
			int m = (int)path.size();

			for (int pi = 1; pi < m; pi++) {
				VERTEX v_xy = VERTEX(path[pi].val[0] - path[n].val[0], path[pi].val[1] - path[n].val[1], 0);
				double length_xy = v_xy.getMagnitude();
				bool f = false;

				for (int pj = n; pj < pi; pj++) {
					VERTEX v1_xy = VERTEX(path[pj].val[0] - path[n].val[0], path[pj].val[1] - path[n].val[1], 0);
					VERTEX v2_xy = doCross(v_xy, v1_xy);

					double s = fabs(v2_xy.val[2]) / length_xy;
					if (s >= 1.0f) {
						f = true;
						break;
					}
				}
				if (f) {
					LINE *ln = new LINE(TERMINAL(path[n].val[1], path[n].val[0]),
						TERMINAL(path[pi - 1].val[1], path[pi - 1].val[0]));
					shp.prims.push_back(ln);
					primCount++;
					n = pi - 1;
				}
			}

			LINE *ln = new LINE(TERMINAL(path[n].val[1], path[n].val[0]),
				TERMINAL(path[m - 1].val[1], path[m - 1].val[0]));
			shp.prims.push_back(ln);
			primCount++;

			if (primCount > 2) {
				if (shp.prims[primCount - 1]->terms[1].isEqual(shp.prims[0]->terms[0]) == false) {
					shp.prims[primCount - 1]->terms[1] = shp.prims[0]->terms[0];
				}
				shp.isGemhole = false;
				shp.depth = 0;
				shp.makePositive();
				conts.push_back(shp);
			}
		}
	}

	simplifyContour(&conts);

	smoothenContour(&conts);

	adjustRingCenter(&rings, &conts, scaleFactor, width);

	placeGemHole(&gems, &conts);
	
	regroupContour(&conts, &contourGroup);

	splitCoutourGroup(&contourGroup, &spContour);

	for (UINT n = 0; n < spContour.size(); n++) {
		generateFlatMeshes(&(spContour[n]), &subnode);
		spContour[n].clear();
	}

	if (subnode.vertex.size() > 0 && subnode.index.size() > 0) nodes->push_back(subnode);

	OptimizeMesh(nodes, true, 50.0f);

	double sMeter = makeRoundEdge(&conts, nodes, depth);
	
	buildGemInnerHole(&conts, nodes);
	
	buildBackWall(depth, nodes);

	buildSideWall(&conts, depth, nodes);

	cutPorngs(&gems, &prongs, &prongNodes, option);
	
	curveNodeEllipse(&gems, &prongs, &rings, nodes, virtualYR1, virtualYR2, yTwist);

	if (option != 3) {
		placeGems(&gems, nodes, scaleFactor);
	}

	placeProngs(&prongs, &prongNodes, nodes);

	cutRings(&conts, &rings, &ringNodes);

	placeRings(&rings, &ringNodes, nodes, depth);

	doSetVertexNormal(nodes);

	DoScale(nodes, scaleFactor);

	for (UINT i = 0; i < rings.size(); i++) {
		rings[i].pos.val[0] *= scaleFactor;
		rings[i].pos.val[1] *= scaleFactor;
		rings[i].pos.val[2] *= scaleFactor;
	}


	VERTEX mx, mn;
	VERTEX center;

	GetBound(&mx, &mn, nodes);
	center = doAverage(mx, mn);

	Move2Center(nodes);

	info[0] = sMeter * depth * scaleFactor * scaleFactor;
	info[1] = mx.val[0] - center.val[0]; info[2] = mx.val[1] - center.val[1]; info[3] = mx.val[2] - center.val[2];
	info[4] = mn.val[0] - center.val[0]; info[5] = mn.val[1] - center.val[1]; info[6] = mn.val[2] - center.val[2];
	if (rings.size() == 2) {
		info[7] = rings[0].pos.val[0] - center.val[0];
		info[8] = rings[0].pos.val[1] - center.val[1];
		info[9] = rings[0].pos.val[2] - center.val[2];
		info[10] = rings[1].pos.val[0] - center.val[0];
		info[11] = rings[1].pos.val[1] - center.val[1];
		info[12] = rings[1].pos.val[2] - center.val[2];
	}


	for (UINT i = 0; i < conts.size(); i++) {
		conts[i].clear();
	}

	conts.clear();
	contourGroup.clear();

	free(src);
	free(crop);
	return 0;
}


void writeAccessor(vector<SUBNODE> *model, FILE *pFile) {
	int n;
	const char *suffix = "],";
	char buf[1024];
	const char *accessor = "{\"bufferView\":%d,\"componentType\":%d,\"count\":%d,\"type\":%s}";
	const char *accessorPrefix = "\"accessors\":[";

	fwrite(accessorPrefix, 1, strlen(accessorPrefix), pFile);
	n = 0;
	for (UINT i = 0; i < model->size(); i++) {
		if (i > 0) fwrite(",", 1, 1, pFile);

		memset(buf, 0, 1024);
		sprintf(buf, accessor, n, 5125, model->at(i).index.size() * 3, "\"SCALAR\"");
		fwrite(buf, 1, strlen(buf), pFile);

		memset(buf, 0, 1024);
		sprintf(buf, accessor, n + 1, 5126, model->at(i).vertex.size(), "\"VEC3\"");
		fwrite(",", 1, 1, pFile);
		fwrite(buf, 1, strlen(buf), pFile);

		memset(buf, 0, 1024);
		sprintf(buf, accessor, n + 2, 5126, model->at(i).vertex.size(), "\"VEC3\"");
		fwrite(",", 1, 1, pFile);
		fwrite(buf, 1, strlen(buf), pFile);

		memset(buf, 0, 1024);
		sprintf(buf, accessor, n + 3, 5126, model->at(i).vertex.size(), "\"VEC2\"");
		fwrite(",", 1, 1, pFile);
		fwrite(buf, 1, strlen(buf), pFile);
		n += 4;
	}
	fwrite(suffix, 1, strlen(suffix), pFile);
}


UINT writeBufferView(vector<SUBNODE> *model, FILE *pFile) {
	int n, m;
	const char *suffix = "], ";
	char buf[1024];
	const char *bufferView = "{\"buffer\":0,\"byteOffset\":%d,\"byteLength\":%d%s}";
	const char *bufferViewforImage = "{\"buffer\":0,\"byteOffset\":%d,\"byteLength\":163}";
	const char *bufferviewPrefix = "\"bufferViews\":[";
	const char *images = "\"images\":[{\"bufferView\":%d,\"mimeType\":\"image/png\"},"
		"{\"bufferView\":%d,\"mimeType\":\"image/png\"},"
		"{\"bufferView\":%d,\"mimeType\":\"image/png\"},"
		"{\"bufferView\":%d,\"mimeType\":\"image/png\"}],";

	fwrite(bufferviewPrefix, 1, strlen(bufferviewPrefix), pFile);
	n = 0; m = 0;
	for (UINT i = 0; i < model->size(); i++) {
		if (i > 0) fwrite(",", 1, 1, pFile);

		memset(buf, 0, 1024);
		sprintf(buf, bufferView, m, model->at(i).index.size() * sizeof(UINT) * 3, ",\"target\":34963");
		fwrite(buf, 1, strlen(buf), pFile);

		m += (int)(model->at(i).index.size() * sizeof(UINT) * 3);
		memset(buf, 0, 1024);
		sprintf(buf, bufferView, m, model->at(i).vertex.size() * sizeof(float) * 3, ",\"target\":34962");
		fwrite(",", 1, 1, pFile);
		fwrite(buf, 1, strlen(buf), pFile);

		m += (int)(model->at(i).vertex.size() * sizeof(float) * 3);
		memset(buf, 0, 1024);
		sprintf(buf, bufferView, m, model->at(i).vertex.size() * sizeof(float) * 3, ",\"target\":34962");
		fwrite(",", 1, 1, pFile);
		fwrite(buf, 1, strlen(buf), pFile);

		m += (int)(model->at(i).vertex.size() * sizeof(float) * 3);
		memset(buf, 0, 1024);
		sprintf(buf, bufferView, m, model->at(i).vertex.size() * sizeof(float) * 2, ",\"target\":34962");
		fwrite(",", 1, 1, pFile);
		fwrite(buf, 1, strlen(buf), pFile);

		m += (int)(model->at(i).vertex.size() * sizeof(float) * 2);
		n += 4;
	}

	for (int i = 0; i < 4; i++) {
		memset(buf, 0, 1024);
		sprintf(buf, bufferViewforImage, m);
		fwrite(",", 1, 1, pFile);
		fwrite(buf, 1, strlen(buf), pFile);
		m += 163;
	}
	fwrite(suffix, 1, strlen(suffix), pFile);

	memset(buf, 0, 1024);
	sprintf(buf, "\"buffers\":[{\"byteLength\":%d}],", m + 4);
	fwrite(buf, 1, strlen(buf), pFile);

	memset(buf, 0, 1024);
	sprintf(buf, images, n, n + 1, n + 2, n + 3);
	fwrite(buf, 1, strlen(buf), pFile);
	return m + 4;
}

void writeMesh(vector<SUBNODE> *model, FILE *pFile) {
	int n = 0;
	const char *suffix = "],";
	char buf[1024];
	const char *meshprefix = "\"meshes\":[";
	//const char *mesh = "{\"name\":\"mesh_id%d\",\"primitives\":[{\"attributes\":{\"POSITION\":%d,\"NORMAL\":%d,\"TEXCOORD_0\":%d},\"indices\":%d,\"material\":0}]}";
	//const char *mesh = "{\"name\":\"%s\",\"primitives\":[{\"attributes\":{\"POSITION\":%d,\"NORMAL\":%d,\"TEXCOORD_0\":%d},\"indices\":%d,\"material\":0}]}";
	const char *mesh = "{\"name\":\"%s\",\"primitives\":[{\"attributes\":{\"POSITION\":%d,\"NORMAL\":%d,\"TEXCOORD_0\":%d},\"indices\":%d}]}";
	fwrite(meshprefix, 1, strlen(meshprefix), pFile);
	for (UINT i = 0; i < model->size(); i++) {
		if (i > 0) fwrite(",", 1, 1, pFile);

		memset(buf, 0, 1024);
		sprintf(buf, mesh, model->at(i).name, n + 1, n + 2, n + 3, n);
		fwrite(buf, 1, strlen(buf), pFile);
		n += 4;
	}

	fwrite(suffix, 1, strlen(suffix), pFile);
}

void writeNodes(vector<SUBNODE> *model, FILE *pFile) {
	int n = 0;
	char buf[10000];
	char nodeChildrenString[10000];
	const char *suffix = "],";
	const char *nodeprefix = "\"nodes\":[{\"children\":%s,\"name\":\"root\"},";
	//const char *nodes = "{\"translation\":[0.0,0.0,0.0],\"scale\":[1.0,1.0,1.0],\"mesh\":%d,\"name\":\"node_id%d\"}";
	const char *nodes = "{\"translation\":[0.0,0.0,0.0],\"scale\":[1.0,1.0,1.0],\"mesh\":%d,\"name\":\"%s\"}";

	makeChildrenNodeString(model->size(), nodeChildrenString, 10000);
	memset(buf, 0, 10000);
	sprintf(buf, nodeprefix, nodeChildrenString);
	fwrite(buf, 1, strlen(buf), pFile);

	for (UINT i = 0; i < model->size(); i++)
	{
		if (i > 0) fwrite(",", 1, 1, pFile);

		memset(buf, 0, 1024);
		sprintf(buf, nodes, n, model->at(i).name);
		fwrite(buf, 1, strlen(buf), pFile);
		n++;
	}
	fwrite(suffix, 1, strlen(suffix), pFile);
}

int writeBinaryData(vector<SUBNODE> *model, FILE *pFile) {
	int n = 0;
	//char workingPath[PATH_MAX];
	//char pngPath[PATH_MAX];

	//memset(workingPath, 0, PATH_MAX);
	//memset(pngPath, 0, PATH_MAX);

	//if (getcwd(workingPath, PATH_MAX) == NULL) return -1;
	//sprintf(pngPath, "%s/png.bin", workingPath);

	for (UINT i = 0; i < model->size(); i++) {
		void *buf = (void *)malloc(model->at(i).index.size() * sizeof(UINT) * 3);
		n = 0;
		for (UINT j = 0; j < model->at(i).index.size(); j++) {
			TRIANGLE t = model->at(i).index[j];
			UINT p0 = t.idx[0]; UINT p1 = t.idx[1]; UINT p2 = t.idx[2];
			((UINT *)buf)[n] = p0; ((UINT *)buf)[n + 1] = p1; ((UINT *)buf)[n + 2] = p2;
			n += 3;
		}
		fwrite(buf, 1, model->at(i).index.size() * sizeof(UINT) * 3, pFile);
		free(buf);

		n = 0;
		buf = (void *)malloc(model->at(i).vertex.size() * sizeof(float) * 3);
		for (UINT j = 0; j < model->at(i).vertex.size(); j++) {
			VERTEX t = model->at(i).vertex[j];
			float p0 = (float)t.val[0]; float p1 = (float)t.val[1]; float p2 = (float)t.val[2];
			((float *)buf)[n] = p0; ((float *)buf)[n + 1] = p1; ((float *)buf)[n + 2] = p2;
			n += 3;
		}
		fwrite(buf, 1, model->at(i).vertex.size() * sizeof(float) * 3, pFile);
		n = 0;
		for (UINT j = 0; j < model->at(i).vertex.size(); j++) {
			VECTOR3D t = model->at(i).vertex[j].nor;
			float p0 = (float)t.val[0]; float p1 = (float)t.val[1]; float p2 = (float)t.val[2];
			((float *)buf)[n] = p0; ((float *)buf)[n + 1] = p1; ((float *)buf)[n + 2] = p2;
			n += 3;
		}
		fwrite(buf, 1, model->at(i).vertex.size() * sizeof(float) * 3, pFile);
		n = 0;
		for (UINT j = 0; j < model->at(i).vertex.size(); j++) {
			((float *)buf)[n] = 1; ((float *)buf)[n + 1] = 1;
			n += 2;
		}
		fwrite(buf, 1, model->at(i).vertex.size() * sizeof(float) * 2, pFile);
		free(buf);
	}

	//FILE *pngFile = fopen(pngPath, "rb");
	//if (pngFile == NULL) return -2;
	UCHAR buf[652];
	//fread(buf, 1, 652, pngFile);
	fwrite(buf, 1, 652, pFile);
	//fclose(pngFile);
	return 0;
}

int exportGLB(vector<SUBNODE> *model, char *fileName)
{
	UINT bufferSize = 0;
	char magic[] = { 0x67, 0x6C, 0x54, 0x46, 0x02, 0x00, 0x00, 0x00, 0x04, 0x0C, 0x00, 0x00, 0x10, 0x08, 0x00, 0x00, 0x4A, 0x53, 0x4F, 0x4E };
	char space[] = { 0x20, 0x20, 0x20, 0x20 };
	char binMark[] = { 0x42, 0x49, 0x4E, 0x00 };
	char endMark[] = { 0x00, 0x00, 0x00, 0x00 };
	const char *header = "{\"asset\":{\"version\":\"2.0\", \"generator\":\"Microsoft GLTF Exporter 2.4.2.8\"},";
	const char *materials = "\"materials\":[{\"pbrMetallicRoughness\":{\"baseColorTexture\":{\"index\":2},\"metallicRoughnessTexture\":{\"index\":3}},"
		"\"alphaMode\":\"MASK\","
		"\"name\":\"126\","
		"\"doubleSided\":true,"
		"\"extensions\":{\"KHR_materials_pbrSpecularGlossiness\":{\"diffuseTexture\":{\"index\":0},\"specularGlossinessTexture\":{\"index\":1}}},"
		"\"extras\":{\"MSFT_sRGBFactors\":false}}],";

	const char *texture = "\"textures\":[{\"name\":\"texture383\",\"sampler\" : 0,\"source\":0},"
		"{\"name\":\"texture384\",\"sampler\":0,\"source\":1},"
		"{\"name\":\"126_bc\",\"sampler\":0,\"source\":2},"
		"{\"name\":\"126_mr\",\"sampler\":0,\"source\":3 }],";
	const char *sampler = "\"samplers\":[{\"minFilter\":9985}],";
	const char *scene = "\"scenes\":[{\"nodes\":[0]}],\"scene\":0,";
	const char *extension = "\"extensionsUsed\":[\"KHR_materials_pbrSpecularGlossiness\"]}";

	cout << "exporting GLB..." << endl;

	if (model->size() == 0) return -3;

	//Move2Center(model);
	FILE *pFile = fopen(fileName, "wb");
	if (pFile == NULL) return -1;

	cout << "writing magic & header..." << endl;
	fwrite(magic, 1, 20, pFile);
	fwrite(header, 1, strlen(header), pFile);

	cout << "writing Accessor..." << endl;
	writeAccessor(model, pFile);

	cout << "writing BufferView..." << endl;
	bufferSize = writeBufferView(model, pFile);

	cout << "writing Material..." << endl;
	fwrite(materials, 1, strlen(materials), pFile);

	cout << "writing Mesh..." << endl;
	writeMesh(model, pFile);

	cout << "writing Node..." << endl;
	writeNodes(model, pFile);

	cout << "writing Sampler..." << endl;
	fwrite(sampler, 1, strlen(sampler), pFile);

	cout << "writing Scene..." << endl;
	fwrite(scene, 1, strlen(scene), pFile);

	cout << "writing Texture..." << endl;
	fwrite(texture, 1, strlen(texture), pFile);

	cout << "writing Extension..." << endl;
	fwrite(extension, 1, strlen(extension), pFile);

	cout << "writing Sapce..." << endl;
	fwrite(space, 1, 4, pFile);

	cout << "writing JSON..." << endl;
	writeJSONSize(pFile);

	cout << "writing BufferSize..." << endl;
	fwrite(&bufferSize, 1, sizeof(UINT), pFile);

	cout << "writing Bin Mark..." << endl;
	fwrite(binMark, 1, 4, pFile);

	cout << "writing BInary Data..." << endl;
	if (writeBinaryData(model, pFile) != 0) {
		return -2;
	}

	cout << "writing End Mark..." << endl;
	fwrite(endMark, 1, 4, pFile);

	cout << "writing GLB size..." << endl;
	writeGLBSize(pFile);

	cout << "Completing..." << endl;
	fclose(pFile);
	return 0;
}

int exportGemModel(char *exportPath, vector<SUBNODE> *nodes) {
	char *ext = exportPath + strlen(exportPath) - 3;

	if (!strcmp(ext, "glb")) {
		return exportGLB(nodes, exportPath);
	}
	else if (!strcmp(ext, "stl")) {
		return exportSTL(nodes, exportPath);
	}
	return -1;
}


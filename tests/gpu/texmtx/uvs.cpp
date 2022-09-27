#include "shared.h"

extern "C" int HAS_DISPLAY;

typedef struct {
	float u, v;
	float nx, ny, nz;
	float x, y, z;
} VertexAllF32;

typedef struct {
	float w;
	float u, v;
	float x, y, z;
} VertexWeightF32;

static __attribute__((aligned(16))) VertexAllF32 vertices_f32[32];
static __attribute__((aligned(16))) VertexWeightF32 verticesw_f32[32];
static __attribute__((aligned(16))) u32 texdata[256 * 256];

static const ScePspFMatrix4 onesAddQ = {
	{1, 0, 0, 0},
	{0, 1, 0, 0},
	{0, 0, 1, 1},
	{0, 0, 1, 1},
};

static const ScePspFMatrix4 halves = {
	{0.5, 0, 0, 0},
	{0, 0.5, 0, 0},
	{0, 0, 0.5, 0},
	{0, 0, 0, 0.5},
};

inline VertexWeightF32 makeVertexWeight32(float w, float uu, float vv, float x, float y, float z) {
	VertexWeightF32 v;
	v.w = w;
	v.u = uu;
	v.v = vv;
	v.x = x;
	v.y = y;
	v.z = z;
	return v;
}

inline VertexAllF32 makeVertexAll32(float uu, float vv, float nx, float ny, float nz, float x, float y, float z) {
	VertexAllF32 v;
	v.u = uu;
	v.v = vv;
	v.nx = nx;
	v.ny = ny;
	v.nz = nz;
	v.x = x;
	v.y = y;
	v.z = z;
	return v;
}

void displayBuffer(const char *reason) {
	checkpoint("%s: TL=%08x TR=%08x BL=%08x BR=%08x", reason, readDispBuffer(0, 0), readDispBuffer(255, 0), readDispBuffer(0, 255), readDispBuffer(255, 255));
}

void init() {
	initDisplay();
	clearDispBuffer(0x44444444);
	displayBuffer("Initial");

	// Let's initialize the texture.
	for (int y = 0; y < 256; ++y) {
		for (int x = 0; x < 256; ++x) {
			texdata[y * 256 + x] = 0xFFFF0000 | (y << 8) | x;
		}
	}
	sceKernelDcacheWritebackInvalidateRange(texdata, sizeof(texdata));

	startFrame();
	sceGuTexImage(0, 256, 256, 256, texdata);
	sceGuTexFlush();
	sceGuTexSync();

	// Set Z to always be +1, so we can verify UV handling.
	sceGuSetMatrix(GU_TEXTURE, &onesAddQ);

	endFrame();
}

void testWeightedUV() {
	startFrame();
	sceGuTexMapMode(GU_TEXTURE_MATRIX, 0, 0);
	sceGuTexProjMapMode(GU_UV);
	sceGuBoneMatrix(0, &halves);

	verticesw_f32[0] = makeVertexWeight32(1.0, 0.0, 2.0, 0.0, 0.0, 0.0);
	verticesw_f32[1] = makeVertexWeight32(1.0, 2.0, 2.0, 2.0, 0.0, 0.0);
	verticesw_f32[2] = makeVertexWeight32(1.0, 2.0, 0.0, 2.0, 2.0, 0.0);
	verticesw_f32[3] = makeVertexWeight32(1.0, 0.0, 0.0, 0.0, 2.0, 0.0);

	sceKernelDcacheWritebackInvalidateRange(verticesw_f32, sizeof(verticesw_f32));
	sceGuDrawArray(GU_TRIANGLE_FAN, GU_WEIGHT_32BITF | GU_WEIGHTS(1) | GU_TEXTURE_32BITF | GU_VERTEX_32BITF | GU_TRANSFORM_3D, 4, NULL, verticesw_f32);
	dirtyDispBuffer();

	endFrame();
	displayBuffer("  Weighted UV");
}

void testScaleUV() {
	startFrame();
	sceGuTexMapMode(GU_TEXTURE_MATRIX, 0, 0);
	sceGuTexProjMapMode(GU_UV);
	sceGuTexOffset(0.25f, 0.25f);
	sceGuTexScale(2.0f, 2.0f);

	vertices_f32[0] = makeVertexAll32(0.0, 1.0, 0.0, 2.0, 1.0, 0.0, 0.0, 0.0);
	vertices_f32[1] = makeVertexAll32(1.0, 1.0, 2.0, 2.0, 1.0, 1.0, 0.0, 0.0);
	vertices_f32[2] = makeVertexAll32(1.0, 0.0, 2.0, 0.0, 1.0, 1.0, 1.0, 0.0);
	vertices_f32[3] = makeVertexAll32(0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 1.0, 0.0);

	sceKernelDcacheWritebackInvalidateRange(vertices_f32, sizeof(vertices_f32));
	sceGuDrawArray(GU_TRIANGLE_FAN, GU_TEXTURE_32BITF | GU_NORMAL_32BITF | GU_VERTEX_32BITF | GU_TRANSFORM_3D, 4, NULL, vertices_f32);
	dirtyDispBuffer();

	sceGuTexOffset(0.0f, 0.0f);
	sceGuTexScale(1.0f, 1.0f);
	endFrame();
	displayBuffer("  UV scaled UV");
}

extern "C" int main(int argc, char *argv[]) {
	init();
	HAS_DISPLAY = 0;

	checkpointNext("Normals:");
	testWeightedUV();
	testScaleUV();
	emulatorEmitScreenshot();

	sceGuTerm();

	return 0;
}
